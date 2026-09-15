#include "ControladorGiro.h"
#include "Config.h"
#include "Motores.h"
#include "ControlRuta.h"
#include "ControlSeguridad.h"
#include "Debug.h"

ControladorGiro controladorGiro;

const char* direccionGiroToString(DireccionGiro dir) {
  switch (dir) {
    case DireccionGiro::DERECHA_HORARIO:     return "DERECHA";
    case DireccionGiro::IZQUIERDA_ANTIHORARIO: return "IZQUIERDA";
    default:                                  return "DETENIDO";
  }
}

ControladorGiro::ControladorGiro() {}

float ControladorGiro::normalizarError(float objDeg, float actDeg) const {
  float e = objDeg - actDeg;
  while (e > 180.0f) e -= 360.0f;
  while (e <= -180.0f) e += 360.0f;
  return e;
}

void ControladorGiro::iniciarGiroAbsoluto(float headingObjetivoDeg, int pwmMinPos, int pwmMinNeg,
                                         float toleranciaDeg, uint32_t ahoraMs) {
  const uint32_t ahora = ahoraMs ? ahoraMs : millis();
  _yawObjetivoDeg = headingObjetivoDeg;
  while (_yawObjetivoDeg >= 360.0f) _yawObjetivoDeg -= 360.0f;
  while (_yawObjetivoDeg < 0.0f) _yawObjetivoDeg += 360.0f;

  _toleranciaDeg = toleranciaDeg;
  _pwmMinPos = pwmMinPos;
  _pwmMinNeg = pwmMinNeg;
  _inicioGiroTotalMs = ahora;
  _inicioIntentoMs = ahora;
  _ultimoAumentoTorqueMs = ahora;
  _inicioPulsoFinoMs = ahora;
  _estableDesdeMs = 0;

  int menor = (_pwmMinPos > 0 && _pwmMinNeg > 0) ? min(_pwmMinPos, _pwmMinNeg)
              : (_pwmMinPos > 0 ? _pwmMinPos : (_pwmMinNeg > 0 ? _pwmMinNeg : PWM_TURN_START));
  _pwmBusqueda = max(PWM_TURN_START, menor - PWM_TURN_START_FLOOR_OFFSET);
  _pwmActual = 0;
  _pwmBoostFrenado = 0;
  _signoGiroAplicado = 0;
  _movGiroConfirmado = false;
  _pulsoFinoEncendido = false;
  _enTolerancia = false;
  _watchdogArmado = false;
  _estado = EstadoLazoGiro::BUSQUEDA_TORQUE;
  _motivoFallo = nullptr;
}

void ControladorGiro::iniciarGiroRelativo(float deltaDeg, float headingActualDeg, int pwmMinPos, int pwmMinNeg,
                                         float toleranciaDeg, uint32_t ahoraMs) {
  float obj = headingActualDeg + deltaDeg;
  while (obj >= 360.0f) obj -= 360.0f;
  while (obj < 0.0f) obj += 360.0f;
  iniciarGiroAbsoluto(obj, pwmMinPos, pwmMinNeg, toleranciaDeg, ahoraMs);
}

void ControladorGiro::cancelar() {
  frenarMotores();
  _estado = EstadoLazoGiro::INACTIVO;
  _direccion = DireccionGiro::NINGUNA;
  _pwmActual = 0;
  _enTolerancia = false;
  _estableDesdeMs = 0;
}

bool ControladorGiro::actualizar(float headingActualMPU, float gyroZFiltradoRadS,
                                 const int64_t deltasEncoders[4], const bool encoderConfiable[4],
                                 uint32_t ahoraMs) {
  if (!estaActivo()) return true;

  _errorAngularDeg = normalizarError(_yawObjetivoDeg, headingActualMPU);
  _errorAbsDeg = fabsf(_errorAngularDeg);
  const int signoDeseado = _errorAngularDeg > 0.0f ? 1 : (_errorAngularDeg < 0.0f ? -1 : 0);
  _direccion = signoDeseado > 0 ? DireccionGiro::DERECHA_HORARIO
             : (signoDeseado < 0 ? DireccionGiro::IZQUIERDA_ANTIHORARIO : DireccionGiro::NINGUNA);

  // Timeouts de seguridad
  if (ahoraMs - _inicioGiroTotalMs > TURN_TIMEOUT_MS) {
    frenarMotores();
    _estado = EstadoLazoGiro::FALLO;
    _motivoFallo = "turn_timeout_total";
    return false;
  }

  // Latch de tolerancia y verificación estricta de reposo del MPU
  if (_errorAbsDeg <= _toleranciaDeg) {
    frenarMotores();
    _pwmActual = 0;
    _signoGiroAplicado = 0;
    _enTolerancia = true;
    _estado = EstadoLazoGiro::ASENTAMIENTO;

    if (fabsf(gyroZFiltradoRadS) > 0.02f) {
      _estableDesdeMs = 0;
      return true;
    }
    if (!_estableDesdeMs) _estableDesdeMs = ahoraMs;
    if (ahoraMs - _estableDesdeMs >= TURN_SETTLE_MS) {
      _estado = EstadoLazoGiro::COMPLETADO;
      _direccion = DireccionGiro::NINGUNA;
      return true;
    }
    return true;
  }

  if (_enTolerancia) {
    _enTolerancia = false;
    _estableDesdeMs = 0;
  }

  // Cálculo de torque según dirección gobernada por MPU
  int minimo = (signoDeseado > 0) ? _pwmMinPos : _pwmMinNeg;
  if (minimo == 0) {
    minimo = _pwmMinPos > 0 ? _pwmMinPos : (_pwmMinNeg > 0 ? _pwmMinNeg : PWM_TURN_START);
  }
  const int pwmLejos = max(PWM_TURN_START, min(PWM_TURN_MAX_LIMIT, minimo + PWM_TURN_FAR_MARGIN));
  const int pwmCerca = min(PWM_TURN_MAX_LIMIT, minimo + PWM_TURN_NEAR_MARGIN);

  // Confirmación de movimiento bilateral y actualización de torque
  const int64_t ticksL = deltasEncoders[0] + deltasEncoders[2];
  const int64_t ticksR = deltasEncoders[1] + deltasEncoders[3];
  if (fabsf(gyroZFiltradoRadS) >= GYRO_MOVEMENT_RAD_S && (ticksL >= 2 && ticksR >= 2)) {
    if (!_movGiroConfirmado) {
      _movGiroConfirmado = true;
      if (signoDeseado > 0) _pwmMinPos = max(_pwmMinPos, _pwmActual);
      else if (signoDeseado < 0) _pwmMinNeg = max(_pwmMinNeg, _pwmActual);
    }
  }

  // Armado del watchdog de stall al alcanzar torque calibrado
  const int torqueCalibrado = (signoDeseado > 0) ? _pwmMinPos : _pwmMinNeg;
  if (!_watchdogArmado && torqueCalibrado > 0 && _pwmActual >= torqueCalibrado) {
    _watchdogArmado = true;
    _ultimoPulsoLadoMs[0] = _ultimoPulsoLadoMs[1] = ahoraMs;
  }
  if (!_movGiroConfirmado && _pwmBusqueda < PWM_TURN_MAX_LIMIT) {
    _ultimoPulsoLadoMs[0] = _ultimoPulsoLadoMs[1] = ahoraMs;
  }

  // Verificación de stall bilateral por lado
  const int64_t ladoTicks[2] = {ticksL, ticksR};
  for (int i = 0; i < 2; ++i) {
    if (ladoTicks[i] != _ticksLadoAnterior[i]) {
      _ticksLadoAnterior[i] = ladoTicks[i];
      _ultimoPulsoLadoMs[i] = ahoraMs;
    } else if (_watchdogArmado && (ahoraMs - _ultimoPulsoLadoMs[i] > TURN_STALL_MS)) {
      frenarMotores();
      _estado = EstadoLazoGiro::FALLO;
      _motivoFallo = (i == 0) ? "turn_stall_left" : "turn_stall_right";
      return false;
    }
  }

  // Timeout por intento y detección de falta de progreso
  if (ahoraMs - _inicioIntentoMs > TURN_ATTEMPT_TIMEOUT_MS) {
    frenarMotores();
    _estado = EstadoLazoGiro::FALLO;
    _motivoFallo = "turn_timeout_attempt";
    return false;
  }
  if (ahoraMs - _inicioIntentoMs > 5000 && !_movGiroConfirmado) {
    frenarMotores();
    _estado = EstadoLazoGiro::FALLO;
    _motivoFallo = "turn_no_progress";
    return false;
  }

  int pwmObj = pwmLejos;
  if (!_movGiroConfirmado) {
    _estado = EstadoLazoGiro::BUSQUEDA_TORQUE;
    if (ahoraMs - _ultimoAumentoTorqueMs >= CAL_RAMP_INTERVAL_MS) {
      _ultimoAumentoTorqueMs = ahoraMs;
      _pwmBusqueda = min(pwmLejos, _pwmBusqueda + CALIBRATION_PWM_STEP);
    }
    pwmObj = _pwmBusqueda;
  } else if (_errorAbsDeg < TURN_BRAKING_ZONE_DEG) {
    pwmObj = pwmCerca + static_cast<int>(roundf((pwmLejos - pwmCerca) * _errorAbsDeg / TURN_BRAKING_ZONE_DEG));

    if (_errorAbsDeg > TURN_HYBRID_THRESHOLD_DEG) {
      // Modo 1: Rampa adaptativa
      _estado = EstadoLazoGiro::RAMPA_ADAPTATIVA;
      _pulsoFinoEncendido = false;
      _inicioPulsoFinoMs = ahoraMs;
      if (fabsf(gyroZFiltradoRadS) < GYRO_MOVEMENT_RAD_S) {
        if (ahoraMs - _ultimoAumentoTorqueMs >= TURN_RAMP_ADAPTIVE_INTERVAL_MS) {
          _ultimoAumentoTorqueMs = ahoraMs;
          _pwmBoostFrenado = min(PWM_TURN_MAX_LIMIT - pwmObj, _pwmBoostFrenado + static_cast<int>(5 * PWM_SCALE_8_TO_10));
        }
      } else {
        if (ahoraMs - _ultimoAumentoTorqueMs >= TURN_RAMP_ADAPTIVE_INTERVAL_MS && _pwmBoostFrenado > 0) {
          _ultimoAumentoTorqueMs = ahoraMs;
          _pwmBoostFrenado = max(0, _pwmBoostFrenado - static_cast<int>(2 * PWM_SCALE_8_TO_10));
        }
      }
      pwmObj = min(PWM_TURN_MAX_LIMIT, pwmObj + _pwmBoostFrenado);
    } else {
      // Modo 2: Micro-pulsos de precisión (< 4.0°)
      _estado = EstadoLazoGiro::MICRO_PULSOS_FINOS;
      const uint32_t deltaPulso = ahoraMs - _inicioPulsoFinoMs;
      if (_pulsoFinoEncendido) {
        if (deltaPulso >= TURN_PULSE_ON_MS) {
          _pulsoFinoEncendido = false;
          _inicioPulsoFinoMs = ahoraMs;
          frenarMotores();
          _pwmActual = 0;
          return true;
        }
        const int pwmKick = max(minimo + static_cast<int>(6 * PWM_SCALE_8_TO_10), PWM_TURN_START);
        pwmObj = min(PWM_TURN_MAX_LIMIT, pwmKick + _pwmBoostFrenado);
      } else {
        frenarMotores();
        _pwmActual = 0;
        if (deltaPulso >= TURN_PULSE_OFF_MS) {
          _pulsoFinoEncendido = true;
          _inicioPulsoFinoMs = ahoraMs;
          if (fabsf(gyroZFiltradoRadS) < GYRO_MOVEMENT_RAD_S) {
            _pwmBoostFrenado = min(PWM_TURN_MAX_LIMIT - minimo,
                                   _pwmBoostFrenado + static_cast<int>(4 * PWM_SCALE_8_TO_10));
          }
        }
        return true;
      }
    }
  } else {
    _estado = EstadoLazoGiro::CRUCERO;
    _pwmBoostFrenado = 0;
    _pulsoFinoEncendido = false;
    _inicioPulsoFinoMs = ahoraMs;
  }

  // Slew rate
  if (_signoGiroAplicado != 0 && signoDeseado != _signoGiroAplicado && _pwmActual > 0) {
    _pwmActual = max(0, _pwmActual - PWM_TURN_SLEW_STEP);
    if (_pwmActual == 0) _signoGiroAplicado = 0;
  } else {
    if (_signoGiroAplicado == 0) _signoGiroAplicado = signoDeseado;
    const int paso = _movGiroConfirmado ? PWM_TURN_SLEW_STEP : PWM_TURN_START_SLEW_STEP;
    if (_errorAbsDeg <= TURN_HYBRID_THRESHOLD_DEG) {
      _pwmActual = pwmObj;
    } else if (_pwmActual < pwmObj) {
      _pwmActual = min(pwmObj, _pwmActual + paso);
    } else {
      _pwmActual = max(pwmObj, _pwmActual - PWM_TURN_SLEW_STEP);
    }
  }

  if (_signoGiroAplicado != 0 && _pwmActual > 0) {
    const float cmPorTick = ControlRuta::distanciaPorTick(WHEEL_DIAMETER_ODOMETRY_CM, ENCODER_PPR);
    const float avgL = ControlSeguridad::promedioConfiableLado(deltasEncoders, encoderConfiable, true);
    const float avgR = ControlSeguridad::promedioConfiableLado(deltasEncoders, encoderConfiable, false);
    const auto balance = ControlRuta::balancearGiroDiferencial(
        _pwmActual, _signoGiroAplicado, avgL, avgR, cmPorTick,
        KP_GIRO_BALANCE_PWM_POR_CM, PWM_GIRO_BALANCE_MAX, PWM_TURN_MAX_LIMIT);

    if (!aplicarVelocidades(balance.pwmL, balance.pwmR)) {
      _estado = EstadoLazoGiro::FALLO;
      _motivoFallo = "motor_output_error";
      frenarMotores();
      return false;
    }
  } else {
    frenarMotores();
  }

  return true;
}

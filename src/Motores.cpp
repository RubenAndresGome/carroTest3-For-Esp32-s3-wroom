#include "Motores.h"
#include "Config.h"
#include "Debug.h"

int pwm_aplicado_L = 0;
int pwm_aplicado_R = 0;
int pwm_solicitado_L = 0;
int pwm_solicitado_R = 0;

static constexpr uint32_t PWM_FREQUENCY = 5000;
static const int MOTOR_PINS[8] = {
  PIN_BL_FWD, PIN_BL_REV, PIN_BR_FWD, PIN_BR_REV,
  PIN_FL_FWD, PIN_FL_REV, PIN_FR_FWD, PIN_FR_REV
};

enum class EstadoMotores : uint8_t {
  NO_CONFIGURADOS,
  MAPA_INVALIDO,
  LISTOS,
  ERROR_SALIDA
};

static EstadoMotores estadoSalidaMotores = EstadoMotores::NO_CONFIGURADOS;
static uint8_t mascaraCanalesPWM = 0;

const char* sentidoToString(SentidoGiro sentido) {
  switch (sentido) {
    case SentidoGiro::ADELANTE: return "ADELANTE";
    case SentidoGiro::ATRAS:    return "ATRAS";
    default:                    return "DETENIDO";
  }
}

static int canalParaPin(int pin) {
  for (int i = 0; i < 8; ++i) if (MOTOR_PINS[i] == pin) return i;
  // Nunca asumir canal 0: un pin desconocido no debe energizar otra salida.
  return -1;
}

static bool parMotorValido(int pinFwd, int pinRev) {
  const int canalFwd = canalParaPin(pinFwd);
  const int canalRev = canalParaPin(pinRev);
  return pinFwd >= 0 && pinRev >= 0 && canalFwd >= 0 && canalRev >= 0 &&
         canalFwd != canalRev;
}

static bool setMotorPWM(int pinFwd, int pinRev, int vel) {
  if (!parMotorValido(pinFwd, pinRev)) return false;
  const int canalFwd = canalParaPin(pinFwd);
  const int canalRev = canalParaPin(pinRev);
  if (vel > 0) {
    ledcWrite(canalFwd, vel);
    ledcWrite(canalRev, 0);
  } else if (vel < 0) {
    ledcWrite(canalFwd, 0);
    ledcWrite(canalRev, -vel);
  } else {
    ledcWrite(canalFwd, 0);
    ledcWrite(canalRev, 0);
  }
  return true;
}

ControladorLado::ControladorLado(const char* nombre,
                                 int pinFwdF, int pinRevF, int polF,
                                 int pinFwdB, int pinRevB, int polB)
  : _nombre(nombre),
    _pinFwdF(pinFwdF), _pinRevF(pinRevF), _polF(polF),
    _pinFwdB(pinFwdB), _pinRevB(pinRevB), _polB(polB) {}

void ControladorLado::configurarPines(const char* nombre,
                                      int pinFwdF, int pinRevF, int polF,
                                      int pinFwdB, int pinRevB, int polB) {
  _nombre = nombre;
  _pinFwdF = pinFwdF; _pinRevF = pinRevF; _polF = polF;
  _pinFwdB = pinFwdB; _pinRevB = pinRevB; _polB = polB;
}

void ControladorLado::setComando(SentidoGiro sentido, int pwm) {
  _sentidoSolicitado = sentido;
  _pwmSolicitado = constrain(pwm, 0, PWM_TURN_MAX_LIMIT);
}

void ControladorLado::setVelocidadFirmada(int velFirmada) {
  velFirmada = constrain(velFirmada, -PWM_TURN_MAX_LIMIT, PWM_TURN_MAX_LIMIT);
  if (velFirmada > 0) {
    setComando(SentidoGiro::ADELANTE, velFirmada);
  } else if (velFirmada < 0) {
    setComando(SentidoGiro::ATRAS, -velFirmada);
  } else {
    setComando(SentidoGiro::DETENIDO, 0);
  }
}

void ControladorLado::detener(uint32_t ahoraMs) {
  if (_estadoInterlock == EstadoInterlock::ACTIVO && _sentidoActivo != SentidoGiro::DETENIDO) {
    _ultimoSentidoEnergizado = _sentidoActivo;
    _apagadoDesdeMs = ahoraMs;
    _apagadoRegistrado = true;
  }
  _sentidoActivo = SentidoGiro::DETENIDO;
  _sentidoPendiente = SentidoGiro::DETENIDO;
  _estadoInterlock = EstadoInterlock::APAGADO;
  _sentidoSolicitado = SentidoGiro::DETENIDO;
  _pwmSolicitado = 0;
  _sentidoAplicado = SentidoGiro::DETENIDO;
  _pwmAplicado = 0;
}

void ControladorLado::detener() {
  detener(millis());
}

const char* ControladorLado::getEstadoInterlockStr() const {
  switch (_estadoInterlock) {
    case EstadoInterlock::ACTIVO: return "ACTIVE";
    case EstadoInterlock::ESPERANDO_INVERSION: return "WAITING_REVERSAL";
    default: return "OFF";
  }
}

int ControladorLado::actualizarInterlock(uint32_t ahoraMs) {
  _pwmSolicitado = constrain(_pwmSolicitado, 0, PWM_TURN_MAX_LIMIT);
  if (_sentidoSolicitado == SentidoGiro::DETENIDO || _pwmSolicitado == 0) {
    detener(ahoraMs);
    return 0;
  }

  if (_estadoInterlock == EstadoInterlock::ACTIVO) {
    if (_sentidoSolicitado == _sentidoActivo) {
      _sentidoAplicado = _sentidoActivo;
      _pwmAplicado = _pwmSolicitado;
      return _pwmAplicado;
    }
    _ultimoSentidoEnergizado = _sentidoActivo;
    _apagadoDesdeMs = ahoraMs;
    _apagadoRegistrado = true;
    _sentidoActivo = SentidoGiro::DETENIDO;
    _sentidoPendiente = _sentidoSolicitado;
    _estadoInterlock = EstadoInterlock::ESPERANDO_INVERSION;
    _sentidoAplicado = SentidoGiro::DETENIDO;
    _pwmAplicado = 0;
    return 0;
  }

  if (_estadoInterlock == EstadoInterlock::ESPERANDO_INVERSION) {
    _sentidoPendiente = _sentidoSolicitado;
    if (static_cast<uint32_t>(ahoraMs - _apagadoDesdeMs) < PWM_DIRECTION_PAUSE_MS) {
      _sentidoAplicado = SentidoGiro::DETENIDO;
      _pwmAplicado = 0;
      return 0;
    }
    _sentidoActivo = _sentidoPendiente;
    _sentidoPendiente = SentidoGiro::DETENIDO;
    _estadoInterlock = EstadoInterlock::ACTIVO;
    _sentidoAplicado = _sentidoActivo;
    _pwmAplicado = _pwmSolicitado;
    return _pwmAplicado;
  }

  if (_apagadoRegistrado && _ultimoSentidoEnergizado != SentidoGiro::DETENIDO &&
      _sentidoSolicitado != _ultimoSentidoEnergizado &&
      static_cast<uint32_t>(ahoraMs - _apagadoDesdeMs) < PWM_DIRECTION_PAUSE_MS) {
    _sentidoPendiente = _sentidoSolicitado;
    _estadoInterlock = EstadoInterlock::ESPERANDO_INVERSION;
    _sentidoAplicado = SentidoGiro::DETENIDO;
    _pwmAplicado = 0;
    return 0;
  }

  _sentidoActivo = _sentidoSolicitado;
  _sentidoPendiente = SentidoGiro::DETENIDO;
  _estadoInterlock = EstadoInterlock::ACTIVO;
  _sentidoAplicado = _sentidoActivo;
  _pwmAplicado = _pwmSolicitado;
  return _pwmAplicado;
}

bool ControladorLado::escribirHardware() {
  int velF = 0;
  int velB = 0;
  if (_sentidoAplicado == SentidoGiro::ADELANTE) {
    velF = _pwmAplicado * _polF;
    velB = _pwmAplicado * _polB;
  } else if (_sentidoAplicado == SentidoGiro::ATRAS) {
    velF = -_pwmAplicado * _polF;
    velB = -_pwmAplicado * _polB;
  }
  const bool okF = setMotorPWM(_pinFwdF, _pinRevF, velF);
  const bool okB = setMotorPWM(_pinFwdB, _pinRevB, velB);
  return okF && okB;
}

void ControladorLado::apagarHardware() {
  setMotorPWM(_pinFwdF, _pinRevF, 0);
  setMotorPWM(_pinFwdB, _pinRevB, 0);
}

ControladorLado ladoIzquierdo("IZQUIERDO",
                              PIN_FL_FWD, PIN_FL_REV, PWM_POLARITY_FL,
                              PIN_BL_FWD, PIN_BL_REV, PWM_POLARITY_BL);

ControladorLado ladoDerecho("DERECHO",
                            PIN_FR_FWD, PIN_FR_REV, PWM_POLARITY_FR,
                            PIN_BR_FWD, PIN_BR_REV, PWM_POLARITY_BR);

void ordenarAvance(int pwm) {
  aplicarVelocidades(pwm, pwm);
}

void ordenarRetroceso(int pwm) {
  aplicarVelocidades(-pwm, -pwm);
}

void ordenarGiroPivote(bool sentidoHorario, int pwm) {
  if (sentidoHorario) {
    aplicarVelocidades(pwm, -pwm);
  } else {
    aplicarVelocidades(-pwm, pwm);
  }
}

bool validarMapaMotores() {
  for (int i = 0; i < 8; ++i) {
    if (MOTOR_PINS[i] < 0 || MOTOR_PINS[i] > 48) return false;
    for (int j = i + 1; j < 8; ++j) {
      if (MOTOR_PINS[i] == MOTOR_PINS[j]) return false;
    }
  }
  return parMotorValido(PIN_FL_FWD, PIN_FL_REV) &&
         parMotorValido(PIN_BL_FWD, PIN_BL_REV) &&
         parMotorValido(PIN_FR_FWD, PIN_FR_REV) &&
         parMotorValido(PIN_BR_FWD, PIN_BR_REV);
}

static void apagarCanalesPWM() {
  for (int canal = 0; canal < 8; ++canal) {
    if ((mascaraCanalesPWM & (1U << canal)) != 0U) ledcWrite(canal, 0);
  }
}

bool aplicarVelocidades(int velIzq, int velDer) {
  const bool mapaValido = validarMapaMotores();
  const bool paresValidos = mapaValido &&
      parMotorValido(PIN_FL_FWD, PIN_FL_REV) &&
      parMotorValido(PIN_BL_FWD, PIN_BL_REV) &&
      parMotorValido(PIN_FR_FWD, PIN_FR_REV) &&
      parMotorValido(PIN_BR_FWD, PIN_BR_REV);
  if (!motoresListos() || !paresValidos) {
    if (!mapaValido || !paresValidos) {
      estadoSalidaMotores = EstadoMotores::MAPA_INVALIDO;
    } else if (estadoSalidaMotores != EstadoMotores::ERROR_SALIDA) {
      estadoSalidaMotores = EstadoMotores::NO_CONFIGURADOS;
    }
    frenarMotores();
    return false;
  }
  const uint32_t ahora = millis();
  ladoIzquierdo.setVelocidadFirmada(velIzq);
  ladoDerecho.setVelocidadFirmada(velDer);

  ladoIzquierdo.actualizarInterlock(ahora);
  ladoDerecho.actualizarInterlock(ahora);

  pwm_solicitado_L = ladoIzquierdo.getVelocidadFirmadaSolicitada();
  pwm_solicitado_R = ladoDerecho.getVelocidadFirmadaSolicitada();
  pwm_aplicado_L = ladoIzquierdo.getVelocidadFirmada();
  pwm_aplicado_R = ladoDerecho.getVelocidadFirmada();

  const bool escrito = ladoIzquierdo.escribirHardware() && ladoDerecho.escribirHardware();
  if (!escrito) {
    estadoSalidaMotores = EstadoMotores::ERROR_SALIDA;
    frenarMotores();
    return false;
  }
  return true;
}

void frenarMotores() {
  const uint32_t ahora = millis();
  ladoIzquierdo.detener(ahora);
  ladoDerecho.detener(ahora);
  ladoIzquierdo.apagarHardware();
  ladoDerecho.apagarHardware();
  apagarCanalesPWM();
  pwm_aplicado_L = 0;
  pwm_aplicado_R = 0;
  pwm_solicitado_L = 0;
  pwm_solicitado_R = 0;
}

bool validarInterlockMotores() {
  ControladorLado prueba("TEST", -1, -1, 1, -1, -1, 1);
  prueba.setVelocidadFirmada(100);
  if (prueba.actualizarInterlock(1000) != 100) return false;
  prueba.setVelocidadFirmada(-100);
  if (prueba.actualizarInterlock(1010) != 0) return false;
  if (prueba.actualizarInterlock(1259) != 0) return false;
  if (prueba.actualizarInterlock(1260) != 100) return false;
  if (prueba.getVelocidadFirmada() != -100) return false;
  if (prueba.getSentido() != SentidoGiro::ATRAS) return false;

  prueba = ControladorLado("TEST", -1, -1, 1, -1, -1, 1);
  prueba.setVelocidadFirmada(90);
  if (prueba.actualizarInterlock(2000) != 90) return false;
  prueba.detener(2020);
  prueba.setVelocidadFirmada(-90);
  if (prueba.actualizarInterlock(2050) != 0) return false;
  if (prueba.actualizarInterlock(2269) != 0) return false;
  if (prueba.actualizarInterlock(2270) != 90) return false;
  if (prueba.getVelocidadFirmada() != -90) return false;

  prueba = ControladorLado("TEST", -1, -1, 1, -1, -1, 1);
  prueba.setVelocidadFirmada(80);
  if (prueba.actualizarInterlock(3000) != 80) return false;
  prueba.setVelocidadFirmada(-80);
  if (prueba.actualizarInterlock(3010) != 0) return false;
  prueba.setVelocidadFirmada(70);
  if (prueba.actualizarInterlock(3050) != 0) return false;
  if (prueba.actualizarInterlock(3259) != 0) return false;
  if (prueba.actualizarInterlock(3260) != 70) return false;
  if (prueba.getVelocidadFirmada() != 70) return false;

  prueba = ControladorLado("TEST", -1, -1, 1, -1, -1, 1);
  prueba.setVelocidadFirmada(80);
  if (prueba.actualizarInterlock(4000) != 80) return false;
  prueba.setVelocidadFirmada(-80);
  if (prueba.actualizarInterlock(4010) != 0) return false;
  prueba.detener(4020);
  if (prueba.getEstadoInterlock() != EstadoInterlock::APAGADO || prueba.getSignoPendiente() != 0 || prueba.getSignoEnergizado() != 0) return false;

  prueba = ControladorLado("TEST", -1, -1, 1, -1, -1, 1);
  prueba.setVelocidadFirmada(PWM_TURN_MAX_LIMIT + 100);
  if (prueba.actualizarInterlock(5000) != PWM_TURN_MAX_LIMIT) return false;

  prueba = ControladorLado("TEST", -1, -1, 1, -1, -1, 1);
  prueba.setVelocidadFirmada(60);
  if (prueba.actualizarInterlock(UINT32_MAX - 50U) != 60) return false;
  prueba.setVelocidadFirmada(-60);
  if (prueba.actualizarInterlock(UINT32_MAX - 40U) != 0) return false;
  if (prueba.actualizarInterlock(208U) != 0) return false;
  if (prueba.actualizarInterlock(209U) != 60) return false;
  if (prueba.getVelocidadFirmada() != -60) return false;
  return true;
}

const char* estadoInterlockL() { return ladoIzquierdo.getEstadoInterlockStr(); }
const char* estadoInterlockR() { return ladoDerecho.getEstadoInterlockStr(); }
int signoEnergizadoL() { return ladoIzquierdo.getSignoEnergizado(); }
int signoEnergizadoR() { return ladoDerecho.getSignoEnergizado(); }
int signoPendienteL() { return ladoIzquierdo.getSignoPendiente(); }
int signoPendienteR() { return ladoDerecho.getSignoPendiente(); }

bool motoresListos() { return estadoSalidaMotores == EstadoMotores::LISTOS; }

const char* estadoMotores() {
  switch (estadoSalidaMotores) {
    case EstadoMotores::MAPA_INVALIDO: return "MAPA_INVALIDO";
    case EstadoMotores::LISTOS: return "LISTOS";
    case EstadoMotores::ERROR_SALIDA: return "ERROR_SALIDA";
    default: return "NO_CONFIGURADOS";
  }
}

void setup_MotorPinsLow() {
  // Bajar todos los GPIO físicamente representables incluso si el mapa tiene
  // duplicados. Un error de configuración no debe impedir desenergizar el
  // resto de entradas del driver durante el arranque.
  for (int i = 0; i < 8; ++i) {
    if (MOTOR_PINS[i] < 0 || MOTOR_PINS[i] > 48) continue;
    pinMode(MOTOR_PINS[i], OUTPUT);
    digitalWrite(MOTOR_PINS[i], LOW);
  }
  if (!validarMapaMotores()) estadoSalidaMotores = EstadoMotores::MAPA_INVALIDO;
}

bool setup_Motores() {
  LOG_CORE("Inicializando Motores DRV8833...");
  // Una reinicialización debe desenergizar primero cualquier canal válido de
  // la configuración anterior. La máscara también permite apagar un setup
  // parcial sin escribir sobre canales que nunca se configuraron.
  apagarCanalesPWM();
  mascaraCanalesPWM = 0;
  if (!validarMapaMotores() || !validarInterlockMotores()) {
    estadoSalidaMotores = EstadoMotores::MAPA_INVALIDO;
    return false;
  }
  for (int canal = 0; canal < 8; ++canal) {
    const double frecuenciaReal = ledcSetup(canal, PWM_FREQUENCY, PWM_RESOLUTION_BITS);
    if (frecuenciaReal <= 0.0) {
      apagarCanalesPWM();
      estadoSalidaMotores = EstadoMotores::ERROR_SALIDA;
      return false;
    }
    mascaraCanalesPWM |= static_cast<uint8_t>(1U << canal);
    ledcAttachPin(MOTOR_PINS[canal], canal);
    // No depender del duty inicial que deje la implementación de LEDC.
    ledcWrite(canal, 0);
  }
  estadoSalidaMotores = EstadoMotores::LISTOS;
  frenarMotores();
  return true;
}

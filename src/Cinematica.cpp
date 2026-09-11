#include "Cinematica.h"
#include "Comandos.h"
#include "Config.h"
#include "ControlRuta.h"
#include "ControlCalibracion.h"
#include "ControlSeguridad.h"
#include "Debug.h"
#include "Eventos.h"
#include "Motores.h"
#include "PoseEstimator.h"
#include "Sensores.h"
#include "Seguridad.h"
#include <Arduino.h>
#include <cmath>
#include <cstring>

namespace {

enum class Fase : uint8_t {
  NINGUNA,
  CAL_CUENTA, CAL_A, CAL_VALIDAR_25, CAL_PAUSA, CAL_B, CAL_PAUSA_RETORNO, CAL_RETORNO,
  GIRO_INICIAL, AVANCE, AVANCE_PULSADO, GIRO_RECUPERACION, GIRO_FINAL, GIRO_SOLO,
  PAUSA_REEVALUACION, ASENTAMIENTO_FINAL, VERIFICAR_FINAL,
  PAUSA_RECENTRADO, GIRO_A_REINGRESO, AVANCE_A_EJE,
  VERIFICAR_REINGRESO, GIRO_A_RUMBO, GIRO_REALINEAR_EJE
};

Fase fase = Fase::NINGUNA;

// --- forward declarations (funciones internas que se llaman entre si) ---
void iniciarBaseGiro(float objetivoDeg, Fase retorno);
void controlarGiro();
void completarGiro();
void iniciarAvance(bool conservar);
bool controlarAvance();
void iniciarAvancePulsado(bool conservar);
bool controlarAvancePulsado();
void iniciarAsentamientoFinal(float distanciaAntesDeFrenarCm);
bool controlarAsentamientoFinal();
void completarPaso();
void iniciarVerificacionFinal();
void verificarObjetivoFinal();
void actualizarErroresTrayectoria();
void iniciarRecuperacionEndpoint();
void iniciarRecentrado(const char* disparador, float distanciaActualCm);
void iniciarAvanceReingreso();
void verificarReingreso();
void reanudarTrasRecentrado();
void reanudarTrasRealineacion();
void iniciarRealineacionEje(float distanciaActualCm);
void actualizarVentanaSupervision(float distanciaMedidaCm);
bool vigilarDesviacionMaxima();
void iniciarPasoInterno();
void fallo(const char* d);

// --- variables del paso (usadas por iniciarAvance, iniciarPaso) ---
float pasoHeading = 0.0f;
float pasoDistanciaCm = 0.0f;
float pasoTargetX = NAN;
float pasoTargetY = NAN;
bool tieneTargetEspacial = false;
float distTargetMinimaCm = 1e9f;
float distanciaPlanificadaCm = 0.0f;
ControlRuta::EstadoPI estadoPIRumbo = {};
uint32_t inicioVerificacionFinalMs = 0;
uint8_t intentosEndpoint = 0;
bool recuperacionEndpointActiva = false;
int direccionTraslacion = 1;
int64_t ticksBaseAsentamiento[4] = {};
float distanciaInicioAsentamientoCm = 0.0f;
float ticksAsentamientoAnterior = 0.0f;
uint32_t inicioAsentamientoMs = 0;
uint32_t ultimoMovimientoAsentamientoMs = 0;
uint32_t inicioDesviacionLateralMs = 0;
uint32_t inicioLateralExtremoMs = 0;
uint32_t inicioCrecimientoLateralMs = 0;
uint32_t inicioFaseRecentradoMs = 0;
float mejorErrorLateralAbsCm = 0.0f;
float distanciaReingresoCm = 0.0f;
int direccionTraslacionPlanificada = 1;
int direccionReingreso = 1;
float rumboReingresoTrayectoDeg = 0.0f;
float rumboReingresoCuerpoDeg = 0.0f;
float errorLateralAnteriorCm = 0.0f;
bool errorLateralAnteriorValido = false;
ControlRuta::SeguimientoProgreso seguimientoProgreso = {};
ControlRuta::EpisodioRecuperacion episodioRecuperacion = {};

float distanciaRestanteObjetivoActivo(bool avanceReingreso) {
  const float distanciaEndpointCm = tieneTargetEspacial
      ? PoseGlobal.distanciaAlObjetivo(pasoTargetX, pasoTargetY)
      : fabsf(pasoErrorLongitudinalCm);
  const float distanciaPuntoReingresoCm = avanceReingreso
      ? PoseGlobal.distanciaAlObjetivo(pasoReingresoXCm, pasoReingresoYCm)
      : 0.0f;
  return ControlRuta::distanciaRestanteObjetivoActivo(
      pasoErrorLongitudinalCm, distanciaEndpointCm,
      distanciaPuntoReingresoCm, recuperacionEndpointActiva,
      avanceReingreso);
}


// ===== helpers de angulo (yaw normalizado 0..360, error -180..180) =====
float errorAng360(float obj, float act) {
  float e = obj - act;
  while (e > 180.0f) e -= 360.0f;
  while (e <= -180.0f) e += 360.0f;
  return e;
}
int aproximar(float v) { return static_cast<int>(lroundf(v)); }

// ===== sensores =====
SensorSnapshot sensar() {
  SensorSnapshot s = {};
  obtenerUltimoSnapshotSensores(s);
  return s;
}
void copiarBase(int64_t dest[4], const SensorSnapshot& s) {
  dest[0]=s.pulsosFL; dest[1]=s.pulsosFR; dest[2]=s.pulsosBL; dest[3]=s.pulsosBR;
}
void deltas(const int64_t base[4], const SensorSnapshot& s, int64_t out[4]) {
  out[0] = llabs(s.pulsosFL-base[0]); out[1] = llabs(s.pulsosFR-base[1]);
  out[2] = llabs(s.pulsosBL-base[2]); out[3] = llabs(s.pulsosBR-base[3]);
}

// ===== terminar / fallar =====
void fin(TipoEvento t, const char* d) {
  frenarMotores(); fase = Fase::NINGUNA;
  ControlRuta::cancelarEpisodioRecuperacion(episodioRecuperacion);
  antiFriccionActiva = false;
  antiFriccionPulsoEncendido = false;
  antiFriccionPwmObjetivo = 0;
  reiniciarControlRumbo();
  registrarMotivoFinalizacion(d);
  if (t == EVT_COMPLETED) { estadoActual = LISTO; progresoComando = 1.0f; }
  else if (t == EVT_FAULT) { estadoActual = FALLO; }
  encolarEvento(t, seqActivo, d);
}
void fallo(const char* d) { fin(EVT_FAULT, d); }

// ============== CALIBRACION (test: rampa continua, ambos sentidos, retorno) ==============
int  signoYawCal = 1;
int  pwmCal = 0;
uint32_t ultimoRampaCalMs = 0;
uint32_t inicioMovCalMs = 0;
uint32_t inicioPruebaRotacionCalMs = 0;
uint32_t inicioAuditoriaGuardCalMs = 0;
uint32_t inicioDesbalanceCalMs = 0;
uint32_t inicioPausaReintentoCalMs = 0;
uint32_t ultimaAuditoriaMaxCalMs = 0;
bool rampaCalCongelada = false;
uint8_t intentoGuardCal = 1;
uint32_t stallMaxCalAcumMs[2] = {};
uint32_t inicioFaseMs = 0;
int64_t ticksBaseCal[4] = {};
int64_t ticksBasePruebaRotacionCal[4] = {};
float yawOrigenCalDeg = 0.0f;
float yawInicioFaseCalDeg = 0.0f;
float xInicioCalCm = 0.0f;
float yInicioCalCm = 0.0f;
int  pwmMinGiroPos = static_cast<int>(148 * PWM_SCALE_8_TO_10), pwmMinGiroNeg = static_cast<int>(148 * PWM_SCALE_8_TO_10);
DiagnosticoCalibracion diagnosticoCal = {};
bool encoderObservadoCalA[4] = {};
bool encoderObservadoCalB[4] = {};

void reiniciarDiagnosticoCalibracion() {
  diagnosticoCal = {};
  diagnosticoCal.activa = true;
  diagnosticoCal.pasosRampaTotal = ControlCalibracion::totalPasosRampa(
      CALIBRATION_PWM_START, CALIBRATION_PWM_END, CALIBRATION_PWM_STEP);
}

void actualizarDiagnosticoCalibracion(
    const int64_t deltasEncoder[4],
    const ControlCalibracion::EvaluacionEncoders& evaluacion) {
  diagnosticoCal.activa = estadoActual == CALIBRANDO;
  diagnosticoCal.pasoRampa = ControlCalibracion::pasoRampaActual(
      pwmCal, CALIBRATION_PWM_START, CALIBRATION_PWM_END,
      CALIBRATION_PWM_STEP);
  diagnosticoCal.pwmObjetivo = pwmCal;
  diagnosticoCal.intentoGuard = intentoGuardCal;
  diagnosticoCal.intentosGuardMax = CAL_GUARD_MAX_ATTEMPTS;
  diagnosticoCal.candidatoDireccion = signoYawCal;
  diagnosticoCal.signoYawEsperado = signoYawCal;
  diagnosticoCal.promedioLados[0] = evaluacion.promedioIzquierdo;
  diagnosticoCal.promedioLados[1] = evaluacion.promedioDerecho;
  diagnosticoCal.ladosValidos[0] = evaluacion.ladoIzquierdoValido;
  diagnosticoCal.ladosValidos[1] = evaluacion.ladoDerechoValido;
  diagnosticoCal.stallAcumuladoMs[0] = stallMaxCalAcumMs[0];
  diagnosticoCal.stallAcumuladoMs[1] = stallMaxCalAcumMs[1];
  for (int i = 0; i < 4; ++i) {
    diagnosticoCal.deltaEncoders[i] = deltasEncoder[i];
    diagnosticoCal.encoderResponde[i] = evaluacion.responde[i];
    diagnosticoCal.encoderAislado[i] = evaluacion.sinRespuestaAislada[i];
    diagnosticoCal.respuestaFaseA[i] = encoderObservadoCalA[i];
    diagnosticoCal.respuestaFaseB[i] = encoderObservadoCalB[i];
  }
}

void iniciarFaseCal(Fase f) {
  fase = f;
  inicioFaseMs = millis();
  inicioPruebaRotacionCalMs = 0;
  inicioAuditoriaGuardCalMs = 0;
  inicioDesbalanceCalMs = 0;
  inicioMovCalMs = 0;
  inicioPausaReintentoCalMs = 0;
  rampaCalCongelada = false;
  intentoGuardCal = 1;
  memset(ticksBasePruebaRotacionCal, 0, sizeof(ticksBasePruebaRotacionCal));
}

void calCuenta() {
  if (millis() - inicioFaseMs < CUENTA_CALIBRACION_MS) { progresoComando = min(0.10f, (millis()-inicioFaseMs)/float(CUENTA_CALIBRACION_MS)*0.10f); return; }
  const SensorSnapshot s = sensar();
  if (!s.mpu_present || s.mpu_stale || !s.mpu_calibrated) { fallo("mpu_unavailable_cal"); return; }
  yawOrigenCalDeg = normalizar360(anguloZ);
  yawInicioFaseCalDeg = yawOrigenCalDeg;
  pwmMinGiroPos=0; pwmMinGiroNeg=0;
  signoYawCal=1; pwmCal=CALIBRATION_PWM_START; ultimoRampaCalMs=millis();
  inicioMovCalMs=0; inicioPruebaRotacionCalMs=0;
  inicioAuditoriaGuardCalMs=0; inicioDesbalanceCalMs=0;
  inicioPausaReintentoCalMs=0; intentoGuardCal=1;
  rampaCalCongelada=false;
  ultimaAuditoriaMaxCalMs=0; stallMaxCalAcumMs[0]=stallMaxCalAcumMs[1]=0;
  for (int i = 0; i < 4; ++i) {
    encoderObservadoCalA[i] = false;
    encoderObservadoCalB[i] = false;
  }
  reiniciarDiagnosticoCalibracion();
  copiarBase(ticksBaseCal, s);
  iniciarFaseCal(Fase::CAL_A);
  strncpy(faseComando, "cal_a", sizeof(faseComando));
}

void calTorque(bool primera) {
  const uint32_t ahora = millis();
  if (inicioPausaReintentoCalMs) {
    frenarMotores();
    diagnosticoCal.motivoGuard = "retry_pause";
    diagnosticoCal.rampaCongelada = false;
    if (ahora - inicioPausaReintentoCalMs < CAL_GUARD_RETRY_PAUSE_MS) return;

    const SensorSnapshot reinicio = sensar();
    copiarBase(ticksBaseCal, reinicio);
    memset(ticksBasePruebaRotacionCal, 0, sizeof(ticksBasePruebaRotacionCal));
    pwmCal = CALIBRATION_PWM_START;
    ultimoRampaCalMs = ahora;
    inicioMovCalMs = 0;
    inicioPruebaRotacionCalMs = 0;
    inicioAuditoriaGuardCalMs = 0;
    inicioDesbalanceCalMs = 0;
    rampaCalCongelada = false;
    ultimaAuditoriaMaxCalMs = 0;
    stallMaxCalAcumMs[0] = stallMaxCalAcumMs[1] = 0;
    inicioPausaReintentoCalMs = 0;
    diagnosticoCal.motivoGuard = "retrying";
    return;
  }
  const SensorSnapshot s = sensar();
  int64_t d[4]; deltas(ticksBaseCal, s, d);
  const ControlCalibracion::EvaluacionEncoders evaluacion =
      ControlCalibracion::evaluarEncoders(d, CAL_TICKS_MOVIMIENTO);
  const bool ladoIzqOk = evaluacion.ladoIzquierdoValido;
  const bool ladoDerOk = evaluacion.ladoDerechoValido;
  const bool ticksOk = ladoIzqOk && ladoDerOk;

  if (ticksOk && !inicioPruebaRotacionCalMs) {
    inicioPruebaRotacionCalMs = ahora;
    // La rampa sigue buscando torque; sólo se marca congelada al confirmar.
    copiarBase(ticksBasePruebaRotacionCal, s);
  }

  int64_t dRel[4] = {};
  if (inicioPruebaRotacionCalMs) {
    deltas(ticksBasePruebaRotacionCal, s, dRel);
  }
  bool fuentesCal[4] = {};
  for (int i = 0; i < 4; ++i) fuentesCal[i] = evaluacion.responde[i];

  const float deltaYaw = errorAng360(heading360, yawInicioFaseCalDeg);
  const ControlCalibracion::EvidenciaPivot evidenciaRel =
      ControlCalibracion::evaluarPivot(
          deltaYaw, dRel, fuentesCal, CAL_TICKS_MOVIMIENTO,
          DESBALANCE_PIVOT_MAX_REL);

  if (pwmCal >= CALIBRATION_PWM_END && evidenciaRel.bilateral &&
      !evidenciaRel.equilibrado) {
    if (!inicioDesbalanceCalMs) inicioDesbalanceCalMs = ahora;
  } else if (pwmCal < CALIBRATION_PWM_END || evidenciaRel.equilibrado) {
    inicioDesbalanceCalMs = 0;
  }
  if (pwmCal >= CALIBRATION_PWM_END) {
    if (!inicioAuditoriaGuardCalMs) inicioAuditoriaGuardCalMs = ahora;
  } else {
    inicioAuditoriaGuardCalMs = 0;
  }
  const uint32_t pruebaMs = inicioAuditoriaGuardCalMs
      ? ahora - inicioAuditoriaGuardCalMs : 0;
  if (ticksOk && ControlCalibracion::signoGyroCorrecto(
          signoYawCal, s.gyro_z_filtrado_rad_s, GYRO_MOVEMENT_RAD_S)) {
    if (!inicioMovCalMs) inicioMovCalMs = ahora;
  } else if (!ticksOk || !ControlCalibracion::signoGyroCorrecto(
          signoYawCal, s.gyro_z_filtrado_rad_s, GYRO_MOVEMENT_RAD_S)) {
    inicioMovCalMs = 0;
  }
  for (int i = 0; i < 4; ++i) {
    if (primera) encoderObservadoCalA[i] |= evaluacion.responde[i];
    else encoderObservadoCalB[i] |= evaluacion.responde[i];
  }
  actualizarDiagnosticoCalibracion(d, evaluacion);
  diagnosticoCal.gyroZRadS = s.gyro_z_filtrado_rad_s;
  diagnosticoCal.deltaYawDeg = deltaYaw;
  diagnosticoCal.tiempoPruebaMs = pruebaMs;
  diagnosticoCal.rotacionConfirmada = false;
  diagnosticoCal.rampaCongelada = rampaCalCongelada;

  const uint32_t yawSostenidoMs = inicioMovCalMs ? (ahora - inicioMovCalMs) : 0;
  const uint32_t desbalanceSostenidoMs = inicioDesbalanceCalMs ? (ahora - inicioDesbalanceCalMs) : 0;

  const ControlCalibracion::ResultadoGuardPivot resultadoGuardInicial =
      ControlCalibracion::evaluarGuardPivot(
          signoYawCal, s.gyro_z_filtrado_rad_s, deltaYaw,
          evidenciaRel, pruebaMs, yawSostenidoMs, desbalanceSostenidoMs,
          GYRO_MOVEMENT_RAD_S, CAL_MOVE_SUSTAINED_MS,
          CAL_PIVOT_GUARD_WINDOW_MS,
          CAL_PIVOT_UNBALANCED_MS, GYRO_MOVEMENT_MIN_YAW_DEG);

  ControlCalibracion::ResultadoGuardPivot resultadoGuard = resultadoGuardInicial;
  if (resultadoGuard == ControlCalibracion::ResultadoGuardPivot::ESPERANDO_TICKS &&
      pwmCal >= CALIBRATION_PWM_END && ticksOk &&
      pruebaMs >= CAL_PIVOT_GUARD_WINDOW_MS) {
    resultadoGuard = ControlCalibracion::ResultadoGuardPivot::ROTACION_NO_CONFIRMADA;
  }

  if (resultadoGuard == ControlCalibracion::ResultadoGuardPivot::SIGNO_INCORRECTO) {
    diagnosticoCal.motivoGuard = "cal_yaw_sign_mismatch";
    frenarMotores();
    fallo("cal_yaw_sign_mismatch");
    return;
  }
  if (resultadoGuard == ControlCalibracion::ResultadoGuardPivot::DESBALANCEADO ||
      resultadoGuard == ControlCalibracion::ResultadoGuardPivot::ROTACION_NO_CONFIRMADA) {
    const auto accion = ControlCalibracion::decidirAccionGuardPivot(
        resultadoGuard, pwmCal, CALIBRATION_PWM_END,
        intentoGuardCal, CAL_GUARD_MAX_ATTEMPTS);
    if (accion == ControlCalibracion::AccionGuardPivot::CONTINUAR_RAMPA) {
      diagnosticoCal.motivoGuard = resultadoGuard ==
          ControlCalibracion::ResultadoGuardPivot::DESBALANCEADO
          ? "balancing_during_ramp" : "ramping_for_yaw";
      inicioAuditoriaGuardCalMs = 0;
      inicioDesbalanceCalMs = 0;
    } else if (accion == ControlCalibracion::AccionGuardPivot::PAUSAR_REINTENTO) {
      ++intentoGuardCal;
      inicioPausaReintentoCalMs = ahora;
      diagnosticoCal.motivoGuard = resultadoGuard ==
          ControlCalibracion::ResultadoGuardPivot::DESBALANCEADO
          ? "retry_unbalanced" : "retry_no_rotation";
      frenarMotores();
      return;
    } else {
      diagnosticoCal.motivoGuard = resultadoGuard ==
          ControlCalibracion::ResultadoGuardPivot::DESBALANCEADO
          ? "cal_pivot_unbalanced" : "cal_rotation_not_confirmed";
      frenarMotores();
      fallo(diagnosticoCal.motivoGuard);
      return;
    }
  }

  const bool rotacionConfirmada =
      resultadoGuard == ControlCalibracion::ResultadoGuardPivot::CONFIRMADO;
  rampaCalCongelada = rotacionConfirmada;
  diagnosticoCal.rampaCongelada = rampaCalCongelada;
  if (rotacionConfirmada) {
    diagnosticoCal.rotacionConfirmada = true;
    diagnosticoCal.motivoGuard = "confirmed";
  } else if (resultadoGuard == ControlCalibracion::ResultadoGuardPivot::CONFIRMANDO_YAW) {
    diagnosticoCal.motivoGuard = "confirming_yaw";
  } else {
    diagnosticoCal.motivoGuard = "waiting_bilateral_ticks";
  }
  const ControlCalibracion::ComandoPivot comandoCal =
      ControlCalibracion::comandoPivotCentrado(
          signoYawCal, pwmCal,
          static_cast<float>(evaluacion.promedioIzquierdo),
          static_cast<float>(evaluacion.promedioDerecho),
          KP_BALANCE_PIVOT_PWM_POR_TICK, PWM_BALANCE_PIVOT_MAX,
          PWM_TURN_MAX_LIMIT);
  if (!aplicarVelocidades(comandoCal.izquierda, comandoCal.derecha)) {
    fallo("motor_output_error");
    return;
  }

  if (rotacionConfirmada) {
    int guardado = min(PWM_TURN_MAX_LIMIT, pwmCal + PWM_CALIBRATION_MARGIN);
    if (primera) pwmMinGiroPos = guardado;
    else pwmMinGiroNeg = guardado;
    frenarMotores();
    if (primera) {
      iniciarBaseGiro(normalizar360(yawOrigenCalDeg + 25.0f), Fase::CAL_VALIDAR_25);
      progresoComando=0.35f;
      strncpy(faseComando,"cal_mas_25",sizeof(faseComando));
    } else {
      iniciarFaseCal(Fase::CAL_PAUSA_RETORNO); progresoComando=0.85f;
      strncpy(faseComando,"cal_retorno",sizeof(faseComando));
    }
    return;
  }

  if (!rampaCalCongelada && ahora - ultimoRampaCalMs >= CAL_RAMP_INTERVAL_MS) {
    ultimoRampaCalMs = ahora;
    pwmCal = min(CALIBRATION_PWM_END, pwmCal + CALIBRATION_PWM_STEP);
  }

  if (pwmCal >= CALIBRATION_PWM_END) {
    if (!ultimaAuditoriaMaxCalMs) ultimaAuditoriaMaxCalMs = ahora;
    const uint32_t lapso = ahora - ultimaAuditoriaMaxCalMs;
    ultimaAuditoriaMaxCalMs = ahora;
    if (!ladoIzqOk) stallMaxCalAcumMs[0] += lapso;
    else stallMaxCalAcumMs[0] = 0;
    if (!ladoDerOk) stallMaxCalAcumMs[1] += lapso;
    else stallMaxCalAcumMs[1] = 0;
    if (stallMaxCalAcumMs[0] >= CAL_MAX_PWM_STALL_MS) { fallo("cal_stall_left"); return; }
    if (stallMaxCalAcumMs[1] >= CAL_MAX_PWM_STALL_MS) { fallo("cal_stall_right"); return; }
  } else {
    ultimaAuditoriaMaxCalMs = 0;
  }
}

void controlarCalibracion() {
  if (estadoActual != CALIBRANDO) return;
  const SensorSnapshot s = sensar();
  if (!s.mpu_present || s.mpu_stale) { fallo("mpu_lost_cal"); return; }
  diagnosticoCal.derivaXCm = PoseGlobal.getX() - xInicioCalCm;
  diagnosticoCal.derivaYCm = PoseGlobal.getY() - yInicioCalCm;
  if (hypotf(diagnosticoCal.derivaXCm, diagnosticoCal.derivaYCm) >
      DERIVA_CENTRO_CAL_MAX_CM) {
    diagnosticoCal.motivoGuard = "cal_origin_drift";
    frenarMotores();
    fallo("cal_origin_drift");
    return;
  }

  switch (fase) {
    case Fase::CAL_CUENTA: calCuenta(); break;
    case Fase::CAL_A: calTorque(true); break;
    case Fase::CAL_VALIDAR_25: controlarGiro(); break;
    case Fase::CAL_PAUSA:
      frenarMotores();
      if (millis()-inicioFaseMs >= PAUSA_RETORNO_CAL_MS) {
        signoYawCal = -1;
        pwmCal=CALIBRATION_PWM_START;
        ultimaAuditoriaMaxCalMs=0;
        stallMaxCalAcumMs[0]=stallMaxCalAcumMs[1]=0;
        copiarBase(ticksBaseCal,s);
        yawInicioFaseCalDeg = heading360;
        ultimoRampaCalMs=millis();
        iniciarFaseCal(Fase::CAL_B);
        strncpy(faseComando,"cal_b",sizeof(faseComando));
        progresoComando=0.60f;
      }
      break;
    case Fase::CAL_B: calTorque(false); break;
    case Fase::CAL_PAUSA_RETORNO:
      frenarMotores();
      if (millis()-inicioFaseMs >= PAUSA_RETORNO_CAL_MS) {
        float retorno = normalizar360(yawOrigenCalDeg);
        iniciarBaseGiro(retorno, Fase::CAL_RETORNO);
      }
      break;
    case Fase::CAL_RETORNO:
      controlarGiro();  // gestiona su propio fin via faseRetorno
      break;
    default: break;
  }
}

// ============== GIRO (generico, hacia heading absoluto, usa reinicio/retry) ==============
float giroObjetivo = 0.0f;
Fase faseRetornoGiro = Fase::NINGUNA;
int   pwmGiroAct = 0;
int   signoGiroApl = 0;
bool  movGiroConfirmado = false;
bool  watchdogGiroArmado = false;
bool  giroEnTol = false;
uint8_t intentoGiro = 1;
uint32_t inicioIntentoGiroMs = 0;
uint32_t inicioGiroTotalMs = 0;
uint32_t estableGiroDesdeMs = 0;
bool pausaReintentoGiroCal = false;
uint32_t inicioPausaReintGiroMs = 0;
int64_t ticksBaseGiroLocal[4] = {};
int64_t ticksLadoGiroAnt[2] = {};
uint32_t ultimoPulsoLadoGiroMs[2] = {};
uint32_t ultimoCtrlGiroMs = 0;
uint32_t ultimoAumentoTorqueGiroMs = 0;
int pwmBusquedaGiro = 0;
int pwmBoostFrenado = 0;
float yawInicioGiroDeg = 0.0f;
uint32_t inicioDesbalanceGiroMs = 0;
uint32_t inicioSignoIncorrectoGiroMs = 0;

void iniciarBaseGiro(float objetivoDeg, Fase retorno) {
  reiniciarControlRumbo();
  const SensorSnapshot s = sensar();
  copiarBase(ticksBaseGiroLocal, s);
  yawInicioGiroDeg = heading360;
  inicioDesbalanceGiroMs = 0;
  inicioSignoIncorrectoGiroMs = 0;
  PoseGlobal.iniciarMedicionTraslacionGiro();
  giroObjetivo = objetivoDeg;
  giroEnTol = false; pwmGiroAct=0; signoGiroApl=0; movGiroConfirmado=false; watchdogGiroArmado=false;
  intentoGiro=1; inicioIntentoGiroMs=millis(); inicioGiroTotalMs=millis();
  estableGiroDesdeMs=0; pausaReintentoGiroCal=false;
  ticksLadoGiroAnt[0]=ticksLadoGiroAnt[1]=0;
  ultimoPulsoLadoGiroMs[0]=ultimoPulsoLadoGiroMs[1]=millis();
  ultimoCtrlGiroMs=0;
  ultimoAumentoTorqueGiroMs=millis();
  pwmBusquedaGiro=PWM_TURN_START;
  pwmBoostFrenado=0;
  faseRetornoGiro = retorno;
  fase = retorno;  // fase global sigue al giro
  if (retorno == Fase::GIRO_INICIAL) strncpy(faseComando,"giro_ini",sizeof(faseComando));
  else if (retorno == Fase::GIRO_FINAL) strncpy(faseComando,"giro_fin",sizeof(faseComando));
  else if (retorno == Fase::GIRO_RECUPERACION) strncpy(faseComando,"recup",sizeof(faseComando));
  else if (retorno == Fase::GIRO_A_REINGRESO) strncpy(faseComando,"recenter_turn",sizeof(faseComando));
  else if (retorno == Fase::GIRO_A_RUMBO) strncpy(faseComando,"recenter_heading",sizeof(faseComando));
  else if (retorno == Fase::GIRO_REALINEAR_EJE) strncpy(faseComando,"realign_axis",sizeof(faseComando));
  else if (retorno == Fase::GIRO_SOLO) strncpy(faseComando,"giro_solo",sizeof(faseComando));
  else if (retorno == Fase::CAL_RETORNO) strncpy(faseComando,"cal_ret",sizeof(faseComando));
}

void reintentarGiro(const char* motivo) {
  frenarMotores(); pwmGiroAct=0; signoGiroApl=0; movGiroConfirmado=false; watchdogGiroArmado=false; giroEnTol=false;
  pwmBoostFrenado=0;
  if (intentoGiro >= TURN_MAX_ATTEMPTS) { fallo(motivo); return; }
  ++intentoGiro;
  pausaReintentoGiroCal = true;
  inicioPausaReintGiroMs = millis();
}

void controlarGiro() {
  if (fase != Fase::GIRO_INICIAL && fase != Fase::GIRO_FINAL &&
      fase != Fase::GIRO_RECUPERACION && fase != Fase::GIRO_A_REINGRESO &&
      fase != Fase::GIRO_A_RUMBO && fase != Fase::GIRO_REALINEAR_EJE &&
      fase != Fase::GIRO_SOLO &&
      fase != Fase::CAL_VALIDAR_25 && fase != Fase::CAL_RETORNO) return;
  uint32_t ahora = millis();
  if (ahora - ultimoCtrlGiroMs < TURN_CONTROL_PERIOD_MS) return;
  ultimoCtrlGiroMs = ahora;

  if (pausaReintentoGiroCal) {
    frenarMotores();
    if (ahora - inicioPausaReintGiroMs < TURN_RETRY_PAUSE_MS) return;
    pausaReintentoGiroCal = false;
    const SensorSnapshot s = sensar();
    copiarBase(ticksBaseGiroLocal, s);
    yawInicioGiroDeg = heading360;
    inicioDesbalanceGiroMs = 0;
    inicioSignoIncorrectoGiroMs = 0;
    ticksLadoGiroAnt[0]=ticksLadoGiroAnt[1]=0;
    ultimoPulsoLadoGiroMs[0]=ultimoPulsoLadoGiroMs[1]=ahora;
    watchdogGiroArmado=false;
    inicioIntentoGiroMs = ahora;
    ultimoAumentoTorqueGiroMs=ahora;
    pwmBusquedaGiro=PWM_TURN_START;
    pwmBoostFrenado=0;
  }

  const SensorSnapshot s = sensar();
  float error = errorAng360(giroObjetivo, heading360);
  float errorAbs = fabsf(error);
  const int signoEsperado = error > 0 ? 1 : -1;
  int torqueCalibrado = signoEsperado > 0 ? pwmMinGiroPos : pwmMinGiroNeg;
  if (torqueCalibrado == 0) {
    torqueCalibrado = pwmMinGiroPos > 0 ? pwmMinGiroPos : (pwmMinGiroNeg > 0 ? pwmMinGiroNeg : PWM_TURN_START);
  }
  if (!watchdogGiroArmado && torqueCalibrado > 0 && pwmGiroAct >= torqueCalibrado) {
    watchdogGiroArmado = true;
    ultimoPulsoLadoGiroMs[0]=ultimoPulsoLadoGiroMs[1]=ahora;
  }

  // Mientras el torque siga incrementándose en rampa para vencer fricción y no haya movimiento confirmado,
  // mantener fresco el temporizador de pulso para evitar falsos stalls durante la búsqueda de torque.
  if (!movGiroConfirmado && pwmBusquedaGiro < PWM_TURN_MAX_LIMIT) {
    ultimoPulsoLadoGiroMs[0] = ultimoPulsoLadoGiroMs[1] = ahora;
  }
  // En aproximación fina (< 5°), el control por micro-pulsos intermitentes es gobernado
  // por el MPU como autoridad angular principal. Mantener frescos los temporizadores
  // para evitar falsos stalls de encoder entre ranuras durante los micro-pulsos.
  if (errorAbs <= TURN_HYBRID_THRESHOLD_DEG) {
    ultimoPulsoLadoGiroMs[0] = ultimoPulsoLadoGiroMs[1] = ahora;
  }

  int64_t d[4]; deltas(ticksBaseGiroLocal, s, d);
  const bool* mascaraEncoders = (fase == Fase::CAL_VALIDAR_25 || fase == Fase::CAL_RETORNO)
      ? nullptr
      : encoderConfiableGlobal;
  const ControlCalibracion::EvaluacionEncoders evaluacionGiro =
      ControlCalibracion::evaluarEncoders(
          d, CAL_TICKS_MOVIMIENTO, mascaraEncoders);
  bool fuentesGiro[4] = {};
  for (int i = 0; i < 4; ++i) fuentesGiro[i] = evaluacionGiro.responde[i];

  if (fase == Fase::CAL_VALIDAR_25) {
    for (int i = 0; i < 4; ++i) {
      if (evaluacionGiro.responde[i]) {
        encoderObservadoCalA[i] = true;
        diagnosticoCal.respuestaFaseA[i] = true;
      }
    }
  } else if (fase == Fase::CAL_RETORNO) {
    for (int i = 0; i < 4; ++i) {
      if (evaluacionGiro.responde[i]) {
        encoderObservadoCalB[i] = true;
        diagnosticoCal.respuestaFaseB[i] = true;
      }
    }
  }

  const ControlCalibracion::EvidenciaPivot evidenciaPivot =
      ControlCalibracion::evaluarPivot(
          errorAng360(heading360, yawInicioGiroDeg), d, fuentesGiro,
           CAL_TICKS_MOVIMIENTO, DESBALANCE_PIVOT_MAX_REL);
  const int signoMovimientoEsperado = signoGiroApl != 0 ? signoGiroApl : signoEsperado;
  if (ControlCalibracion::signoGyroContrario(
          signoMovimientoEsperado, s.gyro_z_filtrado_rad_s,
          GYRO_MOVEMENT_RAD_S) && pwm_aplicado_L != 0 && pwm_aplicado_R != 0) {
    if (!inicioSignoIncorrectoGiroMs) inicioSignoIncorrectoGiroMs = ahora;
    if (ahora - inicioSignoIncorrectoGiroMs >= CAL_MOVE_SUSTAINED_MS) {
      fallo("turn_yaw_sign_mismatch");
      return;
    }
  } else if (!ControlCalibracion::signoGyroContrario(
                 signoMovimientoEsperado, s.gyro_z_filtrado_rad_s,
                 GYRO_MOVEMENT_RAD_S)) {
    inicioSignoIncorrectoGiroMs = 0;
  }
  if (ControlCalibracion::signoGyroCorrecto(
          signoMovimientoEsperado, s.gyro_z_filtrado_rad_s,
          GYRO_MOVEMENT_RAD_S) &&
      evidenciaPivot.bilateral) {
    movGiroConfirmado = true;
  }
  const int64_t ladoTicks[2] = {
      static_cast<int64_t>(lroundf(evidenciaPivot.izquierda)),
      static_cast<int64_t>(lroundf(evidenciaPivot.derecha))};
  for (int i=0; i<2; ++i) {
    if (ladoTicks[i] != ticksLadoGiroAnt[i]) { ticksLadoGiroAnt[i]=ladoTicks[i]; ultimoPulsoLadoGiroMs[i]=ahora; }
    else if (watchdogGiroArmado && ahora-ultimoPulsoLadoGiroMs[i] > TURN_STALL_MS) { reintentarGiro(i==0?"turn_stall_left":"turn_stall_right"); return; }
  }
  if (movGiroConfirmado && evidenciaPivot.bilateral &&
      !evidenciaPivot.equilibrado) {
    if (!inicioDesbalanceGiroMs) inicioDesbalanceGiroMs = ahora;
    if (ahora - inicioDesbalanceGiroMs >= 1500) {
      reintentarGiro("turn_pivot_unbalanced");
      return;
    }
  } else {
    inicioDesbalanceGiroMs = 0;
  }

  if (ahora - inicioGiroTotalMs > TURN_TIMEOUT_MS) { fallo("turn_timeout_total"); return; }
  if (ahora - inicioIntentoGiroMs > TURN_ATTEMPT_TIMEOUT_MS) { reintentarGiro("turn_timeout_attempt"); return; }

  // --- latch de tolerancia ---
  const float tolGiro = (fase == Fase::CAL_RETORNO) ? TOLERANCIA_CALIBRACION_DEG : TOLERANCIA_GIRO_DEG;
  if (errorAbs <= tolGiro) {
    frenarMotores(); pwmGiroAct=0; signoGiroApl=0; giroEnTol=true;
    if (fabsf(s.gyro_z_filtrado_rad_s) > 0.02f) { estableGiroDesdeMs=0; return; }
    if (!estableGiroDesdeMs) estableGiroDesdeMs = ahora;
    if (ahora - estableGiroDesdeMs >= TURN_SETTLE_MS) {
      giroEnTol = false;
      estableGiroDesdeMs = 0;
      completarGiro();
    }
    return;
  }
  if (giroEnTol) {
    if (errorAbs > TURN_REACTIVATION_DEG) {
      giroEnTol = false;
      estableGiroDesdeMs = 0;
      reintentarGiro("turn_drifted");
      return;
    }
    // Si derivo fuera de tolGiro pero dentro de TURN_REACTIVATION_DEG:
    // mientras aun se mueva por inercia, mantener freno.
    if (fabsf(s.gyro_z_filtrado_rad_s) > 0.02f) {
      frenarMotores();
      estableGiroDesdeMs = 0;
      return;
    }
    // Si ya se detuvo fisicamente y quedo fuera de tolGiro, liberar el latch
    // para que la aproximacion hibrida (micro-pulsos) vuelva a converger.
    giroEnTol = false;
    estableGiroDesdeMs = 0;
  }
  estableGiroDesdeMs = 0;

  // --- calcular PWM ---
  int signoDeseado = error>0?1:-1;
  int minimo = signoDeseado>0 ? pwmMinGiroPos : pwmMinGiroNeg;
  if (minimo == 0) {
    minimo = pwmMinGiroPos > 0 ? pwmMinGiroPos : (pwmMinGiroNeg > 0 ? pwmMinGiroNeg : 0);
    if (minimo == 0 && (fase == Fase::CAL_VALIDAR_25 || fase == Fase::CAL_RETORNO)) {
      minimo = PWM_TURN_START;
    }
    if (minimo == 0) { reintentarGiro("turn_not_calibrated"); return; }
  }
  int pwmLejos = max(PWM_TURN_START, min(PWM_TURN_MAX_LIMIT, minimo+PWM_TURN_FAR_MARGIN));
  int pwmCerca = min(PWM_TURN_MAX_LIMIT, minimo+PWM_TURN_NEAR_MARGIN);
  int pwmObj = pwmLejos;

  const bool detectadoSinMovimiento = (fabsf(s.gyro_z_filtrado_rad_s) < GYRO_MOVEMENT_RAD_S);

  if (errorAbs < TURN_BRAKING_ZONE_DEG) {
    pwmObj = pwmCerca + aproximar((pwmLejos - pwmCerca) * errorAbs / TURN_BRAKING_ZONE_DEG);

    if (errorAbs > TURN_HYBRID_THRESHOLD_DEG) {
      // --- MODO 1: Rampa Adaptativa Rápida (5.0° a 25.0°) ---
      if (detectadoSinMovimiento) {
        if (ahora - ultimoAumentoTorqueGiroMs >= TURN_RAMP_ADAPTIVE_INTERVAL_MS) {
          ultimoAumentoTorqueGiroMs = ahora;
          pwmBoostFrenado = min(PWM_TURN_MAX_LIMIT - pwmObj, pwmBoostFrenado + static_cast<int>(5 * PWM_SCALE_8_TO_10));
        }
      } else {
        if (ahora - ultimoAumentoTorqueGiroMs >= TURN_RAMP_ADAPTIVE_INTERVAL_MS && pwmBoostFrenado > 0) {
          ultimoAumentoTorqueGiroMs = ahora;
          pwmBoostFrenado = max(0, pwmBoostFrenado - static_cast<int>(2 * PWM_SCALE_8_TO_10));
        }
      }
      pwmObj = min(PWM_TURN_MAX_LIMIT, pwmObj + pwmBoostFrenado);
    } else {
      // --- MODO 2: aproximación fina continua (0.0° a 5.0°) ---
      pwmObj = max(minimo + static_cast<int>(6 * PWM_SCALE_8_TO_10), PWM_TURN_START);
      if (detectadoSinMovimiento && ahora - ultimoAumentoTorqueGiroMs >= TURN_RAMP_ADAPTIVE_INTERVAL_MS) {
        ultimoAumentoTorqueGiroMs = ahora;
        pwmBoostFrenado = min(PWM_TURN_MAX_LIMIT - pwmObj,
                              pwmBoostFrenado + static_cast<int>(3 * PWM_SCALE_8_TO_10));
      }
      pwmObj = min(PWM_TURN_MAX_LIMIT, pwmObj + pwmBoostFrenado);
    }
  } else {
    pwmBoostFrenado = 0;
  }

  if (!movGiroConfirmado) {
    if (ahora - ultimoAumentoTorqueGiroMs >= CAL_RAMP_INTERVAL_MS) {
      ultimoAumentoTorqueGiroMs = ahora;
      pwmBusquedaGiro = min(PWM_TURN_MAX_LIMIT, pwmBusquedaGiro + CALIBRATION_PWM_STEP);
    }
    pwmObj = max(pwmObj, pwmBusquedaGiro);
  }

  // --- slew con protección de inversión (Motores.cpp añade 250 ms) ---
  if (signoGiroApl!=0 && signoDeseado!=signoGiroApl && pwmGiroAct>0) {
    pwmGiroAct = max(0, pwmGiroAct - PWM_TURN_SLEW_STEP);
    if (pwmGiroAct==0) signoGiroApl=0;
  } else {
    if (signoGiroApl==0) signoGiroApl=signoDeseado;
    int paso = movGiroConfirmado ? PWM_TURN_SLEW_STEP : PWM_TURN_START_SLEW_STEP;
    if (pwmGiroAct<pwmObj) pwmGiroAct=min(pwmObj, pwmGiroAct+paso);
    else pwmGiroAct=max(pwmObj, pwmGiroAct-PWM_TURN_SLEW_STEP);
  }
  bool pulsoGiroEncendido = true;
  if (errorAbs <= TURN_HYBRID_THRESHOLD_DEG) {
    const uint32_t periodoPulso = TURN_PULSE_ON_MS + TURN_PULSE_OFF_MS;
    const uint32_t tCiclo = ahora % periodoPulso;
    pulsoGiroEncendido = (tCiclo < TURN_PULSE_ON_MS);
  }

  if (signoGiroApl != 0 && pwmGiroAct > 0 && pulsoGiroEncendido) {
    // El MPU manda sobre el ángulo; PCNT ajusta únicamente el balance bilateral
    // para que el centro no se traslade durante la contra-rotación.
    const ControlCalibracion::ComandoPivot comando =
        ControlCalibracion::comandoPivotCentrado(
            signoGiroApl, pwmGiroAct, evidenciaPivot.izquierda,
            evidenciaPivot.derecha, KP_BALANCE_PIVOT_PWM_POR_TICK,
            PWM_BALANCE_PIVOT_MAX, PWM_TURN_MAX_LIMIT);
    if (!aplicarVelocidades(comando.izquierda, comando.derecha)) {
      fallo("motor_output_error");
      return;
    }
  } else frenarMotores();

  if (ControlCalibracion::signoGyroCorrecto(
          signoMovimientoEsperado, s.gyro_z_filtrado_rad_s,
          GYRO_MOVEMENT_RAD_S) &&
      evidenciaPivot.bilateral) {
    movGiroConfirmado = true;
  } else if (!movGiroConfirmado && ahora - inicioIntentoGiroMs > 5000) {
    reintentarGiro("turn_no_progress");
  }
}

void completarGiro() {
  frenarMotores();
  giroEnTol = false;
  estableGiroDesdeMs = 0;
  Fase ret = faseRetornoGiro;
  faseRetornoGiro = Fase::NINGUNA;
  if (ret == Fase::GIRO_INICIAL) { iniciarAvance(false); }
  else if (ret == Fase::GIRO_RECUPERACION) { iniciarAvance(true); }
  else if (ret == Fase::GIRO_A_REINGRESO) { iniciarAvanceReingreso(); }
  else if (ret == Fase::GIRO_A_RUMBO) { reanudarTrasRecentrado(); }
  else if (ret == Fase::GIRO_REALINEAR_EJE) { reanudarTrasRealineacion(); }
  else if (ret == Fase::GIRO_FINAL) { iniciarVerificacionFinal(); }
  else if (ret == Fase::GIRO_SOLO) { fin(EVT_COMPLETED, "turn_ok"); }
  else if (ret == Fase::CAL_VALIDAR_25) {
    iniciarFaseCal(Fase::CAL_PAUSA);
    progresoComando=0.55f;
    strncpy(faseComando,"cal_pausa",sizeof(faseComando));
  }
  else if (ret == Fase::CAL_RETORNO) {
    frenarMotores();
    bool confiables[4] = {};
    const ControlInicializacionPCNT::Canal* pcnt = diagnosticoInicializacionPCNT();
    for (int i = 0; i < 4; ++i)
      confiables[i] = pcnt[i].inicializado && (encoderObservadoCalA[i] || encoderObservadoCalB[i]);
    if (!ControlSeguridad::fuentesPorLadoValidas(confiables)) {
      fallo("cal_sensor_unstable_side");
      return;
    }
    const float derivaX = PoseGlobal.getX() - xInicioCalCm;
    const float derivaY = PoseGlobal.getY() - yInicioCalCm;
    const float errorYaw = errorAng360(yawOrigenCalDeg, heading360);
    if (!ControlCalibracion::retornoAlOrigenAceptable(
            derivaX, derivaY, errorYaw, DERIVA_CENTRO_CAL_MAX_CM,
            TOLERANCIA_CALIBRACION_DEG)) {
      fallo("cal_origin_drift");
      return;
    }
    WatchdogSeguridad.aplicarClasificacionEncoders(confiables);
    const SensorSnapshot s = sensar();
    robotCalibrado = true;
    diagnosticoCal.motivoGuard = "cal_ok";
    // La calibración valida el pivote sin convertirlo en un nuevo origen:
    // conserva la pose XY previa y sólo reancla los pulsos/yaw de referencia.
    PoseGlobal.fijarPoseConPulsos(
        xInicioCalCm, yInicioCalCm, 0.0f,
        s.pulsosFL, s.pulsosFR, s.pulsosBL, s.pulsosBR);
    resetOrientacionIMU();
    fin(EVT_COMPLETED, "cal_ok");
  }
}

// ============== AVANCE RECTO (fusion encoders + recuperacion rumbo + compensacion der) ==============
float  rumboObjetivoDeg = 0.0f;
float  distObjetivoCm = 0.0f;
float  distAcumuladaCm = 0.0f;
bool   conservarAcumulado = false;
int64_t ticksBaseAvance[4] = {};
uint32_t inicioAvanceMs = 0;
int64_t ticksLadoAvAnt[2] = {};
uint32_t ultimoPulsoLadoAvMs[2] = {};
uint32_t inicioErrorRumboMs = 0;
uint8_t intentosRecup = 0;
uint8_t saludEnc[4] = {};
uint32_t inicioOutlierEncMs[4] = {};
int64_t ticksPausaClasif[4] = {};
uint32_t inicioPausaReevalMs = 0;
float errorRumboMaxTramo = 0.0f;
uint32_t inicioFaseAntiFriccionMs = 0;
int64_t ticksBaseAntiFriccion[4] = {};

// --- Variables para modo de micro-pulsos de alto par gobernados por tiempo con interlocks ---
enum class PulsoDriveEstado : uint8_t { ON, OFF };
PulsoDriveEstado pulsoDriveEstado = PulsoDriveEstado::ON;
uint32_t inicioPulsoDriveMs = 0;
int64_t ticksBasePulsoOff[4] = {};
uint8_t pulsosConsecutivosSinTicks = 0;
float pasoDistanciaPorPulsoCm = 1.0f;

void reanudarTrasRecentrado() {
  const auto estadoEpisodio = ControlRuta::procesarEpisodioRecuperacion(
      episodioRecuperacion, millis(), RECENTER_TIMEOUT_MS, true);
  if (estadoEpisodio == ControlRuta::EstadoEpisodioRecuperacion::VENCIDO) {
    fallo("recenter_timeout");
    return;
  }
  direccionTraslacion = direccionTraslacionPlanificada;
  pasoEnReversa = direccionTraslacion < 0;
  pasoRumboTrayectoDeg = pasoHeading;
  pasoRumboCuerpoDeg = ControlRuta::rumboCuerpoParaTrayecto(
      pasoHeading, direccionTraslacion);
  rumboObjetivoDeg = pasoRumboCuerpoDeg;
  actualizarErroresTrayectoria();
  distAcumuladaCm = constrain(
      distanciaPlanificadaCm - pasoErrorLongitudinalCm,
      0.0f, distanciaPlanificadaCm);
  ControlRuta::iniciarSeguimientoProgreso(
      seguimientoProgreso, distanciaRestanteObjetivoActivo(false));
  strncpy(pasoFaseRecentrado, "resumed", sizeof(pasoFaseRecentrado));
  iniciarAvance(true);
}

void reanudarTrasRealineacion() {
  direccionTraslacion = direccionTraslacionPlanificada;
  pasoEnReversa = direccionTraslacion < 0;
  pasoRumboTrayectoDeg = pasoHeading;
  pasoRumboCuerpoDeg = ControlRuta::rumboCuerpoParaTrayecto(
      pasoHeading, direccionTraslacion);
  rumboObjetivoDeg = pasoRumboCuerpoDeg;
  strncpy(pasoFaseRecentrado, "realigned", sizeof(pasoFaseRecentrado));
  inicioDesviacionLateralMs = 0;
  inicioLateralExtremoMs = 0;
  inicioCrecimientoLateralMs = 0;
  actualizarErroresTrayectoria();
  mejorErrorLateralAbsCm = fabsf(pasoErrorLateralCm);
  ControlRuta::iniciarSeguimientoProgreso(
      seguimientoProgreso, distanciaRestanteObjetivoActivo(false));
  errorLateralAnteriorValido = false;
  iniciarAvance(true);
}

void actualizarVentanaSupervision(float distanciaMedidaCm) {
  if (distanciaMedidaCm < 0.0f) distanciaMedidaCm = 0.0f;
  while (distanciaMedidaCm >= pasoVentanaInicioCm + RECENTER_MONITOR_WINDOW_CM &&
         pasoVentanaInicioCm + RECENTER_MONITOR_WINDOW_CM <= distanciaPlanificadaCm) {
    pasoVentanaInicioCm += RECENTER_MONITOR_WINDOW_CM;
    ++pasoVentanaIndice;
  }
  pasoVentanaProgresoCm = fmaxf(0.0f, distanciaMedidaCm - pasoVentanaInicioCm);
}

bool progresoLongitudinalIncorrecto(uint32_t ahora) {
  return ControlRuta::actualizarSeguimientoProgreso(
      seguimientoProgreso,
      distanciaRestanteObjetivoActivo(fase == Fase::AVANCE_A_EJE), ahora,
      WRONG_WAY_GROWTH_CM, WRONG_WAY_PERSIST_MS);
}

bool vigilarDesviacionMaxima() {
  if (!tieneTargetEspacial) return false;
  actualizarErroresTrayectoria();
  const uint32_t ahora = millis();
  if (fabsf(pasoErrorLateralCm) > RECENTER_MAX_LATERAL_CM) {
    if (!inicioLateralExtremoMs) inicioLateralExtremoMs = ahora;
    if (ahora - inicioLateralExtremoMs >= RECENTER_TRIGGER_MS) {
      fallo("recenter_out_of_range");
      return true;
    }
  } else {
    inicioLateralExtremoMs = 0;
  }
  return false;
}

void iniciarRealineacionEje(float distanciaActualCm) {
  frenarMotores();
  reiniciarControlRumbo();
  // Congelar la traslación ya observada antes del giro. El nuevo baseline se
  // toma al reanudar, así los ticks del pivote no se cuentan ni se pierde lo
  // recorrido antes del cruce.
  distAcumuladaCm = fmaxf(0.0f, distanciaActualCm);
  pasoGiroHaciaLineaDeg = 0.0f;
  pasoGiroRetornoDeg = 0.0f;
  strncpy(pasoDisparadorRecentrado, "axis_cross_low", sizeof(pasoDisparadorRecentrado));
  strncpy(pasoFaseRecentrado, "realign_pause", sizeof(pasoFaseRecentrado));
  strncpy(pasoDecisionRecuperacion, "realign_axis", sizeof(pasoDecisionRecuperacion));
  errorLateralAnteriorValido = false;
  rumboReingresoCuerpoDeg = ControlRuta::rumboCuerpoParaTrayecto(
      pasoHeading, direccionTraslacionPlanificada);
  inicioFaseRecentradoMs = millis();
  // No se bloquea el súper-ciclo: la pausa se consume como una fase explícita
  // y el giro sólo se habilita después del asentamiento breve.
  fase = Fase::PAUSA_RECENTRADO;
  strncpy(faseComando, "realign_pause", sizeof(faseComando));
}

void actualizarErroresTrayectoria() {
  if (!tieneTargetEspacial) return;
  const ControlRuta::ErroresTrayectoria errores = ControlRuta::calcularErroresTrayectoria(
      PoseGlobal.getX(), PoseGlobal.getY(), pasoTargetX, pasoTargetY,
      pasoHeading, distanciaPlanificadaCm);
  pasoErrorLongitudinalCm = errores.longitudinalCm;
  pasoErrorLateralCm = errores.lateralCm;
  pasoErrorEuclidianoCm = errores.euclidianoCm;
}

bool objetivoAbsolutoAlcanzado() {
  actualizarErroresTrayectoria();
  return pasoObjetivoAbsoluto && ControlRuta::endpointAceptable(
      pasoErrorLateralCm, pasoErrorEuclidianoCm,
      errorAng360(pasoRumboFinalDeg, heading360), TOLERANCIA_ENDPOINT_CM, TOLERANCIA_GIRO_DEG);
}

float mediana4(const int64_t v[4]) {
  return ControlSeguridad::medianaCuatro(v);
}
bool hayPorLado() { return ControlSeguridad::fuentesPorLadoValidas(encoderConfiableGlobal); }
float promedioLado(const int64_t v[4], bool izq) {
  return ControlSeguridad::promedioConfiableLado(v, encoderConfiableGlobal, izq);
}
float estimarTicksAvance(const int64_t v[4]) {
  if (!hayPorLado()) return -1.0f;
  // La mediana decide que canales son coherentes; la distancia siempre se
  // calcula como promedio de las fuentes confiables de cada lado.
  return 0.5f*(promedioLado(v,true)+promedioLado(v,false));
}
void resetConfEncoders() {
  WatchdogSeguridad.prepararRevalidacionEncoders();
  for (int i = 0; i < 4; ++i) {
    saludEnc[i] = encoderConfiableGlobal[i] ? 0 : 2;
    inicioOutlierEncMs[i] = 0;
  }
  resetFiltrosEncoder();
}

void iniciarAvance(bool conservar) {
  if (ControlRuta::tramoRequiereModoPulsado(pasoDistanciaCm, PULSE_DRIVE_THRESHOLD_CM)) {
    iniciarAvancePulsado(conservar);
    return;
  }
  const SensorSnapshot s = sensar();
  if (!conservar) {
    distAcumuladaCm = 0.0f;
    intentosRecup = 0;
    inicioDesviacionLateralMs = 0;
    inicioLateralExtremoMs = 0;
    inicioCrecimientoLateralMs = 0;
    pasoVentanaInicioCm = 0.0f;
    pasoVentanaIndice = 0;
    pasoVentanaProgresoCm = 0.0f;
    errorLateralAnteriorValido = false;
  }
  reiniciarControlRumbo();
  pasoEnReversa = direccionTraslacion < 0;
  // Una recuperación o reevaluación debe preservar los canales que ya fueron
  // descartados. Sólo un paso nuevo vuelve a dar oportunidad a los cuatro.
  if (!conservar) resetConfEncoders();
  antiFriccionActiva = false;
  antiFriccionPulsoEncendido = false;
  antiFriccionPulsoIndice = 0;
  antiFriccionPwmObjetivo = 0;
  antiFriccionMovimientoConfirmado = false;
  conservarAcumulado = conservar;
  distObjetivoCm = pasoDistanciaCm;  // global del paso
  pasoDistanciaObjetivoCm = distObjetivoCm;
  rumboObjetivoDeg = pasoRumboCuerpoDeg;
  copiarBase(ticksBaseAvance, s);
  inicioAvanceMs = millis();
  ticksLadoAvAnt[0] = ticksLadoAvAnt[1] = 0;
  ultimoPulsoLadoAvMs[0] = ultimoPulsoLadoAvMs[1] = millis();
  inicioErrorRumboMs = 0; errorRumboMaxTramo = 0.0f;
  pasoDistanciaRestanteCm = distObjetivoCm;
  pasoFrenoPrevistoCm = 0.0f;
  pasoArrastreFrenoCm = 0.0f;
  pasoAsentamientoMs = 0;
  pasoRampaReversaMs = 0;
  strncpy(pasoLadoFrenoRumbo, "none", sizeof(pasoLadoFrenoRumbo));
  if (tieneTargetEspacial) {
    distTargetMinimaCm = PoseGlobal.distanciaAlObjetivo(pasoTargetX, pasoTargetY);
    actualizarErroresTrayectoria();
    mejorErrorLateralAbsCm = fabsf(pasoErrorLateralCm);
    ControlRuta::iniciarSeguimientoProgreso(
        seguimientoProgreso, distanciaRestanteObjetivoActivo(false));
    pasoUmbralLateralCm = ControlRuta::umbralLateralSegmento(
        distanciaPlanificadaCm, RECENTER_LATERAL_RATIO,
        RECENTER_LATERAL_MIN_CM);
    pasoProporcionLateral = ControlRuta::proporcionLateralSegmento(
        pasoErrorLateralCm, distanciaPlanificadaCm);
    actualizarVentanaSupervision(distAcumuladaCm);
    errorLateralAnteriorCm = pasoErrorLateralCm;
    errorLateralAnteriorValido = true;
  }
  fase = Fase::AVANCE;
  strncpy(faseComando, "avance", sizeof(faseComando));
}

void iniciarRecentrado(const char* disparador, float distanciaActualCm) {
  const bool iniciaDesdeRuta = fase == Fase::AVANCE;
  ControlRuta::abrirEpisodioRecuperacion(episodioRecuperacion, millis());
  actualizarErroresTrayectoria();
  if (pasoIntentoRecentrado >= RECENTER_MAX_ATTEMPTS) {
    fallo("recenter_not_converged");
    return;
  }
  const ControlRuta::PuntoReingreso punto = ControlRuta::calcularPuntoReingreso(
      PoseGlobal.getX(), PoseGlobal.getY(), pasoTargetX, pasoTargetY,
      pasoHeading, distanciaPlanificadaCm, pasoErrorLateralCm,
      RECENTER_LOOKAHEAD_MIN_CM);
  const float rumboRetornoCuerpoDeg = ControlRuta::rumboCuerpoParaTrayecto(
      pasoHeading, direccionTraslacionPlanificada);
  const ControlRuta::CandidatoReingreso candidatoAvance =
      ControlRuta::evaluarCandidatoReingreso(
          PoseGlobal.getX(), PoseGlobal.getY(), punto,
          pasoHeading, distanciaPlanificadaCm, heading360, 1,
          rumboRetornoCuerpoDeg);
  const ControlRuta::CandidatoReingreso candidatoReversa =
      ControlRuta::evaluarCandidatoReingreso(
          PoseGlobal.getX(), PoseGlobal.getY(), punto,
          pasoHeading, distanciaPlanificadaCm, heading360, -1,
          rumboRetornoCuerpoDeg);
  const ControlRuta::CandidatoReingreso candidato =
      ControlRuta::elegirCandidatoReingreso(candidatoAvance, candidatoReversa);
  pasoReingresoXCm = punto.xCm;
  pasoReingresoYCm = punto.yCm;
  distanciaReingresoCm = candidato.distanciaCm;
  if (!isfinite(distanciaReingresoCm) || distanciaReingresoCm < 0.5f ||
      distanciaReingresoCm > STEP_MAX_DISTANCE_CM) {
    fallo("recenter_out_of_range");
    return;
  }
  direccionReingreso = candidato.direccion;
  rumboReingresoTrayectoDeg = candidato.rumboTrayectoDeg;
  rumboReingresoCuerpoDeg = candidato.rumboCuerpoDeg;
  pasoGiroHaciaLineaDeg = candidato.giroHaciaLineaDeg;
  pasoGiroRetornoDeg = candidato.giroRetornoDeg;
  pasoCosteReingresoAvance = candidatoAvance.coste;
  pasoCosteReingresoReversa = candidatoReversa.coste;
  pasoCosteReingresoElegido = candidato.coste;
  pasoRecuperacionUsaReversa = direccionReingreso < 0;
  ++pasoIntentoRecentrado;
  pasoIntentosEndpoint = pasoIntentoRecentrado;
  pasoErrorLateralInicialRecentradoCm = pasoErrorLateralCm;
  pasoMejoraRecentradoCm = 0.0f;
  pasoDistanciaRecuperacionCm = distanciaReingresoCm;
  // La ventana y los temporizadores del tramo anterior no deben contaminar
  // la nueva maniobra. El presupuesto de intentos sí se conserva.
  inicioLateralExtremoMs = 0;
  inicioDesviacionLateralMs = 0;
  inicioCrecimientoLateralMs = 0;
  errorLateralAnteriorValido = false;
  distAcumuladaCm = distanciaActualCm;
  if (iniciaDesdeRuta) direccionTraslacionPlanificada = direccionTraslacion;
  inicioFaseRecentradoMs = millis();
  strncpy(pasoDisparadorRecentrado, disparador,
          sizeof(pasoDisparadorRecentrado));
  strncpy(pasoFaseRecentrado, "pause", sizeof(pasoFaseRecentrado));
  strncpy(pasoDecisionRecuperacion, "axis_recenter",
          sizeof(pasoDecisionRecuperacion));
  frenarMotores();
  reiniciarControlRumbo();
  fase = Fase::PAUSA_RECENTRADO;
  strncpy(faseComando, "recenter_pause", sizeof(faseComando));
}

void iniciarAvanceReingreso() {
  const SensorSnapshot s = sensar();
  direccionTraslacion = direccionReingreso;
  pasoEnReversa = direccionTraslacion < 0;
  distAcumuladaCm = 0.0f;
  distanciaReingresoCm = PoseGlobal.distanciaAlObjetivo(
      pasoReingresoXCm, pasoReingresoYCm);
  ControlRuta::iniciarSeguimientoProgreso(
      seguimientoProgreso, distanciaRestanteObjetivoActivo(true));
  rumboObjetivoDeg = rumboReingresoCuerpoDeg;
  pasoRumboTrayectoDeg = rumboReingresoTrayectoDeg;
  pasoRumboCuerpoDeg = rumboReingresoCuerpoDeg;
  copiarBase(ticksBaseAvance, s);
  inicioAvanceMs = millis();
  ticksLadoAvAnt[0] = ticksLadoAvAnt[1] = 0;
  ultimoPulsoLadoAvMs[0] = ultimoPulsoLadoAvMs[1] = inicioAvanceMs;
  reiniciarControlRumbo();
  fase = Fase::AVANCE_A_EJE;
  strncpy(faseComando, "recenter_drive", sizeof(faseComando));
  strncpy(pasoFaseRecentrado, "drive_to_axis", sizeof(pasoFaseRecentrado));
}

void verificarReingreso() {
  frenarMotores();
  actualizarErroresTrayectoria();
  const float distancia = PoseGlobal.distanciaAlObjetivo(
      pasoReingresoXCm, pasoReingresoYCm);
  pasoMejoraRecentradoCm = fabsf(pasoErrorLateralInicialRecentradoCm) -
                           fabsf(pasoErrorLateralCm);
  if (millis() - inicioFaseRecentradoMs < RECENTER_SETTLE_MS) return;
  if (!ControlRuta::reingresoAceptable(
          pasoErrorLateralCm, distancia, RECENTER_LATERAL_DEADBAND_CM,
          RECENTER_POINT_TOLERANCE_CM)) {
    iniciarRecentrado("verify_retry", 0.0f);
    return;
  }
  direccionTraslacion = direccionTraslacionPlanificada;
  pasoEnReversa = direccionTraslacion < 0;
  strncpy(pasoFaseRecentrado, "restore_heading", sizeof(pasoFaseRecentrado));
  iniciarBaseGiro(ControlRuta::rumboCuerpoParaTrayecto(
      pasoHeading, direccionTraslacion), Fase::GIRO_A_RUMBO);
}

bool detectarOutliers(const int64_t v[4]) {
  if (fase != Fase::AVANCE) return false;
  float med = mediana4(v);
  if (modoDegradado || fabsf(errorAng360(rumboObjetivoDeg, heading360)) > ERROR_MAX_CLASIFICAR_DEG || med < TICKS_MINIMOS_AUDITORIA) {
    for (int i = 0; i < 4; ++i) { inicioOutlierEncMs[i] = 0; if (saludEnc[i] != 2) saludEnc[i] = 0; }
    return false;
  }

  bool persistente=false;
  for (int i=0;i<4;++i) {
    if (ControlSeguridad::encoderEsOutlier(v[i], med, DESACUERDO_MAXIMO_PAR)) {
      if(saludEnc[i]==0)saludEnc[i]=1;
      if(!inicioOutlierEncMs[i])inicioOutlierEncMs[i]=millis();
      persistente |= (millis()-inicioOutlierEncMs[i] >= DESACUERDO_ENCODER_PERSISTENTE_MS);
    } else { inicioOutlierEncMs[i]=0; if(saludEnc[i]!=2)saludEnc[i]=0; }
  }
  return persistente;
}

void iniciarPausaReeval(const int64_t v[4]) {
  frenarMotores();
  reiniciarControlRumbo();
  for (int i=0;i<4;++i) ticksPausaClasif[i]=v[i];
  inicioPausaReevalMs = millis();
  fase = Fase::PAUSA_REEVALUACION;
  strncpy(faseComando, "reeval", sizeof(faseComando));
}

void completarPausaReeval() {
  const ControlSeguridad::ClasificacionEncoders clasificacion =
      ControlSeguridad::clasificarEncoders(ticksPausaClasif, DESACUERDO_MAXIMO_PAR);
  WatchdogSeguridad.aplicarClasificacionEncoders(clasificacion.confiable);
  for (int i=0;i<4;++i) {
    saludEnc[i]=clasificacion.confiable[i]?0:2;
    inicioOutlierEncMs[i]=0;
  }
  if (!clasificacion.ladoIzquierdoValido || !clasificacion.ladoDerechoValido) {
    fallo("enc_no_side");
    return;
  }
  inicioAvanceMs = millis();
  iniciarAvance(true);
}

int pwmAntiFriccion(uint8_t indice) {
  return min(PWM_SAFE_HARD_LIMIT,
             static_cast<int>(ControlSeguridad::nivelAntiFriccion8Bit(indice) *
                              PWM_SCALE_8_TO_10));
}

void iniciarAntiFriccion(const SensorSnapshot& s) {
  frenarMotores();
  copiarBase(ticksBaseAntiFriccion, s);
  antiFriccionActiva = true;
  antiFriccionPulsoEncendido = true;
  antiFriccionPulsoIndice = 1;
  antiFriccionPwmObjetivo = pwmAntiFriccion(antiFriccionPulsoIndice);
  antiFriccionMovimientoConfirmado = false;
  inicioFaseAntiFriccionMs = millis();
  strncpy(faseComando, "anti_friccion", sizeof(faseComando));
}

bool controlarAntiFriccion(const SensorSnapshot& s) {
  if (!antiFriccionActiva) return false;
  int64_t delta[4] = {};
  deltas(ticksBaseAntiFriccion, s, delta);
  const float deltaL = promedioLado(delta, true);
  const float deltaR = promedioLado(delta, false);
  if (ControlSeguridad::movimientoAntiFriccionConfirmado(
          deltaL, deltaR, ANTIFRICTION_SUCCESS_TICKS)) {
    antiFriccionActiva = false;
    antiFriccionPulsoEncendido = false;
    antiFriccionMovimientoConfirmado = true;
    antiFriccionPwmObjetivo = 0;
    ultimoPulsoLadoAvMs[0] = ultimoPulsoLadoAvMs[1] = millis();
    ticksLadoAvAnt[0] = ticksLadoAvAnt[1] = 0;
    strncpy(faseComando, fase == Fase::AVANCE_A_EJE
        ? "recenter_drive" : "avance", sizeof(faseComando));
    return false;
  }

  const uint32_t transcurrido = millis() - inicioFaseAntiFriccionMs;
  if (antiFriccionPulsoEncendido) {
    if (transcurrido < ANTIFRICTION_PULSE_ON_MS) {
      if (!aplicarVelocidades(direccionTraslacion * antiFriccionPwmObjetivo,
                              direccionTraslacion * antiFriccionPwmObjetivo)) {
        fallo("motor_output_error");
        return true;
      }
      return true;
    }
    frenarMotores();
    antiFriccionPulsoEncendido = false;
    inicioFaseAntiFriccionMs = millis();
    return true;
  }

  frenarMotores();
  if (transcurrido < ANTIFRICTION_PULSE_OFF_MS) return true;
  if (antiFriccionPulsoIndice >= ANTIFRICTION_PULSE_COUNT) {
    antiFriccionActiva = false;
    antiFriccionPwmObjetivo = 0;
    const bool sinIzquierda = deltaL < ANTIFRICTION_SUCCESS_TICKS;
    fallo(sinIzquierda ? "drive_stall_left[anti_friction]"
                       : "drive_stall_right[anti_friction]");
    return true;
  }
  ++antiFriccionPulsoIndice;
  antiFriccionPwmObjetivo = pwmAntiFriccion(antiFriccionPulsoIndice);
  antiFriccionPulsoEncendido = true;
  copiarBase(ticksBaseAntiFriccion, s);
  inicioFaseAntiFriccionMs = millis();
  return true;
}

bool controlarAvance() {
  if (fase != Fase::AVANCE && fase != Fase::AVANCE_A_EJE &&
      fase != Fase::PAUSA_REEVALUACION) return false;
  if (fase == Fase::PAUSA_REEVALUACION) {
    if (millis()-inicioPausaReevalMs >= PAUSA_REEVALUACION_MS) completarPausaReeval();
    return false;
  }
  const bool avanceReingreso = fase == Fase::AVANCE_A_EJE;
  const SensorSnapshot s = sensar();
  int64_t d[4]; deltas(ticksBaseAvance, s, d);
  float ticksEst = estimarTicksAvance(d);
  if (ticksEst<0) { fallo("enc_no_estimation"); return false; }
  // La misma escala efectiva debe gobernar el PID y la pose. Antes el PID
  // usaba el diámetro nominal y la odometría otro valor, por lo que el gráfico
  // podía indicar corrección sin que el robot frenara antes.
  const float cmPorTick = ControlRuta::distanciaPorTick(WHEEL_DIAMETER_ODOMETRY_CM, ENCODER_PPR);
  float distMedida = ControlRuta::distanciaConBaseline(
      distAcumuladaCm, ticksEst, cmPorTick);
  if (!avanceReingreso) pasoDistanciaActualCm = distMedida;
  const float objetivoMovimientoCm = avanceReingreso
      ? distanciaReingresoCm : distObjetivoCm;
  float restante = objetivoMovimientoCm - distMedida;
  pasoDistanciaRestanteCm = restante;
  const float pwmReferenciaFreno = restante < DISTANCIA_APROXIMACION_CM
      ? static_cast<float>(VELOCIDAD_APROXIMACION)
      : static_cast<float>(VELOCIDAD_BASE_RECTO);
  pasoFrenoPrevistoCm = ControlRuta::distanciaFrenoPrevista(
      pwmReferenciaFreno, FRENO_RESIDUAL_BASE_CM, FRENO_RESIDUAL_POR_PWM_CM,
      FRENO_RESIDUAL_MAX_CM);
  actualizarErroresTrayectoria();
  pasoAutoridadMpu = s.mpu_present && s.mpu_calibrated && !s.mpu_stale;
  const uint32_t ahora = millis();
  if (tieneTargetEspacial) {
    const float lateralAbs = fabsf(pasoErrorLateralCm);
    if (lateralAbs > RECENTER_MAX_LATERAL_CM) {
      if (!inicioLateralExtremoMs) inicioLateralExtremoMs = ahora;
      if (ahora - inicioLateralExtremoMs >= RECENTER_TRIGGER_MS) {
        fallo("recenter_out_of_range");
        return false;
      }
    } else {
      inicioLateralExtremoMs = 0;
    }
  }
  if (tieneTargetEspacial) {
    pasoUmbralLateralCm = ControlRuta::umbralLateralSegmento(
        distanciaPlanificadaCm, RECENTER_LATERAL_RATIO,
        RECENTER_LATERAL_MIN_CM);
    pasoProporcionLateral = ControlRuta::proporcionLateralSegmento(
        pasoErrorLateralCm, distanciaPlanificadaCm);
    const uint16_t ventanaAnterior = pasoVentanaIndice;
    const float progresoSobreSegmento = constrain(
        distanciaPlanificadaCm - pasoErrorLongitudinalCm,
        0.0f, distanciaPlanificadaCm);
    actualizarVentanaSupervision(progresoSobreSegmento);
    if (!avanceReingreso && ventanaAnterior != pasoVentanaIndice) {
      mejorErrorLateralAbsCm = fabsf(pasoErrorLateralCm);
      inicioCrecimientoLateralMs = 0;
    }
  }

  uint32_t timeout = DRIVE_BASE_TIMEOUT_MS + (uint32_t)(objetivoMovimientoCm * DRIVE_TIMEOUT_PER_CM_MS);
  if (millis()-inicioAvanceMs > timeout) { fallo("drive_timeout"); return false; }
  if (avanceReingreso) {
    const float distanciaPunto = PoseGlobal.distanciaAlObjetivo(
        pasoReingresoXCm, pasoReingresoYCm);
    pasoMejoraRecentradoCm = fabsf(pasoErrorLateralInicialRecentradoCm) -
                             fabsf(pasoErrorLateralCm);
    if (ControlRuta::reingresoAceptable(
            pasoErrorLateralCm, distanciaPunto,
            RECENTER_LATERAL_DEADBAND_CM,
            RECENTER_POINT_TOLERANCE_CM)) {
      frenarMotores();
      inicioFaseRecentradoMs = millis();
      fase = Fase::VERIFICAR_REINGRESO;
      strncpy(faseComando, "recenter_verify", sizeof(faseComando));
      strncpy(pasoFaseRecentrado, "verify", sizeof(pasoFaseRecentrado));
      return false;
    }
    if (ControlRuta::reingresoDivergente(
            pasoErrorLateralInicialRecentradoCm, pasoErrorLateralCm,
            distMedida, millis() - inicioAvanceMs,
            RECENTER_MIN_IMPROVEMENT_CM, RECENTER_EVALUATION_DISTANCE_CM,
            RECENTER_EVALUATION_MS)) {
      fallo("recenter_not_converged");
      return false;
    }
  } else if (ControlRuta::tramoRequiereModoPulsado(restante, PULSE_DRIVE_THRESHOLD_CM)) {
    distAcumuladaCm = distMedida;
    iniciarAvancePulsado(true);
    return false;
  } else if (restante <= TOLERANCIA_DISTANCIA_CM + pasoFrenoPrevistoCm) {
    iniciarAsentamientoFinal(distMedida);
    return false;
  }

  // La corrección lateral no puede ocultar un avance en sentido opuesto. Se
  // aplica también durante el reingreso y el endpoint, con un baseline nuevo
  // por maniobra para no arrastrar el error de la fase anterior.
  if (progresoLongitudinalIncorrecto(ahora)) {
    fallo("route_progress_wrong_sign");
    return false;
  }

  // El paso manual conserva su finalización relativa. Para un waypoint
  // absoluto sólo se acepta este atajo si ya está dentro de 5 cm; en otro
  // caso la verificación final activará la recuperación acotada.
  if (tieneTargetEspacial && !avanceReingreso) {
    float distEspacialActual = PoseGlobal.distanciaAlObjetivo(pasoTargetX, pasoTargetY);
    if (pasoObjetivoAbsoluto && distEspacialActual <= TOLERANCIA_ENDPOINT_CM) {
      iniciarAsentamientoFinal(distMedida);
      return false;
    }
    if (!pasoObjetivoAbsoluto && distEspacialActual <= TOLERANCIA_DISTANCIA_CM) {
      iniciarAsentamientoFinal(distMedida);
      return false;
    }
    if (!pasoObjetivoAbsoluto && distEspacialActual < distTargetMinimaCm) {
      distTargetMinimaCm = distEspacialActual;
    } else if (!pasoObjetivoAbsoluto && distTargetMinimaCm <= 15.0f &&
               (distEspacialActual - distTargetMinimaCm) >= 2.0f) {
      // Sobrepaso espacial detectado (se alejo 2 cm tras estar a menos de 15 cm del destino)
      iniciarAsentamientoFinal(distMedida);
      return false;
    }
  }

  if (!avanceReingreso && pasoObjetivoAbsoluto && !recuperacionEndpointActiva) {
    const float lateralAbs = fabsf(pasoErrorLateralCm);
    const float umbralSalida = ControlRuta::umbralLateralSegmento(
        distanciaPlanificadaCm, RECENTER_LATERAL_EXIT_RATIO,
        RECENTER_LATERAL_MIN_CM);

    const bool cruzoEjeConErrorBajo =
        errorLateralAnteriorValido &&
        ControlRuta::cruzoEjeConErrorBajo(
            errorLateralAnteriorCm, pasoErrorLateralCm,
            RECENTER_LATERAL_MIN_CM) &&
        restante > TOLERANCIA_DISTANCIA_CM;
    if (cruzoEjeConErrorBajo) {
      iniciarRealineacionEje(distMedida);
      return false;
    }

    if (lateralAbs <= umbralSalida) {
      inicioDesviacionLateralMs = 0;
    } else if (lateralAbs > pasoUmbralLateralCm &&
               !inicioDesviacionLateralMs) {
      inicioDesviacionLateralMs = ahora;
    }
    if (lateralAbs < mejorErrorLateralAbsCm) {
      mejorErrorLateralAbsCm = lateralAbs;
      inicioCrecimientoLateralMs = 0;
    } else if (lateralAbs >= mejorErrorLateralAbsCm + RECENTER_GROWTH_TRIGGER_CM) {
      if (!inicioCrecimientoLateralMs) inicioCrecimientoLateralMs = ahora;
    } else if (lateralAbs <= umbralSalida) {
      inicioCrecimientoLateralMs = 0;
    }
    const uint32_t persistencia = inicioDesviacionLateralMs
        ? ahora - inicioDesviacionLateralMs : 0;
    const uint32_t crecimientoMs = inicioCrecimientoLateralMs
        ? ahora - inicioCrecimientoLateralMs : 0;
    if (ControlRuta::debeRecentrar(
            pasoErrorLateralCm, persistencia, pasoUmbralLateralCm,
            RECENTER_TRIGGER_MS, lateralAbs - mejorErrorLateralAbsCm,
            crecimientoMs, RECENTER_GROWTH_TRIGGER_CM,
            RECENTER_GROWTH_MS)) {
      iniciarRecentrado(persistencia >= RECENTER_TRIGGER_MS
          ? "lateral_persistent" : "lateral_growing", distMedida);
      return false;
    }
    errorLateralAnteriorCm = pasoErrorLateralCm;
    errorLateralAnteriorValido = true;
  }


  // stall per side
  if (!hayPorLado()) { fallo("enc_no_side"); return false; }
  int64_t ladoTicks[2]={(encoderConfiableGlobal[0]?d[0]:0)+(encoderConfiableGlobal[2]?d[2]:0),
                         (encoderConfiableGlobal[1]?d[1]:0)+(encoderConfiableGlobal[3]?d[3]:0)};
  for (int i=0; i<2; ++i) {
    if (ladoTicks[i]!=ticksLadoAvAnt[i]) { ticksLadoAvAnt[i]=ladoTicks[i]; ultimoPulsoLadoAvMs[i]=millis(); }
  }
  if (controlarAntiFriccion(s)) return false;
  const bool sinProgreso = millis()-ultimoPulsoLadoAvMs[0] > ANTIFRICTION_TRIGGER_MS ||
                           millis()-ultimoPulsoLadoAvMs[1] > ANTIFRICTION_TRIGGER_MS;
  if (sinProgreso) {
    iniciarAntiFriccion(s);
    controlarAntiFriccion(s);
    return false;
  }
  for (int i=0; i<2; ++i) {
    if (millis()-ultimoPulsoLadoAvMs[i] > DRIVE_STALL_MS) {
      if (intentosRecup < INTENTOS_RECUPERACION_MAX) {
        ++intentosRecup;
        frenarMotores();
        if (!avanceReingreso) distAcumuladaCm = distMedida;
        iniciarBaseGiro(normalizar360(rumboObjetivoDeg), avanceReingreso
            ? Fase::GIRO_A_REINGRESO : Fase::GIRO_RECUPERACION);
        return false;
      }
      fallo(i==0?"drive_stall_left[Cinematica.cpp:503]":"drive_stall_right[Cinematica.cpp:503]"); return false;
    }
  }

  // Mientras se avanza hacia un waypoint absoluto, el rumbo converge de forma
  // gradual a la línea planificada. Un lateral positivo está a la derecha y
  // por eso ordena una corrección negativa (hacia la izquierda).
  if (avanceReingreso) {
    pasoRumboDinamicoDeg = rumboObjetivoDeg;
    pasoControlLateralDeg = 0.0f;
  } else if (recuperacionEndpointActiva) {
    const float rumboTrayecto = normalizar360(
        PoseGlobal.anguloAlObjetivoRad(pasoTargetX, pasoTargetY) * 180.0f / M_PI);
    pasoRumboTrayectoDeg = rumboTrayecto;
    pasoRumboCuerpoDeg = ControlRuta::rumboCuerpoParaTrayecto(
        rumboTrayecto, direccionTraslacion);
    rumboObjetivoDeg = pasoRumboCuerpoDeg;
    pasoControlLateralDeg = 0.0f;
  } else if (pasoObjetivoAbsoluto) {
    const float correccionTrayectoDeg = fabsf(pasoErrorLateralCm) <=
        RECENTER_LATERAL_DEADBAND_CM ? 0.0f :
        ControlRuta::correccionLateralRumboDeg(
            pasoErrorLateralCm, KP_LATERAL_RUMBO_DEG_POR_CM,
            CORRECCION_LATERAL_RUMBO_MAX_DEG);
    // En reversa la geometría de trayecto se conserva, pero el chasis mira al
    // lado opuesto. Por eso la corrección lateral de yaw cambia de signo.
    pasoControlLateralDeg = ControlRuta::correccionLateralParaDireccion(
        correccionTrayectoDeg, direccionTraslacion);
    rumboObjetivoDeg = normalizar360(pasoRumboCuerpoDeg + pasoControlLateralDeg);
  } else {
    pasoControlLateralDeg = 0.0f;
    rumboObjetivoDeg = pasoRumboCuerpoDeg;
  }
  pasoRumboDinamicoDeg = rumboObjetivoDeg;
  float err = errorAng360(rumboObjetivoDeg, heading360);
  pasoErrorRumboDeg = err;
  errorRumboMaxTramo = max(errorRumboMaxTramo, fabsf(err));

  // El pivote sólo se solicita tras la histéresis configurada; los errores
  // menores se absorben en continuo para evitar el ciclo avance/giro.
  if (fabsf(err) > ERROR_RUMBO_RECUPERAR_DEG) {
    if (!inicioErrorRumboMs) inicioErrorRumboMs = millis();
    if (millis() - inicioErrorRumboMs >= ERROR_RUMBO_RECUPERAR_MS) {
      frenarMotores();
      distAcumuladaCm = distMedida;
      if (intentosRecup >= INTENTOS_RECUPERACION_MAX) { fallo("heading_no_recovery"); return false; }
      ++intentosRecup;
      iniciarBaseGiro(normalizar360(rumboObjetivoDeg), avanceReingreso
          ? Fase::GIRO_A_REINGRESO : Fase::GIRO_RECUPERACION);
      return false;
    }
  } else inicioErrorRumboMs = 0;

  // --- PID de rumbo con integral acotada y anti-windup ---
  const ControlRuta::SalidaPI salidaPI = ControlRuta::actualizarPI(
      estadoPIRumbo, err, s.gyro_z_filtrado_rad_s, 0.01f,
      KP_RUMBO_PWM_POR_GRADO, KI_RUMBO_PWM_POR_GRADO_S, KD_RUMBO_PWM_POR_RAD_S,
      PWM_CORRECCION_RUMBO_MAX, ERROR_INTEGRAL_RUMBO_MAX_GRADO_S);
  const float ctrlRumbo = salidaPI.total;
  pasoIntegralRumboGradoS = salidaPI.integralGradoS;
  pasoControlRumboP = salidaPI.p;
  pasoControlRumboI = salidaPI.i;
  pasoControlRumboD = salidaPI.d;
  pasoControlRumboPwm = salidaPI.total;
  float ctrlEnc = 0.0f;
  if (fabsf(err) <= ERROR_ENCODER_AUX_MAX_DEG) {
    const int64_t deltaFiltrado[4] = {
      lroundf(s.delta_pulsos_filtrado_FL), lroundf(s.delta_pulsos_filtrado_FR),
      lroundf(s.delta_pulsos_filtrado_BL), lroundf(s.delta_pulsos_filtrado_BR)
    };
    const float deltaIzq = promedioLado(deltaFiltrado, true);
    const float deltaDer = promedioLado(deltaFiltrado, false);
    float diff = deltaIzq - deltaDer;
    ctrlEnc = constrain(diff * KP_ENCODER_PWM_POR_TICK, -float(PWM_CORRECCION_ENCODER_MAX), float(PWM_CORRECCION_ENCODER_MAX));
  }
  pasoControlEncoderPwm = ctrlEnc;

  const float restantePerfil = max(0.0f, restante - pasoFrenoPrevistoCm);
  int base = restantePerfil < DISTANCIA_APROXIMACION_CM
      ? map(long(restantePerfil * 10), 0, long(DISTANCIA_APROXIMACION_CM * 10),
            VELOCIDAD_PRECISION_RECTO, VELOCIDAD_APROXIMACION)
      : VELOCIDAD_BASE_RECTO;
  base = constrain(base, VELOCIDAD_PRECISION_RECTO, VELOCIDAD_BASE_RECTO);
  if (direccionTraslacion < 0) {
    pasoRampaReversaMs = min<uint32_t>(millis() - inicioAvanceMs, RAMPA_REVERSA_MS);
    const int limiteRampa = map(long(pasoRampaReversaMs), 0, long(RAMPA_REVERSA_MS),
                                VELOCIDAD_PRECISION_RECTO, VELOCIDAD_BASE_RECTO);
    base = min(base, limiteRampa);
  } else {
    pasoRampaReversaMs = 0;
  }
  base = constrain(base, 0, PWM_MAX);

  // Compensacion derecha + reduccion dinamica del lado contrario al angulo desviado (extraida del test aprobado)
  int baseDer = constrain(aproximar(base * factorCompensacionDer), VELOCIDAD_PRECISION_RECTO, PWM_MAX);
  int redL = 0, redR = 0;
  if (ctrlRumbo != 0.0f) {
    // El lado frenado depende de la geometría (+yaw horario), no de la
    // polaridad eléctrica aprendida durante el pivote de calibración.
    if (ControlRuta::frenarLadoIzquierdoParaRumbo(ctrlRumbo, direccionTraslacion)) {
      redL += aproximar(fabsf(ctrlRumbo));
      strncpy(pasoLadoFrenoRumbo, "left", sizeof(pasoLadoFrenoRumbo));
    } else {
      redR += aproximar(fabsf(ctrlRumbo));
      strncpy(pasoLadoFrenoRumbo, "right", sizeof(pasoLadoFrenoRumbo));
    }
  } else {
    strncpy(pasoLadoFrenoRumbo, "none", sizeof(pasoLadoFrenoRumbo));
  }
  if (ctrlEnc > 0) redL += aproximar(ctrlEnc); else redR += aproximar(-ctrlEnc);

  int magL = constrain(base - redL, VELOCIDAD_PRECISION_RECTO, PWM_MAX);
  int magR = constrain(baseDer - redR, VELOCIDAD_PRECISION_RECTO, PWM_MAX);

  if (!aplicarVelocidades(direccionTraslacion * magL, direccionTraslacion * magR)) {
    fallo("motor_output_error");
    return false;
  }

  if (ticksEst >= 2.0f) { /* movimiento ok: se resetea watchdog externo via pulsos */ }
  return false;
}

// ============== MODO DE MICRO-PULSOS DETERMINISTAS DE ALTO PAR CON INTERLOCKS ==============
void iniciarAvancePulsado(bool conservar) {
  const SensorSnapshot s = sensar();
  if (!conservar) {
    distAcumuladaCm = 0.0f;
    intentosRecup = 0;
    inicioDesviacionLateralMs = 0;
    inicioLateralExtremoMs = 0;
    inicioCrecimientoLateralMs = 0;
    resetConfEncoders();
    pasoDistanciaPorPulsoCm = 1.0f;
  }
  reiniciarControlRumbo();
  pasoEnReversa = direccionTraslacion < 0;
  antiFriccionActiva = false;
  conservarAcumulado = conservar;
  distObjetivoCm = pasoDistanciaCm;
  pasoDistanciaObjetivoCm = distObjetivoCm;
  rumboObjetivoDeg = pasoRumboCuerpoDeg;
  copiarBase(ticksBaseAvance, s);
  copiarBase(ticksBasePulsoOff, s);
  inicioAvanceMs = millis();
  inicioPulsoDriveMs = inicioAvanceMs;
  pulsoDriveEstado = PulsoDriveEstado::ON;
  pulsosConsecutivosSinTicks = 0;
  pasoDistanciaRestanteCm = distObjetivoCm - distAcumuladaCm;
  pasoFrenoPrevistoCm = 0.0f;
  pasoArrastreFrenoCm = 0.0f;
  pasoAsentamientoMs = 0;
  pasoRampaReversaMs = 0;
  strncpy(pasoLadoFrenoRumbo, "none", sizeof(pasoLadoFrenoRumbo));
  if (tieneTargetEspacial) {
    distTargetMinimaCm = PoseGlobal.distanciaAlObjetivo(pasoTargetX, pasoTargetY);
    actualizarErroresTrayectoria();
    mejorErrorLateralAbsCm = fabsf(pasoErrorLateralCm);
    ControlRuta::iniciarSeguimientoProgreso(
        seguimientoProgreso, distanciaRestanteObjetivoActivo(false));
    pasoUmbralLateralCm = ControlRuta::umbralLateralSegmento(
        distanciaPlanificadaCm, RECENTER_LATERAL_RATIO,
        RECENTER_LATERAL_MIN_CM);
    pasoProporcionLateral = ControlRuta::proporcionLateralSegmento(
        pasoErrorLateralCm, distanciaPlanificadaCm);
    actualizarVentanaSupervision(distAcumuladaCm);
    errorLateralAnteriorCm = pasoErrorLateralCm;
    errorLateralAnteriorValido = true;
  }
  fase = Fase::AVANCE_PULSADO;
  strncpy(faseComando, "pulse_drive", sizeof(faseComando));
}

// El modo de micro-pulsos se usa precisamente en los tramos cortos, por lo
// que no puede quedar exento del mismo control por rangos que el avance
// continuo. Devuelve true cuando cambió de fase o enclavó un fallo.
bool supervisarRangosAvancePulsado(float distanciaMedidaCm, float restante) {
  (void)distanciaMedidaCm;
  if (!tieneTargetEspacial) {
    return false;
  }

  const uint32_t ahora = millis();
  const float lateralAbs = fabsf(pasoErrorLateralCm);
  pasoUmbralLateralCm = ControlRuta::umbralLateralSegmento(
      distanciaPlanificadaCm, RECENTER_LATERAL_RATIO,
      RECENTER_LATERAL_MIN_CM);
  pasoProporcionLateral = ControlRuta::proporcionLateralSegmento(
      pasoErrorLateralCm, distanciaPlanificadaCm);
  const uint16_t ventanaAnterior = pasoVentanaIndice;
  const float progresoSobreSegmento = constrain(
      distanciaPlanificadaCm - pasoErrorLongitudinalCm,
      0.0f, distanciaPlanificadaCm);
  actualizarVentanaSupervision(progresoSobreSegmento);
  if (ventanaAnterior != pasoVentanaIndice) {
    mejorErrorLateralAbsCm = lateralAbs;
    inicioCrecimientoLateralMs = 0;
  }

  if (lateralAbs > RECENTER_MAX_LATERAL_CM) {
    if (!inicioLateralExtremoMs) inicioLateralExtremoMs = ahora;
    if (ahora - inicioLateralExtremoMs >= RECENTER_TRIGGER_MS) {
      fallo("recenter_out_of_range");
      return true;
    }
  } else {
    inicioLateralExtremoMs = 0;
  }

  // También se vigila el sentido durante la recuperación de endpoint; sólo
  // la corrección lateral se omite en esa subfase.
  if (progresoLongitudinalIncorrecto(ahora)) {
    fallo("route_progress_wrong_sign");
    return true;
  }
  if (!pasoObjetivoAbsoluto || recuperacionEndpointActiva) {
    return false;
  }

  if (errorLateralAnteriorValido &&
      ControlRuta::cruzoEjeConErrorBajo(
          errorLateralAnteriorCm, pasoErrorLateralCm,
          RECENTER_LATERAL_MIN_CM) &&
      restante > TOLERANCIA_DISTANCIA_CM) {
    iniciarRealineacionEje(distanciaMedidaCm);
    return true;
  }

  const float umbralSalida = ControlRuta::umbralLateralSegmento(
      distanciaPlanificadaCm, RECENTER_LATERAL_EXIT_RATIO,
      RECENTER_LATERAL_MIN_CM);
  if (lateralAbs <= umbralSalida) {
    inicioDesviacionLateralMs = 0;
    inicioCrecimientoLateralMs = 0;
  } else if (lateralAbs > pasoUmbralLateralCm &&
             !inicioDesviacionLateralMs) {
    inicioDesviacionLateralMs = ahora;
  }
  if (lateralAbs < mejorErrorLateralAbsCm) {
    mejorErrorLateralAbsCm = lateralAbs;
    inicioCrecimientoLateralMs = 0;
  } else if (lateralAbs >= mejorErrorLateralAbsCm + RECENTER_GROWTH_TRIGGER_CM) {
    if (!inicioCrecimientoLateralMs) inicioCrecimientoLateralMs = ahora;
  }
  const uint32_t persistencia = inicioDesviacionLateralMs
      ? ahora - inicioDesviacionLateralMs : 0;
  const uint32_t crecimientoMs = inicioCrecimientoLateralMs
      ? ahora - inicioCrecimientoLateralMs : 0;
  if (ControlRuta::debeRecentrar(
          pasoErrorLateralCm, persistencia, pasoUmbralLateralCm,
          RECENTER_TRIGGER_MS, lateralAbs - mejorErrorLateralAbsCm,
          crecimientoMs, RECENTER_GROWTH_TRIGGER_CM,
          RECENTER_GROWTH_MS)) {
    iniciarRecentrado(persistencia >= RECENTER_TRIGGER_MS
        ? "lateral_persistent" : "lateral_growing", distanciaMedidaCm);
    return true;
  }

  errorLateralAnteriorCm = pasoErrorLateralCm;
  errorLateralAnteriorValido = true;
  return false;
}

bool controlarAvancePulsado() {
  const SensorSnapshot s = sensar();
  pasoAutoridadMpu = s.mpu_present && s.mpu_calibrated && !s.mpu_stale;
  if (!hayPorLado()) { fallo("enc_no_side"); return false; }
  int64_t d[4]; deltas(ticksBaseAvance, s, d);
  float ticksEst = estimarTicksAvance(d);
  if (ticksEst < 0.0f) { fallo("enc_no_estimation"); return false; }
  const float cmPorTick = ControlRuta::distanciaPorTick(WHEEL_DIAMETER_ODOMETRY_CM, ENCODER_PPR);
  float distMedida = ControlRuta::distanciaConBaseline(
      distAcumuladaCm, ticksEst, cmPorTick);
  pasoDistanciaActualCm = distMedida;
  float restante = distObjetivoCm - distMedida;
  pasoDistanciaRestanteCm = restante;
  progresoComando = distObjetivoCm > 0.0f
      ? constrain(distMedida / distObjetivoCm, 0.0f, 0.99f) : 0.99f;
  actualizarErroresTrayectoria();

  if (supervisarRangosAvancePulsado(distMedida, restante)) {
    return false;
  }

  // Interlock de meta alcanzada o waypoint absoluto
  if (tieneTargetEspacial) {
    float distEspacial = PoseGlobal.distanciaAlObjetivo(pasoTargetX, pasoTargetY);
    if (distEspacial <= TOLERANCIA_ENDPOINT_CM) {
      iniciarAsentamientoFinal(distMedida);
      return false;
    }
  }
  if (ControlRuta::interlockFinPulsado(restante, PULSE_DRIVE_TOLERANCIA_CM, pasoDistanciaPorPulsoCm)) {
    iniciarAsentamientoFinal(distMedida);
    return false;
  }

  const uint32_t ahora = millis();
  if (pulsoDriveEstado == PulsoDriveEstado::ON) {
    const float errRumbo = errorAng360(rumboObjetivoDeg, heading360);
    pasoErrorRumboDeg = errRumbo;
    const ControlRuta::SalidaPulso salida = ControlRuta::calcularPulsoTraccion(
        PULSE_DRIVE_PWM, errRumbo, PULSE_DRIVE_KP_RUMBO_PWM,
        PULSE_DRIVE_MAX_DIFF_PWM, PWM_TURN_MAX_LIMIT);
    if (!aplicarVelocidades(direccionTraslacion * salida.pwmIzquierdo,
                            direccionTraslacion * salida.pwmDerecho)) {
      fallo("motor_output_error");
      return false;
    }
    if (ahora - inicioPulsoDriveMs >= PULSE_DRIVE_ON_MS) {
      frenarMotores();
      pulsoDriveEstado = PulsoDriveEstado::OFF;
      inicioPulsoDriveMs = ahora;
      copiarBase(ticksBasePulsoOff, s);
    }
    return false;
  }

  // PulsoDriveEstado::OFF: motores frenados mientras se mide y amortigua
  frenarMotores();
  if (ahora - inicioPulsoDriveMs >= PULSE_DRIVE_OFF_MS) {
    int64_t dPulso[4]; deltas(ticksBasePulsoOff, s, dPulso);
    const float ticksPulso = estimarTicksAvance(dPulso);
    const float deltaMedidoCm = ticksPulso > 0.0f ? ticksPulso * cmPorTick : 0.0f;

    // Interlock de atasco físico (anti-stall)
    if (ticksPulso <= 0.0f && fabsf(s.gyro_z_filtrado_rad_s) < 0.06f) {
      ++pulsosConsecutivosSinTicks;
      if (pulsosConsecutivosSinTicks >= PULSE_DRIVE_STALL_MAX) {
        fallo("drive_stall_pulses");
        return false;
      }
    } else {
      pulsosConsecutivosSinTicks = 0;
      pasoDistanciaPorPulsoCm = ControlRuta::actualizarMemoriaImpulso(
          pasoDistanciaPorPulsoCm, deltaMedidoCm);
    }

    if (ControlRuta::interlockFinPulsado(restante - deltaMedidoCm,
                                        PULSE_DRIVE_TOLERANCIA_CM,
                                        pasoDistanciaPorPulsoCm)) {
      iniciarAsentamientoFinal(distMedida);
      return false;
    }

    pulsoDriveEstado = PulsoDriveEstado::ON;
    inicioPulsoDriveMs = ahora;
  }
  return false;
}

// ============== PASO (orquestacion: giro_inicial -> avance -> giro_final -> completado) ==============
void iniciarAsentamientoFinal(float distanciaAntesDeFrenarCm) {
  frenarMotores();
  const SensorSnapshot s = sensar();
  copiarBase(ticksBaseAsentamiento, s);
  distanciaInicioAsentamientoCm = distanciaAntesDeFrenarCm;
  ticksAsentamientoAnterior = 0.0f;
  inicioAsentamientoMs = millis();
  ultimoMovimientoAsentamientoMs = inicioAsentamientoMs;
  pasoArrastreFrenoCm = 0.0f;
  pasoAsentamientoMs = 0;
  fase = Fase::ASENTAMIENTO_FINAL;
  strncpy(faseComando, "asentando", sizeof(faseComando));
}

bool controlarAsentamientoFinal() {
  const SensorSnapshot s = sensar();
  int64_t d[4]; deltas(ticksBaseAsentamiento, s, d);
  const float ticksArrastre = estimarTicksAvance(d);
  if (ticksArrastre < 0.0f) { fallo("enc_no_estimation_settle"); return false; }
  const float cmPorTick = ControlRuta::distanciaPorTick(WHEEL_DIAMETER_ODOMETRY_CM, ENCODER_PPR);
  pasoArrastreFrenoCm = ticksArrastre * cmPorTick;
  pasoDistanciaActualCm = distanciaInicioAsentamientoCm + pasoArrastreFrenoCm;
  pasoDistanciaRestanteCm = distObjetivoCm - pasoDistanciaActualCm;
  const uint32_t ahora = millis();
  pasoAsentamientoMs = ahora - inicioAsentamientoMs;

  // Sólo se confirma el paso cuando cesaron nuevos ticks y la IMU dejó de
  // reportar rotación. Así el siguiente tramo no parte de una pose congelada.
  if (ticksArrastre > ticksAsentamientoAnterior + 0.25f ||
      fabsf(s.gyro_z_filtrado_rad_s) >= 0.04f) {
    ultimoMovimientoAsentamientoMs = ahora;
  }
  ticksAsentamientoAnterior = ticksArrastre;
  actualizarErroresTrayectoria();

  const bool reposoConfirmado =
      pasoAsentamientoMs >= ASENTAMIENTO_MIN_MS &&
      ahora - ultimoMovimientoAsentamientoMs >= ASENTAMIENTO_SIN_PULSOS_MS;
  if (reposoConfirmado || pasoAsentamientoMs >= ASENTAMIENTO_MAX_MS) {
    return true;
  }
  return false;
}

void iniciarVerificacionFinal() {
  frenarMotores();
  inicioVerificacionFinalMs = millis();
  fase = Fase::VERIFICAR_FINAL;
  strncpy(faseComando, "verif_fin", sizeof(faseComando));
}

void iniciarRecuperacionEndpoint() {
  ControlRuta::abrirEpisodioRecuperacion(episodioRecuperacion, millis());
  actualizarErroresTrayectoria();
  const float distancia = PoseGlobal.distanciaAlObjetivo(pasoTargetX, pasoTargetY);
  pasoDistanciaRecuperacionCm = distancia;
  if (!isfinite(distancia) || distancia < 0.5f || distancia > STEP_MAX_DISTANCE_CM) {
    strncpy(pasoDecisionRecuperacion, "invalid_distance", sizeof(pasoDecisionRecuperacion));
    fallo("endpoint_not_reached");
    return;
  }
  const ControlRuta::DecisionEndpoint decision = ControlRuta::decidirEndpointSeguro(
      true, false, pasoIntentoRecentrado, INTENTOS_RECUPERACION_ENDPOINT_MAX, distancia,
      DISTANCIA_MINIMA_RECUPERACION_ENDPOINT_CM);
  if (decision == ControlRuta::DecisionEndpoint::CALIBRAR) {
    strncpy(pasoDecisionRecuperacion, "strict_residual", sizeof(pasoDecisionRecuperacion));
    fallo("endpoint_not_reached");
    return;
  }
  if (decision == ControlRuta::DecisionEndpoint::FALLAR) {
    strncpy(pasoDecisionRecuperacion, "attempt_limit", sizeof(pasoDecisionRecuperacion));
    fallo("endpoint_not_reached");
    return;
  }
  ++pasoIntentoRecentrado;
  intentosEndpoint = pasoIntentoRecentrado;
  pasoIntentosEndpoint = pasoIntentoRecentrado;
  recuperacionEndpointActiva = true;
  const ControlRuta::PuntoReingreso objetivoReingreso = {
      pasoTargetX, pasoTargetY, 0.0f};
  const auto candidatoAvance = ControlRuta::evaluarCandidatoReingreso(
      PoseGlobal.getX(), PoseGlobal.getY(), objetivoReingreso,
      pasoHeading, distanciaPlanificadaCm, heading360, 1,
      pasoRumboFinalDeg);
  const auto candidatoReversa = ControlRuta::evaluarCandidatoReingreso(
      PoseGlobal.getX(), PoseGlobal.getY(), objetivoReingreso,
      pasoHeading, distanciaPlanificadaCm, heading360, -1,
      pasoRumboFinalDeg);
  const auto candidato = ControlRuta::elegirCandidatoReingreso(
      candidatoAvance, candidatoReversa);
  direccionTraslacion = candidato.direccion;
  direccionReingreso = candidato.direccion;
  rumboReingresoTrayectoDeg = candidato.rumboTrayectoDeg;
  rumboReingresoCuerpoDeg = candidato.rumboCuerpoDeg;
  pasoGiroHaciaLineaDeg = candidato.giroHaciaLineaDeg;
  pasoGiroRetornoDeg = candidato.giroRetornoDeg;
  pasoCosteReingresoAvance = candidatoAvance.coste;
  pasoCosteReingresoReversa = candidatoReversa.coste;
  pasoCosteReingresoElegido = candidato.coste;
  pasoRecuperacionUsaReversa = direccionTraslacion < 0;
  pasoEnReversa = pasoRecuperacionUsaReversa;
  const float rumboRecuperacion = candidato.rumboTrayectoDeg;
  strncpy(pasoDecisionRecuperacion, pasoRecuperacionUsaReversa
      ? "reverse_no_pivot" : "forward_recovery", sizeof(pasoDecisionRecuperacion));
  strncpy(pasoModoEfectivo, "recovery", sizeof(pasoModoEfectivo));
  pasoDistanciaCm = distancia;
  pasoDistanciaObjetivoCm = distancia;
  distAcumuladaCm = 0.0f;
  inicioFaseRecentradoMs = millis();
  ControlRuta::iniciarSeguimientoProgreso(
      seguimientoProgreso, distanciaRestanteObjetivoActivo(false));
  pasoRumboTrayectoDeg = rumboRecuperacion;
  pasoRumboCuerpoDeg = ControlRuta::rumboCuerpoParaTrayecto(
      rumboRecuperacion, direccionTraslacion);
  if (fabsf(errorAng360(pasoRumboCuerpoDeg, heading360)) <= TOLERANCIA_GIRO_DEG) {
    // La reversa al punto posterior conserva el chasis: pasa directo a avance
    // y evita el giro que rompía la secuencia de pasos consecutivos.
    iniciarAvance(true);
  } else {
    iniciarBaseGiro(pasoRumboCuerpoDeg, Fase::GIRO_RECUPERACION);
  }
}

void verificarObjetivoFinal() {
  frenarMotores();
  if (fabsf(errorAng360(pasoRumboFinalDeg, heading360)) > TOLERANCIA_GIRO_DEG) {
    iniciarBaseGiro(pasoRumboFinalDeg, Fase::GIRO_FINAL);
    return;
  }
  // La ventana de asentamiento exige que tanto la orientación como el punto
  // absoluto permanezcan válidos durante los 300 ms completos; no basta con
  // que vuelvan a coincidir justo al instante de confirmar el paso.
  if (pasoObjetivoAbsoluto && !objetivoAbsolutoAlcanzado()) {
    iniciarRecuperacionEndpoint();
    return;
  }
  if (millis() - inicioVerificacionFinalMs >= TURN_SETTLE_MS) {
    completarPaso();
  }
}

void completarPaso() {
  const float distancia = pasoObjetivoAbsoluto
      ? PoseGlobal.distanciaAlObjetivo(pasoTargetX, pasoTargetY) : 0.0f;
  const ControlRuta::DecisionEndpoint decision = ControlRuta::decidirEndpointSeguro(
      pasoObjetivoAbsoluto, objetivoAbsolutoAlcanzado(), pasoIntentoRecentrado,
      INTENTOS_RECUPERACION_ENDPOINT_MAX, distancia,
      DISTANCIA_MINIMA_RECUPERACION_ENDPOINT_CM);
  if (decision == ControlRuta::DecisionEndpoint::CALIBRAR) {
    pasoDistanciaRecuperacionCm = distancia;
    strncpy(pasoDecisionRecuperacion, "strict_residual", sizeof(pasoDecisionRecuperacion));
    fallo("endpoint_not_reached");
    return;
  }
  if (decision == ControlRuta::DecisionEndpoint::FALLAR) {
    fallo("endpoint_not_reached");
    return;
  }
  if (decision == ControlRuta::DecisionEndpoint::RECUPERAR) {
    iniciarRecuperacionEndpoint();
    return;
  }
  recuperacionEndpointActiva = false;
  progresoComando = 1.0f;
  fin(EVT_COMPLETED, "step_ok");
}

void iniciarPasoInterno() {
  float errInicial = errorAng360(pasoRumboCuerpoDeg, heading360);
  if (fabsf(errInicial) <= TOLERANCIA_GIRO_DEG) {
    iniciarAvance(false);
  } else {
    iniciarBaseGiro(pasoRumboCuerpoDeg, Fase::GIRO_INICIAL);
  }
}

} // namespace

// ===== API publica =====
float normalizar360(float a) {
  a = fmodf(a, 360.0f);
  if (a < 0.0f) a += 360.0f;
  return a;
}
void reiniciarControlRumbo() {
  estadoPIRumbo.integralGradoS = 0.0f;
  pasoIntegralRumboGradoS = 0.0f;
  pasoControlRumboPwm = 0.0f;
  pasoControlRumboP = 0.0f;
  pasoControlRumboI = 0.0f;
  pasoControlRumboD = 0.0f;
  pasoControlEncoderPwm = 0.0f;
  pasoControlLateralDeg = 0.0f;
}

void registrarMotivoFinalizacion(const char* detalle) {
  strncpy(pasoMotivoFinalizacion, detalle ? detalle : "", sizeof(pasoMotivoFinalizacion) - 1);
  pasoMotivoFinalizacion[sizeof(pasoMotivoFinalizacion) - 1] = '\0';
}
bool enFaseAvance() {
  return fase == Fase::AVANCE || fase == Fase::AVANCE_PULSADO ||
         fase == Fase::AVANCE_A_EJE;
}
bool enFaseTraslacion() {
  return fase == Fase::AVANCE || fase == Fase::AVANCE_PULSADO ||
         fase == Fase::AVANCE_A_EJE ||
         fase == Fase::ASENTAMIENTO_FINAL;
}
bool enFaseGiro() {
  return fase == Fase::GIRO_INICIAL || fase == Fase::GIRO_RECUPERACION ||
         fase == Fase::GIRO_A_REINGRESO || fase == Fase::GIRO_A_RUMBO ||
         fase == Fase::GIRO_REALINEAR_EJE ||
         fase == Fase::GIRO_FINAL || fase == Fase::GIRO_SOLO ||
         fase == Fase::CAL_VALIDAR_25 || fase == Fase::CAL_RETORNO;
}
bool enFaseCalibracion() {
  return estadoActual == CALIBRANDO;
}
DiagnosticoCalibracion obtenerDiagnosticoCalibracion() {
  DiagnosticoCalibracion copia = diagnosticoCal;
  copia.activa = estadoActual == CALIBRANDO;
  return copia;
}

bool iniciarCalibracion(int seq) {
  if (estadoActual != DESARMADO && estadoActual != LISTO) return false;
  const SensorSnapshot s = sensar();
  if (!s.mpu_present || !s.mpu_calibrated || s.mpu_stale) return false;
  seqActivo = seq; robotCalibrado = false;
  xInicioCalCm = PoseGlobal.getX();
  yInicioCalCm = PoseGlobal.getY();
  ++pasoEjecucionId;
  pasoObjetivoAbsoluto = false;
  tieneTargetEspacial = false;
  registrarMotivoFinalizacion("");
  WatchdogSeguridad.prepararRevalidacionEncoders();
  reiniciarDiagnosticoCalibracion();
  fase = Fase::CAL_CUENTA; inicioFaseMs = millis();
  strncpy(faseComando, "cal", sizeof(faseComando));
  estadoActual = CALIBRANDO; progresoComando = 0.0f;
  encolarEvento(EVT_ACCEPTED, seq, "accepted");
  return true;
}

bool iniciarPaso(float heading, float distanciaCm, int seq, float targetX, float targetY,
                 bool objetivoAbsoluto, ModoPaso modoPaso) {
  if (estadoActual != LISTO) return false;
  // Rechazar NaN/Inf antes de normalizar o tocar cualquier estado del paso.
  if (!std::isfinite(heading) || !std::isfinite(distanciaCm)) return false;
  if (distanciaCm < 0.5f || distanciaCm > STEP_MAX_DISTANCE_CM) return false;
  if (objetivoAbsoluto && (!std::isfinite(targetX) || !std::isfinite(targetY) ||
                           fabsf(targetX) > STEP_TARGET_MAX_ABS_CM ||
                           fabsf(targetY) > STEP_TARGET_MAX_ABS_CM)) return false;
  heading = normalizar360(heading);
  if (seq == 1) { ultimoSeqCompletado = 0; }
  if (seq <= ultimoSeqCompletado) {
    encolarEvento(EVT_COMPLETED, seq, "already_done");
    return true;
  }
  seqActivo = seq;
  ++pasoEjecucionId;
  pasoHeading = heading;
  pasoRumboTrayectoDeg = heading;
  switch (modoPaso) {
    case PASO_REVERSA:
      direccionTraslacion = -1;
      strncpy(pasoModoSolicitado, "reverse", sizeof(pasoModoSolicitado));
      break;
    case PASO_AUTO:
      direccionTraslacion = ControlRuta::reversaAutomatica(
          heading, heading360, UMBRAL_REVERSA_AUTOMATICA_DEG) ? -1 : 1;
      strncpy(pasoModoSolicitado, "auto", sizeof(pasoModoSolicitado));
      break;
    case PASO_ADELANTE:
    default:
      direccionTraslacion = 1;
      strncpy(pasoModoSolicitado, "forward", sizeof(pasoModoSolicitado));
      break;
  }
  strncpy(pasoModoEfectivo, direccionTraslacion < 0 ? "reverse" : "forward",
          sizeof(pasoModoEfectivo));
  pasoEnReversa = direccionTraslacion < 0;
  pasoRumboCuerpoDeg = ControlRuta::rumboCuerpoParaTrayecto(heading, direccionTraslacion);
  // El modo automático conserva la orientación de entrada si decide reversa;
  // el planificador puede terminar toda la misión con turn_to si necesita un
  // cardinal explícito. Los comandos antiguos siguen terminando en heading.
  pasoRumboFinalDeg = (modoPaso == PASO_AUTO && direccionTraslacion < 0)
      ? normalizar360(heading360)
      : (direccionTraslacion < 0 && modoPaso == PASO_REVERSA
          ? normalizar360(heading360) : heading);
  pasoDistanciaCm = distanciaCm;
  pasoHeadingObjetivo = pasoRumboFinalDeg;
  pasoDistanciaObjetivoCm = distanciaCm;
  pasoDistanciaActualCm = 0.0f;
  pasoDistanciaRestanteCm = distanciaCm;
  pasoFrenoPrevistoCm = 0.0f;
  pasoArrastreFrenoCm = 0.0f;
  pasoAsentamientoMs = 0;
  pasoObjetivoAbsoluto = objetivoAbsoluto;
  pasoIntentosEndpoint = 0;
  registrarMotivoFinalizacion("");
  strncpy(pasoDecisionRecuperacion, "none", sizeof(pasoDecisionRecuperacion));
  pasoDistanciaRecuperacionCm = 0.0f;
  pasoRecuperacionUsaReversa = false;
  pasoIntentoRecentrado = 0;
  pasoReingresoXCm = NAN;
  pasoReingresoYCm = NAN;
  pasoErrorLateralInicialRecentradoCm = 0.0f;
  pasoMejoraRecentradoCm = 0.0f;
  pasoProporcionLateral = 0.0f;
  pasoUmbralLateralCm = 0.0f;
  pasoVentanaInicioCm = 0.0f;
  pasoVentanaProgresoCm = 0.0f;
  pasoVentanaIndice = 0;
  pasoGiroHaciaLineaDeg = 0.0f;
  pasoGiroRetornoDeg = 0.0f;
  pasoCosteReingresoAvance = 0.0f;
  pasoCosteReingresoReversa = 0.0f;
  pasoCosteReingresoElegido = 0.0f;
  pasoAutoridadMpu = false;
  ControlRuta::cancelarEpisodioRecuperacion(episodioRecuperacion);
  inicioFaseRecentradoMs = 0;
  strncpy(pasoFaseRecentrado, "inactive", sizeof(pasoFaseRecentrado));
  strncpy(pasoDisparadorRecentrado, "none", sizeof(pasoDisparadorRecentrado));
  intentosEndpoint = 0;
  recuperacionEndpointActiva = false;
  direccionReingreso = direccionTraslacion;
  rumboReingresoTrayectoDeg = heading;
  rumboReingresoCuerpoDeg = pasoRumboCuerpoDeg;
  errorLateralAnteriorCm = 0.0f;
  errorLateralAnteriorValido = false;
  distanciaPlanificadaCm = distanciaCm;
  direccionTraslacionPlanificada = direccionTraslacion;

  if (objetivoAbsoluto) {
    pasoTargetX = targetX;
    pasoTargetY = targetY;
    tieneTargetEspacial = true;
  } else {
    float headingRad = heading * M_PI / 180.0f;
    pasoTargetX = PoseGlobal.getX() + distanciaCm * sinf(headingRad);
    pasoTargetY = PoseGlobal.getY() + distanciaCm * cosf(headingRad);
    tieneTargetEspacial = true;
  }
  pasoTargetXObjetivoCm = pasoTargetX;
  pasoTargetYObjetivoCm = pasoTargetY;
  actualizarErroresTrayectoria();

  distTargetMinimaCm = 1e9f;

  fase = Fase::GIRO_INICIAL;
  estadoActual = EJECUTANDO;
  progresoComando = 0.0f;
  encolarEvento(EVT_ACCEPTED, seq, "accepted");
  iniciarPasoInterno();
  return true;
}

bool iniciarGiroAbsoluto(float heading, int seq) {
  if (estadoActual != LISTO) return false;
  // normalizar360(NaN) conserva NaN; validarlo aquí evita contaminar el
  // objetivo y el estado global del giro.
  if (!std::isfinite(heading)) return false;
  heading = normalizar360(heading);
  if (seq == 1) ultimoSeqCompletado = 0;
  if (seq <= ultimoSeqCompletado) {
    encolarEvento(EVT_COMPLETED, seq, "already_done");
    return true;
  }
  seqActivo = seq;
  ++pasoEjecucionId;
  pasoHeadingObjetivo = heading;
  pasoDistanciaObjetivoCm = 0.0f;
  pasoDistanciaActualCm = 0.0f;
  pasoObjetivoAbsoluto = false;
  tieneTargetEspacial = false;
  pasoTargetXObjetivoCm = NAN;
  pasoTargetYObjetivoCm = NAN;
  registrarMotivoFinalizacion("");
  estadoActual = EJECUTANDO;
  progresoComando = 0.0f;
  encolarEvento(EVT_ACCEPTED, seq, "accepted");
  iniciarBaseGiro(heading, Fase::GIRO_SOLO);
  return true;
}


void cancelarMovimiento(const char* detalle) {
  const int cancelado = seqActivo;
  const bool falloEnclavado = ControlSeguridad::stopDebePreservarFallo(
      estadoActual == FALLO, estadoActual == ESTOP);
  frenarMotores(); fase = Fase::NINGUNA;
  ControlRuta::cancelarEpisodioRecuperacion(episodioRecuperacion);
  reiniciarControlRumbo();
  antiFriccionActiva = false;
  antiFriccionPulsoEncendido = false;
  antiFriccionPwmObjetivo = 0;
  if (falloEnclavado) {
    // STOP siempre desenergiza, pero nunca rearma silenciosamente un fallo.
    // El operador debe usar CLEAR_FAULT para volver a LISTO.
    progresoComando = 0.0f;
    return;
  }
  registrarMotivoFinalizacion(detalle ? detalle : "stopped");
  estadoActual = robotCalibrado ? LISTO : DESARMADO;
  progresoComando = 0.0f;
  if (cancelado != 0) encolarEvento(EVT_REJECTED, cancelado, detalle ? detalle : "stopped");
}

void forzarFalloMovimiento(const char* detalle) {
  fin(EVT_FAULT, detalle ? detalle : "control_connection_lost");
}

void controlarMovimiento() {
  if (estadoActual == CALIBRANDO) { controlarCalibracion(); return; }
  if (estadoActual != EJECUTANDO) return;

  // El MPU es la autoridad angular de toda maniobra autónoma. Ante una
  // muestra ausente, vieja o sin calibración se desenergiza y se enclava el
  // fallo; los encoders permanecen únicamente como banderas diagnósticas.
  if (fase != Fase::NINGUNA) {
    const SensorSnapshot s = sensar();
    pasoAutoridadMpu = s.mpu_present && s.mpu_calibrated && !s.mpu_stale;
    if (!pasoAutoridadMpu) {
      frenarMotores();
      strncpy(pasoDisparadorRecentrado, "mpu_lost", sizeof(pasoDisparadorRecentrado));
      strncpy(pasoFaseRecentrado, "fault", sizeof(pasoFaseRecentrado));
      fallo("mpu_lost_recovery");
      return;
    }
  }
  // El límite lateral también debe gobernar pausas, giros y asentamiento: si
  // el robot ya quedó fuera de la zona segura, no se intenta corregirlo desde
  // una subfase que no está avanzando sobre la línea.
  if (vigilarDesviacionMaxima()) return;

  if (ControlRuta::procesarEpisodioRecuperacion(
          episodioRecuperacion, millis(), RECENTER_TIMEOUT_MS) ==
      ControlRuta::EstadoEpisodioRecuperacion::VENCIDO) {
    fallo("recenter_timeout");
    return;
  }

  switch (fase) {
    case Fase::GIRO_INICIAL:
    case Fase::GIRO_RECUPERACION:
    case Fase::GIRO_A_REINGRESO:
    case Fase::GIRO_A_RUMBO:
    case Fase::GIRO_REALINEAR_EJE:
    case Fase::GIRO_FINAL:
    case Fase::GIRO_SOLO:
    case Fase::CAL_RETORNO:
      controlarGiro();
      break;
    case Fase::AVANCE:
    case Fase::AVANCE_PULSADO:
    case Fase::PAUSA_REEVALUACION:
    case Fase::AVANCE_A_EJE:
      if (fase == Fase::AVANCE_PULSADO) {
        if (controlarAvancePulsado()) {
          iniciarVerificacionFinal();
        }
      } else if (controlarAvance()) {
        iniciarVerificacionFinal();
      }
      break;
    case Fase::PAUSA_RECENTRADO:
      frenarMotores();
      if (millis() - inicioFaseRecentradoMs >= RECENTER_SETTLE_MS) {
        const float rumbo = rumboReingresoCuerpoDeg;
        if (strncmp(pasoDecisionRecuperacion, "realign_axis", sizeof(pasoDecisionRecuperacion)) == 0) {
          strncpy(pasoFaseRecentrado, "realign_axis", sizeof(pasoFaseRecentrado));
          iniciarBaseGiro(rumbo, Fase::GIRO_REALINEAR_EJE);
        } else {
          strncpy(pasoFaseRecentrado, "turn_to_axis", sizeof(pasoFaseRecentrado));
          iniciarBaseGiro(rumbo, Fase::GIRO_A_REINGRESO);
        }
      }
      break;
    case Fase::VERIFICAR_REINGRESO:
      verificarReingreso();
      break;
    case Fase::ASENTAMIENTO_FINAL:
      if (controlarAsentamientoFinal()) {
        iniciarVerificacionFinal();
      }
      break;
    case Fase::VERIFICAR_FINAL:
      verificarObjetivoFinal();
      break;
    default: break;
  }

  // progreso general
  if (estadoActual == EJECUTANDO || estadoActual == CALIBRANDO) {
    if (fase == Fase::GIRO_INICIAL) progresoComando = min(0.30f, progresoComando);
    else if (fase == Fase::AVANCE || fase == Fase::AVANCE_PULSADO || fase == Fase::ASENTAMIENTO_FINAL)
      progresoComando = 0.30f + 0.60f * fminf(1.0f, pasoDistanciaActualCm/max(pasoDistanciaObjetivoCm,0.1f));
    else if (fase == Fase::GIRO_FINAL) progresoComando = 0.90f + 0.10f * min(1.0f, (millis()-estableGiroDesdeMs)/float(TURN_SETTLE_MS));
  }
}

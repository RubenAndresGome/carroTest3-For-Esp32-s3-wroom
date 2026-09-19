#include "Cinematica.h"
#include "Comandos.h"
#include "Config.h"
#include "ControlRuta.h"
#include "ControlAngular.h"
#include "ControlCalibracion.h"
#include "ControlSeguridad.h"
#include "Debug.h"
#include "Eventos.h"
#include "Motores.h"
#include "PoseEstimator.h"
#include "Sensores.h"
#include "MemoriaTorque.h"
#include "ControlTorque.h"
#include <Arduino.h>
#include <cmath>

namespace {

enum class Fase : uint8_t {
  NINGUNA,
  CAL_CUENTA, CAL_A, CAL_VALIDAR_25, CAL_PAUSA, CAL_B, CAL_PAUSA_RETORNO, CAL_RETORNO,
  GIRO_INICIAL, AVANCE, GIRO_RECUPERACION, GIRO_FINAL, GIRO_SOLO,
  PAUSA_REEVALUACION, ASENTAMIENTO_FINAL, VERIFICAR_FINAL,
  PAUSA_PRE_GIRO, PAUSA_PRE_AVANCE
};

Fase fase = Fase::NINGUNA;

// --- forward declarations (funciones internas que se llaman entre si) ---
void iniciarBaseGiro(float objetivoDeg, Fase retorno);
void controlarGiro();
void completarGiro();
void iniciarPausaPreGiro();
void controlarPausaPreGiro();
void iniciarPausaPreAvance(bool conservar);
void controlarPausaPreAvance();
void iniciarAvance(bool conservar);
bool controlarAvance();
void iniciarAsentamientoFinal(float distanciaAntesDeFrenarCm);
bool controlarAsentamientoFinal();
void completarPaso();
void iniciarVerificacionFinal();
void verificarObjetivoFinal();
void actualizarErroresTrayectoria();
[[maybe_unused]] void iniciarRecuperacionEndpoint();
[[maybe_unused]] void completarPasoConCorreccionPendiente();
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
// Origen real de la recta directriz del tramo. Se fija en la pose del robot al
// iniciar cada avance para que el desvio lateral inicial sea exactamente cero.
float pasoOrigenTramoXCm = 0.0f;
float pasoOrigenTramoYCm = 0.0f;
ControlRuta::EstadoPI estadoPIRumbo = {};
uint32_t inicioVerificacionFinalMs = 0;
uint8_t intentosEndpoint = 0;
bool recuperacionEndpointActiva = false;
bool giroFinalRealizado = false;
int direccionTraslacion = 1;
int64_t ticksBaseAsentamiento[4] = {};
float distanciaInicioAsentamientoCm = 0.0f;
float ticksAsentamientoAnterior = 0.0f;
uint32_t inicioAsentamientoMs = 0;
uint32_t ultimoMovimientoAsentamientoMs = 0;
uint32_t inicioPulsoAproximacionMs = 0;
bool pulsoAproximacionEncendido = false;
float distAcumuladaCm = 0.0f;
bool pausaPreAvanceConservar = false;


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
  antiFriccionActiva = false;
  antiFriccionPulsoEncendido = false;
  antiFriccionPwmObjetivo = 0;
  pulsoAproximacionEncendido = false;
  inicioPulsoAproximacionMs = 0;
  reiniciarControlRumbo();
  registrarMotivoFinalizacion(d);
  if (t == EVT_COMPLETED) { estadoActual = LISTO; progresoComando = 1.0f; }
  else if (t == EVT_FAULT) { estadoActual = FALLO; }
  encolarEvento(t, seqActivo, d);
}
void fallo(const char* d) { fin(EVT_FAULT, d); }

// ============== CALIBRACION (test: rampa continua, ambos sentidos, retorno) ==============
int  candidatoCal = 1;
int  pwmCal = 0;
uint32_t ultimoRampaCalMs = 0;
uint32_t inicioMovCalMs = 0;
uint32_t inicioPausaReintentoCalMs = 0;
uint32_t ultimaAuditoriaMaxCalMs = 0;
uint32_t stallMaxCalAcumMs[2] = {};
uint32_t inicioFaseMs = 0;
int64_t ticksBaseCal[4] = {};
float yawInicioCalDeg = 0.0f;
int  candidatoGiroPos = 1, candidatoGiroNeg = -1;
int  pwmMinGiroPos = static_cast<int>(148 * PWM_SCALE_8_TO_10), pwmMinGiroNeg = static_cast<int>(148 * PWM_SCALE_8_TO_10);
DiagnosticoCalibracion diagnosticoCal = {};

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
  diagnosticoCal.candidatoDireccion = candidatoCal;
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
  }
}

void conservarEncodersAisladosDelDiagnostico() {
  bool degradado = false;
  for (int i = 0; i < 4; ++i) {
    if (diagnosticoCal.encoderAislado[i]) {
      encoderConfiableGlobal[i] = false;
      estadoSaludEncoderGlobal[i] = EstadoSaludEncoder::EXCLUDED;
    }
    degradado |= !encoderConfiableGlobal[i];
  }
  modoDegradado = degradado;
}


void iniciarFaseCal(Fase f) { fase = f; inicioFaseMs = millis(); }

static bool intentoInvertidoCalA = false;

void calCuenta() {
  if (millis() - inicioFaseMs < CUENTA_CALIBRACION_MS) { progresoComando = min(0.10f, (millis()-inicioFaseMs)/float(CUENTA_CALIBRACION_MS)*0.10f); return; }
  const SensorSnapshot s = sensar();
  if (!s.mpu_present || s.mpu_stale || !s.mpu_calibrated) { fallo("mpu_unavailable_cal"); return; }
  recalibrarOffsetIMU(150);
  yawInicioCalDeg = normalizar360(anguloZ);
  pwmMinGiroPos=0; pwmMinGiroNeg=0; candidatoGiroPos=0; candidatoGiroNeg=0;
  candidatoCal=1; pwmCal=CALIBRATION_PWM_START; ultimoRampaCalMs=millis(); inicioMovCalMs=0; inicioPausaReintentoCalMs=0;
  ultimaAuditoriaMaxCalMs=0; stallMaxCalAcumMs[0]=stallMaxCalAcumMs[1]=0;
  intentoInvertidoCalA = false;
  reiniciarDiagnosticoCalibracion();
  const SensorSnapshot fresca = sensar();
  copiarBase(ticksBaseCal, fresca);
  iniciarFaseCal(Fase::CAL_A);
  strncpy(faseComando, "cal_a", sizeof(faseComando));
}

void calTorque(bool primera) {
  const uint32_t ahora = millis();
  if (inicioPausaReintentoCalMs) {
    frenarMotores();
    if (ahora - inicioPausaReintentoCalMs < CAL_RETRY_PAUSE_MS) return;
    inicioPausaReintentoCalMs = 0;
    const SensorSnapshot reinicio = sensar();
    copiarBase(ticksBaseCal, reinicio);
    pwmCal = (primera && intentoInvertidoCalA) ? CALIBRATION_PWM_RETRY_START : CALIBRATION_PWM_START;
    ultimoRampaCalMs = ahora;
    inicioMovCalMs = 0;
    ultimaAuditoriaMaxCalMs = 0;
    stallMaxCalAcumMs[0]=stallMaxCalAcumMs[1]=0;
  }
  const SensorSnapshot s = sensar();
  int64_t d[4]; deltas(ticksBaseCal, s, d);
  const ControlCalibracion::EvaluacionEncoders evaluacion =
      ControlCalibracion::evaluarEncoders(d, CAL_TICKS_MOVIMIENTO, encoderConfiableGlobal);
  const ControlCalibracion::EvidenciaMovimiento evidencia =
      ControlCalibracion::evaluarMovimiento(fabsf(s.gyro_z_filtrado_rad_s),
                                            GYRO_MOVEMENT_RAD_S, d, CAL_TICKS_MOVIMIENTO);
  const bool ladoIzqOk = evaluacion.ladoIzquierdoValido;
  const bool ladoDerOk = evaluacion.ladoDerechoValido;
  // MPU como autoridad angular central: el movimiento de calibracion exige confirmacion del MPU
  // (gyroConfirmado) corroborada por PCNT (al menos un encoder responde o algun lado valido con ticks).
  const bool movimientoDetectado = evidencia.gyroConfirmado &&
                                   (evidencia.pcntCorroborado || ladoIzqOk || ladoDerOk);
  actualizarDiagnosticoCalibracion(d, evaluacion);
  if (!aplicarVelocidades(candidatoCal * pwmCal, -candidatoCal * pwmCal)) {
    fallo("motor_output_error");
    return;
  }

  if (movimientoDetectado) {
    if (!inicioMovCalMs) inicioMovCalMs = ahora;
    if (ahora - inicioMovCalMs >= CAL_MOVE_SUSTAINED_MS) {
      conservarEncodersAisladosDelDiagnostico();
      int guardado = min(PWM_TURN_MAX_LIMIT, pwmCal + PWM_CALIBRATION_MARGIN);
      frenarMotores();
      if (primera) {
        if (s.gyro_z_filtrado_rad_s < 0) {
          if (intentoInvertidoCalA) {
            fallo("cal_yaw_sign_mismatch");
            return;
          }
          intentoInvertidoCalA = true;
          candidatoCal = -candidatoCal;
          inicioPausaReintentoCalMs = ahora;
          inicioMovCalMs = 0;
          return;
        }
        candidatoGiroPos = candidatoCal;
        pwmMinGiroPos = guardado;
        iniciarBaseGiro(normalizar360(yawInicioCalDeg + CALIBRACION_GIRO_TEST_DEG), Fase::CAL_VALIDAR_25);
        progresoComando=0.35f;
        strncpy(faseComando,"cal_mas_25",sizeof(faseComando));
      }
      else {
        if (s.gyro_z_filtrado_rad_s > 0) {
          candidatoCal = -candidatoCal;
          inicioPausaReintentoCalMs = ahora;
          inicioMovCalMs = 0;
          return;
        }
        candidatoGiroNeg = candidatoCal;
        pwmMinGiroNeg = guardado;
        if (candidatoGiroPos == candidatoGiroNeg) { fallo("cal_dir_failed"); return; }
        iniciarFaseCal(Fase::CAL_PAUSA_RETORNO); progresoComando=0.85f; strncpy(faseComando,"cal_retorno",sizeof(faseComando));
      }
    }
  } else { inicioMovCalMs = 0; }

  if (ahora - ultimoRampaCalMs >= CAL_RAMP_INTERVAL_MS) {
    ultimoRampaCalMs = ahora;
    pwmCal = min(CALIBRATION_PWM_END, pwmCal + CALIBRATION_PWM_STEP);
  }

  if (pwmCal >= CALIBRATION_PWM_END && !evidencia.gyroConfirmado) {
    if (!ultimaAuditoriaMaxCalMs) ultimaAuditoriaMaxCalMs = ahora;
    const uint32_t lapso = ahora - ultimaAuditoriaMaxCalMs;
    ultimaAuditoriaMaxCalMs = ahora;
    if (!ladoIzqOk) stallMaxCalAcumMs[0] += lapso;
    if (!ladoDerOk) stallMaxCalAcumMs[1] += lapso;
    if (stallMaxCalAcumMs[0] >= CAL_MAX_PWM_STALL_MS || stallMaxCalAcumMs[1] >= CAL_MAX_PWM_STALL_MS) {
      if (primera && !intentoInvertidoCalA) {
        // El candidato inicial no logro mover el chasis (stall en polaridad inicial).
        // Invertir a candidato opuesto y reintentar busqueda en CAL_A.
        intentoInvertidoCalA = true;
        candidatoCal = -candidatoCal;
        inicioPausaReintentoCalMs = ahora;
        inicioMovCalMs = 0;
        ultimaAuditoriaMaxCalMs = 0;
        stallMaxCalAcumMs[0] = stallMaxCalAcumMs[1] = 0;
        frenarMotores();
        return;
      }
      fallo(stallMaxCalAcumMs[0] >= CAL_MAX_PWM_STALL_MS ? "cal_stall_left" : "cal_stall_right");
      return;
    }
  } else {
    ultimaAuditoriaMaxCalMs = 0;
    if (evidencia.gyroConfirmado) {
      stallMaxCalAcumMs[0] = stallMaxCalAcumMs[1] = 0;
    }
  }
}

void controlarCalibracion() {
  if (estadoActual != CALIBRANDO) return;
  const SensorSnapshot s = sensar();
  if (!s.mpu_present || s.mpu_stale) { fallo("mpu_lost_cal"); return; }

  switch (fase) {
    case Fase::CAL_CUENTA: calCuenta(); break;
    case Fase::CAL_A: calTorque(true); break;
    case Fase::CAL_VALIDAR_25: controlarGiro(); break;
    case Fase::CAL_PAUSA:
      frenarMotores();
      if (millis()-inicioFaseMs >= PAUSA_RETORNO_CAL_MS) {
        candidatoCal = -candidatoGiroPos;
        int minPwmB = (pwmMinGiroPos > 0)
            ? max(CALIBRATION_PWM_START, pwmMinGiroPos - static_cast<int>(25 * PWM_SCALE_8_TO_10))
            : CALIBRATION_PWM_START;
        pwmCal = minPwmB;
        inicioPausaReintentoCalMs = 0;
        inicioMovCalMs = 0;
        ultimaAuditoriaMaxCalMs = 0;
        stallMaxCalAcumMs[0] = stallMaxCalAcumMs[1] = 0;
        copiarBase(ticksBaseCal, s);
        ultimoRampaCalMs = millis();
        iniciarFaseCal(Fase::CAL_B);
        strncpy(faseComando, "cal_b", sizeof(faseComando));
        progresoComando = 0.60f;
      }
      break;
    case Fase::CAL_B: calTorque(false); break;
    case Fase::CAL_PAUSA_RETORNO:
      frenarMotores();
      if (millis()-inicioFaseMs >= PAUSA_RETORNO_CAL_MS) {
        float retorno = normalizar360(yawInicioCalDeg);
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
uint32_t inicioPulsoFinoGiroMs = 0;
bool pulsoFinoGiroEncendido = false;
uint8_t pulsosFinosGiro = 0;
uint32_t inicioFrenoToleranciaMs = 0;
uint32_t ultimoJerkMs = 0;
bool jerkActivo = false;
ControlSeguridad::EstadoVigilanciaDivergenciaGiro vigilanciaDivergenciaGiro;
int ultimoSignoErrorGiro = 0;

void iniciarBaseGiro(float objetivoDeg, Fase retorno) {
  // Giro y calibracion sostienen 247/255; por encima solo rafagas acotadas.
  establecerLimiteContinuoPwm(PWM_TURN_CONTINUO_LIMIT);
  reiniciarControlRumbo();
  PoseGlobal.iniciarMedicionTraslacionGiro();
  const SensorSnapshot s = sensar();
  copiarBase(ticksBaseGiroLocal, s);
  giroObjetivo = objetivoDeg;
  giroEnTol = false; movGiroConfirmado=false; watchdogGiroArmado=false;
  intentoGiro=1; inicioIntentoGiroMs=millis(); inicioGiroTotalMs=millis();
  estableGiroDesdeMs=0; pausaReintentoGiroCal=false;
  inicioPulsoFinoGiroMs=millis(); pulsoFinoGiroEncendido=false; pulsosFinosGiro=0;
  inicioFrenoToleranciaMs=0; ultimoJerkMs=0; jerkActivo=false;
  ticksLadoGiroAnt[0]=ticksLadoGiroAnt[1]=0;
  ultimoPulsoLadoGiroMs[0]=ultimoPulsoLadoGiroMs[1]=millis();
  ultimoCtrlGiroMs=0;
  ultimoAumentoTorqueGiroMs=millis();
  float errorIni = errorAng360(objetivoDeg, heading360);
  float errorIniAbs = fabsf(errorIni);
  vigilanciaDivergenciaGiro.reiniciar(errorIniAbs);
  int signoIni = errorIni > 0 ? 1 : -1;
  ultimoSignoErrorGiro = signoIni;
  int minIni = signoIni > 0 ? pwmMinGiroPos : pwmMinGiroNeg;
  if (errorIniAbs > TURN_HYBRID_THRESHOLD_DEG) {
    // Macro-giro: Arranque suave desde el par calibrado o piso seguro, permitiendo
    // que la rampa slew acelere sin romper la adherencia estática de las ruedas
    int arranqueSentido = (minIni > 0)
        ? max(PWM_TURN_FLOOR_MIN, minIni - PWM_TURN_START_FLOOR_OFFSET)
        : PWM_TURN_START_MACRO;
    pwmBusquedaGiro = arranqueSentido;
    pwmGiroAct = arranqueSentido;
    signoGiroApl = signoIni;
  } else {
    // Micro-giro fino (<4.0°): iniciar en cero para que la máquina trifásica comience con pulso limpio
    if (minIni > 0) {
      pwmBusquedaGiro = max(PWM_TURN_START, minIni - PWM_TURN_START_FLOOR_OFFSET);
    } else {
      pwmBusquedaGiro = PWM_TURN_START;
    }
    pwmGiroAct = 0;
    signoGiroApl = 0;
  }
  pwmBoostFrenado=0;
  faseRetornoGiro = retorno;
  fase = retorno;  // fase global sigue al giro
  if (retorno == Fase::GIRO_INICIAL) strncpy(faseComando,"giro_ini",sizeof(faseComando));
  else if (retorno == Fase::GIRO_FINAL) strncpy(faseComando,"giro_fin",sizeof(faseComando));
  else if (retorno == Fase::GIRO_RECUPERACION) strncpy(faseComando,"recup",sizeof(faseComando));
  else if (retorno == Fase::GIRO_SOLO) strncpy(faseComando,"giro_solo",sizeof(faseComando));
  else if (retorno == Fase::CAL_RETORNO) strncpy(faseComando,"cal_ret",sizeof(faseComando));
}

void reintentarGiro(const char* motivo) {
  frenarMotores(); pwmGiroAct=0; signoGiroApl=0; movGiroConfirmado=false; watchdogGiroArmado=false; giroEnTol=false;
  pwmBoostFrenado=0; inicioFrenoToleranciaMs=0; ultimoJerkMs=0; jerkActivo=false;
  ultimoSignoErrorGiro = 0;
  if (intentoGiro >= TURN_MAX_ATTEMPTS) { fallo(motivo); return; }
  ++intentoGiro;
  pausaReintentoGiroCal = true;
  inicioPausaReintGiroMs = millis();
}

void controlarGiro() {
  if (fase != Fase::GIRO_INICIAL && fase != Fase::GIRO_FINAL && fase != Fase::GIRO_RECUPERACION && fase != Fase::GIRO_SOLO && fase != Fase::CAL_VALIDAR_25 && fase != Fase::CAL_RETORNO) return;
  uint32_t ahora = millis();
  if (ahora - ultimoCtrlGiroMs < TURN_CONTROL_PERIOD_MS) return;
  ultimoCtrlGiroMs = ahora;

  if (pausaReintentoGiroCal) {
    frenarMotores();
    if (ahora - inicioPausaReintGiroMs < TURN_RETRY_PAUSE_MS) return;
    pausaReintentoGiroCal = false;
    const SensorSnapshot s = sensar();
    copiarBase(ticksBaseGiroLocal, s);
    ticksLadoGiroAnt[0]=ticksLadoGiroAnt[1]=0;
    ultimoPulsoLadoGiroMs[0]=ultimoPulsoLadoGiroMs[1]=ahora;
    watchdogGiroArmado=false;
    inicioIntentoGiroMs = ahora;
    ultimoAumentoTorqueGiroMs=ahora;
    float errorReint = errorAng360(giroObjetivo, heading360);
    vigilanciaDivergenciaGiro.reiniciar(fabsf(errorReint));
    int signoReint = errorReint > 0 ? 1 : -1;
    int minReint = signoReint > 0 ? pwmMinGiroPos : pwmMinGiroNeg;
    int pwmArranque;
    if (minReint > 0) {
      pwmArranque = max(PWM_TURN_START, minReint - PWM_TURN_START_FLOOR_OFFSET);
    } else {
      pwmArranque = PWM_TURN_START;
    }
    // Si el intento anterior no confirmó movimiento, conservar el PWM máximo
    // alcanzado en lugar de reiniciar desde cero. Esto evita que reintentos
    // sucesivos gasten todo TURN_ATTEMPT_TIMEOUT subiendo la rampa desde el
    // mismo punto sin nunca llegar al torque de arranque.
    pwmBusquedaGiro = max(pwmArranque, pwmBusquedaGiro);
    pwmBoostFrenado=0;
  }

  const SensorSnapshot s = sensar();
  float error = errorAng360(giroObjetivo, heading360);
  float errorAbs = fabsf(error);
  const int signoEsperado = error > 0 ? 1 : -1;
  if (ultimoSignoErrorGiro != 0 && signoEsperado != ultimoSignoErrorGiro) {
    // El chasis cruzó el rumbo objetivo (inercia / sobrepaso transitorio).
    // Reiniciar vigilancia de divergencia desde el nuevo error para permitir
    // que el lazo converja desde el nuevo lado sin disparar falsas divergencias.
    vigilanciaDivergenciaGiro.reiniciar(errorAbs);
    ultimoSignoErrorGiro = signoEsperado;
  }
  int torqueCalibrado = signoEsperado > 0 ? pwmMinGiroPos : pwmMinGiroNeg;
  if (torqueCalibrado == 0) {
    torqueCalibrado = pwmMinGiroPos > 0 ? pwmMinGiroPos : (pwmMinGiroNeg > 0 ? pwmMinGiroNeg : PWM_TURN_START);
  }
  if (!watchdogGiroArmado && torqueCalibrado > 0 && pwmGiroAct >= torqueCalibrado) {
    watchdogGiroArmado = true;
    ultimoPulsoLadoGiroMs[0]=ultimoPulsoLadoGiroMs[1]=ahora;
  }

  int64_t d[4]; deltas(ticksBaseGiroLocal, s, d);
  int64_t ladoTicks[2] = {
    (encoderConfiableGlobal[0] ? d[0] : 0) + (encoderConfiableGlobal[2] ? d[2] : 0),
    (encoderConfiableGlobal[1] ? d[1] : 0) + (encoderConfiableGlobal[3] ? d[3] : 0)
  };
  const bool ladoIzqMoviendo = (encoderConfiableGlobal[0] && d[0] >= 2) ||
                               (encoderConfiableGlobal[2] && d[2] >= 2);
  const bool ladoDerMoviendo = (encoderConfiableGlobal[1] && d[1] >= 2) ||
                               (encoderConfiableGlobal[3] && d[3] >= 2);
  const bool ambosLadosMoviendo = ladoIzqMoviendo && ladoDerMoviendo;
  const bool gyroGirando = (fabsf(s.gyro_z_filtrado_rad_s) >= GYRO_MOVEMENT_RAD_S);
  if (gyroGirando || ambosLadosMoviendo) {
    if (!movGiroConfirmado) {
      movGiroConfirmado = true;
      int torqueConfirmado = max(PWM_TURN_FLOOR_MIN, pwmGiroAct);
      if (signoEsperado > 0) pwmMinGiroPos = max(pwmMinGiroPos, torqueConfirmado);
      else pwmMinGiroNeg = max(pwmMinGiroNeg, torqueConfirmado);
      solicitarGuardarTorque(
          pwmMinGiroPos / PWM_SCALE_8_TO_10,
          pwmMinGiroNeg / PWM_SCALE_8_TO_10,
          candidatoGiroPos,
          candidatoGiroNeg
      );
    }
  }

  // Mientras no haya movimiento confirmado, mantener fresco el temporizador de pulso para evitar falsos stalls
  if (!movGiroConfirmado) {
    ultimoPulsoLadoGiroMs[0] = ultimoPulsoLadoGiroMs[1] = ahora;
  }

  for (int i=0; i<2; ++i) {
    if (ladoTicks[i] != ticksLadoGiroAnt[i]) {
      ticksLadoGiroAnt[i]=ladoTicks[i];
      ultimoPulsoLadoGiroMs[i]=ahora;
    } else if (gyroGirando) {
      ultimoPulsoLadoGiroMs[i] = ahora;
    } else if (watchdogGiroArmado && ahora-ultimoPulsoLadoGiroMs[i] > TURN_STALL_MS) {
      reintentarGiro(i==0?"turn_stall_left":"turn_stall_right");
      return;
    }
  }

  const uint32_t timeoutGiro = (fase == Fase::CAL_RETORNO) ? CAL_RETURN_TIMEOUT_MS : TURN_TIMEOUT_MS;
  if (ahora - inicioGiroTotalMs > timeoutGiro) { fallo(fase == Fase::CAL_RETORNO ? "cal_return_timeout" : "turn_timeout_total"); return; }
  if (ahora - inicioIntentoGiroMs > TURN_ATTEMPT_TIMEOUT_MS) { reintentarGiro("turn_timeout_attempt"); return; }

  // Guarda activa de divergencia angular (corte inmediato ante giro inverso o trompo descontrolado)
  const float umbralDivergencia = (fase == Fase::CAL_VALIDAR_25 || fase == Fase::CAL_RETORNO)
      ? TURN_DIVERGENCE_CALIBRATION_THRESHOLD_DEG
      : TURN_DIVERGENCE_THRESHOLD_DEG;
  const uint32_t timeoutDivergencia = (fase == Fase::CAL_VALIDAR_25 || fase == Fase::CAL_RETORNO)
      ? TURN_DIVERGENCE_CALIBRATION_TIMEOUT_MS
      : TURN_DIVERGENCE_TIMEOUT_MS;

  const bool movimientoPresenteGiro = movGiroConfirmado || (fabsf(s.gyro_z_filtrado_rad_s) >= GYRO_MOVEMENT_RAD_S) || ambosLadosMoviendo;
  if (ControlSeguridad::evaluarDivergenciaGiro(vigilanciaDivergenciaGiro, errorAbs, ahora, umbralDivergencia, timeoutDivergencia, movimientoPresenteGiro)) {
    // 1. Frenado dinámico activo inmediato para neutralizar inercia no deseada
    frenarMotoresActivo();
    pwmGiroAct = 0;
    signoGiroApl = 0;

    // 2. Reparación inteligente del error con if condicional:
    // Caso A: Si estamos en CAL_RETORNO y el robot ya está en las inmediaciones del reposo original (<= 12.0°)
    if (fase == Fase::CAL_RETORNO && errorAbs <= 12.0f) {
      completarGiro();
      return;
    }

    // Caso B: Si aún quedan reintentos en la máquina de estados de giro
    if (intentoGiro < TURN_MAX_ATTEMPTS) {
      reintentarGiro((fase == Fase::CAL_VALIDAR_25 || fase == Fase::CAL_RETORNO) ? "cal_yaw_divergence" : "turn_angular_divergence");
      return;
    }

    // Caso C: Solo si se agotaron todos los reintentos permitidos
    fallo((fase == Fase::CAL_VALIDAR_25 || fase == Fase::CAL_RETORNO) ? "cal_yaw_divergence" : "turn_angular_divergence");
    return;
  }

  // --- latch de tolerancia y freno dinámico activo por MPU ---
  // Tolerancia estricta de 1.0 grado. Fallback a 1.5 grados tras varios
  // micro-pulsos si el giroscopo esta en reposo absoluto: evita bloquear el
  // paso por ruido del MPU o backlash mecanico.
  const float tolGiro = (fase == Fase::CAL_RETORNO)
      ? TOLERANCIA_CALIBRACION_DEG
      : (pulsosFinosGiro >= TURN_PULSOS_FALLBACK ? TOLERANCIA_GIRO_FALLBACK_DEG
                                                 : TOLERANCIA_GIRO_DEG);
  if (errorAbs <= tolGiro) {
    pwmGiroAct = 0;
    signoGiroApl = 0;
    if (!giroEnTol) {
      giroEnTol = true;
      inicioFrenoToleranciaMs = ahora;
      frenarMotoresActivo();
      estableGiroDesdeMs = 0;
      return;
    }
    if (ahora - inicioFrenoToleranciaMs < TURN_BRAKE_ACTIVE_MS) {
      frenarMotoresActivo();
      return;
    }
    // Tras el pulso de freno activo (Back-EMF), reposo quieto para muestreo de giróscopo MPU
    frenarMotores();
    if (fabsf(s.gyro_z_filtrado_rad_s) > 0.02f) {
      estableGiroDesdeMs = 0;
      return;
    }
    if (!estableGiroDesdeMs) estableGiroDesdeMs = ahora;
    if (ahora - estableGiroDesdeMs >= TURN_SETTLE_MS) {
      completarGiro();
    }
    return;
  }
  // Si el chasis salió de la tolerancia tras frenar (inercia o rebote),
  // desenganchar inmediatamente para que el lazo fino vuelva a pulsar
  if (giroEnTol) {
    giroEnTol = false;
    estableGiroDesdeMs = 0;
    inicioFrenoToleranciaMs = 0;
    vigilanciaDivergenciaGiro.reiniciar(errorAbs);
  }
  estableGiroDesdeMs = 0;

  // Sacudida dinámica anti-bloqueo (dynamic jerk): si el chasis está a alta potencia pero no confirma movimiento tras 350 ms
  if (!movGiroConfirmado && (ahora - inicioIntentoGiroMs > 350)) {
    if (ahora - ultimoJerkMs >= 400) {
      ultimoJerkMs = ahora;
      jerkActivo = true;
    }
  }
  if (jerkActivo) {
    if (ahora - ultimoJerkMs < 40) {
      frenarMotoresActivo();
      pwmGiroAct = 0;
      return;
    } else {
      jerkActivo = false;
      pwmGiroAct = PWM_TURN_MAX_LIMIT;
    }
  }

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
  int pwmCerca = max(PWM_TURN_FLOOR_MIN, min(PWM_TURN_MAX_LIMIT, minimo - static_cast<int>(15 * PWM_SCALE_8_TO_10)));

  const bool detectadoSinMovimiento = (fabsf(s.gyro_z_filtrado_rad_s) < GYRO_MOVEMENT_RAD_S);

  int pwmObj = pwmLejos;
  if (!movGiroConfirmado) {
    if (ahora - ultimoAumentoTorqueGiroMs >= CAL_RAMP_INTERVAL_MS) {
      ultimoAumentoTorqueGiroMs = ahora;
      pwmBusquedaGiro = min(PWM_TURN_MAX_LIMIT, pwmBusquedaGiro + CALIBRATION_PWM_STEP);
    }
    pwmObj = max(pwmLejos, pwmBusquedaGiro);
  } else if (errorAbs < TURN_BRAKING_ZONE_DEG) {
    pwmObj = pwmCerca + aproximar((pwmLejos - pwmCerca) * errorAbs / TURN_BRAKING_ZONE_DEG);

    if (errorAbs > TURN_HYBRID_THRESHOLD_DEG) {
      // --- MODO 1: Rampa Adaptativa Rápida (4.0° a 15.0°) ---
      pulsoFinoGiroEncendido = false;
      inicioPulsoFinoGiroMs = ahora;
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
      // --- MODO 2: aproximación fina por micro-pulsos trifásicos (<4.0°) ---
      // Fase 1: 250 ms ON a par firme (75%-85% PWM, piso 70%)
      // Fase 2: 80 ms Freno Activo dinámico (Back-EMF DRV8833 IN1=1, IN2=1)
      // Fase 3: 120 ms Reposo mecánico total y lectura estricta de MPU (IN1=0, IN2=0)
      const uint32_t deltaPulso = ahora - inicioPulsoFinoGiroMs;
      if (pulsoFinoGiroEncendido) {
        if (deltaPulso >= TURN_PULSE_ON_MS) {
          pulsoFinoGiroEncendido = false;
          inicioPulsoFinoGiroMs = ahora;
          frenarMotoresActivo();
          pwmGiroAct = 0;
          return;
        }
        int pwmKick = max(PWM_TURN_FLOOR_MIN, max(minimo + static_cast<int>(15 * PWM_SCALE_8_TO_10), PWM_TURN_START));
        pwmObj = min(PWM_TURN_MAX_LIMIT, pwmKick + pwmBoostFrenado);
      } else {
        if (deltaPulso < TURN_BRAKE_ACTIVE_MS) {
          // Fase 2: Freno activo dinámico
          frenarMotoresActivo();
          pwmGiroAct = 0;
          return;
        } else if (deltaPulso < (TURN_BRAKE_ACTIVE_MS + TURN_PULSE_OFF_MS)) {
          // Fase 3: Reposo mecánico y estabilización para lectura MPU
          frenarMotores();
          pwmGiroAct = 0;
          return;
        } else {
          // Fin del ciclo completo de reposo (200 ms OFF totales) -> Iniciar nuevo pulso ON
          pulsoFinoGiroEncendido = true;
          if (pulsosFinosGiro < 255) ++pulsosFinosGiro;
          inicioPulsoFinoGiroMs = ahora;
          if (detectadoSinMovimiento) {
            pwmBoostFrenado = min(PWM_TURN_MAX_LIMIT - minimo,
                                  pwmBoostFrenado + static_cast<int>(10 * PWM_SCALE_8_TO_10));
          }
          int pwmKick = max(PWM_TURN_FLOOR_MIN, max(minimo + static_cast<int>(15 * PWM_SCALE_8_TO_10), PWM_TURN_START));
          pwmObj = min(PWM_TURN_MAX_LIMIT, pwmKick + pwmBoostFrenado);
        }
      }
    }
  } else {
    pwmBoostFrenado = 0;
    pulsoFinoGiroEncendido = false;
    inicioPulsoFinoGiroMs = ahora;
  }

  // Control de tracción por giróscopo MPU6050: si la velocidad angular ya superó el régimen seguro,
  // congelar el aumento de par para no romper el agarre estático ni causar derrape
  if (errorAbs > TURN_HYBRID_THRESHOLD_DEG && fabsf(s.gyro_z_filtrado_rad_s) >= MAX_TURN_RATE_RAD_S) {
    if (pwmObj > pwmGiroAct) pwmObj = pwmGiroAct;
  }

  // --- slew con protección de inversión (Motores.cpp añade 250 ms) ---
  if (signoGiroApl!=0 && signoDeseado!=signoGiroApl && pwmGiroAct>0) {
    pwmGiroAct = max(0, pwmGiroAct - PWM_TURN_SLEW_STEP);
    if (pwmGiroAct==0) signoGiroApl=0;
  } else {
    if (signoGiroApl==0) signoGiroApl=signoDeseado;
    int paso = movGiroConfirmado ? PWM_TURN_SLEW_STEP : PWM_TURN_START_SLEW_STEP;
    if (errorAbs <= TURN_HYBRID_THRESHOLD_DEG) {
      pwmGiroAct = pwmObj;
    } else if (pwmGiroAct<pwmObj) {
      pwmGiroAct=min(pwmObj, pwmGiroAct+paso);
    } else {
      int pasoBajada = (errorAbs < TURN_BRAKING_ZONE_DEG) ? PWM_TURN_RAMP_DOWN_STEP : PWM_TURN_SLEW_STEP;
      pwmGiroAct=max(pwmObj, pwmGiroAct-pasoBajada);
    }
  }
  if (signoGiroApl != 0 && pwmGiroAct > 0) {
    int cand = signoGiroApl > 0 ? candidatoGiroPos : candidatoGiroNeg;
    if (cand == 0) {
      if (candidatoGiroPos != 0) cand = (signoGiroApl > 0) ? candidatoGiroPos : -candidatoGiroPos;
      else if (candidatoGiroNeg != 0) cand = (signoGiroApl < 0) ? candidatoGiroNeg : -candidatoGiroNeg;
      else cand = (signoGiroApl > 0) ? 1 : -1;
    }
    const float ticksIzq = ControlSeguridad::promedioConfiableLado(d, encoderConfiableGlobal, true);
    const float ticksDer = ControlSeguridad::promedioConfiableLado(d, encoderConfiableGlobal, false);
    // Solo aplicar compensación traslacional si ambos lados cuentan con encoders confiables y movimiento medido,
    // evitando sesgos unilaterales hacia adelante ante fallos o retrasos de lectura de un lado
    const bool encodersBilateralesListos = (encoderConfiableGlobal[0] || encoderConfiableGlobal[2]) &&
                                          (encoderConfiableGlobal[1] || encoderConfiableGlobal[3]) &&
                                          (ticksIzq > 0.0f && ticksDer > 0.0f);
    const float tIzq = encodersBilateralesListos ? ticksIzq : 0.0f;
    const float tDer = encodersBilateralesListos ? ticksDer : 0.0f;
    const auto salidaGiro = ControlRuta::balancearGiroDiferencial(
        pwmGiroAct, cand, tIzq, tDer,
        ControlRuta::distanciaPorTick(WHEEL_DIAMETER_ODOMETRY_CM, ENCODER_PPR),
        KP_GIRO_BALANCE_PWM_POR_CM, PWM_GIRO_BALANCE_MAX, PWM_TURN_MAX_LIMIT);
    if (!aplicarVelocidades(salidaGiro.pwmL, salidaGiro.pwmR)) {
      fallo("motor_output_error");
      return;
    }
  } else frenarMotores();

  if (estadoActual == CALIBRANDO) {
    diagnosticoCal.pwmObjetivo = pwmGiroAct;
    diagnosticoCal.pasoRampa = ControlCalibracion::pasoRampaActual(
        pwmGiroAct, CALIBRATION_PWM_START, CALIBRATION_PWM_END, CALIBRATION_PWM_STEP);
  }

  const uint32_t limiteSinProgresoMs = 15000;
  if (ahora - inicioIntentoGiroMs > limiteSinProgresoMs && !movGiroConfirmado) {
    reintentarGiro("turn_no_progress");
  }
}

void completarGiro() {
  frenarMotores();
  Fase ret = faseRetornoGiro;
  faseRetornoGiro = Fase::NINGUNA;
  if (ret == Fase::GIRO_INICIAL) {
    const bool conservar = pausaPreAvanceConservar || (distAcumuladaCm > 0.0f) || (pasoDistanciaActualCm > 0.0f);
    iniciarPausaPreAvance(conservar);
  }
  else if (ret == Fase::GIRO_RECUPERACION) { iniciarPausaPreAvance(true); }
  else if (ret == Fase::GIRO_FINAL) {
    giroFinalRealizado = true;
    iniciarVerificacionFinal();
  }
  else if (ret == Fase::GIRO_SOLO) { fin(EVT_COMPLETED, "turn_ok"); }
  else if (ret == Fase::CAL_VALIDAR_25) {
    iniciarFaseCal(Fase::CAL_PAUSA);
    progresoComando=0.55f;
    strncpy(faseComando,"cal_pausa",sizeof(faseComando));
  }
  else if (ret == Fase::CAL_RETORNO) {
    frenarMotores();
    delay(50);
    solicitarGuardarTorque(
        pwmMinGiroPos / PWM_SCALE_8_TO_10,
        pwmMinGiroNeg / PWM_SCALE_8_TO_10,
        candidatoGiroPos,
        candidatoGiroNeg
    );
    robotCalibrado = true; PoseGlobal.reset(); resetOrientacionIMU();
    fin(EVT_COMPLETED, "cal_ok");
  }
}

// ============== AVANCE RECTO (fusion encoders + recuperacion rumbo + compensacion der) ==============
float  rumboObjetivoDeg = 0.0f;
float  distObjetivoCm = 0.0f;
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

void actualizarErroresTrayectoria() {
  if (!tieneTargetEspacial) return;
  // El rumbo de la recta es el del trayecto activo: en recuperacion se
  // recalcula hacia el waypoint, de modo que el desvio lateral siempre se mide
  // respecto a la direccion que el chasis realmente debe seguir.
  const ControlRuta::ErroresTrayectoria errores =
      ControlRuta::calcularErroresTrayectoriaAnclado(
          PoseGlobal.getX(), PoseGlobal.getY(), pasoOrigenTramoXCm,
          pasoOrigenTramoYCm, pasoTargetX, pasoTargetY, pasoRumboTrayectoDeg,
          distanciaPlanificadaCm);
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
  // Media aritmetica total de encoders saludables: sin distincion de lado y sin
  // la regla de minimos que sesgaba a la baja con un encoder sub-lector.
  return ControlSeguridad::mediaEncodersSaludables(v, encoderConfiableGlobal);
}
void resetConfEncoders() {
  for (int i = 0; i < 4; ++i) {
    saludEnc[i] = encoderConfiableGlobal[i] ? 0 : 2;
    inicioOutlierEncMs[i] = 0;
  }
  resetFiltrosEncoder();
}

void iniciarAvance(bool conservar) {
  // Avance recto sostenido estrictamente a 242/255; por encima solo rafagas.
  establecerLimiteContinuoPwm(PWM_SAFE_HARD_LIMIT);
  const SensorSnapshot s = sensar();
  if (!conservar) { distAcumuladaCm = 0.0f; intentosRecup = 0; }
  // Invarianza de marco local (ControlRuta.h:121): la directriz del tramo se
  // ancla siempre en la pose real del robot al iniciar el avance para que el
  // desvío lateral en t=0 sea exactamente cero y no se herede error residual de
  // pasos anteriores ("efecto cangrejo"). El endpoint se valida al final.
  pasoOrigenTramoXCm = PoseGlobal.getX();
  pasoOrigenTramoYCm = PoseGlobal.getY();
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
  }
  fase = Fase::AVANCE;
  pulsoAproximacionEncendido = false;
  inicioPulsoAproximacionMs = millis();
  strncpy(faseComando, "avance", sizeof(faseComando));
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
  modoDegradado = clasificacion.modoDegradado;
  for (int i=0;i<4;++i) {
    encoderConfiableGlobal[i]=clasificacion.confiable[i];
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
  // Autoridad central del MPU: si el chasis roto o se traslado, el movimiento
  // existe aunque un encoder degradado no lo reporte. Sin esta evidencia, la
  // recuperacion anti-friccion enclavaba FALLO con el robot girando.
  const bool mpuConfirma = fabsf(s.gyro_z_filtrado_rad_s) >= GYRO_MOVEMENT_RAD_S;
  if (ControlSeguridad::movimientoAntiFriccionConfirmado(
          deltaL, deltaR, ANTIFRICTION_SUCCESS_TICKS) || mpuConfirma) {
    antiFriccionActiva = false;
    antiFriccionPulsoEncendido = false;
    antiFriccionMovimientoConfirmado = true;
    antiFriccionPwmObjetivo = 0;
    ultimoPulsoLadoAvMs[0] = ultimoPulsoLadoAvMs[1] = millis();
    ticksLadoAvAnt[0] = ticksLadoAvAnt[1] = 0;
    strncpy(faseComando, "avance", sizeof(faseComando));
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
  if (fase != Fase::AVANCE && fase != Fase::PAUSA_REEVALUACION) return false;
  if (fase == Fase::PAUSA_REEVALUACION) {
    if (millis()-inicioPausaReevalMs >= PAUSA_REEVALUACION_MS) completarPausaReeval();
    return false;
  }
  const SensorSnapshot s = sensar();
  int64_t d[4]; deltas(ticksBaseAvance, s, d);
  float ticksEst = estimarTicksAvance(d);
  if (ticksEst<0) { fallo("enc_no_estimation"); return false; }
  // La misma escala efectiva debe gobernar el PID y la pose. Antes el PID
  // usaba el diámetro nominal y la odometría otro valor, por lo que el gráfico
  // podía indicar corrección sin que el robot frenara antes.
  const float cmPorTick = ControlRuta::distanciaPorTick(WHEEL_DIAMETER_ODOMETRY_CM, ENCODER_PPR);
  float distMedida = distAcumuladaCm + ticksEst * cmPorTick;
  pasoDistanciaActualCm = distMedida;
  float restante = distObjetivoCm - distMedida;
  pasoDistanciaRestanteCm = restante;
  const float distAproxCalculo = direccionTraslacion < 0 ? DISTANCIA_APROXIMACION_REVERSA_CM : DISTANCIA_APROXIMACION_CM;
  const float pwmReferenciaFreno = restante < distAproxCalculo
      ? static_cast<float>(VELOCIDAD_APROXIMACION)
      : static_cast<float>(VELOCIDAD_BASE_RECTO);
  pasoFrenoPrevistoCm = ControlRuta::distanciaFrenoPrevista(
      pwmReferenciaFreno, FRENO_RESIDUAL_BASE_CM, FRENO_RESIDUAL_POR_PWM_CM,
      FRENO_RESIDUAL_MAX_CM);
  actualizarErroresTrayectoria();

  uint32_t timeout = DRIVE_BASE_TIMEOUT_MS + (uint32_t)(distObjetivoCm * DRIVE_TIMEOUT_PER_CM_MS);
  if (millis()-inicioAvanceMs > timeout) { fallo("drive_timeout"); return false; }
  if (restante <= TOLERANCIA_DISTANCIA_CM) {
    iniciarAsentamientoFinal(distMedida);
    return false;
  }

  // El paso manual conserva su finalización relativa. Para un waypoint
  // absoluto sólo se acepta este atajo si ya está dentro de 5 cm; en otro
  // caso la verificación final activará la recuperación acotada.
  if (tieneTargetEspacial) {
    float distEspacialActual = PoseGlobal.distanciaAlObjetivo(pasoTargetX, pasoTargetY);
    if (pasoObjetivoAbsoluto && distEspacialActual <= TOLERANCIA_ENDPOINT_CM) {
      iniciarAsentamientoFinal(distMedida);
      return false;
    }
    if (!pasoObjetivoAbsoluto && distEspacialActual <= TOLERANCIA_DISTANCIA_CM) {
      iniciarAsentamientoFinal(distMedida);
      return false;
    }
    if (distEspacialActual < distTargetMinimaCm) {
      distTargetMinimaCm = distEspacialActual;
    } else if (distTargetMinimaCm <= 15.0f &&
               (distEspacialActual - distTargetMinimaCm) >= 2.0f) {
      // Sobrepaso espacial detectado (se alejó 2 cm tras estar a menos de 15 cm del destino)
      iniciarAsentamientoFinal(distMedida);
      return false;
    }

    // Guarda longitudinal estricta para objetivos absolutos:
    // Si pasoErrorLongitudinalCm <= 0.0f tras haber iniciado el avance (distMedida >= 5.0f o distObjetivoCm < 10.0f),
    // el robot ya alcanzó o superó la coordenada longitudinal meta; frenar de inmediato.
    if (pasoObjetivoAbsoluto && pasoErrorLongitudinalCm <= 0.0f && (distMedida >= 5.0f || distObjetivoCm < 10.0f)) {
      iniciarAsentamientoFinal(distMedida);
      return false;
    }
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
        distAcumuladaCm = distMedida;
        iniciarBaseGiro(normalizar360(rumboObjetivoDeg), Fase::GIRO_RECUPERACION);
        return false;
      }
      fallo(i==0?"drive_stall_left[Cinematica.cpp:503]":"drive_stall_right[Cinematica.cpp:503]"); return false;
    }
  }

  // Mientras se avanza hacia un waypoint absoluto, el rumbo converge de forma
  // gradual a la línea planificada. Un lateral positivo está a la derecha y
  // por eso ordena una corrección negativa (hacia la izquierda).
  if (recuperacionEndpointActiva) {
    const float rumboTrayecto = normalizar360(
        PoseGlobal.anguloAlObjetivoRad(pasoTargetX, pasoTargetY) * 180.0f / M_PI);
    pasoRumboTrayectoDeg = rumboTrayecto;
    pasoRumboCuerpoDeg = ControlRuta::rumboCuerpoParaTrayecto(
        rumboTrayecto, direccionTraslacion);
    rumboObjetivoDeg = pasoRumboCuerpoDeg;
    pasoControlLateralDeg = 0.0f;
  } else if (pasoObjetivoAbsoluto) {
    const float correccionTrayectoDeg = ControlRuta::correccionLateralRumboDeg(
        pasoErrorLateralCm, KP_LATERAL_RUMBO_DEG_POR_CM,
        CORRECCION_LATERAL_RUMBO_MAX_DEG);
    // La corrección lateral orienta el chasis para cerrar el desvío hacia la
    // línea planificada. Conserva el mismo sentido angular relativo a pasoRumboCuerpoDeg.
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
  // En los últimos 15 cm de aproximación final, se inhibe el pivote en el lugar para que
  // el lazo continuo PID guíe suavemente al robot hasta el punto meta sin interrupciones.
  // Lazo angular graduado (ControlAngular): A) diferencial suave sin detener;
  // B) diferencial fuerte con retencion; C) pausa + pivote + reanudar conteo;
  // D) cierre final sin pivote. El pivote solo se autoriza tras la histeresis.
  if (fabsf(err) > UMBRAL_ANGULAR_TRANSITORIO_DEG) {
    if (!inicioErrorRumboMs) inicioErrorRumboMs = millis();
  } else {
    inicioErrorRumboMs = 0;
  }
  const uint32_t msEnError = inicioErrorRumboMs ? (millis() - inicioErrorRumboMs) : 0;
  const ControlAngular::DecisionAngular angular = ControlAngular::evaluarLazoAngular(
      err, restante, msEnError, UMBRAL_ANGULAR_CONTINUO_DEG,
      UMBRAL_ANGULAR_TRANSITORIO_DEG, DISTANCIA_CIERRE_ANGULAR_CM,
      TIEMPO_SOSTENIDO_RECUPERACION_MS);
  pasoModoAngular = static_cast<uint8_t>(angular.modo);
  pasoModulacionIzq = angular.modLadoIzq;
  pasoModulacionDer = angular.modLadoDer;
  const float errRumboCuerpo = errorAng360(pasoRumboCuerpoDeg, heading360);
  const bool autorizarPivote = angular.solicitarPivote && (fabsf(errRumboCuerpo) > TOLERANCIA_CARDINAL_ESTRICTA_DEG);
  if (autorizarPivote) {
    frenarMotores();
    distAcumuladaCm = distMedida;
    if (intentosRecup >= INTENTOS_RECUPERACION_MAX) {
      // Sin fallo enclavado: parada suave con diagnostico explicito para el
      // operador en vez de heading_no_recovery.
      registrarMotivoFinalizacion("heading_recovery_exhausted");
      fin(EVT_COMPLETED, "heading_recovery_exhausted_soft");
      return false;
    }
    ++intentosRecup;
    iniciarBaseGiro(normalizar360(pasoRumboCuerpoDeg), Fase::GIRO_RECUPERACION);
    return false;
  }

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
  if (fabsf(err) <= ERROR_ENCODER_AUX_MAX_DEG && KP_ENCODER_PWM_POR_TICK > 0.0f) {
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

  // --- SELECCIÓN ADAPTATIVA DE VELOCIDAD DE CRUCERO (85% - 90% - 100%) ---
  // Se evalúa la velocidad filtrada de encoders en ticks/s para mantener un régimen de traslación estable
  static int nivelCruceroActual = VELOCIDAD_BASE_RECTO;
  static uint32_t ultimoCambioCruceroMs = 0;
  if (!conservarAcumulado && (millis() - inicioAvanceMs < 40)) {
    nivelCruceroActual = VELOCIDAD_BASE_RECTO;
    ultimoCambioCruceroMs = millis();
  }
  const float ticksSegL = fabsf(s.delta_pulsos_filtrado_FL + s.delta_pulsos_filtrado_BL) * 0.5f * (1000.0f / float(SENSOR_PERIOD_MS));
  const float ticksSegR = fabsf(s.delta_pulsos_filtrado_FR + s.delta_pulsos_filtrado_BR) * 0.5f * (1000.0f / float(SENSOR_PERIOD_MS));
  const float ticksSegPromedio = (ticksSegL + ticksSegR) * 0.5f;

  if (millis() - ultimoCambioCruceroMs >= 200) {
    if (ticksSegPromedio < 25.0f && millis() - inicioAvanceMs >= 300) {
      if (nivelCruceroActual == PWM_CRUCERO_85) nivelCruceroActual = PWM_CRUCERO_90;
      else if (nivelCruceroActual == PWM_CRUCERO_90) nivelCruceroActual = PWM_CRUCERO_100;
      ultimoCambioCruceroMs = millis();
    } else if (ticksSegPromedio > 75.0f) {
      if (nivelCruceroActual == PWM_CRUCERO_100) nivelCruceroActual = PWM_CRUCERO_90;
      else if (nivelCruceroActual == PWM_CRUCERO_90) nivelCruceroActual = PWM_CRUCERO_85;
      ultimoCambioCruceroMs = millis();
    }
  }

  const float distAprox = direccionTraslacion < 0 ? DISTANCIA_APROXIMACION_REVERSA_CM : DISTANCIA_APROXIMACION_CM;
  const float restantePerfil = max(0.0f, restante - pasoFrenoPrevistoCm);
  int base = restantePerfil < distAprox
      ? map(long(restantePerfil * 10), 0, long(distAprox * 10),
            VELOCIDAD_PRECISION_RECTO, VELOCIDAD_APROXIMACION)
      : nivelCruceroActual;
  base = constrain(base, VELOCIDAD_PRECISION_RECTO, nivelCruceroActual);
  if (direccionTraslacion < 0) {
    pasoRampaReversaMs = min<uint32_t>(millis() - inicioAvanceMs, RAMPA_REVERSA_MS);
    const int limiteRampa = map(long(pasoRampaReversaMs), 0, long(RAMPA_REVERSA_MS),
                                VELOCIDAD_PRECISION_RECTO, nivelCruceroActual);
    base = min(base, limiteRampa);
  } else {
    pasoRampaReversaMs = 0;
  }
  base = constrain(base, 0, PWM_MAX);

  // --- MODO DE MICRO-PULSOS DE APROXIMACIÓN FINAL (<= DISTANCIA_MICRO_PULSOS_CM) ---
  if (restante <= DISTANCIA_MICRO_PULSOS_CM) {
    strncpy(faseComando, "avance_pulso", sizeof(faseComando));
    const uint32_t deltaPulso = millis() - inicioPulsoAproximacionMs;
    if (pulsoAproximacionEncendido) {
      if (deltaPulso >= APPROACH_PULSE_ON_MS) {
        pulsoAproximacionEncendido = false;
        inicioPulsoAproximacionMs = millis();
        frenarMotoresActivo();
        return false;
      }
      base = APPROACH_PULSE_PWM;
    } else {
      frenarMotoresActivo();
      if (deltaPulso >= APPROACH_PULSE_OFF_MS) {
        pulsoAproximacionEncendido = true;
        inicioPulsoAproximacionMs = millis();
      }
      return false;
    }
  } else {
    pulsoAproximacionEncendido = false;
    inicioPulsoAproximacionMs = millis();
  }

  // Compensacion derecha + direccionamiento diferencial simetrico (preserva empuje neto hacia adelante)
  // Compensacion adaptativa por lado: corrige la asimetria de los reductores
  // TT. El HMI la calibra y persiste por tipo de piso.
  // La modulacion del lazo angular se aplica sobre el crucero antes del PID:
  // en modo B el lado interno retiene par y el externo empuja para reorientar
  // sin detener el avance.
  const int baseIzq = constrain(
      aproximar(base * perfilCompensacion.getLadoIzq() * angular.modLadoIzq),
      VELOCIDAD_MINIMA_DIFERENCIAL, PWM_MAX);
  int baseDer = constrain(
      aproximar(base * perfilCompensacion.getLadoDer() * angular.modLadoDer),
      VELOCIDAD_MINIMA_DIFERENCIAL, PWM_MAX);
  int redL = 0, redR = 0;
  int boostL = 0, boostR = 0;
  if (ctrlRumbo != 0.0f) {
    const int cand = ctrlRumbo > 0.0f ? candidatoGiroPos : candidatoGiroNeg;
    // La calibración decide el lado en avance; en reversa se intercambia para
    // conservar el mismo signo físico de corrección del yaw.
    const int diffPwm = aproximar(fabsf(ctrlRumbo));
    const int diffMitad = diffPwm / 2;
    if (ControlRuta::frenarLadoIzquierdoParaRumbo(cand, direccionTraslacion)) {
      redL += diffMitad;
      boostR += diffMitad; // Reparto simétrico: empuje longitudinal constante sin caídas bruscas de par
      strncpy(pasoLadoFrenoRumbo, "left", sizeof(pasoLadoFrenoRumbo));
    } else {
      redR += diffMitad;
      boostL += diffMitad;
      strncpy(pasoLadoFrenoRumbo, "right", sizeof(pasoLadoFrenoRumbo));
    }
  } else {
    strncpy(pasoLadoFrenoRumbo, "none", sizeof(pasoLadoFrenoRumbo));
  }
  // MPU tiene autoridad exclusiva: encoders no interfieren en corrección de rumbo
  if (ctrlEnc > 0.0f) redL += aproximar(ctrlEnc); else if (ctrlEnc < 0.0f) redR += aproximar(-ctrlEnc);

  int magL = constrain(baseIzq - redL + boostL, VELOCIDAD_MINIMA_DIFERENCIAL, PWM_SAFE_HARD_LIMIT);
  int magR = constrain(baseDer - redR + boostR, VELOCIDAD_MINIMA_DIFERENCIAL, PWM_SAFE_HARD_LIMIT);

  if (!aplicarVelocidades(direccionTraslacion * magL, direccionTraslacion * magR)) {
    fallo("motor_output_error");
    return false;
  }

  if (ticksEst >= 2.0f) { /* movimiento ok: se resetea watchdog externo via pulsos */ }
  return false;
}

// ============== PASO (orquestacion: giro_inicial -> avance -> giro_final -> completado) ==============
void iniciarAsentamientoFinal(float distanciaAntesDeFrenarCm) {
  establecerLimiteContinuoPwm(PWM_SAFE_HARD_LIMIT);
  frenarMotoresActivo();
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
    // Re-armar el freno activo mientras el chasis siga rodando por inercia:
    // el pulso inicial de 300 ms puede expirar antes de que el robot se
    // detenga en losa lisa.
    if (ahora - inicioAsentamientoMs > DURACION_FRENO_ACTIVO_MS) {
      frenarMotoresActivo();
    }
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

[[maybe_unused]] void iniciarRecuperacionEndpoint() {
  actualizarErroresTrayectoria();
  const float distancia = PoseGlobal.distanciaAlObjetivo(pasoTargetX, pasoTargetY);
  pasoDistanciaRecuperacionCm = distancia;
  if (!isfinite(distancia) || distancia < 0.5f || distancia > STEP_MAX_DISTANCE_CM) {
    strncpy(pasoDecisionRecuperacion, "invalid_distance", sizeof(pasoDecisionRecuperacion));
    fallo("endpoint_not_reached");
    return;
  }
  const ControlRuta::DecisionEndpoint decision = ControlRuta::decidirEndpointSeguro(
      true, false, intentosEndpoint, INTENTOS_RECUPERACION_ENDPOINT_MAX, distancia,
      DISTANCIA_MINIMA_RECUPERACION_ENDPOINT_CM);
  if (decision == ControlRuta::DecisionEndpoint::CALIBRAR) {
    // El residual corto no admite una corrección repetible por inercia. No se
    // bloquea la misión: el siguiente waypoint absoluto podrá absorberlo y la
    // telemetría deja marcada la calibración pendiente.
    strncpy(pasoDecisionRecuperacion, "soft_complete", sizeof(pasoDecisionRecuperacion));
    completarPasoConCorreccionPendiente();
    return;
  }
  if (decision == ControlRuta::DecisionEndpoint::FALLAR) {
    strncpy(pasoDecisionRecuperacion, "attempt_limit", sizeof(pasoDecisionRecuperacion));
    fallo("endpoint_not_reached");
    return;
  }
  ++intentosEndpoint;
  pasoIntentosEndpoint = intentosEndpoint;
  // Se conserva el rumbo final. Si el punto quedó detrás, se usa reversa y no
  // se ordena un pivote de 180° que pueda romper la siguiente secuencia.
  recuperacionEndpointActiva = true;
  const float rumboRecuperacion = normalizar360(
      PoseGlobal.anguloAlObjetivoRad(pasoTargetX, pasoTargetY) * 180.0f / M_PI);
  direccionTraslacion = ControlRuta::reversaAutomatica(
      rumboRecuperacion, heading360, UMBRAL_REVERSA_AUTOMATICA_DEG) ? -1 : 1;
  pasoRecuperacionUsaReversa = direccionTraslacion < 0;
  pasoEnReversa = pasoRecuperacionUsaReversa;
  strncpy(pasoDecisionRecuperacion, pasoRecuperacionUsaReversa
      ? "reverse_no_pivot" : "forward_recovery", sizeof(pasoDecisionRecuperacion));
  strncpy(pasoModoEfectivo, "recovery", sizeof(pasoModoEfectivo));
  pasoDistanciaCm = distancia;
  pasoDistanciaObjetivoCm = distancia;
  distAcumuladaCm = 0.0f;
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
  if (!giroFinalRealizado && fabsf(errorAng360(pasoRumboFinalDeg, heading360)) > TOLERANCIA_CARDINAL_ESTRICTA_DEG) {
    iniciarBaseGiro(pasoRumboFinalDeg, Fase::GIRO_FINAL);
    return;
  }
  const SensorSnapshot s = sensar();
  if (fabsf(s.gyro_z_filtrado_rad_s) > 0.02f) {
    inicioVerificacionFinalMs = millis();
    return;
  }
  if (millis() - inicioVerificacionFinalMs >= PAUSA_ESTABILIZACION_POST_PASO_MS) {
    completarPaso();
  }
}

void completarPaso() {
  recuperacionEndpointActiva = false;
  progresoComando = 1.0f;
  fin(EVT_COMPLETED, "step_ok");
}

[[maybe_unused]] void completarPasoConCorreccionPendiente() {
  recuperacionEndpointActiva = false;
  progresoComando = 1.0f;
  // `completed` permite al planificador enviar el siguiente waypoint. El
  // detalle y el objeto recovery conservan la desviación para calibrar después.
  fin(EVT_COMPLETED, "step_ok_endpoint_soft");
}

uint32_t inicioPausaPreGiroMs = 0;
uint32_t inicioPausaPreAvanceMs = 0;

void iniciarPausaPreGiro() {
  frenarMotores();
  inicioPausaPreGiroMs = millis();
  fase = Fase::PAUSA_PRE_GIRO;
  strncpy(faseComando, "pausa_pre_giro", sizeof(faseComando));
}

void controlarPausaPreGiro() {
  frenarMotores();
  const SensorSnapshot s = sensar();
  if (millis() - inicioPausaPreGiroMs < PAUSA_ESTABILIZACION_POST_PASO_MS ||
      fabsf(s.gyro_z_filtrado_rad_s) >= 0.02f) {
    return;
  }
  // Verificacion estricta: no se libera la recta con rumbo contaminado. Cualquier
  // desvio superior a 1.0 grado se corrige con un micro-giro antes de avanzar.
  float err = errorAng360(pasoRumboCuerpoDeg, heading360);
  if (fabsf(err) <= TOLERANCIA_CARDINAL_ESTRICTA_DEG) {
    iniciarAvance(pausaPreAvanceConservar);
  } else {
    iniciarBaseGiro(pasoRumboCuerpoDeg, Fase::GIRO_INICIAL);
  }
}

void iniciarPausaPreAvance(bool conservar) {
  frenarMotores();
  inicioPausaPreAvanceMs = millis();
  pausaPreAvanceConservar = conservar;
  fase = Fase::PAUSA_PRE_AVANCE;
  strncpy(faseComando, "pausa_pre_avance", sizeof(faseComando));
}

void controlarPausaPreAvance() {
  frenarMotores();
  const SensorSnapshot s = sensar();
  if (millis() - inicioPausaPreAvanceMs < PAUSA_ESTABILIZACION_POST_GIRO_MS ||
      fabsf(s.gyro_z_filtrado_rad_s) >= 0.02f) {
    return;
  }
  float err = errorAng360(pasoRumboCuerpoDeg, heading360);
  if (fabsf(err) <= TOLERANCIA_CARDINAL_ESTRICTA_DEG) {
    iniciarAvance(pausaPreAvanceConservar);
  } else if (fabsf(err) > UMBRAL_REACTIVACION_GIRO_PRE_AVANCE_DEG) {
    // Solo si excede la cota de histéresis (>2.5°), reorientar con micro-pulsos
    iniciarBaseGiro(pasoRumboCuerpoDeg, Fase::GIRO_INICIAL);
  } else {
    // Zona de histéresis (1.5° a 2.5°): el giróscopo ya está estable en reposo;
    // proceder al avance sin quedar atrapado en contragiros oscilatorios.
    iniciarAvance(pausaPreAvanceConservar);
  }
}

void iniciarPasoInterno() {
  giroFinalRealizado = false;
  float errInicial = errorAng360(pasoRumboCuerpoDeg, heading360);
  if (fabsf(errInicial) <= TOLERANCIA_CARDINAL_ESTRICTA_DEG) {
    iniciarPausaPreAvance(false);
  } else {
    iniciarPausaPreGiro();
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
bool enFaseAvance() { return fase == Fase::AVANCE; }
bool enFaseTraslacion() {
  return fase == Fase::AVANCE || fase == Fase::ASENTAMIENTO_FINAL;
}
bool enFaseGiro() {
  return fase == Fase::GIRO_INICIAL || fase == Fase::GIRO_RECUPERACION ||
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
  // La calibracion explora torque hasta 247/255 sostenido (rafagas a 255).
  establecerLimiteContinuoPwm(PWM_TURN_CONTINUO_LIMIT);
  const SensorSnapshot s = sensar();
  if (!s.mpu_present || !s.mpu_calibrated || s.mpu_stale) return false;
  seqActivo = seq; robotCalibrado = false;
  ++pasoEjecucionId;
  pasoObjetivoAbsoluto = false;
  tieneTargetEspacial = false;
  registrarMotivoFinalizacion("");
  reiniciarDiagnosticoCalibracion();
  fase = Fase::CAL_CUENTA; inicioFaseMs = millis();
  strncpy(faseComando, "cal", sizeof(faseComando));
  estadoActual = CALIBRANDO; progresoComando = 0.0f;
  encolarEvento(EVT_ACCEPTED, seq, "accepted");
  return true;
}

bool aplicarCalibracionInyectada(int pwmPos8, int pwmNeg8, int candPos, int candNeg, int seq) {
  if (estadoActual != DESARMADO && estadoActual != LISTO) return false;
  const SensorSnapshot s = sensar();
  if (!s.mpu_present || !s.mpu_calibrated || s.mpu_stale) return false;

  int finalPos = pwmPos8;
  int finalNeg = pwmNeg8;
  int finalCandPos = candPos;
  int finalCandNeg = candNeg;

  // Si no se especificaron valores o vienen en cero, intentar recuperar de MemoriaTorque (SPIFFS)
  if (finalPos == 0 || finalNeg == 0) {
    if (!obtenerCalibracionVigente(finalPos, finalNeg, finalCandPos, finalCandNeg)) {
      return false;
    }
  }

  // Validar límites de seguridad
  if (finalPos < ControlTorque::PWM_MIN_8BIT || finalPos > ControlTorque::PWM_MAX_8BIT ||
      finalNeg < ControlTorque::PWM_MIN_8BIT || finalNeg > ControlTorque::PWM_MAX_8BIT ||
      (finalCandPos != 1 && finalCandPos != -1) ||
      (finalCandNeg != 1 && finalCandNeg != -1) ||
      finalCandPos == finalCandNeg) {
    return false;
  }

  frenarMotores();
  pwmMinGiroPos = static_cast<int>(finalPos * PWM_SCALE_8_TO_10);
  pwmMinGiroNeg = static_cast<int>(finalNeg * PWM_SCALE_8_TO_10);
  candidatoGiroPos = finalCandPos;
  candidatoGiroNeg = finalCandNeg;

  solicitarGuardarTorque(finalPos, finalNeg, finalCandPos, finalCandNeg);

  PoseGlobal.reset();
  resetOrientacionIMU();
  robotCalibrado = true;
  estadoActual = LISTO;
  progresoComando = 1.0f;
  strncpy(faseComando, "injected_ok", sizeof(faseComando));
  if (seq > 0) {
    encolarEvento(EVT_COMPLETED, seq, "cal_ok");
  }
  return true;
}

bool intentarAutoRestaurarCalibracion() {
  if (robotCalibrado || estadoActual != DESARMADO) return false;
  const SensorSnapshot s = sensar();
  if (!s.mpu_present || !s.mpu_calibrated || s.mpu_stale) return false;
  int pos8 = 0, neg8 = 0, cpos = 0, cneg = 0;
  if (!obtenerCalibracionVigente(pos8, neg8, cpos, cneg)) return false;
  return aplicarCalibracionInyectada(pos8, neg8, cpos, cneg, 0);
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
  distAcumuladaCm = 0.0f;
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
  intentosEndpoint = 0;
  recuperacionEndpointActiva = false;
  giroFinalRealizado = false;
  distanciaPlanificadaCm = distanciaCm;

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
  // Semilla del anclaje de la recta: siempre anclado en pose real actual
  pasoOrigenTramoXCm = PoseGlobal.getX();
  pasoOrigenTramoYCm = PoseGlobal.getY();
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
  reiniciarControlRumbo();
  antiFriccionActiva = false;
  antiFriccionPulsoEncendido = false;
  antiFriccionPwmObjetivo = 0;
  giroFinalRealizado = false;
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

void controlarMovimiento() {
  if (estadoActual == CALIBRANDO) { controlarCalibracion(); return; }
  if (estadoActual != EJECUTANDO) return;

  switch (fase) {
    case Fase::PAUSA_PRE_GIRO:
      controlarPausaPreGiro();
      break;
    case Fase::GIRO_INICIAL:
    case Fase::GIRO_RECUPERACION:
    case Fase::GIRO_FINAL:
    case Fase::GIRO_SOLO:
    case Fase::CAL_RETORNO:
      controlarGiro();
      break;
    case Fase::PAUSA_PRE_AVANCE:
      controlarPausaPreAvance();
      break;
    case Fase::AVANCE:
    case Fase::PAUSA_REEVALUACION:
      if (controlarAvance()) {
        iniciarVerificacionFinal();
      }
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
    if (fase == Fase::PAUSA_PRE_GIRO) progresoComando = 0.05f;
    else if (fase == Fase::GIRO_INICIAL) progresoComando = min(0.30f, progresoComando);
    else if (fase == Fase::PAUSA_PRE_AVANCE) progresoComando = 0.30f;
    else if (fase == Fase::AVANCE || fase == Fase::ASENTAMIENTO_FINAL)
      progresoComando = 0.30f + 0.60f * fminf(1.0f, pasoDistanciaActualCm/max(pasoDistanciaObjetivoCm,0.1f));
    else if (fase == Fase::GIRO_FINAL) progresoComando = 0.90f + 0.10f * min(1.0f, (millis()-estableGiroDesdeMs)/float(TURN_SETTLE_MS));
  }
}

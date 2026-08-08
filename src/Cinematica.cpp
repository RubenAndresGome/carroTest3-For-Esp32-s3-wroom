#include "Cinematica.h"
#include "Comandos.h"
#include "Config.h"
#include "ControlRuta.h"
#include "ControlSeguridad.h"
#include "Debug.h"
#include "Eventos.h"
#include "Motores.h"
#include "PoseEstimator.h"
#include "Sensores.h"
#include <Arduino.h>
#include <cmath>

namespace {

enum class Fase : uint8_t {
  NINGUNA,
  CAL_CUENTA, CAL_A, CAL_VALIDAR_25, CAL_PAUSA, CAL_B, CAL_PAUSA_RETORNO, CAL_RETORNO,
  GIRO_INICIAL, AVANCE, GIRO_RECUPERACION, GIRO_FINAL, GIRO_SOLO,
  PAUSA_REEVALUACION, ASENTAMIENTO_FINAL, VERIFICAR_FINAL
};

Fase fase = Fase::NINGUNA;

// --- forward declarations (funciones internas que se llaman entre si) ---
void iniciarBaseGiro(float objetivoDeg, Fase retorno);
void controlarGiro();
void completarGiro();
void iniciarAvance(bool conservar);
bool controlarAvance();
void iniciarAsentamientoFinal(float distanciaAntesDeFrenarCm);
bool controlarAsentamientoFinal();
void completarPaso();
void iniciarVerificacionFinal();
void verificarObjetivoFinal();
void actualizarErroresTrayectoria();
void iniciarRecuperacionEndpoint();
void completarPasoConCorreccionPendiente();
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
uint8_t intentoCal = 1;
uint32_t ultimaAuditoriaMaxCalMs = 0;
uint32_t stallMaxCalAcumMs[2] = {};
uint32_t inicioFaseMs = 0;
int64_t ticksBaseCal[4] = {};
float yawInicioCalDeg = 0.0f;
int  candidatoGiroPos = -1, candidatoGiroNeg = 1;
int  pwmMinGiroPos = static_cast<int>(148 * PWM_SCALE_8_TO_10), pwmMinGiroNeg = static_cast<int>(148 * PWM_SCALE_8_TO_10);


void iniciarFaseCal(Fase f) { fase = f; inicioFaseMs = millis(); }

void calCuenta() {
  if (millis() - inicioFaseMs < CUENTA_CALIBRACION_MS) { progresoComando = min(0.10f, (millis()-inicioFaseMs)/float(CUENTA_CALIBRACION_MS)*0.10f); return; }
  const SensorSnapshot s = sensar();
  if (!s.mpu_present || s.mpu_stale || !s.mpu_calibrated) { fallo("mpu_unavailable_cal"); return; }
  yawInicioCalDeg = normalizar360(anguloZ);
  pwmMinGiroPos=0; pwmMinGiroNeg=0; candidatoGiroPos=0; candidatoGiroNeg=0;
  intentoCal=1; candidatoCal=1; pwmCal=CALIBRATION_PWM_START; ultimoRampaCalMs=millis(); inicioMovCalMs=0; inicioPausaReintentoCalMs=0;
  ultimaAuditoriaMaxCalMs=0; stallMaxCalAcumMs[0]=stallMaxCalAcumMs[1]=0;
  copiarBase(ticksBaseCal, s);
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
    pwmCal = CALIBRATION_PWM_START;
    ultimoRampaCalMs = ahora;
    inicioMovCalMs = 0;
    ultimaAuditoriaMaxCalMs = 0;
    stallMaxCalAcumMs[0]=stallMaxCalAcumMs[1]=0;
  }
  const SensorSnapshot s = sensar();
  int64_t d[4]; deltas(ticksBaseCal, s, d);
  const bool ladoIzqOk = (d[0]+d[2])/2 >= CAL_TICKS_MOVIMIENTO;
  const bool ladoDerOk = (d[1]+d[3])/2 >= CAL_TICKS_MOVIMIENTO;
  bool ticksOk = ladoIzqOk && ladoDerOk;
  bool gyroOk = fabsf(s.gyro_z_filtrado_rad_s) >= GYRO_MOVEMENT_RAD_S;
  aplicarVelocidades(-candidatoCal * pwmCal, candidatoCal * pwmCal);

  if (ticksOk && gyroOk) {
    if (!inicioMovCalMs) inicioMovCalMs = ahora;
    if (ahora - inicioMovCalMs >= CAL_MOVE_SUSTAINED_MS) {
      int guardado = min(PWM_TURN_MAX_LIMIT, pwmCal + PWM_CALIBRATION_MARGIN);
      if (s.gyro_z_filtrado_rad_s > 0) { candidatoGiroPos=candidatoCal; pwmMinGiroPos=guardado; }
      else { candidatoGiroNeg=candidatoCal; pwmMinGiroNeg=guardado; }
      frenarMotores();
      if (primera) {
        if (s.gyro_z_filtrado_rad_s < 0) {
          candidatoCal = -candidatoCal;
          inicioPausaReintentoCalMs = ahora;
          inicioMovCalMs = 0;
          return;
        }
        iniciarBaseGiro(normalizar360(yawInicioCalDeg + 25.0f), Fase::CAL_VALIDAR_25);
        progresoComando=0.35f;
        strncpy(faseComando,"cal_mas_25",sizeof(faseComando));
      }
      else {
        if (candidatoGiroPos==candidatoGiroNeg) { fallo("cal_dir_failed"); return; }
        iniciarFaseCal(Fase::CAL_PAUSA_RETORNO); progresoComando=0.85f; strncpy(faseComando,"cal_retorno",sizeof(faseComando));
      }
    }
  } else { inicioMovCalMs = 0; }

  if (ahora - ultimoRampaCalMs >= CAL_RAMP_INTERVAL_MS) {
    ultimoRampaCalMs = ahora;
    pwmCal = min(CALIBRATION_PWM_END, pwmCal + CALIBRATION_PWM_STEP);
  }

  if (pwmCal >= CALIBRATION_PWM_END) {
    if (!ultimaAuditoriaMaxCalMs) ultimaAuditoriaMaxCalMs = ahora;
    const uint32_t lapso = ahora - ultimaAuditoriaMaxCalMs;
    ultimaAuditoriaMaxCalMs = ahora;
    if (!ladoIzqOk) stallMaxCalAcumMs[0] += lapso;
    if (!ladoDerOk) stallMaxCalAcumMs[1] += lapso;
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

  switch (fase) {
    case Fase::CAL_CUENTA: calCuenta(); break;
    case Fase::CAL_A: calTorque(true); break;
    case Fase::CAL_VALIDAR_25: controlarGiro(); break;
    case Fase::CAL_PAUSA:
      frenarMotores();
      if (millis()-inicioFaseMs >= PAUSA_RETORNO_CAL_MS) { candidatoCal=-candidatoGiroPos; pwmCal=CALIBRATION_PWM_START; intentoCal=1; inicioPausaReintentoCalMs=0; inicioMovCalMs=0; ultimaAuditoriaMaxCalMs=0; stallMaxCalAcumMs[0]=stallMaxCalAcumMs[1]=0; copiarBase(ticksBaseCal,s); ultimoRampaCalMs=millis(); iniciarFaseCal(Fase::CAL_B); strncpy(faseComando,"cal_b",sizeof(faseComando)); progresoComando=0.60f; }
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

void iniciarBaseGiro(float objetivoDeg, Fase retorno) {
  reiniciarControlRumbo();
  const SensorSnapshot s = sensar();
  copiarBase(ticksBaseGiroLocal, s);
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

  if (fabsf(s.gyro_z_filtrado_rad_s) >= GYRO_MOVEMENT_RAD_S || (llabs(s.pulsosFL-ticksBaseGiroLocal[0])>=4 && llabs(s.pulsosFR-ticksBaseGiroLocal[1])>=4))
    movGiroConfirmado = true;

  // Mientras el torque siga incrementándose en rampa para vencer fricción y no haya movimiento confirmado,
  // mantener fresco el temporizador de pulso para evitar falsos stalls durante la búsqueda de torque.
  if (!movGiroConfirmado && pwmBusquedaGiro < PWM_TURN_MAX_LIMIT) {
    ultimoPulsoLadoGiroMs[0] = ultimoPulsoLadoGiroMs[1] = ahora;
  }

  int64_t d[4]; deltas(ticksBaseGiroLocal, s, d);
  int64_t ladoTicks[2] = {d[0]+d[2], d[1]+d[3]};
  for (int i=0; i<2; ++i) {
    if (ladoTicks[i] != ticksLadoGiroAnt[i]) { ticksLadoGiroAnt[i]=ladoTicks[i]; ultimoPulsoLadoGiroMs[i]=ahora; }
    else if (watchdogGiroArmado && ahora-ultimoPulsoLadoGiroMs[i] > TURN_STALL_MS) { reintentarGiro(i==0?"turn_stall_left":"turn_stall_right"); return; }
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
      completarGiro();
    }
    return;
  }
  if (giroEnTol) {
    frenarMotores();
    if (errorAbs > TURN_REACTIVATION_DEG) reintentarGiro("turn_drifted");
    return;
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
  if (signoGiroApl != 0 && pwmGiroAct > 0) {
    int cand = signoGiroApl > 0 ? candidatoGiroPos : candidatoGiroNeg;
    if (cand == 0) {
      if (candidatoGiroPos != 0) cand = (signoGiroApl > 0) ? candidatoGiroPos : -candidatoGiroPos;
      else if (candidatoGiroNeg != 0) cand = (signoGiroApl < 0) ? candidatoGiroNeg : -candidatoGiroNeg;
      else cand = (signoGiroApl > 0) ? -1 : 1;
    }
    aplicarVelocidades(-cand * pwmGiroAct, cand * pwmGiroAct);
  } else frenarMotores();

  if (fabsf(s.gyro_z_filtrado_rad_s)>=GYRO_MOVEMENT_RAD_S || (ladoTicks[0]>=4&&ladoTicks[1]>=4)) {
    movGiroConfirmado = true;
  } else if (ahora - inicioIntentoGiroMs > 5000) { reintentarGiro("turn_no_progress"); }
}

void completarGiro() {
  frenarMotores();
  Fase ret = faseRetornoGiro;
  faseRetornoGiro = Fase::NINGUNA;
  if (ret == Fase::GIRO_INICIAL) { iniciarAvance(false); }
  else if (ret == Fase::GIRO_RECUPERACION) { iniciarAvance(true); }
  else if (ret == Fase::GIRO_FINAL) { iniciarVerificacionFinal(); }
  else if (ret == Fase::GIRO_SOLO) { fin(EVT_COMPLETED, "turn_ok"); }
  else if (ret == Fase::CAL_VALIDAR_25) {
    iniciarFaseCal(Fase::CAL_PAUSA);
    progresoComando=0.55f;
    strncpy(faseComando,"cal_pausa",sizeof(faseComando));
  }
  else if (ret == Fase::CAL_RETORNO) {
    frenarMotores();
    delay(50);
    robotCalibrado = true; PoseGlobal.reset(); resetOrientacionIMU();
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
bool encoderConfiable[4] = {true,true,true,true};
uint8_t saludEnc[4] = {};
uint32_t inicioOutlierEncMs[4] = {};
int64_t ticksPausaClasif[4] = {};
uint32_t inicioPausaReevalMs = 0;
float errorRumboMaxTramo = 0.0f;

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
bool hayPorLado() { return (encoderConfiable[0]||encoderConfiable[2]) && (encoderConfiable[1]||encoderConfiable[3]); }
float promedioLado(const int64_t v[4], bool izq) {
  return ControlSeguridad::promedioConfiableLado(v, encoderConfiable, izq);
}
float estimarTicksAvance(const int64_t v[4]) {
  float med = mediana4(v);
  if (!modoDegradado) return med;
  if (!hayPorLado()) return -1.0f;
  return 0.5f*(promedioLado(v,true)+promedioLado(v,false));
}
void resetConfEncoders() {
  modoDegradado = false;
  for (int i = 0; i < 4; ++i) {
    encoderConfiable[i] = true;
    encoderConfiableGlobal[i] = true;
    saludEnc[i] = 0;
    inicioOutlierEncMs[i] = 0;
  }
  resetFiltrosEncoder();
}

void iniciarAvance(bool conservar) {
  const SensorSnapshot s = sensar();
  if (!conservar) { distAcumuladaCm = 0.0f; intentosRecup = 0; }
  reiniciarControlRumbo();
  pasoEnReversa = direccionTraslacion < 0;
  // Una recuperación o reevaluación debe preservar los canales que ya fueron
  // descartados. Sólo un paso nuevo vuelve a dar oportunidad a los cuatro.
  if (!conservar) resetConfEncoders();
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
    encoderConfiable[i]=clasificacion.confiable[i];
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
  const float pwmReferenciaFreno = restante < DISTANCIA_APROXIMACION_CM
      ? static_cast<float>(VELOCIDAD_APROXIMACION)
      : static_cast<float>(VELOCIDAD_BASE_RECTO);
  pasoFrenoPrevistoCm = ControlRuta::distanciaFrenoPrevista(
      pwmReferenciaFreno, FRENO_RESIDUAL_BASE_CM, FRENO_RESIDUAL_POR_PWM_CM,
      FRENO_RESIDUAL_MAX_CM);
  actualizarErroresTrayectoria();

  uint32_t timeout = DRIVE_BASE_TIMEOUT_MS + (uint32_t)(distObjetivoCm * DRIVE_TIMEOUT_PER_CM_MS);
  if (millis()-inicioAvanceMs > timeout) { fallo("drive_timeout"); return false; }
  if (restante <= TOLERANCIA_DISTANCIA_CM + pasoFrenoPrevistoCm) {
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
    if (!pasoObjetivoAbsoluto && distEspacialActual < distTargetMinimaCm) {
      distTargetMinimaCm = distEspacialActual;
    } else if (!pasoObjetivoAbsoluto && distTargetMinimaCm <= 15.0f &&
               (distEspacialActual - distTargetMinimaCm) >= 2.0f) {
      // Sobrepaso espacial detectado (se alejo 2 cm tras estar a menos de 15 cm del destino)
      iniciarAsentamientoFinal(distMedida);
      return false;
    }
  }


  // outlier persistence
  if (detectarOutliers(d)) { iniciarPausaReeval(d); return false; }

  // stall per side
  int64_t ladoTicks[2]={(encoderConfiable[0]?d[0]:0)+(encoderConfiable[2]?d[2]:0),
                         (encoderConfiable[1]?d[1]:0)+(encoderConfiable[3]?d[3]:0)};
  for (int i=0; i<2; ++i) {
    if (ladoTicks[i]!=ticksLadoAvAnt[i]) { ticksLadoAvAnt[i]=ladoTicks[i]; ultimoPulsoLadoAvMs[i]=millis(); }
    else if (millis()-ultimoPulsoLadoAvMs[i] > DRIVE_STALL_MS) {
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
      iniciarBaseGiro(normalizar360(rumboObjetivoDeg), Fase::GIRO_RECUPERACION);
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
    const int cand = ctrlRumbo > 0.0f ? candidatoGiroPos : candidatoGiroNeg;
    // La calibración decide el lado en avance; en reversa se intercambia para
    // conservar el mismo signo físico de corrección del yaw.
    if (ControlRuta::frenarLadoIzquierdoParaRumbo(cand, direccionTraslacion)) {
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

  aplicarVelocidades(direccionTraslacion * magL, direccionTraslacion * magR);

  if (ticksEst >= 2.0f) { /* movimiento ok: se resetea watchdog externo via pulsos */ }
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
      pasoObjetivoAbsoluto, objetivoAbsolutoAlcanzado(), intentosEndpoint,
      INTENTOS_RECUPERACION_ENDPOINT_MAX, distancia,
      DISTANCIA_MINIMA_RECUPERACION_ENDPOINT_CM);
  if (decision == ControlRuta::DecisionEndpoint::CALIBRAR) {
    pasoDistanciaRecuperacionCm = distancia;
    strncpy(pasoDecisionRecuperacion, "soft_complete", sizeof(pasoDecisionRecuperacion));
    completarPasoConCorreccionPendiente();
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

void completarPasoConCorreccionPendiente() {
  recuperacionEndpointActiva = false;
  progresoComando = 1.0f;
  // `completed` permite al planificador enviar el siguiente waypoint. El
  // detalle y el objeto recovery conservan la desviación para calibrar después.
  fin(EVT_COMPLETED, "step_ok_endpoint_soft");
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

bool iniciarCalibracion(int seq) {
  if (estadoActual != DESARMADO && estadoActual != LISTO) return false;
  const SensorSnapshot s = sensar();
  if (!s.mpu_present || !s.mpu_calibrated || s.mpu_stale) return false;
  seqActivo = seq; robotCalibrado = false;
  ++pasoEjecucionId;
  pasoObjetivoAbsoluto = false;
  tieneTargetEspacial = false;
  registrarMotivoFinalizacion("");
  fase = Fase::CAL_CUENTA; inicioFaseMs = millis();
  strncpy(faseComando, "cal", sizeof(faseComando));
  estadoActual = CALIBRANDO; progresoComando = 0.0f;
  encolarEvento(EVT_ACCEPTED, seq, "accepted");
  return true;
}

bool iniciarPaso(float heading, float distanciaCm, int seq, float targetX, float targetY,
                 bool objetivoAbsoluto, ModoPaso modoPaso) {
  if (estadoActual != LISTO) return false;
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
  intentosEndpoint = 0;
  recuperacionEndpointActiva = false;
  distanciaPlanificadaCm = distanciaCm;

  if (std::isfinite(targetX) && std::isfinite(targetY)) {
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
  frenarMotores(); fase = Fase::NINGUNA;
  reiniciarControlRumbo();
  registrarMotivoFinalizacion(detalle ? detalle : "stopped");
  estadoActual = robotCalibrado ? LISTO : DESARMADO;
  progresoComando = 0.0f;
  if (cancelado != 0) encolarEvento(EVT_REJECTED, cancelado, detalle ? detalle : "stopped");
}

void controlarMovimiento() {
  if (estadoActual == CALIBRANDO) { controlarCalibracion(); return; }
  if (estadoActual != EJECUTANDO) return;

  switch (fase) {
    case Fase::GIRO_INICIAL:
    case Fase::GIRO_RECUPERACION:
    case Fase::GIRO_FINAL:
    case Fase::GIRO_SOLO:
    case Fase::CAL_RETORNO:
      controlarGiro();
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
    if (fase == Fase::GIRO_INICIAL) progresoComando = min(0.30f, progresoComando);
    else if (fase == Fase::AVANCE || fase == Fase::ASENTAMIENTO_FINAL)
      progresoComando = 0.30f + 0.60f * fminf(1.0f, pasoDistanciaActualCm/max(pasoDistanciaObjetivoCm,0.1f));
    else if (fase == Fase::GIRO_FINAL) progresoComando = 0.90f + 0.10f * min(1.0f, (millis()-estableGiroDesdeMs)/float(TURN_SETTLE_MS));
  }
}

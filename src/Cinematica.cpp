#include "Cinematica.h"
#include "Comandos.h"
#include "Config.h"
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
  GIRO_INICIAL, AVANCE, GIRO_RECUPERACION, GIRO_FINAL,
  PAUSA_REEVALUACION
};

Fase fase = Fase::NINGUNA;

// --- forward declarations (funciones internas que se llaman entre si) ---
void iniciarBaseGiro(float objetivoDeg, Fase retorno);
void controlarGiro();
void completarGiro();
void iniciarAvance(bool conservar);
bool controlarAvance();
void completarPaso();
void fallo(const char* d);

// --- variables del paso (usadas por iniciarAvance, iniciarPaso) ---
float pasoHeading = 0.0f;
float pasoDistanciaCm = 0.0f;
float pasoTargetX = NAN;
float pasoTargetY = NAN;
bool tieneTargetEspacial = false;
float distTargetMinimaCm = 1e9f;


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
uint32_t ultimoMicroPulsoMs = 0;
bool microPulsoEncendido = false;

void iniciarBaseGiro(float objetivoDeg, Fase retorno) {
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
  pwmBoostFrenado=0; ultimoMicroPulsoMs=0; microPulsoEncendido=false;
  faseRetornoGiro = retorno;
  fase = retorno;  // fase global sigue al giro
  if (retorno == Fase::GIRO_INICIAL) strncpy(faseComando,"giro_ini",sizeof(faseComando));
  else if (retorno == Fase::GIRO_FINAL) strncpy(faseComando,"giro_fin",sizeof(faseComando));
  else if (retorno == Fase::GIRO_RECUPERACION) strncpy(faseComando,"recup",sizeof(faseComando));
  else if (retorno == Fase::CAL_RETORNO) strncpy(faseComando,"cal_ret",sizeof(faseComando));
}

void reintentarGiro(const char* motivo) {
  frenarMotores(); pwmGiroAct=0; signoGiroApl=0; movGiroConfirmado=false; watchdogGiroArmado=false; giroEnTol=false;
  pwmBoostFrenado=0; ultimoMicroPulsoMs=0; microPulsoEncendido=false;
  if (intentoGiro >= TURN_MAX_ATTEMPTS) { fallo(motivo); return; }
  ++intentoGiro;
  pausaReintentoGiroCal = true;
  inicioPausaReintGiroMs = millis();
}

void controlarGiro() {
  if (fase != Fase::GIRO_INICIAL && fase != Fase::GIRO_FINAL && fase != Fase::GIRO_RECUPERACION && fase != Fase::CAL_VALIDAR_25 && fase != Fase::CAL_RETORNO) return;
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
    pwmBoostFrenado=0; ultimoMicroPulsoMs=0; microPulsoEncendido=false;
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

  if (abs(s.gyro_z_filtrado_rad_s) >= GYRO_MOVEMENT_RAD_S || (llabs(s.pulsosFL-ticksBaseGiroLocal[0])>=4 && llabs(s.pulsosFR-ticksBaseGiroLocal[1])>=4))
    movGiroConfirmado = true;

  // Mientras el torque siga incrementándose en rampa para vencer fricción y no haya movimiento confirmado,
  // mantener fresco el temporizador de pulso para evitar falsos stalls durante la búsqueda de torque.
  if (!movGiroConfirmado && pwmBusquedaGiro < PWM_SAFE_HARD_LIMIT) {
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
      // --- MODO 2: Micro-Pulsos de Exactitud (0.0° a 5.0°) ---
      pwmObj = max(minimo + static_cast<int>(6 * PWM_SCALE_8_TO_10), PWM_TURN_START);
      if (ahora - ultimoMicroPulsoMs >= (microPulsoEncendido ? TURN_PULSE_ON_MS : TURN_PULSE_OFF_MS)) {
        ultimoMicroPulsoMs = ahora;
        microPulsoEncendido = !microPulsoEncendido;
      }
      if (!microPulsoEncendido) {
        pwmObj = 0; // Pausa breve para evaluacion IMU entre pulsos
      }
    }
  } else {
    pwmBoostFrenado = 0;
    microPulsoEncendido = false;
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
    /* movimiento detectado: ok */
  } else if (ahora - inicioIntentoGiroMs > 5000) { reintentarGiro("turn_no_progress"); }
}

void completarGiro() {
  frenarMotores();
  Fase ret = faseRetornoGiro;
  faseRetornoGiro = Fase::NINGUNA;
  if (ret == Fase::GIRO_INICIAL) { iniciarAvance(false); }
  else if (ret == Fase::GIRO_RECUPERACION) { iniciarAvance(true); }
  else if (ret == Fase::GIRO_FINAL) { completarPaso(); }
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

float mediana4(const int64_t v[4]) {
  int64_t o[4]={v[0],v[1],v[2],v[3]};
  for (int i=1;i<4;++i) { int64_t x=o[i]; int j=i-1; while(j>=0&&o[j]>x){o[j+1]=o[j];--j;} o[j+1]=x; }
  return (o[1]+o[2])*0.5f;
}
bool hayPorLado() { return (encoderConfiable[0]||encoderConfiable[2]) && (encoderConfiable[1]||encoderConfiable[3]); }
float promedioLado(const int64_t v[4], bool izq) {
  int a=izq?0:1, b=izq?2:3; float s=0; int n=0;
  if(encoderConfiable[a]){s+=v[a];++n;} if(encoderConfiable[b]){s+=v[b];++n;}
  return n?s/n:0.0f;
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
  resetConfEncoders();
  conservarAcumulado = conservar;
  distObjetivoCm = pasoDistanciaCm;  // global del paso
  pasoDistanciaObjetivoCm = distObjetivoCm;
  rumboObjetivoDeg = pasoHeading;   // global del paso (heading del destino)
  copiarBase(ticksBaseAvance, s);
  inicioAvanceMs = millis();
  ticksLadoAvAnt[0] = ticksLadoAvAnt[1] = 0;
  ultimoPulsoLadoAvMs[0] = ultimoPulsoLadoAvMs[1] = millis();
  inicioErrorRumboMs = 0; errorRumboMaxTramo = 0.0f;
  if (tieneTargetEspacial) {
    distTargetMinimaCm = PoseGlobal.distanciaAlObjetivo(pasoTargetX, pasoTargetY);
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
    if (fabsf(v[i]-med)/max(1.0f,fabsf(med)) > DESACUERDO_MAXIMO_PAR) {
      if(saludEnc[i]==0)saludEnc[i]=1;
      if(!inicioOutlierEncMs[i])inicioOutlierEncMs[i]=millis();
      persistente |= (millis()-inicioOutlierEncMs[i] >= DESACUERDO_ENCODER_PERSISTENTE_MS);
    } else { inicioOutlierEncMs[i]=0; if(saludEnc[i]!=2)saludEnc[i]=0; }
  }
  return persistente;
}

void iniciarPausaReeval(const int64_t v[4]) {
  frenarMotores();
  for (int i=0;i<4;++i) ticksPausaClasif[i]=v[i];
  inicioPausaReevalMs = millis();
  fase = Fase::PAUSA_REEVALUACION;
  strncpy(faseComando, "reeval", sizeof(faseComando));
}

void completarPausaReeval() {
  bool nc[4];
  float med = mediana4(ticksPausaClasif);
  float den = max(1.0f, fabsf(med));
  modoDegradado = false;
  for (int i=0;i<4;++i) {
    nc[i] = fabsf(ticksPausaClasif[i]-med)/den <= 0.40f;
    encoderConfiable[i]=nc[i]; encoderConfiableGlobal[i]=nc[i];
    saludEnc[i]=nc[i]?0:2; inicioOutlierEncMs[i]=0;
    modoDegradado |= !nc[i];
  }
  if (!hayPorLado()) { fallo("enc_no_side"); return; }
  const SensorSnapshot s = sensar();
  copiarBase(ticksBaseAvance, s);
  ticksLadoAvAnt[0]=ticksLadoAvAnt[1]=0;
  ultimoPulsoLadoAvMs[0]=ultimoPulsoLadoAvMs[1]=millis();
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
  float cmPorTick = (PI*WHEEL_DIAMETER_CM)/ENCODER_PPR;
  float distMedida = distAcumuladaCm + ticksEst * cmPorTick;
  pasoDistanciaActualCm = distMedida;
  float restante = distObjetivoCm - distMedida;

  uint32_t timeout = DRIVE_BASE_TIMEOUT_MS + (uint32_t)(distObjetivoCm * DRIVE_TIMEOUT_PER_CM_MS);
  if (millis()-inicioAvanceMs > timeout) { fallo("drive_timeout"); return false; }
  if (restante <= TOLERANCIA_DISTANCIA_CM) { return true; }

  // Guardia Doble Espacial (PoseGlobal): comprobacion en tiempo real del objetivo espacial
  if (tieneTargetEspacial) {
    float distEspacialActual = PoseGlobal.distanciaAlObjetivo(pasoTargetX, pasoTargetY);
    if (distEspacialActual <= TOLERANCIA_DISTANCIA_CM) {
      return true;
    }
    if (distEspacialActual < distTargetMinimaCm) {
      distTargetMinimaCm = distEspacialActual;
    } else if (distTargetMinimaCm <= 15.0f && (distEspacialActual - distTargetMinimaCm) >= 2.0f) {
      // Sobrepaso espacial detectado (se alejo 2 cm tras estar a menos de 15 cm del destino)
      return true;
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

  float err = errorAng360(rumboObjetivoDeg, heading360);
  errorRumboMaxTramo = max(errorRumboMaxTramo, fabsf(err));

  // Si la desviacion de rumbo supera el 20% (20.0°), pausar en marcha y ejecutar reorientacion de pivote
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

  // --- calcular PWM y reduccion dinamica en marcha ---
  float ctrlRumbo = constrain(err * KP_RUMBO_PWM_POR_GRADO - s.gyro_z_filtrado_rad_s * KD_RUMBO_PWM_POR_RAD_S,
                              -float(PWM_CORRECCION_RUMBO_MAX), float(PWM_CORRECCION_RUMBO_MAX));
  float ctrlEnc = 0.0f;
  if (fabsf(err) <= 2.0f) {
    const float deltaIzq = 0.5f * (s.delta_pulsos_filtrado_FL + s.delta_pulsos_filtrado_BL);
    const float deltaDer = 0.5f * (s.delta_pulsos_filtrado_FR + s.delta_pulsos_filtrado_BR);
    float diff = deltaIzq - deltaDer;
    ctrlEnc = constrain(diff * KP_ENCODER_PWM_POR_TICK, -float(PWM_CORRECCION_ENCODER_MAX), float(PWM_CORRECCION_ENCODER_MAX));
  }

  int base = restante < 15.0f ? map(long(restante * 10), 20, 150, VELOCIDAD_MINIMA_RECTO, VELOCIDAD_APROXIMACION) : VELOCIDAD_BASE_RECTO;
  base = constrain(base, VELOCIDAD_MINIMA_RECTO, VELOCIDAD_BASE_RECTO);
  base = constrain(base, 0, PWM_MAX);

  // Compensacion derecha + reduccion dinamica del lado contrario al angulo desviado (extraida del test aprobado)
  int baseDer = constrain(aproximar(base * factorCompensacionDer), VELOCIDAD_MINIMA_RECTO, PWM_MAX);
  int redL = 0, redR = 0;
  if (ctrlRumbo != 0.0f) {
    const int cand = ctrlRumbo > 0.0f ? candidatoGiroPos : candidatoGiroNeg;
    // La calibracion decide que lado frena para corregir yaw
    if (cand > 0) redL += aproximar(fabsf(ctrlRumbo));
    else redR += aproximar(fabsf(ctrlRumbo));
  }
  if (ctrlEnc > 0) redL += aproximar(ctrlEnc); else redR += aproximar(-ctrlEnc);

  int magL = constrain(base - redL, VELOCIDAD_MINIMA_RECTO, PWM_MAX);
  int magR = constrain(baseDer - redR, VELOCIDAD_MINIMA_RECTO, PWM_MAX);

  aplicarVelocidades(magL, magR);

  if (ticksEst >= 2.0f) { /* movimiento ok: se resetea watchdog externo via pulsos */ }
  return false;
}

// ============== PASO (orquestacion: giro_inicial -> avance -> giro_final -> completado) ==============
void completarPaso() {
  progresoComando = 1.0f;
  fin(EVT_COMPLETED, "step_ok");
}

void iniciarPasoInterno() {
  float errInicial = errorAng360(pasoHeading, heading360);
  if (fabsf(errInicial) <= TOLERANCIA_GIRO_DEG) {
    iniciarAvance(false);
  } else {
    iniciarBaseGiro(pasoHeading, Fase::GIRO_INICIAL);
  }
}

} // namespace

// ===== API publica =====
float normalizar360(float a) {
  a = fmodf(a, 360.0f);
  if (a < 0.0f) a += 360.0f;
  return a;
}
bool enFaseAvance() { return fase == Fase::AVANCE; }
bool enFaseGiro() {
  return fase == Fase::GIRO_INICIAL || fase == Fase::GIRO_RECUPERACION ||
         fase == Fase::GIRO_FINAL || fase == Fase::CAL_VALIDAR_25 || fase == Fase::CAL_RETORNO;
}
bool enFaseCalibracion() {
  return estadoActual == CALIBRANDO;
}

bool iniciarCalibracion(int seq) {
  if (estadoActual != DESARMADO && estadoActual != LISTO) return false;
  const SensorSnapshot s = sensar();
  if (!s.mpu_present || !s.mpu_calibrated || s.mpu_stale) return false;
  seqActivo = seq; robotCalibrado = false;
  fase = Fase::CAL_CUENTA; inicioFaseMs = millis();
  strncpy(faseComando, "cal", sizeof(faseComando));
  estadoActual = CALIBRANDO; progresoComando = 0.0f;
  encolarEvento(EVT_ACCEPTED, seq, "accepted");
  return true;
}

bool iniciarPaso(float heading, float distanciaCm, int seq, float targetX, float targetY) {
  if (estadoActual != LISTO) return false;
  if (distanciaCm < 0.5f || distanciaCm > STEP_MAX_DISTANCE_CM) return false;
  heading = normalizar360(heading);
  if (seq == 1) { ultimoSeqCompletado = 0; }
  if (seq <= ultimoSeqCompletado) {
    encolarEvento(EVT_COMPLETED, seq, "already_done");
    return true;
  }
  seqActivo = seq;
  pasoHeading = heading;
  pasoDistanciaCm = distanciaCm;
  pasoHeadingObjetivo = heading;
  pasoDistanciaObjetivoCm = distanciaCm;
  pasoDistanciaActualCm = 0.0f;

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

  distTargetMinimaCm = 1e9f;

  fase = Fase::GIRO_INICIAL;
  estadoActual = EJECUTANDO;
  progresoComando = 0.0f;
  encolarEvento(EVT_ACCEPTED, seq, "accepted");
  iniciarPasoInterno();
  return true;
}


void cancelarMovimiento(const char* detalle) {
  const int cancelado = seqActivo;
  frenarMotores(); fase = Fase::NINGUNA;
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
    case Fase::CAL_RETORNO:
      controlarGiro();
      break;
    case Fase::AVANCE:
    case Fase::PAUSA_REEVALUACION:
      if (controlarAvance()) {
        if (fabsf(errorAng360(rumboObjetivoDeg, heading360)) <= TOLERANCIA_GIRO_DEG) completarPaso();
        else iniciarBaseGiro(normalizar360(rumboObjetivoDeg), Fase::GIRO_FINAL);
      }
      break;
    default: break;
  }

  // progreso general
  if (estadoActual == EJECUTANDO || estadoActual == CALIBRANDO) {
    if (fase == Fase::GIRO_INICIAL) progresoComando = min(0.30f, progresoComando);
    else if (fase == Fase::AVANCE) progresoComando = 0.30f + 0.60f * fminf(1.0f, pasoDistanciaActualCm/max(pasoDistanciaObjetivoCm,0.1f));
    else if (fase == Fase::GIRO_FINAL) progresoComando = 0.90f + 0.10f * min(1.0f, (millis()-estableGiroDesdeMs)/float(TURN_SETTLE_MS));
  }
}

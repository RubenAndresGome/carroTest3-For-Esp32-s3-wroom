#include "Cinematica.h"
#include "Comandos.h"
#include "Config.h"
#include "Debug.h"
#include "Eventos.h"
#include "Motores.h"
#include "PoseEstimator.h"
#include "Seguridad.h"
#include "Sensores.h"
#include <Arduino.h>
#include <cmath>

float distanciaInicialManejo = 0;
float distanciaObjetivoCM = 0;
float anguloObjetivoGrados = 0;
unsigned long tiempoInicioGiro = 0;
unsigned long tiempoInicioAvance = 0;

namespace {
enum class FaseMovimiento : uint8_t {
  NINGUNA, MOVE_GIRO, MOVE_AVANCE,
  MOVE_ALINEACION_FINAL, DRIVE, TURN,
  RECOVERY_TURN
};
enum class EjeMovimiento : uint8_t { INVALIDO, X, Y };

FaseMovimiento fase = FaseMovimiento::NINGUNA;
float distanciaFirmadaCm = 0.0f;
float rumboTramoDeg = 0.0f;
float yawInicioGiroDeg = 0.0f;
float anguloSolicitadoGiroDeg = 0.0f;
float progresoGiroActualDeg = 0.0f;
float errorGiroActualDeg = 0.0f;
float errorPredichoGiroActualDeg = 0.0f;
int pwmObjetivoGiroActual = 0;
const char* faseFrenadoGiroActual = "OFF";
float rumboCardinalObjetivoDeg = NAN;
bool rutaAlineadaGruesa = false;
bool rutaAlineadaPrecisa = false;
char ultimoRechazoMovimiento[48] = {};
int64_t ticksBase[4] = {};
// La distancia se integra por incrementos. Recalcular todos los ticks desde
// ticksBase con una mascara de salud nueva reinterpretaba el pasado: al excluir
// FL, por ejemplo, (FL + BL) / 2 se convertia repentinamente en BL y adelantaba
// falsamente el final del tramo.
int64_t ticksUltimaIntegracion[4] = {};
float distanciaIntegradaCm = 0.0f;
bool encoderConfiable[4] = {true, true, true, true};
enum class SaludEncoder : uint8_t { OK, SOSPECHOSO, EXCLUIDO };
SaludEncoder saludEncoder[4] = {SaludEncoder::OK, SaludEncoder::OK,
                                SaludEncoder::OK, SaludEncoder::OK};
uint32_t inicioOutlierEncoderMs[4] = {};
const char* estimadorDistanciaActual = "fusion_lados";
float errorRumboPredichoActualDeg = 0.0f;
float correccionRumboPwmActual = 0.0f;
float correccionEncoderPwmActual = 0.0f;
uint8_t intentosRecuperacionRumbo = 0;

uint32_t inicioSinProgresoMs = 0;
uint32_t estableDesdeMs = 0;
uint32_t inicioErrorRumboMs = 0;
FaseMovimiento faseDespuesRecuperacion = FaseMovimiento::NINGUNA;
float distanciaRestanteRecuperacionMm = 0.0f;
int candidatoGiroPositivo = TURN_CANDIDATE_POSITIVE_YAW;
int candidatoGiroNegativo = TURN_CANDIDATE_NEGATIVE_YAW;
int pwmGiroPositivo = VELOCIDAD_MINIMA_GIRO_INICIAL;
int pwmGiroNegativo = VELOCIDAD_MINIMA_GIRO_INICIAL;
int pwmGiroActual = 0;
int signoGiroAplicado = 0;
int signoObjetivoIntento = 0;
bool movimientoGiroConfirmado = false;
uint8_t intentoGiroActual = 1;
uint8_t intentoAvanceActual = 1;
uint32_t pausaReintentoGiroHastaMs = 0;
uint32_t pausaReintentoAvanceHastaMs = 0;
float distanciaRestanteReintentoMm = 0.0f;
float yawObjetivoAbsolutoGiroDeg = 0.0f;
bool polaridadCorregidaEnManiobra = false;
char motivoRecuperacionAutonoma[48] = "none";
uint32_t ultimoControlGiroMs = 0;
int64_t ticksLadoGiroAnteriores[2] = {};
uint32_t ultimoPulsoLadoMs[2] = {};
bool ladoFisicamenteEnergizado[2] = {};
uint32_t inicioEnergiaFisicaLadoMs[2] = {};
ModoGiroSolicitado modoSolicitadoGiro = ModoGiroSolicitado::AUTO;
int pwmBusquedaTorqueGiro = VELOCIDAD_MINIMA_GIRO_INICIAL;
uint32_t ultimoEscalonTorqueGiroMs = 0;
uint32_t pausaCruceGiroHastaMs = 0;
uint32_t inicioImuSinRespuestaMs = 0;
uint32_t inicioDireccionIncorrectaMs = 0;
float yawReferenciaRespuestaDeg = 0.0f;
float yawReferenciaProgresoDeg = 0.0f;

void terminar(TipoEvento tipo, const char* detalle);

void copiarMotivoRecuperacion(const char* detalle) {
  strncpy(motivoRecuperacionAutonoma, detalle ? detalle : "unknown",
          sizeof(motivoRecuperacionAutonoma) - 1);
  motivoRecuperacionAutonoma[sizeof(motivoRecuperacionAutonoma) - 1] = '\0';
}

int pasoRampaGiro(bool movimientoConfirmado) {
  return movimientoConfirmado ? PWM_TURN_SLEW_STEP : PWM_TURN_START_SLEW_STEP;
}

void reiniciarSeguimientoEnergiaFisica() {
  ladoFisicamenteEnergizado[0] = ladoFisicamenteEnergizado[1] = false;
  inicioEnergiaFisicaLadoMs[0] = inicioEnergiaFisicaLadoMs[1] = 0;
}

void reiniciarModoGiroArco() {
  modoSolicitadoGiro = ModoGiroSolicitado::AUTO;
  rutaAlineadaGruesa = false;
  rutaAlineadaPrecisa = false;
  reiniciarSeguimientoEnergiaFisica();
}

void aplicarGiroSeguro(int candidato, int pwm) {
  aplicarVelocidades(-candidato * pwm, candidato * pwm);
}

float normalizar180(float valor) {
  while (valor > 180.0f) valor -= 360.0f;
  while (valor < -180.0f) valor += 360.0f;
  return valor;
}

bool segmentoEsOrtogonal(float dx, float dy) {
  return fabsf(dx) <= ORTHOGONAL_TOLERANCE_CM ||
         fabsf(dy) <= ORTHOGONAL_TOLERANCE_CM;
}

float toleranciaDistanciaParaObjetivo(float objetivoCm) {
  return min(TOLERANCIA_DISTANCIA_CM,
             max(TOLERANCIA_DISTANCIA_MIN_CM,
                 fabsf(objetivoCm) * TOLERANCIA_DISTANCIA_FRACCION));
}

EjeMovimiento resolverEjeMovimiento(float dx, float dy) {
  const float ax = fabsf(dx);
  const float ay = fabsf(dy);
  const bool candidatoX = ay <= ROUTE_CROSS_TRACK_RESIDUAL_CM &&
                          ax > ORTHOGONAL_TOLERANCE_CM &&
                          (ay <= ORTHOGONAL_TOLERANCE_CM ||
                           ax >= ay * ROUTE_AXIS_DOMINANCE_RATIO);
  const bool candidatoY = ax <= ROUTE_CROSS_TRACK_RESIDUAL_CM &&
                          ay > ORTHOGONAL_TOLERANCE_CM &&
                          (ax <= ORTHOGONAL_TOLERANCE_CM ||
                           ay >= ax * ROUTE_AXIS_DOMINANCE_RATIO);
  if (candidatoX && !candidatoY) return EjeMovimiento::X;
  if (candidatoY && !candidatoX) return EjeMovimiento::Y;
  if (candidatoX && candidatoY) return ax >= ay ? EjeMovimiento::X : EjeMovimiento::Y;
  return EjeMovimiento::INVALIDO;
}

int aproximarEntero(float valor) {
  return static_cast<int>(valor >= 0.0f ? valor + 0.5f : valor - 0.5f);
}

int ladoReducidoParaCandidato(int candidato) {
  return candidato > 0 ? 0 : 1;
}

float limitarCorreccionRecuperacion(float error) {
  return constrain(error, -HEADING_RECOVERY_MAX_STEP_DEG,
                   HEADING_RECOVERY_MAX_STEP_DEG);
}

float toleranciaParaFase(FaseMovimiento faseEvaluada) {
  (void)faseEvaluada;
  return TOLERANCIA_GIRO_DEG;
}

bool requiereAlineacionCardinal(float error) {
  return fabsf(error) > TOLERANCIA_GIRO_DEG;
}

int signoControlGiroContinuo(float error) {
  if (fabsf(error) <= TOLERANCIA_GIRO_DEG) return 0;
  return error > 0.0f ? 1 : -1;
}

bool requiereReintentoPorCruce(float error, int signoIntento, float tolerancia) {
  if (fabsf(error) <= tolerancia) return false;
  const int signoError = error > 0.0f ? 1 : -1;
  return signoIntento != 0 && signoError != signoIntento;
}

int calcularPwmObjetivoGiro(float errorAbs, int minimoCalibrado) {
  const int pwmCerca = min(PWM_SAFE_HARD_LIMIT,
      max(VELOCIDAD_MINIMA_GIRO_INICIAL, minimoCalibrado + PWM_TURN_NEAR_MARGIN));
  const int pwmLejos = min(PWM_SAFE_HARD_LIMIT,
      max(pwmCerca, minimoCalibrado + PWM_TURN_FAR_MARGIN));
  if (errorAbs >= TURN_BRAKING_ZONE_DEG) return pwmLejos;
  const float fraccion = constrain(errorAbs / TURN_BRAKING_ZONE_DEG, 0.0f, 1.0f);
  return pwmCerca + aproximarEntero((pwmLejos - pwmCerca) * fraccion);
}

bool direccionGiroIncorrecta(float progresoDeg, float objetivoDeg) {
  return fabsf(progresoDeg) >= TURN_DIRECTION_MISMATCH_DEG &&
         progresoDeg * objetivoDeg < 0.0f;
}

float proyectarDistanciaCardinalCm(float rumboDeg, float dx, float dy) {
  const float rumbo = normalizar180(rumboDeg);
  if (fabsf(rumbo - 90.0f) < 1.0f) return max(0.0f, dx);
  if (fabsf(rumbo + 90.0f) < 1.0f) return max(0.0f, -dx);
  if (fabsf(rumbo) < 1.0f) return max(0.0f, dy);
  return max(0.0f, -dy);
}

float distanciaRestanteProyectadaRutaCm() {
  return proyectarDistanciaCardinalCm(
      rumboCardinalObjetivoDeg,
      nuevoDestinoX - PoseGlobal.getX(),
      nuevoDestinoY - PoseGlobal.getY());
}

int siguienteEscalonTorqueGiro(int pwmActual) {
  return min(PWM_SAFE_HARD_LIMIT, pwmActual + CALIBRATION_PWM_STEP);
}

bool modoAdmitePivotContinuo(ModoGiroSolicitado modo) {
  switch (modo) {
    case ModoGiroSolicitado::AUTO:
    case ModoGiroSolicitado::PIVOT:
    case ModoGiroSolicitado::ARC_LEFT_ACTIVE:
    case ModoGiroSolicitado::ARC_RIGHT_ACTIVE:
      return true;
  }
  return false;
}

float rumboCardinalParaSegmento(float dx, float dy) {
  // El frente del robot al inicializar define +Y y yaw 0°.
  if (fabsf(dx) >= fabsf(dy)) return dx >= 0.0f ? 90.0f : -90.0f;
  return dy >= 0.0f ? 0.0f : 180.0f;
}

float errorCardinal(float objetivoDeg, float actualDeg) {
  return normalizar180(objetivoDeg - actualDeg);
}

void copiarIdActivo(const char* id, const char* nombre) {
  strncpy(comandoActivoId, id ? id : "", sizeof(comandoActivoId) - 1);
  comandoActivoId[sizeof(comandoActivoId) - 1] = '\0';
  strncpy(comandoActivoNombre, nombre ? nombre : "", sizeof(comandoActivoNombre) - 1);
  comandoActivoNombre[sizeof(comandoActivoNombre) - 1] = '\0';
  progresoComando = 0.0f;
}

SensorSnapshot sensores() {
  return snapshotSensoresControl();
}

void copiarTicks(const SensorSnapshot& s, int64_t destino[4]) {
  destino[0] = s.pulsosFL; destino[1] = s.pulsosFR;
  destino[2] = s.pulsosBL; destino[3] = s.pulsosBR;
}

void deltasDesde(const SensorSnapshot& s, const int64_t base[4], int64_t salida[4]) {
  salida[0] = llabs(s.pulsosFL - base[0]); salida[1] = llabs(s.pulsosFR - base[1]);
  salida[2] = llabs(s.pulsosBL - base[2]); salida[3] = llabs(s.pulsosBR - base[3]);
}

int64_t ticksTotalesLado(const SensorSnapshot& s, int lado) {
  return lado == 0 ? s.pulsosFL + s.pulsosBL : s.pulsosFR + s.pulsosBR;
}

float mediana4(const int64_t valores[4]) {
  int64_t orden[4] = {valores[0], valores[1], valores[2], valores[3]};
  for (int i = 1; i < 4; ++i) {
    int64_t v = orden[i]; int j = i - 1;
    while (j >= 0 && orden[j] > v) { orden[j + 1] = orden[j]; --j; }
    orden[j + 1] = v;
  }
  return (orden[1] + orden[2]) * 0.5f;
}

bool hayEncoderPorLado(const bool confiables[4]) {
  return (confiables[0] || confiables[2]) && (confiables[1] || confiables[3]);
}

bool hayEncoderPorLado() {
  return hayEncoderPorLado(encoderConfiable);
}

float promedioLadoConMascara(const int64_t valores[4], const bool confiables[4],
                             bool izquierdo) {
  const int a = izquierdo ? 0 : 1, b = izquierdo ? 2 : 3;
  float suma = 0; int cantidad = 0;
  if (confiables[a]) { suma += valores[a]; ++cantidad; }
  if (confiables[b]) { suma += valores[b]; ++cantidad; }
  return cantidad ? suma / cantidad : 0.0f;
}

float promedioLado(const int64_t valores[4], bool izquierdo) {
  return promedioLadoConMascara(valores, encoderConfiable, izquierdo);
}

float promedioFiltradoLado(const SensorSnapshot& s, bool izquierdo) {
  const float valores[4] = {
      s.delta_pulsos_filtrado_FL, s.delta_pulsos_filtrado_FR,
      s.delta_pulsos_filtrado_BL, s.delta_pulsos_filtrado_BR};
  const int a = izquierdo ? 0 : 1, b = izquierdo ? 2 : 3;
  float suma = 0.0f;
  int cantidad = 0;
  if (encoderConfiable[a]) { suma += valores[a]; ++cantidad; }
  if (encoderConfiable[b]) { suma += valores[b]; ++cantidad; }
  return cantidad ? suma / cantidad : 0.0f;
}

void actualizarSaludEncoders(const int64_t valores[4], float errorRumbo) {
  const uint32_t ahora = millis();
  const bool degradadoAntes = modoDegradado;
  const char* nombres[4] = {"fl", "fr", "bl", "br"};
  const int pares[2][2] = {{0, 2}, {1, 3}};

  // Comparar cada rueda solamente con su compañera del mismo lado. La mediana
  // global convertía una asimetría real entre lados en cuatro falsos outliers.
  // Si una rueda queda en cero y la otra avanza, se excluye sólo la detenida.
  for (const auto& par : pares) {
    const int a = par[0], b = par[1];
    const float mayor = static_cast<float>(max(valores[a], valores[b]));
    if (mayor < ENCODER_MIN_AUDIT_TICKS || fabsf(errorRumbo) > 3.0f) {
      for (int i : par) {
        if (saludEncoder[i] != SaludEncoder::EXCLUIDO) saludEncoder[i] = SaludEncoder::OK;
        inicioOutlierEncoderMs[i] = 0;
      }
      continue;
    }

    const float desacuerdo = fabsf(static_cast<float>(valores[a] - valores[b])) /
                             max(mayor, 1.0f);
    if (desacuerdo <= ENCODER_MAX_RELATIVE_DISAGREEMENT) {
      for (int i : par) {
        if (saludEncoder[i] != SaludEncoder::EXCLUIDO) saludEncoder[i] = SaludEncoder::OK;
        inicioOutlierEncoderMs[i] = 0;
      }
      continue;
    }

    const int sospechoso = valores[a] < valores[b] ? a : b;
    const int activo = sospechoso == a ? b : a;
    if (saludEncoder[activo] != SaludEncoder::EXCLUIDO) {
      saludEncoder[activo] = SaludEncoder::OK;
      inicioOutlierEncoderMs[activo] = 0;
    }
    if (saludEncoder[sospechoso] == SaludEncoder::EXCLUIDO) continue;
    saludEncoder[sospechoso] = SaludEncoder::SOSPECHOSO;
    if (!inicioOutlierEncoderMs[sospechoso]) inicioOutlierEncoderMs[sospechoso] = ahora;
    if (ahora - inicioOutlierEncoderMs[sospechoso] >= ENCODER_OUTLIER_PERSISTENCE_MS) {
      saludEncoder[sospechoso] = SaludEncoder::EXCLUIDO;
      encoderConfiable[sospechoso] = false;
      encoderConfiableGlobal[sospechoso] = false;
      char detalle[48];
      snprintf(detalle, sizeof(detalle), "encoder_%s_excluded_degraded", nombres[sospechoso]);
      encolarEvento(EVT_PROGRESS, comandoActivoId, detalle, progresoComando);
    }
  }
  modoDegradado = !(encoderConfiable[0] && encoderConfiable[1] && encoderConfiable[2] && encoderConfiable[3]);
  if (!degradadoAntes && modoDegradado) {
    encolarEvento(EVT_PROGRESS, comandoActivoId, "encoder_fusion_degraded", progresoComando);
  }
}

float clasificarYEstimar(const int64_t valores[4], float errorRumbo) {
  actualizarSaludEncoders(valores, errorRumbo);
  if (!hayEncoderPorLado()) return -1.0f;
  const float izquierda = promedioLado(valores, true);
  const float derecha = promedioLado(valores, false);
  estimadorDistanciaActual = modoDegradado ? "fusion_lados_degradada" : "fusion_lados";
  return 0.5f * (izquierda + derecha);
}

void terminar(TipoEvento tipo, const char* detalle) {
  frenarMotores();
  fase = FaseMovimiento::NINGUNA;
  pausaReintentoGiroHastaMs = 0;
  pausaReintentoAvanceHastaMs = 0;
  pwmObjetivoGiroActual = 0;
  faseFrenadoGiroActual = "OFF";
  estableDesdeMs = 0;
  progresoComando = tipo == EVT_COMPLETED ? 1.0f : progresoComando;
  estadoActual = tipo == EVT_FAULT ? FAULT_SENSOR : (robotCalibrado ? IDLE : UNCALIBRATED);
  encolarEvento(tipo, comandoActivoId, detalle);
}

void iniciarBaseDistancia(float distanciaMm, FaseMovimiento nuevaFase,
                          bool conservarFusion = false) {
  const SensorSnapshot s = sensores();
  copiarTicks(s, ticksBase);
  copiarTicks(s, ticksUltimaIntegracion);
  distanciaIntegradaCm = 0.0f;
  if (!conservarFusion) {
    const bool degradadoAntes = modoDegradado;
    for (int i = 0; i < 4; ++i) {
      encoderConfiable[i] = true;
      encoderConfiableGlobal[i] = true;
      saludEncoder[i] = SaludEncoder::OK;
      inicioOutlierEncoderMs[i] = 0;
    }
    modoDegradado = false;
    estimadorDistanciaActual = "fusion_lados";
    intentosRecuperacionRumbo = 0;
    if (degradadoAntes && comandoActivoId[0]) {
      encolarEvento(EVT_PROGRESS, comandoActivoId, "encoder_fusion_restored", progresoComando);
    }
  }
  distanciaFirmadaCm = distanciaMm / 10.0f;
  distanciaObjetivoCM = fabsf(distanciaFirmadaCm);
  rumboTramoDeg = PoseGlobal.getThetaDeg();
  tiempoInicioAvance = millis();
  inicioSinProgresoMs = millis();
  inicioErrorRumboMs = 0;
  errorRumboPredichoActualDeg = 0.0f;
  correccionRumboPwmActual = 0.0f;
  correccionEncoderPwmActual = 0.0f;
  estableDesdeMs = 0;
  fase = nuevaFase;
  estadoActual = AVANZANDO;
}

void prepararIntentoGiro(float anguloDeg, FaseMovimiento nuevaFase,
                         EstadoRobot estadoGiro, bool nuevaManiobra) {
  PoseGlobal.iniciarMedicionTraslacionGiro();
  const SensorSnapshot s = sensores();
  copiarTicks(s, ticksBase);
  anguloObjetivoGrados = anguloDeg;
  anguloSolicitadoGiroDeg = anguloDeg;
  yawInicioGiroDeg = obtenerYawIMUDeg();
  progresoGiroActualDeg = 0.0f;
  errorGiroActualDeg = anguloDeg;
  errorPredichoGiroActualDeg = anguloDeg;
  pwmObjetivoGiroActual = 0;
  faseFrenadoGiroActual = "RAMP_UP";
  tiempoInicioGiro = millis();
  inicioSinProgresoMs = millis();
  estableDesdeMs = 0;
  pwmGiroActual = 0;
  signoGiroAplicado = 0;
  signoObjetivoIntento = anguloDeg > 0.0f ? 1 : -1;
  movimientoGiroConfirmado = false;
  if (nuevaManiobra) {
    intentoGiroActual = 1;
    polaridadCorregidaEnManiobra = false;
    yawObjetivoAbsolutoGiroDeg = yawInicioGiroDeg + anguloDeg;
    pausaReintentoGiroHastaMs = 0;
    copiarMotivoRecuperacion("none");
  }
  const int minimoInicial = anguloDeg > 0.0f ? pwmGiroPositivo : pwmGiroNegativo;
  pwmBusquedaTorqueGiro = calcularPwmObjetivoGiro(
      TURN_BRAKING_ZONE_DEG, max(VELOCIDAD_MINIMA_GIRO_INICIAL, minimoInicial));
  ultimoEscalonTorqueGiroMs = millis();
  pausaCruceGiroHastaMs = 0;
  inicioImuSinRespuestaMs = 0;
  inicioDireccionIncorrectaMs = 0;
  yawReferenciaRespuestaDeg = yawInicioGiroDeg;
  yawReferenciaProgresoDeg = yawInicioGiroDeg;
  ultimoControlGiroMs = 0;
  ticksLadoGiroAnteriores[0] = ticksLadoGiroAnteriores[1] = 0;
  ultimoPulsoLadoMs[0] = ultimoPulsoLadoMs[1] = millis();
  reiniciarSeguimientoEnergiaFisica();
  fase = nuevaFase;
  estadoActual = estadoGiro;
}

void iniciarBaseGiro(float anguloDeg, FaseMovimiento nuevaFase,
                     EstadoRobot estadoGiro = GIRANDO) {
  prepararIntentoGiro(anguloDeg, nuevaFase, estadoGiro, true);
}

bool agotarOProgramarReintentoGiro(const char* motivo,
                                   bool corregirPolaridad = false) {
  frenarMotores();
  pwmGiroActual = 0;
  pwmObjetivoGiroActual = 0;
  signoGiroAplicado = 0;
  copiarMotivoRecuperacion(motivo);

  if (corregirPolaridad && !polaridadCorregidaEnManiobra) {
    const int anteriorPositivo = candidatoGiroPositivo;
    candidatoGiroPositivo = candidatoGiroNegativo;
    candidatoGiroNegativo = anteriorPositivo;
    polaridadCorregidaEnManiobra = true;
    encolarEvento(EVT_PROGRESS, comandoActivoId,
                  "turn_polarity_corrected_from_yaw", progresoComando);
  }

  if (intentoGiroActual >= AUTONOMOUS_MOTION_MAX_ATTEMPTS) {
    char detalle[48];
    snprintf(detalle, sizeof(detalle), "%s_exhausted", motivo ? motivo : "turn_retry");
    terminar(EVT_FAULT, detalle);
    return false;
  }

  ++intentoGiroActual;
  pausaReintentoGiroHastaMs = millis() + AUTONOMOUS_RETRY_PAUSE_MS;
  faseFrenadoGiroActual = "RECOVERY_RETRY_PAUSE";
  estadoActual = RECOVERING;
  reiniciarSeguimientoEnergiaFisica();
  inicioImuSinRespuestaMs = 0;
  inicioDireccionIncorrectaMs = 0;
  encolarEvento(EVT_PROGRESS, comandoActivoId, motivo, progresoComando);
  return true;
}

bool agotarOProgramarReintentoAvance(const char* motivo) {
  frenarMotores();
  copiarMotivoRecuperacion(motivo);
  if (intentoAvanceActual >= AUTONOMOUS_MOTION_MAX_ATTEMPTS) {
    char detalle[48];
    snprintf(detalle, sizeof(detalle), "%s_exhausted", motivo ? motivo : "drive_retry");
    terminar(EVT_FAULT, detalle);
    return false;
  }

  const float restante = fase == FaseMovimiento::MOVE_AVANCE
      ? distanciaRestanteProyectadaRutaCm()
      : max(0.0f, distanciaObjetivoCM - distanciaIntegradaCm);
  distanciaRestanteReintentoMm =
      (distanciaFirmadaCm < 0.0f ? -1.0f : 1.0f) * restante * 10.0f;
  ++intentoAvanceActual;
  pausaReintentoAvanceHastaMs = millis() + AUTONOMOUS_RETRY_PAUSE_MS;
  encolarEvento(EVT_PROGRESS, comandoActivoId, motivo, progresoComando);
  return true;
}

bool iniciarAvanceRuta(bool conservarFusion = false) {
  const float restante = distanciaRestanteProyectadaRutaCm();
  const float tolerancia = toleranciaDistanciaParaObjetivo(distanciaInicialManejo);
  if (restante <= tolerancia) {
    rumboCardinalObjetivoDeg = RUMBO_REPOSO_RUTA_DEG;
    const float errorFinal = errorCardinal(
        RUMBO_REPOSO_RUTA_DEG, PoseGlobal.getThetaDeg());
    if (fabsf(errorFinal) <= TOLERANCIA_GIRO_DEG) {
      rutaAlineadaPrecisa = true;
      terminar(EVT_COMPLETED, "move_completed_by_turn_translation");
    } else {
      rutaAlineadaPrecisa = false;
      iniciarBaseGiro(errorFinal, FaseMovimiento::MOVE_ALINEACION_FINAL);
    }
    return false;
  }
  iniciarBaseDistancia(restante * 10.0f, FaseMovimiento::MOVE_AVANCE,
                       conservarFusion);
  rumboTramoDeg = rumboCardinalObjetivoDeg;
  return true;
}
}

bool validarControlGiroSeguro() {
  const int64_t ticksPrueba[4] = {0, 10, 10, 10};
  const bool flExcluido[4] = {false, true, true, true};
  const bool ladoIzquierdoPerdido[4] = {false, true, false, true};
  const int pwmCercaEsperado = min(PWM_SAFE_HARD_LIMIT,
      VELOCIDAD_MINIMA_GIRO_INICIAL + PWM_TURN_NEAR_MARGIN);
  const int pwmLejosEsperado = min(PWM_SAFE_HARD_LIMIT,
      VELOCIDAD_MINIMA_GIRO_INICIAL + PWM_TURN_FAR_MARGIN);
  return toleranciaParaFase(FaseMovimiento::MOVE_GIRO) == 2.0f &&
         !requiereAlineacionCardinal(2.0f) &&
         requiereAlineacionCardinal(-2.01f) &&
         rumboCardinalParaSegmento(100.0f, 0.0f) == 90.0f &&
         rumboCardinalParaSegmento(-100.0f, 0.0f) == -90.0f &&
         rumboCardinalParaSegmento(0.0f, 100.0f) == 0.0f &&
         rumboCardinalParaSegmento(0.0f, -100.0f) == 180.0f &&
         MPU_YAW_POLARITY == -1.0f &&
         TURN_CANDIDATE_POSITIVE_YAW == -1 &&
         TURN_CANDIDATE_NEGATIVE_YAW == 1 &&
         errorCardinal(0.0f, 360.0f) == 0.0f &&
         errorCardinal(90.0f, -270.0f) == 0.0f &&
         errorCardinal(-90.0f, 270.0f) == 0.0f &&
         errorCardinal(180.0f, -180.0f) == 0.0f &&
         errorCardinal(90.0f, 0.0f) == 90.0f &&
         errorCardinal(-90.0f, 0.0f) == -90.0f &&
         fabsf(errorCardinal(180.0f, 0.0f)) == 180.0f &&
         signoControlGiroContinuo(0.0f) == 0 &&
         signoControlGiroContinuo(2.0f) == 0 &&
         signoControlGiroContinuo(-2.0f) == 0 &&
         signoControlGiroContinuo(2.01f) == 1 &&
         signoControlGiroContinuo(-2.01f) == -1 &&
         HEADING_RECOVERY_TRIGGER_DEG == 12.0f &&
         HEADING_RECOVERY_HOLD_MS == 300 &&
         HEADING_RECOVERY_MAX_STEP_DEG == 25.0f &&
         HEADING_RECOVERY_MAX_ATTEMPTS == 7 &&
         limitarCorreccionRecuperacion(60.0f) == 25.0f &&
         limitarCorreccionRecuperacion(-60.0f) == -25.0f &&
         ladoReducidoParaCandidato(1) == 0 &&
         ladoReducidoParaCandidato(-1) == 1 &&
         hayEncoderPorLado(flExcluido) &&
         !hayEncoderPorLado(ladoIzquierdoPerdido) &&
         promedioLadoConMascara(ticksPrueba, flExcluido, true) == 10.0f &&
         promedioLadoConMascara(ticksPrueba, flExcluido, false) == 10.0f &&
         PWM_SAFE_LIMIT_8BIT == 230 &&
         segmentoEsOrtogonal(100.0f, 0.1f) &&
         segmentoEsOrtogonal(0.1f, -100.0f) &&
         !segmentoEsOrtogonal(0.11f, 0.11f) &&
         fabsf(toleranciaDistanciaParaObjetivo(5.0f) - 0.5f) < 0.001f &&
         fabsf(toleranciaDistanciaParaObjetivo(20.0f) - 2.0f) < 0.001f &&
         resolverEjeMovimiento(10.0f, 2.0f) == EjeMovimiento::X &&
         resolverEjeMovimiento(-10.0f, -2.5f) == EjeMovimiento::X &&
         resolverEjeMovimiento(2.0f, 10.0f) == EjeMovimiento::Y &&
         resolverEjeMovimiento(-2.5f, -10.0f) == EjeMovimiento::Y &&
         resolverEjeMovimiento(2.0f, 2.0f) == EjeMovimiento::INVALIDO &&
         resolverEjeMovimiento(3.0f, 10.0f) == EjeMovimiento::INVALIDO &&
         fabsf(normalizar180(-270.0f) - 90.0f) < 0.001f &&
         fabsf(normalizar180(270.0f) + 90.0f) < 0.001f &&
         fabsf(normalizar180(360.0f)) < 0.001f &&
         fabsf(normalizar180(-360.0f)) < 0.001f &&
         fabsf(normalizar180(359.0f) + 1.0f) < 0.001f &&
         fabsf(normalizar180(-359.0f) - 1.0f) < 0.001f &&
         modoAdmitePivotContinuo(ModoGiroSolicitado::AUTO) &&
         modoAdmitePivotContinuo(ModoGiroSolicitado::PIVOT) &&
         modoAdmitePivotContinuo(ModoGiroSolicitado::ARC_LEFT_ACTIVE) &&
         modoAdmitePivotContinuo(ModoGiroSolicitado::ARC_RIGHT_ACTIVE) &&
         pasoRampaGiro(false) == static_cast<int>(2 * PWM_SCALE_8_TO_10) &&
         pasoRampaGiro(true) == static_cast<int>(2 * PWM_SCALE_8_TO_10) &&
         calcularPwmObjetivoGiro(30.0f, VELOCIDAD_MINIMA_GIRO_INICIAL) == pwmLejosEsperado &&
         calcularPwmObjetivoGiro(0.0f, VELOCIDAD_MINIMA_GIRO_INICIAL) == pwmCercaEsperado &&
         calcularPwmObjetivoGiro(20.0f, VELOCIDAD_MINIMA_GIRO_INICIAL) >
             calcularPwmObjetivoGiro(8.0f, VELOCIDAD_MINIMA_GIRO_INICIAL) &&
         !direccionGiroIncorrecta(2.9f, 90.0f) &&
         direccionGiroIncorrecta(-3.0f, 90.0f) &&
         direccionGiroIncorrecta(3.0f, -90.0f) &&
         !direccionGiroIncorrecta(3.0f, 90.0f) &&
         proyectarDistanciaCardinalCm(90.0f, 35.0f, 12.0f) == 35.0f &&
         proyectarDistanciaCardinalCm(-90.0f, -35.0f, 12.0f) == 35.0f &&
         proyectarDistanciaCardinalCm(0.0f, 12.0f, 35.0f) == 35.0f &&
         proyectarDistanciaCardinalCm(180.0f, 12.0f, -35.0f) == 35.0f &&
         proyectarDistanciaCardinalCm(90.0f, -2.0f, 0.0f) == 0.0f &&
         TURN_IMU_RESPONSE_TIMEOUT_MS == 500 &&
         TURN_DIRECTION_MISMATCH_HOLD_MS == 250 &&
         AUTONOMOUS_MOTION_MAX_ATTEMPTS == 7 &&
         AUTONOMOUS_RETRY_PAUSE_MS == 750 &&
         siguienteEscalonTorqueGiro(PWM_SAFE_HARD_LIMIT) == PWM_SAFE_HARD_LIMIT &&
         siguienteEscalonTorqueGiro(PWM_SAFE_HARD_LIMIT - CALIBRATION_PWM_STEP) ==
             PWM_SAFE_HARD_LIMIT;
}

bool iniciarCalibracion(const char* command_id) {
  if (estadoActual != UNCALIBRATED && estadoActual != IDLE) return false;
  copiarIdActivo(command_id, "calibrate");
  if (!habilitarControlSimpleSinCalibracion()) {
    comandoActivoId[0] = '\0';
    comandoActivoNombre[0] = '\0';
    return false;
  }
  frenarMotores();
  PoseGlobal.reset();
  resetOrientacionIMU();
  encolarEvento(EVT_COMPLETED, command_id, "simple_control_ready");
  return true;
}

bool habilitarControlSimpleSinCalibracion() {
  const SensorSnapshot s = sensores();
  if (!s.mpu_present || !s.mpu_calibrated || s.mpu_stale) return false;
  // Polaridad y potencia tomadas del ensayo aprobado. No se mueve el robot
  // para descubrirlas: queda listo al terminar la calibración estática de MPU.
  candidatoGiroPositivo = TURN_CANDIDATE_POSITIVE_YAW;
  candidatoGiroNegativo = TURN_CANDIDATE_NEGATIVE_YAW;
  pwmGiroPositivo = PWM_SAFE_HARD_LIMIT;
  pwmGiroNegativo = PWM_SAFE_HARD_LIMIT;
  robotCalibrado = true;
  if (estadoActual == UNCALIBRATED) estadoActual = IDLE;
  return true;
}

bool iniciarGiroRelativo(float angulo_deg, const char* command_id, ModoGiroSolicitado modo) {
  if (!robotCalibrado || estadoActual != IDLE || fabsf(angulo_deg) > 360.0f) return false;
  if (!modoAdmitePivotContinuo(modo)) return false;
  reiniciarModoGiroArco();
  modoSolicitadoGiro = modo;
  copiarIdActivo(command_id, "turn");
  if (fabsf(angulo_deg) <= 0.001f) {
    frenarMotores();
    encolarEvento(EVT_COMPLETED, comandoActivoId, "turn_zero_noop");
    return true;
  }
  iniciarBaseGiro(angulo_deg, FaseMovimiento::TURN);
  return true;
}

bool iniciarDistancia(float distancia_mm, const char* command_id) {
  if (!robotCalibrado || estadoActual != IDLE || distancia_mm == 0.0f || fabsf(distancia_mm) > MAX_AUTONOMOUS_SEGMENT_MM) return false;
  reiniciarModoGiroArco();
  copiarIdActivo(command_id, "drive");
  intentoAvanceActual = 1;
  pausaReintentoAvanceHastaMs = 0;
  copiarMotivoRecuperacion("none");
  iniciarBaseDistancia(distancia_mm, FaseMovimiento::DRIVE);
  return true;
}

bool calcularDestino(float x_destino, float y_destino, const char* command_id) {
  if (!robotCalibrado || estadoActual != IDLE) return false;
  ultimoRechazoMovimiento[0] = '\0';
  const float xActual = PoseGlobal.getX();
  const float yActual = PoseGlobal.getY();
  const float dx = x_destino - xActual;
  const float dy = y_destino - yActual;
  const EjeMovimiento eje = resolverEjeMovimiento(dx, dy);
  if (eje == EjeMovimiento::INVALIDO) {
    strncpy(ultimoRechazoMovimiento, "non_orthogonal_segment", sizeof(ultimoRechazoMovimiento) - 1);
    return false;
  }
  const bool moverEnX = eje == EjeMovimiento::X;
  const float desplazamientoPrincipal = moverEnX ? dx : dy;
  const float distancia = fabsf(desplazamientoPrincipal);
  if (distancia <= 0.05f || distancia * 10.0f > MAX_AUTONOMOUS_SEGMENT_MM) {
    strncpy(ultimoRechazoMovimiento, "distance_limit", sizeof(ultimoRechazoMovimiento) - 1);
    return false;
  }
  reiniciarModoGiroArco();
  nuevoDestinoX = moverEnX ? x_destino : xActual;
  nuevoDestinoY = moverEnX ? yActual : y_destino;
  copiarIdActivo(command_id, "move");
  intentoAvanceActual = 1;
  pausaReintentoAvanceHastaMs = 0;
  copiarMotivoRecuperacion("none");
  rutaAlineadaGruesa = false;
  rutaAlineadaPrecisa = false;
  rumboCardinalObjetivoDeg = moverEnX
      ? (desplazamientoPrincipal >= 0.0f ? 90.0f : -90.0f)
      : (desplazamientoPrincipal >= 0.0f ? 0.0f : 180.0f);
  const float giro = errorCardinal(rumboCardinalObjetivoDeg, PoseGlobal.getThetaDeg());
  distanciaInicialManejo = distancia;
  if (!requiereAlineacionCardinal(giro)) {
    rutaAlineadaGruesa = true;
    rutaAlineadaPrecisa = true;
    iniciarBaseDistancia(distancia * 10.0f, FaseMovimiento::MOVE_AVANCE);
    rumboTramoDeg = rumboCardinalObjetivoDeg;
  } else {
    iniciarBaseGiro(giro, FaseMovimiento::MOVE_GIRO);
  }
  return true;
}

void cancelarMovimiento(const char* detalle) {
  const EstadoRobot estadoPrevio = estadoActual;
  if (comandoActivoId[0]) encolarEvento(EVT_REJECTED, comandoActivoId, detalle ? detalle : "cancelled");
  frenarMotores(); fase = FaseMovimiento::NINGUNA;
  if (estadoPrevio == ESTOP_LATCHED) estadoActual = ESTOP_LATCHED;
  else if (estadoPrevio == FAULT_SENSOR) estadoActual = FAULT_SENSOR;
  else estadoActual = robotCalibrado ? IDLE : UNCALIBRATED;
  progresoComando = 0.0f;
}

void abortarMovimientoPorSeguridad(const char* detalle) {
  if (comandoActivoId[0]) {
    terminar(EVT_FAULT, detalle ? detalle : "motion_aborted");
    return;
  }
  frenarMotores();
  fase = FaseMovimiento::NINGUNA;
  pwmObjetivoGiroActual = 0;
  faseFrenadoGiroActual = "OFF";
}

void controlarCalibracion() {
  // Conservado como API compatible; la calibración motriz fue eliminada.
  // Ninguna transición normal entra ya en CALIBRATING.
  if (estadoActual == CALIBRATING) {
    frenarMotores();
    estadoActual = robotCalibrado ? IDLE : UNCALIBRATED;
  }
}

void controlarGiro() {
  if (estadoActual != GIRANDO && estadoActual != RECOVERING) return;
  const uint32_t ahora = millis();
  if (ahora - ultimoControlGiroMs < TURN_CONTROL_PERIOD_MS) return;
  ultimoControlGiroMs = ahora;

  if (pausaReintentoGiroHastaMs != 0) {
    frenarMotores();
    faseFrenadoGiroActual = "RECOVERY_RETRY_PAUSE";
    if (static_cast<int32_t>(ahora - pausaReintentoGiroHastaMs) < 0) return;
    pausaReintentoGiroHastaMs = 0;
    const float residual = yawObjetivoAbsolutoGiroDeg - obtenerYawIMUDeg();
    prepararIntentoGiro(residual, fase, RECOVERING, false);
    encolarEvento(EVT_PROGRESS, comandoActivoId,
                  "turn_retry_started", progresoComando);
    return;
  }

  const SensorSnapshot s = sensores();
  const float progreso = obtenerYawIMUDeg() - yawInicioGiroDeg;
  const float error = anguloObjetivoGrados - progreso;
  const float toleranciaActiva = toleranciaParaFase(fase);
  progresoGiroActualDeg = progreso;
  errorGiroActualDeg = error;
  // La MPU actual manda. El ensayo aprobado no apagaba el giro por una
  // predicción futura; por eso la telemetría predicha coincide con el error real.
  errorPredichoGiroActualDeg = error;
  // 100% queda reservado al evento terminal.
  const float fraccionGiro = min(0.99f, fabsf(progreso) / max(fabsf(anguloObjetivoGrados), 0.1f));
  progresoComando = fraccionGiro;
  if (ahora - tiempoInicioGiro > TURN_TIMEOUT_MS) {
    agotarOProgramarReintentoGiro("turn_timeout");
    return;
  }

  int64_t delta[4]; deltasDesde(s, ticksBase, delta);
  const int64_t ticksLado[2] = {delta[0] + delta[2], delta[1] + delta[3]};
  const int pwmFisicoLado[2] = {abs(pwm_aplicado_L), abs(pwm_aplicado_R)};
  const int minimoDireccion = error >= 0.0f ? pwmGiroPositivo : pwmGiroNegativo;
  const int umbralActividad = max(VELOCIDAD_MINIMA_GIRO_INICIAL,
                                  minimoDireccion - PWM_CALIBRATION_MARGIN);
  for (int lado = 0; lado < 2; ++lado) {
    // No se arma el watchdog durante la parte de la rampa que todavía no
    // alcanza el torque medido. Así un PWM incapaz de vencer fricción no se
    // confunde con un atasco.
    if (pwmFisicoLado[lado] < umbralActividad) {
      ladoFisicamenteEnergizado[lado] = false;
      inicioEnergiaFisicaLadoMs[lado] = 0;
      continue;
    }
    if (!ladoFisicamenteEnergizado[lado]) {
      ladoFisicamenteEnergizado[lado] = true;
      inicioEnergiaFisicaLadoMs[lado] = ahora;
      ticksLadoGiroAnteriores[lado] = ticksLado[lado];
      ultimoPulsoLadoMs[lado] = ahora;
      encolarEvento(EVT_PROGRESS, comandoActivoId,
                    lado == 0 ? "turn_left_physical_power_started"
                              : "turn_right_physical_power_started",
                    progresoComando);
      continue;
    }
    if (ticksLado[lado] != ticksLadoGiroAnteriores[lado]) {
      ticksLadoGiroAnteriores[lado] = ticksLado[lado];
      ultimoPulsoLadoMs[lado] = ahora;
    } else if (ahora - ultimoPulsoLadoMs[lado] > TURN_STALL_TIMEOUT_MS) {
      agotarOProgramarReintentoGiro(
          lado == 0 ? "turn_left_side_stalled_retry"
                    : "turn_right_side_stalled_retry");
      return;
    }
  }
  const bool ticksConfirmanGiro = llabs(ticksLado[0]) >= 4 && llabs(ticksLado[1]) >= 4;
  const bool ambosLadosEnergizados = ladoFisicamenteEnergizado[0] &&
                                     ladoFisicamenteEnergizado[1];
  const float yawActualDeg = obtenerYawIMUDeg();
  if (fabsf(yawActualDeg - yawReferenciaRespuestaDeg) >= TURN_IMU_MIN_RESPONSE_DEG) {
    yawReferenciaRespuestaDeg = yawActualDeg;
    inicioImuSinRespuestaMs = 0;
  } else if (ticksConfirmanGiro && ambosLadosEnergizados &&
             fabsf(error) > toleranciaActiva) {
    if (inicioImuSinRespuestaMs == 0) inicioImuSinRespuestaMs = ahora;
    if (ahora - inicioImuSinRespuestaMs >= TURN_IMU_RESPONSE_TIMEOUT_MS) {
      terminar(EVT_FAULT, "turn_imu_no_response");
      return;
    }
  } else {
    inicioImuSinRespuestaMs = 0;
  }

  if (direccionGiroIncorrecta(progreso, anguloObjetivoGrados)) {
    if (inicioDireccionIncorrectaMs == 0) inicioDireccionIncorrectaMs = ahora;
    if (ahora - inicioDireccionIncorrectaMs >= TURN_DIRECTION_MISMATCH_HOLD_MS) {
      if (polaridadCorregidaEnManiobra) {
        terminar(EVT_FAULT, "turn_direction_mismatch_after_polarity_correction");
      } else {
        agotarOProgramarReintentoGiro("turn_direction_mismatch_retry", true);
      }
      return;
    }
  } else {
    inicioDireccionIncorrectaMs = 0;
  }

  if (!movimientoGiroConfirmado && fabsf(s.gyro_z_filtrado_rad_s) >= GYRO_MOVEMENT_RAD_S &&
      ticksConfirmanGiro) {
    movimientoGiroConfirmado = true;
    inicioSinProgresoMs = ahora;
  }

  if (fabsf(error) <= toleranciaActiva) {
    frenarMotores();
    pwmGiroActual = 0;
    pwmObjetivoGiroActual = 0;
    faseFrenadoGiroActual = "VERIFY_IN_RANGE_300MS";
    signoGiroAplicado = 0;
    pausaCruceGiroHastaMs = 0;
    if (estableDesdeMs == 0) estableDesdeMs = ahora;
    if (ahora - estableDesdeMs < TURN_OVERSHOOT_PAUSE_MS) return;
    if (fase == FaseMovimiento::RECOVERY_TURN) {
      if (faseDespuesRecuperacion == FaseMovimiento::MOVE_AVANCE) {
        rutaAlineadaPrecisa = true;
        encolarEvento(EVT_PROGRESS, comandoActivoId,
                      "move_heading_recovery_aligned", progresoComando);
      }
      if (faseDespuesRecuperacion == FaseMovimiento::MOVE_AVANCE) {
        if (iniciarAvanceRuta(true)) {
          estadoActual = ESPERANDO_ESTABILIZACION;
          tiempoEspera = ahora;
        }
      } else {
        iniciarBaseDistancia(distanciaRestanteRecuperacionMm,
                             faseDespuesRecuperacion, true);
        estadoActual = ESPERANDO_ESTABILIZACION;
        tiempoEspera = ahora;
      }
    } else if (fase == FaseMovimiento::MOVE_GIRO) {
      rutaAlineadaGruesa = true;
      rutaAlineadaPrecisa = true;
      encolarEvento(EVT_PROGRESS, comandoActivoId,
                    "move_cardinal_alignment_stable", progresoComando);
      if (iniciarAvanceRuta()) {
        estadoActual = ESPERANDO_ESTABILIZACION;
        tiempoEspera = ahora;
      }
    } else if (fase == FaseMovimiento::MOVE_ALINEACION_FINAL) {
      rutaAlineadaPrecisa = true;
      terminar(EVT_COMPLETED, "move_completed_zero_aligned");
    } else terminar(EVT_COMPLETED, "turn_completed");
    return;
  }

  if (estableDesdeMs != 0) {
    // La inercia sacó al robot del margen antes de completar los 300 ms. La
    // pausa empezó al apagar los motores, así que sólo se espera el remanente.
    pausaCruceGiroHastaMs = estableDesdeMs + TURN_OVERSHOOT_PAUSE_MS;
    estableDesdeMs = 0;
  }
  if (pausaCruceGiroHastaMs != 0) {
    frenarMotores();
    faseFrenadoGiroActual = "OVERSHOOT_PAUSE_300MS";
    if (static_cast<int32_t>(ahora - pausaCruceGiroHastaMs) < 0) return;
    pausaCruceGiroHastaMs = 0;
  }

  // Si hubo sobrepaso, se replica el comportamiento sencillo solicitado:
  // parada completa durante 300 ms y luego intento en sentido opuesto.
  signoObjetivoIntento = signoControlGiroContinuo(error);
  const int signoDeseado = signoObjetivoIntento;
  if (signoGiroAplicado != 0 && signoDeseado != signoGiroAplicado) {
    frenarMotores();
    pwmGiroActual = 0;
    pwmObjetivoGiroActual = 0;
    signoGiroAplicado = 0;
    pausaCruceGiroHastaMs = ahora + TURN_OVERSHOOT_PAUSE_MS;
    faseFrenadoGiroActual = "OVERSHOOT_PAUSE_300MS";
    return;
  }

  const int minimo = signoObjetivoIntento > 0 ? pwmGiroPositivo : pwmGiroNegativo;
  const float errorAbs = fabsf(error);
  const int pwmPerfil = calcularPwmObjetivoGiro(errorAbs, minimo);
  if (!movimientoGiroConfirmado &&
      ahora - ultimoEscalonTorqueGiroMs >= CALIBRATION_PWM_STEP_INTERVAL_MS) {
    ultimoEscalonTorqueGiroMs = ahora;
    pwmBusquedaTorqueGiro = siguienteEscalonTorqueGiro(pwmBusquedaTorqueGiro);
  }
  // La búsqueda de torque nunca introduce pausas: mientras gyro y ambos lados
  // no confirmen movimiento, el techo sube gradualmente hasta 230/255.
  const int pwmObjetivo = movimientoGiroConfirmado
      ? pwmPerfil : max(pwmPerfil, pwmBusquedaTorqueGiro);

  if (!movimientoGiroConfirmado) {
    faseFrenadoGiroActual = "TORQUE_SEARCH";
  } else if (errorAbs <= TURN_NEAR_ZONE_DEG) {
    faseFrenadoGiroActual = "NEAR_MINIMUM";
  } else if (errorAbs < TURN_BRAKING_ZONE_DEG) {
    faseFrenadoGiroActual = "BRAKING";
  } else {
    faseFrenadoGiroActual = movimientoGiroConfirmado ? "CRUISE" : "RAMP_UP";
  }
  pwmObjetivoGiroActual = constrain(pwmObjetivo, 0, PWM_SAFE_HARD_LIMIT);

  // Rampa suave obligatoria desde cero; una vez en marcha no hay estados de
  // predicción, costa ni alineación gruesa/precisa.
  if (signoGiroAplicado == 0) {
    signoGiroAplicado = signoDeseado;
    pwmGiroActual = 0;
  }
  const int pasoSubida = pasoRampaGiro(movimientoGiroConfirmado);
  if (pwmGiroActual < pwmObjetivo) pwmGiroActual = min(pwmObjetivo, pwmGiroActual + pasoSubida);
  else pwmGiroActual = max(pwmObjetivo, pwmGiroActual - PWM_TURN_BRAKE_SLEW_STEP);

  if (signoGiroAplicado != 0 && pwmGiroActual > 0) {
    const int candidato = signoGiroAplicado > 0 ? candidatoGiroPositivo : candidatoGiroNegativo;
    aplicarGiroSeguro(candidato, pwmGiroActual);
  } else {
    frenarMotores();
  }

  if (fabsf(yawActualDeg - yawReferenciaProgresoDeg) >= TURN_ANGULAR_PROGRESS_DEG) {
    yawReferenciaProgresoDeg = yawActualDeg;
    inicioSinProgresoMs = ahora;
  } else if (ahora - inicioSinProgresoMs > TURN_NO_ANGULAR_PROGRESS_TIMEOUT_MS) {
    agotarOProgramarReintentoGiro("turn_no_angular_progress_retry");
  }
}

uint8_t intentoGiroActualTelemetria() {
  const bool giroActivo = fase == FaseMovimiento::TURN ||
                          fase == FaseMovimiento::MOVE_GIRO ||
                          fase == FaseMovimiento::RECOVERY_TURN ||
                          fase == FaseMovimiento::MOVE_ALINEACION_FINAL;
  return giroActivo ? intentoGiroActual : 0;
}

uint32_t pausaReintentoRestanteMs() {
  if (pausaReintentoGiroHastaMs == 0) return 0;
  const int32_t restante = static_cast<int32_t>(pausaReintentoGiroHastaMs - millis());
  return restante > 0 ? static_cast<uint32_t>(restante) : 0;
}

uint8_t intentoAvanceActualTelemetria() { return intentoAvanceActual; }

uint32_t pausaReintentoAvanceRestanteMs() {
  if (pausaReintentoAvanceHastaMs == 0) return 0;
  const int32_t restante = static_cast<int32_t>(pausaReintentoAvanceHastaMs - millis());
  return restante > 0 ? static_cast<uint32_t>(restante) : 0;
}

const char* motivoRecuperacionAutonomaTelemetria() {
  return motivoRecuperacionAutonoma;
}

int pwmCalibracionActual() {
  return 0;
}

uint8_t intentoTorqueCalibracionTelemetria() {
  return 0;
}

bool busquedaTorqueCalibracionActiva() {
  return false;
}

const char* modoGiroTexto() {
  return "PIVOT_CONTINUOUS";
}

const char* modoGiroSolicitadoTexto() {
  switch (modoSolicitadoGiro) {
    case ModoGiroSolicitado::PIVOT: return "PIVOT";
    case ModoGiroSolicitado::ARC_LEFT_ACTIVE: return "ARC_LEFT_ACTIVE";
    case ModoGiroSolicitado::ARC_RIGHT_ACTIVE: return "ARC_RIGHT_ACTIVE";
    default: return "AUTO";
  }
}

const char* motivoModoGiroArco() {
  return "disabled_continuous_pivot";
}

uint32_t pausaEntradaArcoRestanteMs() {
  return 0;
}

const char* faseCalibracionTexto() {
  return "NOT_REQUIRED";
}

int signoFijoGiroTelemetria() {
  return signoObjetivoIntento;
}

float anguloSolicitadoGiroTelemetria() {
  return anguloSolicitadoGiroDeg;
}

float objetivoFisicoGiroTelemetria() {
  return anguloObjetivoGrados;
}

uint8_t vueltasExtendidasGiroTelemetria() {
  // Campo legado conservado durante la transición del HMI. El controlador ya
  // no crea vueltas automáticas.
  return 0;
}

float rumboCardinalObjetivoTelemetria() {
  return isnan(rumboCardinalObjetivoDeg) ? 0.0f : rumboCardinalObjetivoDeg;
}

const char* faseMovimientoTexto() {
  switch (fase) {
    case FaseMovimiento::MOVE_GIRO: return "TURN_CARDINAL";
    case FaseMovimiento::MOVE_AVANCE: return "DRIVE_STRAIGHT";
    case FaseMovimiento::MOVE_ALINEACION_FINAL: return "FINAL_ALIGN";
    case FaseMovimiento::DRIVE: return "DRIVE";
    case FaseMovimiento::TURN: return "TURN";
    case FaseMovimiento::RECOVERY_TURN: return "RECOVERY_TURN";
    default: return "OFF";
  }
}

const char* etapaAlineacionTexto() {
  switch (fase) {
    case FaseMovimiento::MOVE_GIRO: return "CONTINUOUS";
    case FaseMovimiento::MOVE_AVANCE: return "HEADING_PD";
    case FaseMovimiento::RECOVERY_TURN: return "RECOVERY_PRECISE";
    case FaseMovimiento::MOVE_ALINEACION_FINAL: return "FINAL_PRECISE";
    case FaseMovimiento::TURN: return "TURN_PRECISE";
    default: return "OFF";
  }
}

float toleranciaGiroActualTelemetria() {
  switch (fase) {
    case FaseMovimiento::MOVE_GIRO:
    case FaseMovimiento::MOVE_ALINEACION_FINAL:
    case FaseMovimiento::RECOVERY_TURN:
    case FaseMovimiento::TURN:
      return toleranciaParaFase(fase);
    default:
      return 0.0f;
  }
}

float errorRumboCardinalTelemetria() {
  return isnan(rumboCardinalObjetivoDeg)
      ? 0.0f
      : normalizar180(rumboCardinalObjetivoDeg - PoseGlobal.getThetaDeg());
}

uint32_t estabilidadRumboActualMs() {
  return estableDesdeMs == 0 ? 0 : millis() - estableDesdeMs;
}

bool rutaAlineadaGruesaTelemetria() {
  return rutaAlineadaGruesa;
}

bool rutaAlineadaPrecisaTelemetria() {
  return rutaAlineadaPrecisa;
}

const char* faseArcoTexto() {
  return "OFF";
}

uint16_t pulsosArcoTelemetria() {
  return 0;
}

uint8_t pulsosArcoSinProgresoTelemetria() {
  return 0;
}

uint8_t transicionesFallbackGiroTelemetria() {
  return 0;
}

bool ladoGiroFisicamenteEnergizado(size_t lado) {
  return lado < 2 && ladoFisicamenteEnergizado[lado];
}

uint32_t inicioEnergiaFisicaGiroMs(size_t lado) {
  return lado < 2 ? inicioEnergiaFisicaLadoMs[lado] : 0;
}

const char* motivoUltimoRechazoMovimiento() {
  return ultimoRechazoMovimiento[0] ? ultimoRechazoMovimiento : "move_invalid";
}

float progresoGiroTelemetria() {
  return progresoGiroActualDeg;
}

float errorGiroTelemetria() {
  return errorGiroActualDeg;
}

float errorPredichoGiroTelemetria() {
  return errorPredichoGiroActualDeg;
}

int pwmObjetivoGiroTelemetria() {
  return pwmObjetivoGiroActual;
}

const char* faseFrenadoGiroTelemetria() {
  return faseFrenadoGiroActual;
}

uint32_t tiempoImuSinRespuestaGiroMs() {
  return inicioImuSinRespuestaMs == 0 ? 0 : millis() - inicioImuSinRespuestaMs;
}

uint32_t tiempoDireccionIncorrectaGiroMs() {
  return inicioDireccionIncorrectaMs == 0
      ? 0 : millis() - inicioDireccionIncorrectaMs;
}

float distanciaRestanteEjeRutaTelemetria() {
  if (isnan(rumboCardinalObjetivoDeg)) return 0.0f;
  return distanciaRestanteProyectadaRutaCm();
}

const char* saludEncoderTexto(size_t indice) {
  if (indice >= 4) return "unknown";
  switch (saludEncoder[indice]) {
    case SaludEncoder::OK: return "ok";
    case SaludEncoder::SOSPECHOSO: return "suspect";
    case SaludEncoder::EXCLUIDO: return "excluded";
    default: return "unknown";
  }
}

uint8_t encodersConfiablesLado(size_t lado) {
  if (lado > 1) return 0;
  const int a = lado == 0 ? 0 : 1;
  const int b = lado == 0 ? 2 : 3;
  return static_cast<uint8_t>((encoderConfiable[a] ? 1 : 0) +
                              (encoderConfiable[b] ? 1 : 0));
}

const char* estimadorDistanciaTexto() { return estimadorDistanciaActual; }
float errorRumboPredichoTelemetria() { return errorRumboPredichoActualDeg; }
float correccionRumboPwmTelemetria() { return correccionRumboPwmActual; }
float correccionEncoderPwmTelemetria() { return correccionEncoderPwmActual; }
uint8_t intentoRecuperacionRumboTelemetria() { return intentosRecuperacionRumbo; }
float velocidadFiltradaLadoTelemetria(size_t lado) {
  if (lado > 1) return 0.0f;
  const SensorSnapshot s = sensores();
  const float delta = promedioFiltradoLado(s, lado == 0);
  return delta * (PI * WHEEL_DIAMETER_ODOMETRY_CM / ENCODER_PPR) *
         (1000.0f / SENSOR_PERIOD_MS);
}

int64_t ticksUltimoPulsoCalibracion(size_t indice) {
  (void)indice;
  return 0;
}

void controlarAvance() {
  if (estadoActual != AVANZANDO) return;
  const uint32_t ahora = millis();
  if (pausaReintentoAvanceHastaMs != 0) {
    frenarMotores();
    if (static_cast<int32_t>(ahora - pausaReintentoAvanceHastaMs) < 0) return;
    pausaReintentoAvanceHastaMs = 0;
    const FaseMovimiento faseReanudada = fase;
    if (faseReanudada == FaseMovimiento::MOVE_AVANCE) {
      iniciarAvanceRuta(false);
    } else {
      iniciarBaseDistancia(distanciaRestanteReintentoMm, faseReanudada, false);
    }
    encolarEvento(EVT_PROGRESS, comandoActivoId,
                  "drive_retry_started", progresoComando);
    return;
  }

  for (int lado = 0; lado < 2; ++lado) {
    if (WatchdogSeguridad.tiempoStallLadoMs(lado) <
        AUTONOMOUS_SIDE_STALL_TIMEOUT_MS) continue;
    agotarOProgramarReintentoAvance(
        lado == 0 ? "drive_left_side_stalled_retry"
                  : "drive_right_side_stalled_retry");
    return;
  }

  const SensorSnapshot s = sensores();
  int64_t delta[4]; deltasDesde(s, ticksBase, delta);
  const float errorRumbo = normalizar180(rumboTramoDeg - PoseGlobal.getThetaDeg());
  const float gyroDegS = s.gyro_z_filtrado_rad_s * 180.0f / PI;
  const float rumboPredicho = PoseGlobal.getThetaDeg() +
      gyroDegS * (TURN_PREDICTION_LOOKAHEAD_MS / 1000.0f);
  const float errorRumboPredicho = normalizar180(rumboTramoDeg - rumboPredicho);
  errorRumboPredichoActualDeg = errorRumboPredicho;
  const float ticks = clasificarYEstimar(delta, errorRumbo);
  if (ticks < 0) {
    agotarOProgramarReintentoAvance("encoder_side_unavailable_retry");
    return;
  }
  // Integrar solamente la muestra nueva con la mascara vigente. `ticks` sigue
  // siendo acumulativo para auditar salud y telemetria, pero nunca se vuelve a
  // ponderar el historial cuando un encoder pasa a EXCLUIDO.
  const int64_t actuales[4] = {s.pulsosFL, s.pulsosFR, s.pulsosBL, s.pulsosBR};
  int64_t incremento[4];
  for (int i = 0; i < 4; ++i) {
    incremento[i] = actuales[i] - ticksUltimaIntegracion[i];
    ticksUltimaIntegracion[i] = actuales[i];
  }
  const float incrementoIzquierda = promedioLado(incremento, true);
  const float incrementoDerecha = promedioLado(incremento, false);
  const float incrementoCentro = 0.5f * (incrementoIzquierda + incrementoDerecha);
  distanciaIntegradaCm += fabsf(incrementoCentro) *
                          (PI * WHEEL_DIAMETER_ODOMETRY_CM) / ENCODER_PPR;
  const float distancia = distanciaIntegradaCm;
  const float restante = distanciaObjetivoCM - distancia;
  progresoComando = constrain(distancia / max(distanciaObjetivoCM, 0.1f), 0.0f, 1.0f);
  const uint32_t timeout = 8000 + static_cast<uint32_t>(distanciaObjetivoCM * 250.0f);
  if (millis() - tiempoInicioAvance > timeout) {
    agotarOProgramarReintentoAvance("drive_timeout_retry");
    return;
  }
  const float toleranciaLlegada = toleranciaDistanciaParaObjetivo(distanciaObjetivoCM);
  if (restante <= toleranciaLlegada) {
    if (fase == FaseMovimiento::MOVE_AVANCE) {
      frenarMotores();
      // El rumbo del tramo sólo se conserva mientras avanza. Al alcanzar el
      // waypoint, cada paso vuelve al cero global antes de emitir completed;
      // el siguiente waypoint no puede arrancar con el robot invertido.
      rumboCardinalObjetivoDeg = RUMBO_REPOSO_RUTA_DEG;
      const float errorFinal = errorCardinal(
          RUMBO_REPOSO_RUTA_DEG, PoseGlobal.getThetaDeg());
      if (fabsf(errorFinal) <= TOLERANCIA_GIRO_DEG) {
        rutaAlineadaPrecisa = true;
        terminar(EVT_COMPLETED, "move_completed_zero_aligned");
      } else {
        rutaAlineadaPrecisa = false;
        iniciarBaseGiro(errorFinal, FaseMovimiento::MOVE_ALINEACION_FINAL);
      }
    } else terminar(EVT_COMPLETED, "drive_completed");
    return;
  }

  if (ticks >= 2.0f) inicioSinProgresoMs = millis();
  else if (millis() - inicioSinProgresoMs > 2500) {
    agotarOProgramarReintentoAvance("drive_no_progress_retry");
    return;
  }

  if (fabsf(errorRumbo) > HEADING_RECOVERY_TRIGGER_DEG) {
    if (!inicioErrorRumboMs) inicioErrorRumboMs = millis();
    if (millis() - inicioErrorRumboMs >= HEADING_RECOVERY_HOLD_MS) {
      if (intentosRecuperacionRumbo >= HEADING_RECOVERY_MAX_ATTEMPTS) {
        terminar(EVT_FAULT, "heading_recovery_exhausted");
        return;
      }
      ++intentosRecuperacionRumbo;
      faseDespuesRecuperacion = fase;
      if (fase == FaseMovimiento::MOVE_AVANCE) rutaAlineadaPrecisa = false;
      distanciaRestanteRecuperacionMm = (distanciaFirmadaCm < 0 ? -1.0f : 1.0f) * restante * 10.0f;
      const float correccionSegmentada = limitarCorreccionRecuperacion(errorRumbo);
      iniciarBaseGiro(correccionSegmentada, FaseMovimiento::RECOVERY_TURN);
      estadoActual = RECOVERING;
      encolarEvento(EVT_PROGRESS, comandoActivoId,
                    "heading_recovery_segment_started", progresoComando);
      return;
    }
  } else inicioErrorRumboMs = 0;
  const float controlRumbo = constrain(
      errorRumboPredicho * KP_RUMBO_PWM_POR_GRADO -
          s.gyro_z_filtrado_rad_s * KD_RUMBO_PWM_POR_RAD_S,
      -static_cast<float>(PWM_CORRECCION_RUMBO_MAX), static_cast<float>(PWM_CORRECCION_RUMBO_MAX));
  // La corrección dinámica usa las ventanas filtradas, como el ensayo. Los
  // ticks acumulados quedan reservados para distancia y clasificación.
  const float diferenciaTicks = promedioFiltradoLado(s, true) - promedioFiltradoLado(s, false);
  const float controlEncoder = fabsf(errorRumbo) <= 2.0f &&
      millis() - tiempoInicioAvance >= ENCODER_CONTROL_GRACE_MS
      ? constrain(diferenciaTicks * KP_ENCODER_PWM_POR_TICK,
                  -static_cast<float>(PWM_CORRECCION_ENCODER_MAX), static_cast<float>(PWM_CORRECCION_ENCODER_MAX))
      : 0.0f;
  // El torque de pivot no es el mínimo de avance. Mezclarlos fijaba ambos
  // lados en 230/255 e impedía que la corrección de rumbo redujera un lado.
  const int minimoCalibrado = VELOCIDAD_MINIMA_RECTO;
  int base = restante < 15.0f
      ? map(static_cast<long>(restante * 10), 20, 150, minimoCalibrado, VELOCIDAD_APROXIMACION)
      : VELOCIDAD_BASE_RECTO;
  const uint32_t transcurrido = millis() - tiempoInicioAvance;
  const int techoArranque = transcurrido >= DRIVE_ACCELERATION_MS ? VELOCIDAD_BASE_RECTO
      : minimoCalibrado + static_cast<int>(
          (VELOCIDAD_BASE_RECTO - minimoCalibrado) * transcurrido / DRIVE_ACCELERATION_MS);
  base = constrain(min(base, techoArranque), minimoCalibrado, VELOCIDAD_BASE_RECTO);
  const int signo = distanciaFirmadaCm < 0 ? -1 : 1;
  int reduccionIzquierda = 0;
  int reduccionDerecha = 0;
  if (controlRumbo != 0.0f) {
    const int candidato = controlRumbo > 0.0f ? candidatoGiroPositivo : candidatoGiroNegativo;
    if (ladoReducidoParaCandidato(candidato) == 0) {
      reduccionIzquierda += aproximarEntero(fabsf(controlRumbo));
    } else {
      reduccionDerecha += aproximarEntero(fabsf(controlRumbo));
    }
  }
  if (controlEncoder > 0.0f) reduccionIzquierda += aproximarEntero(controlEncoder);
  else reduccionDerecha += aproximarEntero(-controlEncoder);
  correccionRumboPwmActual = controlRumbo;
  correccionEncoderPwmActual = controlEncoder;
  int izquierda = constrain(base - reduccionIzquierda, minimoCalibrado, PWM_SAFE_HARD_LIMIT);
  int derecha = constrain(base - reduccionDerecha, minimoCalibrado, PWM_SAFE_HARD_LIMIT);
  aplicarVelocidades(signo * izquierda, signo * derecha);
}

#include "Estado.h"
#include <stddef.h>
#if __has_include("Secrets.h")
#include "Secrets.h"
#else
#error "Falta include/Secrets.h; configure credenciales locales antes de compilar el firmware."
#endif

// Limites de Arduino-ESP32 para WiFi.softAP(): SSID de 1..32 bytes y
// contrasena WPA de 8..63 bytes. Se valida el tamano del arreglo literal
// (incluye el terminador NUL) sin imprimir ni copiar los valores.
constexpr bool contenidoSinNul(const char* valor, size_t longitud) {
  return longitud == 0 ||
         (valor[longitud - 1] != '\0' && contenidoSinNul(valor, longitud - 1));
}

template <size_t N>
constexpr bool cadenaConfiguracionValida(const char (&valor)[N]) {
  return N > 0 && valor[N - 1] == '\0' && contenidoSinNul(valor, N - 1);
}

static_assert(sizeof(WIFI_AP_SSID) >= 2 && sizeof(WIFI_AP_SSID) <= 33,
              "WIFI_AP_SSID debe tener entre 1 y 32 bytes para ESP32 SoftAP");
static_assert(sizeof(WIFI_AP_PASSWORD) >= 9 && sizeof(WIFI_AP_PASSWORD) <= 64,
              "WIFI_AP_PASSWORD debe tener entre 8 y 63 bytes para ESP32 SoftAP");
static_assert(cadenaConfiguracionValida(WIFI_AP_SSID),
              "WIFI_AP_SSID no puede contener bytes NUL embebidos");
static_assert(cadenaConfiguracionValida(WIFI_AP_PASSWORD),
              "WIFI_AP_PASSWORD no puede contener bytes NUL embebidos");

const char* ssid_AP = WIFI_AP_SSID;
const char* password_AP = WIFI_AP_PASSWORD;

char sessionId[17] = "";
int ultimoSeqCompletado = 0;
int seqActivo = 0;
char faseComando[20] = "";

EstadoRobot estadoActual = DESARMADO;
bool robotCalibrado = false;
float progresoComando = 0.0f;
char ultimoFalloDetalle[40] = "";

float anguloZ = 0.0f;
float heading360 = 0.0f;

bool modoDegradado = false;
bool encoderConfiableGlobal[4] = {true, true, true, true};
EstadoSaludEncoder estadoSaludEncoderGlobal[4] = {
    EstadoSaludEncoder::HEALTHY, EstadoSaludEncoder::HEALTHY,
    EstadoSaludEncoder::HEALTHY, EstadoSaludEncoder::HEALTHY
};
uint32_t encoderSinPulsosMs[4] = {};
float encoderDesviacionPct[4] = {};
uint8_t encoderMuestrasReingreso[4] = {};
float encoderFusionDeltaL = 0.0f;
float encoderFusionDeltaR = 0.0f;
uint32_t encoderSinProgresoLadoMs[2] = {};
bool antiFriccionActiva = false;
bool antiFriccionPulsoEncendido = false;
uint8_t antiFriccionPulsoIndice = 0;
int antiFriccionPwmObjetivo = 0;
bool antiFriccionMovimientoConfirmado = false;

float factorCompensacionDer = 1.0f;

float pasoHeadingObjetivo = 0.0f;
float pasoDistanciaObjetivoCm = 0.0f;
float pasoDistanciaActualCm = 0.0f;
bool pasoObjetivoAbsoluto = false;
float pasoTargetXObjetivoCm = NAN;
float pasoTargetYObjetivoCm = NAN;
float pasoErrorLongitudinalCm = 0.0f;
float pasoErrorLateralCm = 0.0f;
float pasoErrorEuclidianoCm = 0.0f;
float pasoRumboDinamicoDeg = 0.0f;
float pasoErrorRumboDeg = 0.0f;
float pasoControlRumboPwm = 0.0f;
float pasoControlRumboP = 0.0f;
float pasoControlRumboI = 0.0f;
float pasoControlRumboD = 0.0f;
float pasoControlEncoderPwm = 0.0f;
float pasoControlLateralDeg = 0.0f;
float pasoIntegralRumboGradoS = 0.0f;
uint32_t pasoRampaReversaMs = 0;
char pasoLadoFrenoRumbo[8] = "none";
bool pasoEnReversa = false;
uint32_t pasoEjecucionId = 0;
uint8_t pasoIntentosEndpoint = 0;
char pasoMotivoFinalizacion[40] = "";
char pasoDecisionRecuperacion[24] = "none";
float pasoDistanciaRecuperacionCm = 0.0f;
bool pasoRecuperacionUsaReversa = false;
char pasoModoSolicitado[10] = "forward";
char pasoModoEfectivo[10] = "forward";
float pasoRumboTrayectoDeg = 0.0f;
float pasoRumboCuerpoDeg = 0.0f;
float pasoRumboFinalDeg = 0.0f;
float pasoDistanciaRestanteCm = 0.0f;
float pasoFrenoPrevistoCm = 0.0f;
float pasoArrastreFrenoCm = 0.0f;
uint32_t pasoAsentamientoMs = 0;

#include "Estado.h"
#if __has_include("Secrets.h")
#include "Secrets.h"
#else
#include "Secrets.example.h"
#endif

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

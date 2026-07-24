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

float factorCompensacionDer = 0.95f;

float pasoHeadingObjetivo = 0.0f;
float pasoDistanciaObjetivoCm = 0.0f;
float pasoDistanciaActualCm = 0.0f;

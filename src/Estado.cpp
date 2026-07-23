#include "Estado.h"
#if __has_include("Secrets.h")
#include "Secrets.h"
#else
#include "Secrets.example.h"
#endif

const char* ssid_AP = WIFI_AP_SSID;
const char* password_AP = WIFI_AP_PASSWORD;

long pulsosIzquierdos = 0;
long pulsosDerechos = 0;

float anguloZ = 0.0;

float nuevoDestinoX = 0;
float nuevoDestinoY = 0;

int pwm_target_l = 0;
int pwm_target_r = 0;
unsigned long tiempoTestingInicio = 0;
unsigned long duracionTesting = 0;

EstadoRobot estadoActual = UNCALIBRATED;
unsigned long tiempoEspera = 0;
unsigned long tiempoAnterior = 0;
unsigned long inicioGiro = 0;

bool robotCalibrado = false;
bool modoDegradado = false;
bool encoderConfiableGlobal[4] = {true, true, true, true};
float progresoComando = 0.0f;
char comandoActivoId[33] = {};
char comandoActivoNombre[16] = {};
char ultimoTerminalId[33] = {};
char ultimoTerminalTipo[12] = {};
char ultimoTerminalDetalle[32] = {};
EstadoAutoevaluacion estadoAutoevaluacion = SELF_TEST_NOT_RUN;
char detalleAutoevaluacion[48] = "not_run";

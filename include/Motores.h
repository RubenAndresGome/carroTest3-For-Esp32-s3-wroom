#pragma once
#include <Arduino.h>

extern int pwm_aplicado_L;
extern int pwm_aplicado_R;
extern int pwm_solicitado_L;
extern int pwm_solicitado_R;

// La salida PWM sólo queda habilitada después de setup_Motores(). El estado
// permite distinguir un mapa inválido de un controlador aún no inicializado.
bool validarMapaMotores();
bool motoresListos();
const char* estadoMotores();

void setup_MotorPinsLow();
bool validarInterlockMotores();
bool setup_Motores();
bool aplicarVelocidades(int velIzq, int velDer);
// Limite continuo vigente: 242/255 en avance, 247/255 en giros y calibracion.
// Por encima de el solo se autoriza una rafaga acotada de PWM_BURST_MAX_MS.
void establecerLimiteContinuoPwm(int limite);
int limiteContinuoPwm();
void frenarMotores();
void frenarMotoresActivo();
void actualizarFrenoActivo();
bool frenoActivoEnCurso();
const char *estadoInterlockL();
const char *estadoInterlockR();
int signoEnergizadoL();
int signoEnergizadoR();
int signoPendienteL();
int signoPendienteR();

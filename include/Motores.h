#pragma once
#include <Arduino.h>

extern int pwm_aplicado_L;
extern int pwm_aplicado_R;
extern int pwm_solicitado_L;
extern int pwm_solicitado_R;

void setup_MotorPinsLow();
bool validarInterlockMotores();
void setup_Motores();
void aplicarVelocidades(int velIzq, int velDer);
void frenarMotores();
const char *estadoInterlockL();
const char *estadoInterlockR();
int signoEnergizadoL();
int signoEnergizadoR();
int signoPendienteL();
int signoPendienteR();

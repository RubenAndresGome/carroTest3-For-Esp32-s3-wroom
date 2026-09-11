#pragma once
#include <Arduino.h>

extern int pwm_aplicado_L;
extern int pwm_aplicado_R;
extern int pwm_solicitado_L;
extern int pwm_solicitado_R;

enum class MotorId : uint8_t { FL = 0, BL = 1, FR = 2, BR_WHEEL = 3 };

struct DiagnosticoSalidaMotor {
  int pwmLogico = 0;
  int gpioActivo = -1;
  int duty = 0;
  int dutyAvance = 0;
  int dutyReversa = 0;
};

// La salida PWM sólo queda habilitada después de setup_Motores(). El estado
// permite distinguir un mapa inválido de un controlador aún no inicializado.
bool validarMapaMotores();
bool motoresListos();
const char* estadoMotores();

void setup_MotorPinsLow();
bool validarInterlockMotores();
bool setup_Motores();
bool aplicarVelocidades(int velIzq, int velDer);
void frenarMotores();
const char *estadoInterlockL();
const char *estadoInterlockR();
int signoEnergizadoL();
int signoEnergizadoR();
int signoPendienteL();
int signoPendienteR();
DiagnosticoSalidaMotor diagnosticoSalidaMotor(MotorId motor);

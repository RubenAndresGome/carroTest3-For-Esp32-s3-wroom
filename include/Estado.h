#pragma once
#include <Arduino.h>

enum EstadoRobot {
  UNCALIBRATED,
  CALIBRATING,
  IDLE,
  GIRANDO,
  ESPERANDO_ESTABILIZACION,
  AVANZANDO,
  MANUAL,
  TESTING,
  ESTOP_LATCHED,
  SAFE_STOP_COMMS,
  FAULT_SENSOR,
  RECOVERING
};

// Odometría Cruda
extern long pulsosIzquierdos;
extern long pulsosDerechos;

// Giroscopio
extern float anguloZ;

// Objetivos Autónomos
extern float nuevoDestinoX;
extern float nuevoDestinoY;

// Testing y Manual
extern int pwm_target_l;
extern int pwm_target_r;
extern unsigned long tiempoTestingInicio;
extern unsigned long duracionTesting;

// Máquina de estados
extern EstadoRobot estadoActual;
extern unsigned long tiempoEspera;
extern unsigned long tiempoAnterior;
extern unsigned long inicioGiro;

extern bool robotCalibrado;
extern bool modoDegradado;
extern bool encoderConfiableGlobal[4];
extern float progresoComando;
extern char comandoActivoId[33];
extern char comandoActivoNombre[16];
extern char ultimoTerminalId[33];
extern char ultimoTerminalTipo[12];
extern char ultimoTerminalDetalle[32];

enum EstadoAutoevaluacion : uint8_t { SELF_TEST_NOT_RUN, SELF_TEST_RUNNING, SELF_TEST_PASSED, SELF_TEST_FAILED };
extern EstadoAutoevaluacion estadoAutoevaluacion;
extern char detalleAutoevaluacion[48];

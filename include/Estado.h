#pragma once
#include <Arduino.h>

// Máquina de estados simplificada del robot de memoria corta.
// El robot ejecuta UN paso a la vez y reporta el resultado con su seq.
enum EstadoRobot : uint8_t {
  DESARMADO,    // recién iniciado o sin sesión válida; requiere hello + calibrate
  CALIBRANDO,   // búsqueda de torque + retorno a yaw inicial
  LISTO,        // calibrado y esperando un paso
  EJECUTANDO,   // ejecutando un paso (giro inicial, avance, giro final)
  ESTOP,        // parada de emergencia enclavada; requiere clear_fault
  FALLO         // fallo de sensor/motor; requiere clear_fault
};

// Sesión gestionada por el backend. El robot solo conserva el identificador
// y el último seq completado para soportar reconexiones.
extern char sessionId[17];
extern int ultimoSeqCompletado;
extern int seqActivo;
extern char faseComando[20];  // "cal", "giro_ini", "avance", "recup", "giro_fin"

extern EstadoRobot estadoActual;
extern bool robotCalibrado;
extern float progresoComando;
extern char ultimoFalloDetalle[40];

// IMU: anguloZ es el yaw continuo en grados (integrado en Sensores.cpp).
// heading360 es el yaw normalizado a [0, 360) para la lógica de pasos.
extern float anguloZ;
extern float heading360;

// Encoders
extern bool modoDegradado;
extern bool encoderConfiableGlobal[4];

// Compensación de asimetría mecánica del lado derecho (del test aprobado).
extern float factorCompensacionDer;

// Objetivos del paso en curso (para telemetría)
extern float pasoHeadingObjetivo;
extern float pasoDistanciaObjetivoCm;
extern float pasoDistanciaActualCm;

#pragma once
#include <Arduino.h>

// Protocolo de comandos del robot de memoria corta.
// Cada comando lleva un seq entero asignado por el backend; el robot ejecuta
// un solo comando de movimiento a la vez y reporta accepted/completed/fault
// con ese mismo seq para que el backend marque el paso en SQLite.
enum TipoComando : uint8_t {
    CMD_NINGUNO,
    CMD_CALIBRATE,    // calibración de torque + retorno a yaw inicial
    CMD_STEP,         // paso atómico: girar a heading, avanzar cm, volver a 0°
    CMD_STOP,         // cancela el paso actual y queda LISTO
    CMD_ESTOP,        // parada de emergencia enclavada
    CMD_CLEAR_FAULT,  // rearma desde ESTOP o FALLO
    CMD_SET_COMP,     // factor de compensación del lado derecho (0.80..1.00)
    CMD_RESET_POSE    // pone x, y y yaw en cero
};

struct ComandoRed {
    TipoComando tipo;
    int seq;
    float heading;      // STEP: rumbo absoluto objetivo en grados [0, 360)
    float distanciaCm;  // STEP: distancia a avanzar en cm (siempre positiva)
    float factor;       // SET_COMP: factor de compensación
};

extern QueueHandle_t colaComandos;
extern volatile bool flag_ESTOP_ISR;
extern volatile int seq_ESTOP_pendiente;

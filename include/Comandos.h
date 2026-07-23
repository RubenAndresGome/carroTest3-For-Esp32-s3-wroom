#pragma once
#include <Arduino.h>

enum TipoComando {
    CMD_INVALID,
    CMD_CALIBRATE,
    CMD_ESTOP,
    CMD_STOP,
    CMD_CLEAR_ROUTE,
    CMD_RESET_POSE,
    CMD_CLEAR_FAULT,
    CMD_MOVE_ABS,
    CMD_DRIVE,
    CMD_TURN,
    CMD_MANUAL,
    CMD_TEST_PWM,
    CMD_SELF_TEST,
    CMD_MISSION_START
};

enum class ModoGiroSolicitado : uint8_t {
    AUTO = 0,
    PIVOT,
    ARC_LEFT_ACTIVE,
    ARC_RIGHT_ACTIVE
};

struct ComandoRed {
    // UUID hexadecimal generado por Python (32 caracteres + terminador).
    // Las colas FreeRTOS copian la estructura completa, por lo que el ID se
    // conserva hasta accepted/completed/rejected.
    char id[33];
    TipoComando tipo;
    float arg_x;
    float arg_y;
    float distancia_mm;
    float angulo_deg;
    ModoGiroSolicitado modo_giro;
    int pwm_l;
    int pwm_r;
    uint32_t tiempo_ms;
    char mission_id[33];
    uint32_t mission_revision;
};

extern QueueHandle_t colaComandos;
extern QueueHandle_t buzonManual;
extern volatile bool flag_ESTOP_ISR;

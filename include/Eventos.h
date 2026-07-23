#pragma once
#include <Arduino.h>

enum TipoEvento {
    EVT_COMPLETED,
    EVT_REJECTED,
    EVT_FAULT,
    EVT_PROGRESS,
    EVT_TELEMETRY
};

struct EventoRed {
    TipoEvento tipo;
    char cmd_id[33];
    char detalle[32];
    float progreso;
};

extern QueueHandle_t colaEventosRed;

void encolarEvento(TipoEvento tipo, const char* cmd_id, const char* detalle, float progreso = 0.0f);

#pragma once
#include <Arduino.h>

enum TipoEvento : uint8_t {
    EVT_ACCEPTED,
    EVT_REJECTED,
    EVT_COMPLETED,
    EVT_FAULT,
    EVT_PROGRESS
};

struct EventoRed {
    TipoEvento tipo;
    int seq;
    char detalle[40];
    float progreso;
};

extern QueueHandle_t colaEventosRed;

void encolarEvento(TipoEvento tipo, int seq, const char* detalle, float progreso = 0.0f);

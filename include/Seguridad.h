#pragma once
#include <Arduino.h>
#include "Estado.h"
#include "Sensores.h"

class Seguridad {
public:
    Seguridad();
    bool auditarSalud(const SensorSnapshot &snap, int pwm_L, int pwm_R);
    void forzarEStop();
    void resetFallo();

private:
    unsigned long inicio_movimiento_ms;
    int64_t pulsos_movimiento_iniciales[4];
};

extern Seguridad WatchdogSeguridad;

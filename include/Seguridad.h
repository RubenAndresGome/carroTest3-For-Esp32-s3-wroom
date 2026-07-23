#pragma once
#include <Arduino.h>
#include "Estado.h"
#include "Sensores.h"

class Seguridad {
public:
    Seguridad();

    void alimentarWatchdogRed();
    void registrarHeartbeat();

    bool auditarSalud(const SensorSnapshot &snap, int pwm_L, int pwm_R);
    static bool validarLogicaWatchdog();
    uint32_t tiempoStallMs(size_t indice) const;
    uint32_t tiempoStallLadoMs(size_t lado) const;

    void forzarEStop();
    void resetFallo();

private:
    unsigned long ultimo_comando_ms;
    unsigned long ultimo_heartbeat_ms;
    int64_t pulsos_movimiento_iniciales[4];
    unsigned long ultima_auditoria_ms;
    unsigned long tiempo_energizado_sin_pulsos_ms[4];
    int64_t pulsos_lado_iniciales[2];
    unsigned long tiempo_energizado_sin_pulsos_lado_ms[2];
    EstadoRobot estado_supervisado;

    // Parada segura si el controlador PC deja de enviar JSON/heartbeat.
    const unsigned long TIMEOUT_WIFI_MANUAL_MS = 400;
};

extern Seguridad WatchdogSeguridad;

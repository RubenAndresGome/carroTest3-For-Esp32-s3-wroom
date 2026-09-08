#pragma once
#include <Arduino.h>
#include "Estado.h"
#include "Sensores.h"

enum class ResultadoRearme : uint8_t {
    REARMADO,
    SIN_FALLO_ACTIVO,
    MOTORES_NO_DISPONIBLES,
    PCNT_NO_DISPONIBLE
};

class Seguridad {
public:
    Seguridad();
    void actualizarSaludEncoders(const SensorSnapshot &snap, int pwm_L, int pwm_R);
    bool auditarSalud(const SensorSnapshot &snap, int pwm_L, int pwm_R);
    void forzarEStop();
    ResultadoRearme resetFallo();
    void reiniciarSaludEncoders();
    void prepararRevalidacionEncoders();
    void aplicarClasificacionEncoders(const bool confiables[4]);

private:
    unsigned long inicio_movimiento_ms;
    int64_t pulsos_movimiento_iniciales[4];
    uint32_t inicio_ventana_encoder_ms;
    int64_t pulsos_ventana_encoder[4];
    int64_t pulsos_lado_anteriores[2];
    uint32_t ultimo_progreso_lado_ms[2];
};

extern Seguridad WatchdogSeguridad;

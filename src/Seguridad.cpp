#include "Seguridad.h"
#include "Debug.h"
#include "Motores.h"
#include "Config.h"
#include "Eventos.h"

Seguridad WatchdogSeguridad;

Seguridad::Seguridad() {
    inicio_movimiento_ms = 0;
    for (int i = 0; i < 4; ++i) pulsos_movimiento_iniciales[i] = 0;
}

bool Seguridad::auditarSalud(const SensorSnapshot &snap, int pwm_L, int pwm_R) {
    if (estadoActual == ESTOP || estadoActual == FALLO) return true;
    if (estadoActual != EJECUTANDO) {
        inicio_movimiento_ms = 0;
        return false;
    }
    int pwm_max = abs(pwm_L);
    if (abs(pwm_R) > pwm_max) pwm_max = abs(pwm_R);

    if (pwm_max <= static_cast<int>(40 * PWM_SCALE_8_TO_10)) {
        inicio_movimiento_ms = 0;
        return false;
    }

    unsigned long ahora = millis();
    if (inicio_movimiento_ms == 0) {
        inicio_movimiento_ms = ahora;
        pulsos_movimiento_iniciales[0] = snap.pulsosFL;
        pulsos_movimiento_iniciales[1] = snap.pulsosFR;
        pulsos_movimiento_iniciales[2] = snap.pulsosBL;
        pulsos_movimiento_iniciales[3] = snap.pulsosBR;
        return false;
    }
    if (ahora - inicio_movimiento_ms <= TIMEOUT_STALL_MS) return false;

    const int64_t actuales[4] = {snap.pulsosFL, snap.pulsosFR, snap.pulsosBL, snap.pulsosBR};
    const bool exige_izq = abs(pwm_L) > static_cast<int>(40 * PWM_SCALE_8_TO_10);
    const bool exige_der = abs(pwm_R) > static_cast<int>(40 * PWM_SCALE_8_TO_10);
    const bool sf0 = exige_izq && llabs(actuales[0] - pulsos_movimiento_iniciales[0]) < 1;
    const bool sf2 = exige_izq && llabs(actuales[2] - pulsos_movimiento_iniciales[2]) < 1;
    const bool sf1 = exige_der && llabs(actuales[1] - pulsos_movimiento_iniciales[1]) < 1;
    const bool sf3 = exige_der && llabs(actuales[3] - pulsos_movimiento_iniciales[3]) < 1;
    if (sf0 || sf1 || sf2 || sf3) {
        LOG_CORE("STALL: encoder sin pulsos bajo PWM.");
        frenarMotores();
        estadoActual = FALLO;
        const char* det = "stall";
        if (sf0) det = "stall_FL"; else if (sf2) det = "stall_BL";
        else if (sf1) det = "stall_FR"; else if (sf3) det = "stall_BR";
        encolarEvento(EVT_FAULT, seqActivo, det);
        return true;
    }
    inicio_movimiento_ms = ahora;
    pulsos_movimiento_iniciales[0] = actuales[0];
    pulsos_movimiento_iniciales[1] = actuales[1];
    pulsos_movimiento_iniciales[2] = actuales[2];
    pulsos_movimiento_iniciales[3] = actuales[3];
    return false;
}

void Seguridad::forzarEStop() {
    LOG_CORE("E-STOP enclavado.");
    frenarMotores();
    estadoActual = ESTOP;
}

void Seguridad::resetFallo() {
    frenarMotores();
    if (estadoActual != ESTOP && estadoActual != FALLO) {
        LOG_CORE("Rearme ignorado: sin fallo activo.");
        return;
    }
    LOG_CORE("Sistema rearmado.");
    estadoActual = robotCalibrado ? LISTO : DESARMADO;
    inicio_movimiento_ms = 0;
}

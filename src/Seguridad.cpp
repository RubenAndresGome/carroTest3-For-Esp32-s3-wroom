#include "Seguridad.h"
#include "Debug.h"
#include "Motores.h"
#include "Config.h"
#include "Eventos.h"
#include "Cinematica.h"
#include "ControlSeguridad.h"

Seguridad WatchdogSeguridad;

Seguridad::Seguridad() {
    inicio_movimiento_ms = 0;
    inicio_ventana_encoder_ms = 0;
    for (int i = 0; i < 4; ++i) {
        pulsos_movimiento_iniciales[i] = 0;
        pulsos_ventana_encoder[i] = 0;
    }
    pulsos_lado_anteriores[0] = pulsos_lado_anteriores[1] = 0;
    ultimo_progreso_lado_ms[0] = ultimo_progreso_lado_ms[1] = 0;
}

void Seguridad::reiniciarSaludEncoders() {
    modoDegradado = false;
    inicio_ventana_encoder_ms = 0;
    for (int i = 0; i < 4; ++i) {
        encoderConfiableGlobal[i] = true;
        estadoSaludEncoderGlobal[i] = EstadoSaludEncoder::HEALTHY;
        encoderSinPulsosMs[i] = 0;
        encoderDesviacionPct[i] = 0.0f;
        encoderMuestrasReingreso[i] = 0;
    }
    encoderFusionDeltaL = encoderFusionDeltaR = 0.0f;
    encoderSinProgresoLadoMs[0] = encoderSinProgresoLadoMs[1] = 0;
    ultimo_progreso_lado_ms[0] = ultimo_progreso_lado_ms[1] = 0;
}

void Seguridad::prepararRevalidacionEncoders() {
    inicio_ventana_encoder_ms = 0;
    encoderFusionDeltaL = encoderFusionDeltaR = 0.0f;
    encoderSinProgresoLadoMs[0] = encoderSinProgresoLadoMs[1] = 0;
    ultimo_progreso_lado_ms[0] = ultimo_progreso_lado_ms[1] = 0;
    for (int i = 0; i < 4; ++i) {
        encoderSinPulsosMs[i] = 0;
        encoderDesviacionPct[i] = 0.0f;
        encoderMuestrasReingreso[i] = 0;
        estadoSaludEncoderGlobal[i] = encoderConfiableGlobal[i]
            ? EstadoSaludEncoder::HEALTHY : EstadoSaludEncoder::RECOVERING;
    }
    modoDegradado = false;
    for (bool confiable : encoderConfiableGlobal) modoDegradado |= !confiable;
}

void Seguridad::aplicarClasificacionEncoders(const bool confiables[4]) {
    inicio_ventana_encoder_ms = 0;
    modoDegradado = false;
    for (int i = 0; i < 4; ++i) {
        encoderConfiableGlobal[i] = confiables[i];
        estadoSaludEncoderGlobal[i] = confiables[i]
            ? EstadoSaludEncoder::HEALTHY : EstadoSaludEncoder::EXCLUDED;
        encoderSinPulsosMs[i] = 0;
        encoderDesviacionPct[i] = 0.0f;
        encoderMuestrasReingreso[i] = 0;
        modoDegradado |= !confiables[i];
    }
}

void Seguridad::actualizarSaludEncoders(const SensorSnapshot &snap, int pwm_L, int pwm_R) {
    const uint32_t ahora = millis();
    const int64_t actuales[4] = {snap.pulsosFL, snap.pulsosFR, snap.pulsosBL, snap.pulsosBR};
    // La calibración conserva evidencia por fase sin mutar la máscara global.
    // La clasificación persistente se reanuda en movimiento normal/manual.
    const bool auditando = (estadoActual == EJECUTANDO) || (estadoActual == MANUAL);
    const bool ladoExigido[2] = {
        auditando && abs(pwm_L) >= ENCODER_HEALTH_PWM_MIN,
        auditando && abs(pwm_R) >= ENCODER_HEALTH_PWM_MIN
    };

    // Progreso por lado con la mascara persistente. Un encoder excluido no
    // puede mantener fresco el watchdog ni sesgar la odometria.
    const int64_t ladoActual[2] = {
        (encoderConfiableGlobal[0] ? actuales[0] : 0) +
            (encoderConfiableGlobal[2] ? actuales[2] : 0),
        (encoderConfiableGlobal[1] ? actuales[1] : 0) +
            (encoderConfiableGlobal[3] ? actuales[3] : 0)
    };
    for (int lado = 0; lado < 2; ++lado) {
        if (!ladoExigido[lado]) {
            pulsos_lado_anteriores[lado] = ladoActual[lado];
            ultimo_progreso_lado_ms[lado] = ahora;
            encoderSinProgresoLadoMs[lado] = 0;
        } else if (ladoActual[lado] != pulsos_lado_anteriores[lado]) {
            pulsos_lado_anteriores[lado] = ladoActual[lado];
            ultimo_progreso_lado_ms[lado] = ahora;
            encoderSinProgresoLadoMs[lado] = 0;
        } else {
            if (ultimo_progreso_lado_ms[lado] == 0) ultimo_progreso_lado_ms[lado] = ahora;
            encoderSinProgresoLadoMs[lado] = ahora - ultimo_progreso_lado_ms[lado];
        }
    }

    if (!auditando) {
        inicio_ventana_encoder_ms = ahora;
        for (int i = 0; i < 4; ++i) pulsos_ventana_encoder[i] = actuales[i];
        return;
    }
    if (inicio_ventana_encoder_ms == 0) {
        inicio_ventana_encoder_ms = ahora;
        for (int i = 0; i < 4; ++i) pulsos_ventana_encoder[i] = actuales[i];
        return;
    }
    const uint32_t lapso = ahora - inicio_ventana_encoder_ms;
    if (lapso < ENCODER_HEALTH_WINDOW_MS) return;

    int64_t delta[4] = {};
    for (int i = 0; i < 4; ++i) {
        delta[i] = llabs(actuales[i] - pulsos_ventana_encoder[i]);
        pulsos_ventana_encoder[i] = actuales[i];
    }
    inicio_ventana_encoder_ms = ahora;
    const float referencia = ControlSeguridad::medianaCuatro(delta);

    for (int i = 0; i < 4; ++i) {
        const bool pwmExigido = ladoExigido[(i == 0 || i == 2) ? 0 : 1];
        const int pareja = i < 2 ? i + 2 : i - 2;
        const float desviacion = fabsf(static_cast<float>(delta[i]) - referencia) /
            fmaxf(1.0f, fabsf(referencia));
        const float desacuerdoPareja = fabsf(static_cast<float>(delta[i] - delta[pareja])) /
            fmaxf(1.0f, fabsf(static_cast<float>(delta[pareja])));
        encoderDesviacionPct[i] = desviacion * 100.0f;
        // Los dos motores de un mismo lado reciben el mismo PWM. Compararlos
        // entre si evita excluir un lado completo cuando el PID reduce su
        // velocidad respecto al lado contrario durante una correccion.
        // No excluir dos encoders que sí entregan pulsos sólo porque sus
        // magnitudes difieren. En la sesión ADB 137 eso marcó simultáneamente
        // FL/FR/BL/BR como EXCLUDED en 500 ms y provocó enc_no_estimation pese
        // a que tres canales avanzaban. Sólo el cero aislado es concluyente.
        const bool incoherente = ControlSeguridad::encoderSinRespuestaAislada(
            delta[i], delta[pareja], pwmExigido);

        if (encoderConfiableGlobal[i]) {
            if (incoherente) {
                encoderSinPulsosMs[i] += lapso;
                if (encoderSinPulsosMs[i] >= ENCODER_SUSPECT_MS)
                    estadoSaludEncoderGlobal[i] = EstadoSaludEncoder::SUSPECT;
                if (encoderSinPulsosMs[i] >= ENCODER_EXCLUDE_MS) {
                    encoderConfiableGlobal[i] = false;
                    estadoSaludEncoderGlobal[i] = EstadoSaludEncoder::EXCLUDED;
                    encoderMuestrasReingreso[i] = 0;
                }
            } else {
                encoderSinPulsosMs[i] = 0;
                estadoSaludEncoderGlobal[i] = EstadoSaludEncoder::HEALTHY;
            }
        } else {
            const bool coherente = pwmExigido && delta[i] > 0 && delta[pareja] > 0 &&
                desacuerdoPareja <= DESACUERDO_MAXIMO_PAR;
            if (coherente) {
                estadoSaludEncoderGlobal[i] = EstadoSaludEncoder::RECOVERING;
                if (encoderMuestrasReingreso[i] < ENCODER_REJOIN_WINDOWS)
                    ++encoderMuestrasReingreso[i];
                if (encoderMuestrasReingreso[i] >= ENCODER_REJOIN_WINDOWS) {
                    encoderConfiableGlobal[i] = true;
                    estadoSaludEncoderGlobal[i] = EstadoSaludEncoder::HEALTHY;
                    encoderSinPulsosMs[i] = 0;
                    encoderMuestrasReingreso[i] = 0;
                }
            } else {
                estadoSaludEncoderGlobal[i] = EstadoSaludEncoder::EXCLUDED;
                encoderMuestrasReingreso[i] = 0;
            }
        }
    }

    modoDegradado = false;
    for (bool confiable : encoderConfiableGlobal) modoDegradado |= !confiable;
    encoderFusionDeltaL = ControlSeguridad::promedioConfiableLado(
        delta, encoderConfiableGlobal, true);
    encoderFusionDeltaR = ControlSeguridad::promedioConfiableLado(
        delta, encoderConfiableGlobal, false);
}

bool Seguridad::auditarSalud(const SensorSnapshot &snap, int pwm_L, int pwm_R) {
    if (estadoActual == ESTOP || estadoActual == FALLO) return true;
    if (estadoActual != EJECUTANDO && estadoActual != CALIBRANDO && estadoActual != MANUAL) {
        inicio_movimiento_ms = 0;
        return false;
    }
    int pwm_max = abs(pwm_L);
    if (abs(pwm_R) > pwm_max) pwm_max = abs(pwm_R);

    if (pwm_max <= static_cast<int>(40 * PWM_SCALE_8_TO_10)) {
        inicio_movimiento_ms = 0;
        return false;
    }

    // Cinematica aplica durante CALIBRANDO cortes específicos por MPU, PCNT,
    // PWM máximo y progreso de retorno. El watchdog genérico por lado no debe
    // contradecir esa autoridad con fallos genéricos por lado.
    if (estadoActual == CALIBRANDO || enFaseCalibracion()) {
        inicio_movimiento_ms = 0;
        return false;
    }

    // Selector dinámico de timeout por fase activa
    unsigned long timeout_ms = TIMEOUT_STALL_AVANCE_MS;
    if (estadoActual == MANUAL) {
        timeout_ms = MANUAL_STALL_TIMEOUT_MS;
    } else if (enFaseGiro()) {
        timeout_ms = TIMEOUT_STALL_GIRO_MS;       // 12 s para maniobras de giro
    } else if (enFaseAvance()) {
        timeout_ms = TIMEOUT_STALL_AVANCE_MS;     // 7 s para avance recto
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
    if (ahora - inicio_movimiento_ms <= timeout_ms) return false;

    // Si el IMU reporta rotacion o movimiento fisico aceptable, el robot se esta moviendo
    if (fabsf(snap.gyro_z_filtrado_rad_s) >= GYRO_MOVEMENT_RAD_S) {
        inicio_movimiento_ms = ahora;
        pulsos_movimiento_iniciales[0] = snap.pulsosFL;
        pulsos_movimiento_iniciales[1] = snap.pulsosFR;
        pulsos_movimiento_iniciales[2] = snap.pulsosBL;
        pulsos_movimiento_iniciales[3] = snap.pulsosBR;
        return false;
    }

    const int64_t actuales[4] = {snap.pulsosFL, snap.pulsosFR, snap.pulsosBL, snap.pulsosBR};
    const bool exige_izq = abs(pwm_L) > static_cast<int>(60 * PWM_SCALE_8_TO_10);
    const bool exige_der = abs(pwm_R) > static_cast<int>(60 * PWM_SCALE_8_TO_10);
    const bool fl_zero = llabs(actuales[0] - pulsos_movimiento_iniciales[0]) < 1;
    const bool fr_zero = llabs(actuales[1] - pulsos_movimiento_iniciales[1]) < 1;
    const bool bl_zero = llabs(actuales[2] - pulsos_movimiento_iniciales[2]) < 1;
    const bool br_zero = llabs(actuales[3] - pulsos_movimiento_iniciales[3]) < 1;

    const bool sf_left  = ControlSeguridad::ladoEnStall(exige_izq, fl_zero, bl_zero);
    const bool sf_right = ControlSeguridad::ladoEnStall(exige_der, fr_zero, br_zero);

    if (sf_left || sf_right) {
        LOG_CORE("STALL Seguridad.cpp: motor/lado completo sin pulsos bajo PWM.");
        frenarMotores();
        reiniciarControlRumbo();
        estadoActual = FALLO;
        char detMsg[64];
        const char* modoTag = enFaseCalibracion() ? "cal" : (enFaseGiro() ? "giro" : "avance");
        snprintf(detMsg, sizeof(detMsg), "stall_%s_%s[pwmL=%d,pwmR=%d]", modoTag, sf_left ? "left" : "right", pwm_L, pwm_R);
        registrarMotivoFinalizacion(detMsg);
        encolarEvento(EVT_FAULT, seqActivo, detMsg);
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
    reiniciarControlRumbo();
    registrarMotivoFinalizacion("estop");
    strncpy(ultimoFalloDetalle, "estop", sizeof(ultimoFalloDetalle) - 1);
    ultimoFalloDetalle[sizeof(ultimoFalloDetalle) - 1] = '\0';
    estadoActual = ESTOP;
}

ResultadoRearme Seguridad::resetFallo() {
    frenarMotores();
    reiniciarControlRumbo();
    if (estadoActual != ESTOP && estadoActual != FALLO) {
        LOG_CORE("Rearme ignorado: sin fallo activo.");
        return ResultadoRearme::SIN_FALLO_ACTIVO;
    }
    if (!motoresListos()) {
        LOG_CORE("Rearme rechazado: salida de motores no disponible.");
        estadoActual = FALLO;
        registrarMotivoFinalizacion("motor_output_unavailable");
        return ResultadoRearme::MOTORES_NO_DISPONIBLES;
    }
    if (!pcntInicializados()) {
        LOG_CORE("Rearme rechazado: inicializacion PCNT incompleta.");
        estadoActual = FALLO;
        registrarMotivoFinalizacion("pcnt_init_failed");
        strncpy(ultimoFalloDetalle, "pcnt_init_failed", sizeof(ultimoFalloDetalle) - 1);
        ultimoFalloDetalle[sizeof(ultimoFalloDetalle) - 1] = '\0';
        return ResultadoRearme::PCNT_NO_DISPONIBLE;
    }
    LOG_CORE("Sistema rearmado.");
    estadoActual = robotCalibrado ? LISTO : DESARMADO;
    inicio_movimiento_ms = 0;
    prepararRevalidacionEncoders();
    return ResultadoRearme::REARMADO;
}

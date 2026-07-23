#include "Seguridad.h"
#include "Debug.h"
#include "Motores.h"
#include "Config.h"
#include "Eventos.h"
#include "Cinematica.h"

Seguridad WatchdogSeguridad;

namespace {
unsigned long actualizarAcumulado(unsigned long actual, bool energizado, bool hayPulso,
                                  bool conservarDurantePausa, unsigned long deltaT) {
    if (hayPulso) return 0;
    if (!energizado) return conservarDurantePausa ? actual : 0;
    if (deltaT == 0 || deltaT >= 1000) return actual;
    return actual + deltaT;
}

bool energizadoParaWatchdog(int pwm, bool buscandoTorque) {
    const int umbralNormal = static_cast<int>(40 * PWM_SCALE_8_TO_10);
    return buscandoTorque ? abs(pwm) >= CALIBRATION_PWM_END : abs(pwm) > umbralNormal;
}

unsigned long limiteStallParaEstado(EstadoRobot estado) {
    if (estado == CALIBRATING) return CALIBRATION_STALL_ACCUMULATED_MS;
    if (estado == GIRANDO || estado == RECOVERING) return TURN_STALL_TIMEOUT_MS;
    return NORMAL_STALL_TIMEOUT_MS;
}

int64_t pulsosFusionadosLado(const int64_t valores[4], int lado) {
    return lado == 0 ? valores[0] + valores[2] : valores[1] + valores[3];
}
}

Seguridad::Seguridad() {
    ultimo_comando_ms = 0;
    ultimo_heartbeat_ms = 0;
    ultima_auditoria_ms = 0;
    estado_supervisado = UNCALIBRATED;
    for (int i = 0; i < 4; ++i) {
        pulsos_movimiento_iniciales[i] = 0;
        tiempo_energizado_sin_pulsos_ms[i] = 0;
    }
    for (int lado = 0; lado < 2; ++lado) {
        pulsos_lado_iniciales[lado] = 0;
        tiempo_energizado_sin_pulsos_lado_ms[lado] = 0;
    }
}

void Seguridad::registrarHeartbeat() {
    ultimo_heartbeat_ms = millis();
}

void Seguridad::alimentarWatchdogRed() {
    ultimo_comando_ms = millis();
}

bool Seguridad::auditarSalud(const SensorSnapshot &snap, int pwm_L, int pwm_R) {
    if (estadoActual == ESTOP_LATCHED || estadoActual == FAULT_SENSOR) {
        return true;
    }

    if (estadoActual == MANUAL) {
        if (millis() - ultimo_comando_ms > TIMEOUT_WIFI_MANUAL_MS) {
            LOG_CORE("ALERTA: Perdida de Wi-Fi en MANUAL. Freno de seguridad.");
            frenarMotores();
            estadoActual = SAFE_STOP_COMMS;
            encolarEvento(EVT_FAULT, comandoActivoId, "manual_lease_expired");
            return true;
        }
    }

    const unsigned long ahora = millis();
    const unsigned long deltaT = ultima_auditoria_ms == 0 ? 0 : ahora - ultima_auditoria_ms;
    ultima_auditoria_ms = ahora;
    const bool movimientoActivo = estadoActual == AVANZANDO || estadoActual == GIRANDO ||
        estadoActual == MANUAL || estadoActual == CALIBRATING ||
        estadoActual == RECOVERING || estadoActual == TESTING;
    const int64_t actuales[4] = {snap.pulsosFL, snap.pulsosFR, snap.pulsosBL, snap.pulsosBR};

    if (!movimientoActivo || estado_supervisado != estadoActual) {
        estado_supervisado = estadoActual;
        for (int i = 0; i < 4; ++i) {
            pulsos_movimiento_iniciales[i] = actuales[i];
            tiempo_energizado_sin_pulsos_ms[i] = 0;
        }
        pulsos_lado_iniciales[0] = pulsosFusionadosLado(actuales, 0);
        pulsos_lado_iniciales[1] = pulsosFusionadosLado(actuales, 1);
        tiempo_energizado_sin_pulsos_lado_ms[0] = 0;
        tiempo_energizado_sin_pulsos_lado_ms[1] = 0;
        return false;
    }

    // Durante la rampa de calibracion, un PWM bajo que aun no vence friccion
    // estatica no demuestra un atasco. El acumulador individual se arma al
    // alcanzar el maximo solicitado; en los giros de validacion sigue usando
    // el umbral normal de movimiento.
    const bool buscandoTorque = busquedaTorqueCalibracionActiva();
    const bool ladoEnergizado[2] = {
        energizadoParaWatchdog(pwm_L, buscandoTorque),
        energizadoParaWatchdog(pwm_R, buscandoTorque)
    };
    const char* nombresNormal[4] = {"stall_fl", "stall_fr", "stall_bl", "stall_br"};
    const char* nombresCal[4] = {"calibration_stall_fl", "calibration_stall_fr",
                                 "calibration_stall_bl", "calibration_stall_br"};
    bool hayPulsoRueda[4] = {};
    bool ruedaEnergizada[4] = {};

    for (int i = 0; i < 4; ++i) {
        const bool hayPulso = llabs(actuales[i] - pulsos_movimiento_iniciales[i]) >= 1;
        const bool energizado = (i == 0 || i == 2) ? ladoEnergizado[0] : ladoEnergizado[1];
        hayPulsoRueda[i] = hayPulso;
        ruedaEnergizada[i] = energizado;
        if (hayPulso) pulsos_movimiento_iniciales[i] = actuales[i];
        tiempo_energizado_sin_pulsos_ms[i] = actualizarAcumulado(
            tiempo_energizado_sin_pulsos_ms[i], energizado, hayPulso,
            estadoActual == CALIBRATING, deltaT);
    }

    const unsigned long limite = limiteStallParaEstado(estadoActual);
    const bool giroControlado = estadoActual == GIRANDO || estadoActual == RECOVERING ||
        (estadoActual == CALIBRATING && intentoGiroActualTelemetria() > 0);
    // Cinematica es la única autoridad durante el pivot continuo: arma el
    // watchdog después del torque calibrado y supervisa actividad por lado.
    // Seguridad conserva sus acumulados diagnósticos sin duplicar esa decisión.
    if (giroControlado) return false;

    // En autónomo se supervisa el conjunto motriz por lado. Un encoder
    // individual puede excluirse de la fusión si su compañero confirma que el
    // lado se mueve; perder ambos encoders bajo PWM físico sigue siendo fallo.
    if (estadoActual == AVANZANDO) {
        for (int lado = 0; lado < 2; ++lado) {
            const int64_t pulsosLado = pulsosFusionadosLado(actuales, lado);
            const bool hayPulsoLado = pulsosLado != pulsos_lado_iniciales[lado];
            if (hayPulsoLado) pulsos_lado_iniciales[lado] = pulsosLado;
            tiempo_energizado_sin_pulsos_lado_ms[lado] = actualizarAcumulado(
                tiempo_energizado_sin_pulsos_lado_ms[lado], ladoEnergizado[lado],
                hayPulsoLado, false, deltaT);
            // En autonomo este watchdog sólo mide y publica el tiempo. La
            // maquina de movimiento consume el acumulado y ejecuta primero su
            // escalera de freno, reposo y reintento. Seguridad sigue siendo la
            // autoridad inmediata para E-STOP, manual y protecciones externas.
        }
        return false;
    }

    for (int i = 0; i < 4; ++i) {
        if (!ruedaEnergizada[i] || hayPulsoRueda[i] ||
            tiempo_energizado_sin_pulsos_ms[i] < limite) continue;

        LOG_CORE("ERROR CRITICO: motor/encoder sin pulsos bajo PWM.");
        frenarMotores();
        estadoActual = FAULT_SENSOR;
        encolarEvento(EVT_FAULT, comandoActivoId,
                      estado_supervisado == CALIBRATING ? nombresCal[i] : nombresNormal[i]);
        return true;
    }

    return false;
}

bool Seguridad::validarLogicaWatchdog() {
    if (energizadoParaWatchdog(CALIBRATION_PWM_END - 1, true)) return false;
    if (!energizadoParaWatchdog(CALIBRATION_PWM_END, true)) return false;
    if (!energizadoParaWatchdog(static_cast<int>(41 * PWM_SCALE_8_TO_10), false)) return false;
    if (limiteStallParaEstado(AVANZANDO) != NORMAL_STALL_TIMEOUT_MS ||
        limiteStallParaEstado(GIRANDO) != TURN_STALL_TIMEOUT_MS ||
        limiteStallParaEstado(RECOVERING) != TURN_STALL_TIMEOUT_MS ||
        limiteStallParaEstado(CALIBRATING) != CALIBRATION_STALL_ACCUMULATED_MS) return false;
    unsigned long ladoSinPulsos = 0;
    for (int ciclo = 0; ciclo < 25; ++ciclo) {
        ladoSinPulsos = actualizarAcumulado(ladoSinPulsos, true, false, false, 100);
    }
    if (ladoSinPulsos != AUTONOMOUS_SIDE_STALL_TIMEOUT_MS) return false;
    if (actualizarAcumulado(ladoSinPulsos, true, true, false, 100) != 0) return false;
    const int64_t base[4] = {100, 200, 300, 400};
    const int64_t soloBlActivo[4] = {100, 200, 301, 400};
    const int64_t ladoIzquierdoInactivo[4] = {100, 201, 300, 401};
    if (pulsosFusionadosLado(soloBlActivo, 0) == pulsosFusionadosLado(base, 0)) return false;
    if (pulsosFusionadosLado(ladoIzquierdoInactivo, 0) != pulsosFusionadosLado(base, 0)) return false;
    if (pulsosFusionadosLado(ladoIzquierdoInactivo, 1) == pulsosFusionadosLado(base, 1)) return false;
    unsigned long acumulados[4] = {0, 0, 0, 0};
    for (int ciclo = 0; ciclo < 8; ++ciclo) {
        for (int i = 0; i < 4; ++i) {
            acumulados[i] = actualizarAcumulado(acumulados[i], true, i != 0, true, 100);
        }
    }
    if (acumulados[0] != CALIBRATION_STALL_ACCUMULATED_MS ||
        acumulados[1] != 0 || acumulados[2] != 0 || acumulados[3] != 0) return false;
    if (actualizarAcumulado(200, false, false, true, 100) != 200) return false;
    if (actualizarAcumulado(200, false, false, false, 100) != 0) return false;
    if (actualizarAcumulado(200, true, true, true, 100) != 0) return false;
    return true;
}

uint32_t Seguridad::tiempoStallMs(size_t indice) const {
    return indice < 4 ? tiempo_energizado_sin_pulsos_ms[indice] : 0;
}

uint32_t Seguridad::tiempoStallLadoMs(size_t lado) const {
    return lado < 2 ? tiempo_energizado_sin_pulsos_lado_ms[lado] : 0;
}

void Seguridad::forzarEStop() {
    LOG_CORE("ACTIVANDO E-STOP LOGICO");
    frenarMotores();
    estadoActual = ESTOP_LATCHED;
}

void Seguridad::resetFallo() {
    frenarMotores();
    if (estadoActual != ESTOP_LATCHED && estadoActual != FAULT_SENSOR && estadoActual != SAFE_STOP_COMMS) {
        LOG_CORE("Rearme rechazado: no hay fallo activo.");
        return;
    }
    LOG_CORE("Rearmando sistema tras fallo.");
    estadoActual = robotCalibrado ? IDLE : UNCALIBRATED;
    for (int i = 0; i < 4; ++i) tiempo_energizado_sin_pulsos_ms[i] = 0;
    for (int lado = 0; lado < 2; ++lado) tiempo_energizado_sin_pulsos_lado_ms[lado] = 0;
    alimentarWatchdogRed();
}

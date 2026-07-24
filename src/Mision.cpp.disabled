#include "Mision.h"

#include "Cinematica.h"
#include "Estado.h"
#include "Eventos.h"
#include "Motores.h"
#include <Preferences.h>

namespace {
PuntoMision puntos[MAX_MISSION_SEGMENTS] = {};
size_t cantidadPuntos = 0;
size_t totalPuntos = 0;
size_t indicePaso = 0;
size_t checkpointPaso = 0;
uint32_t revisionActual = 0;
char missionIdActual[33] = {};
char commandIdInicio[33] = {};
char commandIdPaso[33] = {};
bool activa = false;
bool pasoEnCurso = false;
bool completada = false;
bool interrumpida = false;
uint32_t pasoPersistido = 0;  // índice activo + 1; cero significa ninguno.
char stepIdPersistido[33] = {};
Preferences preferencias;

void guardarCheckpoint() {
    if (!preferencias.begin("robot-mission", false)) return;
    preferencias.putString("id", missionIdActual);
    preferencias.putUInt("rev", revisionActual);
    preferencias.putUInt("done", static_cast<uint32_t>(checkpointPaso));
    preferencias.putUInt("total", static_cast<uint32_t>(totalPuntos));
    preferencias.putUInt("active", pasoPersistido);
    preferencias.putString("step", stepIdPersistido);
    preferencias.end();
}

void crearIdPaso(size_t indice) {
    strncpy(commandIdPaso, puntos[indice].step_id, sizeof(commandIdPaso) - 1);
    commandIdPaso[32] = '\0';
}
}

void inicializarPersistenciaMision() {
    if (!preferencias.begin("robot-mission", true)) return;
    String id = preferencias.getString("id", "");
    revisionActual = preferencias.getUInt("rev", 0);
    checkpointPaso = preferencias.getUInt("done", 0);
    totalPuntos = preferencias.getUInt("total", 0);
    pasoPersistido = preferencias.getUInt("active", 0);
    String stepId = preferencias.getString("step", "");
    preferencias.end();
    if (id.length() == 32) strncpy(missionIdActual, id.c_str(), sizeof(missionIdActual) - 1);
    if (stepId.length() == 32) strncpy(stepIdPersistido, stepId.c_str(), sizeof(stepIdPersistido) - 1);
    interrumpida = pasoPersistido > 0 && pasoPersistido > checkpointPaso;
}

bool misionAutonomaCoincide(const char* missionId, uint32_t revision,
                            const PuntoMision* nuevos, size_t cantidad) {
    if (!missionId || strcmp(missionId, missionIdActual) || revision != revisionActual ||
        cantidad != totalPuntos) return false;
    if (cantidadPuntos == 0) return true;  // Ya completada o sólo queda el checkpoint.
    for (size_t i = 0; i < cantidad; ++i) {
        if (strcmp(puntos[i].step_id, nuevos[i].step_id) ||
            fabsf(puntos[i].x_cm - nuevos[i].x_cm) > 0.001f ||
            fabsf(puntos[i].y_cm - nuevos[i].y_cm) > 0.001f) return false;
    }
    return true;
}

bool cargarMisionAutonoma(const char* missionId, uint32_t revision,
                          const PuntoMision* nuevos, size_t cantidad) {
    if (!missionId || strlen(missionId) != 32 || !nuevos || cantidad == 0 ||
        cantidad > MAX_MISSION_SEGMENTS || activa || estadoActual != IDLE) return false;
    const bool reanudacion = !strcmp(missionIdActual, missionId) &&
                             revisionActual == revision && checkpointPaso <= cantidad;
    const size_t checkpointConservado = reanudacion ? checkpointPaso : 0;
    for (size_t i = 0; i < cantidad; ++i) {
        if (!isfinite(nuevos[i].x_cm) || !isfinite(nuevos[i].y_cm)) return false;
        puntos[i] = nuevos[i];
    }
    cantidadPuntos = cantidad;
    totalPuntos = cantidad;
    indicePaso = checkpointConservado;
    checkpointPaso = checkpointConservado;
    revisionActual = revision;
    strncpy(missionIdActual, missionId, sizeof(missionIdActual) - 1);
    missionIdActual[32] = '\0';
    commandIdInicio[0] = commandIdPaso[0] = '\0';
    pasoEnCurso = false;
    completada = false;
    if (!reanudacion) {
        interrumpida = false;
        pasoPersistido = 0;
        stepIdPersistido[0] = '\0';
    }
    guardarCheckpoint();
    return true;
}

bool iniciarMisionAutonoma(const char* commandId, const char* missionId,
                           uint32_t revision) {
    if (!misionAutonomaCargada() || activa || interrumpida || estadoActual != IDLE ||
        !commandId || !missionId || strcmp(missionId, missionIdActual) ||
        revision != revisionActual) return false;
    strncpy(commandIdInicio, commandId, sizeof(commandIdInicio) - 1);
    commandIdInicio[32] = '\0';
    activa = true;
    pasoEnCurso = false;
    indicePaso = checkpointPaso;
    return true;
}

void procesarMisionAutonoma() {
    if (!activa) return;
    if (pasoEnCurso) {
        if (strcmp(ultimoTerminalId, commandIdPaso)) return;
        if (!strcmp(ultimoTerminalTipo, "completed")) {
            ++indicePaso;
            checkpointPaso = indicePaso;
            pasoEnCurso = false;
            pasoPersistido = 0;
            stepIdPersistido[0] = '\0';
            guardarCheckpoint();
            encolarEvento(EVT_PROGRESS, commandIdInicio, "mission_checkpoint",
                          static_cast<float>(checkpointPaso) / cantidadPuntos);
        } else {
            activa = false;
            pasoEnCurso = false;
            interrumpida = true;
            guardarCheckpoint();
            encolarEvento(EVT_FAULT, commandIdInicio, ultimoTerminalDetalle);
        }
        return;
    }
    if (estadoActual != IDLE) return;
    if (indicePaso >= cantidadPuntos) {
        activa = false;
        cantidadPuntos = 0;  // libera automáticamente los puntos; queda checkpoint mínimo.
        completada = true;
        guardarCheckpoint();
        encolarEvento(EVT_COMPLETED, commandIdInicio, "mission_completed_zero_aligned");
        return;
    }
    crearIdPaso(indicePaso);
    pasoPersistido = static_cast<uint32_t>(indicePaso + 1);
    strncpy(stepIdPersistido, commandIdPaso, sizeof(stepIdPersistido) - 1);
    stepIdPersistido[32] = '\0';
    guardarCheckpoint();
    if (!calcularDestino(puntos[indicePaso].x_cm, puntos[indicePaso].y_cm, commandIdPaso)) {
        activa = false;
        pasoPersistido = 0;
        stepIdPersistido[0] = '\0';
        guardarCheckpoint();
        encolarEvento(EVT_FAULT, commandIdInicio, motivoUltimoRechazoMovimiento());
        return;
    }
    pasoEnCurso = true;
}

void detenerMisionAutonoma(bool conservarRuta) {
    if (pasoPersistido > checkpointPaso) {
        interrumpida = true;
        guardarCheckpoint();
    }
    activa = false;
    pasoEnCurso = false;
    commandIdPaso[0] = '\0';
    if (!conservarRuta) liberarMisionAutonoma();
}

void liberarMisionAutonoma() {
    activa = false;
    pasoEnCurso = false;
    cantidadPuntos = totalPuntos = indicePaso = checkpointPaso = 0;
    revisionActual = 0;
    missionIdActual[0] = commandIdInicio[0] = commandIdPaso[0] = '\0';
    completada = false;
    interrumpida = false;
    pasoPersistido = 0;
    stepIdPersistido[0] = '\0';
    if (preferencias.begin("robot-mission", false)) {
        preferencias.clear();
        preferencias.end();
    }
}

bool misionAutonomaCargada() { return cantidadPuntos > 0; }
bool misionAutonomaActiva() { return activa; }
const char* idMisionAutonoma() { return missionIdActual; }
uint32_t revisionMisionAutonoma() { return revisionActual; }
size_t pasoMisionActual() { return indicePaso; }
size_t pasosMisionCompletados() { return checkpointPaso; }
size_t totalPasosMision() { return totalPuntos; }
const char* estadoMisionAutonoma() {
    if (activa) return pasoEnCurso ? "EXECUTING" : "BETWEEN_STEPS";
    if (interrumpida) return "INTERRUPTED";
    if (cantidadPuntos) return "LOADED";
    if (completada || (totalPuntos && checkpointPaso >= totalPuntos)) return "COMPLETED";
    return missionIdActual[0] ? "CHECKPOINT_ONLY" : "EMPTY";
}
const char* idPasoMisionActual() { return stepIdPersistido; }
bool misionAutonomaInterrumpida() { return interrumpida; }

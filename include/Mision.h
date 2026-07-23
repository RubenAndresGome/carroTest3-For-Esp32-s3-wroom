#pragma once

#include <Arduino.h>

constexpr size_t MAX_MISSION_SEGMENTS = 32;

struct PuntoMision {
    float x_cm;
    float y_cm;
    char step_id[33];
};

bool cargarMisionAutonoma(const char* missionId, uint32_t revision,
                          const PuntoMision* puntos, size_t cantidad);
bool iniciarMisionAutonoma(const char* commandId, const char* missionId,
                           uint32_t revision);
bool misionAutonomaCoincide(const char* missionId, uint32_t revision,
                            const PuntoMision* puntos, size_t cantidad);
void procesarMisionAutonoma();
void detenerMisionAutonoma(bool conservarRuta = true);
void liberarMisionAutonoma();
void inicializarPersistenciaMision();

bool misionAutonomaCargada();
bool misionAutonomaActiva();
const char* idMisionAutonoma();
uint32_t revisionMisionAutonoma();
size_t pasoMisionActual();
size_t pasosMisionCompletados();
size_t totalPasosMision();
const char* estadoMisionAutonoma();
const char* idPasoMisionActual();
bool misionAutonomaInterrumpida();

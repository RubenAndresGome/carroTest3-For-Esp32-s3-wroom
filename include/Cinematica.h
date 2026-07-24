#pragma once

#include "Estado.h"

// API del control de movimiento del robot de memoria corta.
// iniciarPaso ejecuta el paso atómico completo: girar al rumbo absoluto,
// avanzar la distancia y reorientarse a 0° antes de reportar.
bool iniciarCalibracion(int seq);
bool iniciarPaso(float heading, float distanciaCm, int seq);
void cancelarMovimiento(const char* detalle);
void controlarMovimiento();

// Utilidades expuestas para main.cpp
float normalizar360(float angulo);
bool enFaseAvance();

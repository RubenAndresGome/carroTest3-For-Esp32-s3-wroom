#pragma once

#include "Estado.h"
#include "Comandos.h"

#include <cmath>

// API del control de movimiento del robot de memoria corta.
// El firmware conserva memoria corta: ejecuta un paso o un giro absoluto y
// reporta el resultado antes de aceptar la siguiente maniobra de misión.
bool iniciarCalibracion(int seq);
bool iniciarPaso(float heading, float distanciaCm, int seq, float targetX = NAN, float targetY = NAN,
                 bool objetivoAbsoluto = false, ModoPaso modoPaso = PASO_ADELANTE);
bool iniciarGiroAbsoluto(float heading, int seq);

void cancelarMovimiento(const char* detalle);
void controlarMovimiento();
void reiniciarControlRumbo();
void registrarMotivoFinalizacion(const char* detalle);

// Utilidades expuestas para main.cpp y Seguridad.cpp
float normalizar360(float angulo);
bool enFaseAvance();
bool enFaseTraslacion();
bool enFaseGiro();
bool enFaseCalibracion();

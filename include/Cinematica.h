#pragma once

#include "Estado.h"
#include "Comandos.h"

#include <cmath>

struct DiagnosticoCalibracion {
  bool activa = false;
  uint8_t pasoRampa = 0;
  uint8_t pasosRampaTotal = 0;
  int pwmObjetivo = 0;
  int candidatoDireccion = 0;
  int64_t deltaEncoders[4] = {};
  int64_t promedioLados[2] = {};
  bool ladosValidos[2] = {};
  bool encoderResponde[4] = {};
  bool encoderAislado[4] = {};
  bool respuestaFaseA[4] = {};
  bool respuestaFaseB[4] = {};
  uint32_t stallAcumuladoMs[2] = {};
};

// API del control de movimiento del robot de memoria corta.
// El firmware conserva memoria corta: ejecuta un paso o un giro absoluto y
// reporta el resultado antes de aceptar la siguiente maniobra de misión.
bool iniciarCalibracion(int seq);
bool iniciarPaso(float heading, float distanciaCm, int seq, float targetX = NAN, float targetY = NAN,
                 bool objetivoAbsoluto = false, ModoPaso modoPaso = PASO_ADELANTE);
bool iniciarGiroAbsoluto(float heading, int seq);

void cancelarMovimiento(const char* detalle);
void forzarFalloMovimiento(const char* detalle);
void controlarMovimiento();
void reiniciarControlRumbo();
void registrarMotivoFinalizacion(const char* detalle);

// Utilidades expuestas para main.cpp y Seguridad.cpp
float normalizar360(float angulo);
bool enFaseAvance();
bool enFaseTraslacion();
bool enFaseGiro();
bool enFaseCalibracion();
DiagnosticoCalibracion obtenerDiagnosticoCalibracion();

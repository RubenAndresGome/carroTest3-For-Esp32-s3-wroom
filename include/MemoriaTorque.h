#pragma once

#include <Arduino.h>

struct DiagnosticoMemoriaTorque {
  bool montada = false;
  bool cargada = false;
  bool persistenciaPendiente = false;
  uint8_t cantidad = 0;
  int promedioPositivo8 = 140;
  int promedioNegativo8 = 140;
  int promedioPositivoDerecho8 = 140;
  int promedioNegativoDerecho8 = 140;
  int basePositiva8 = 140;
  int baseNegativa8 = 140;
  int basePositivaDerecha8 = 140;
  int baseNegativaDerecha8 = 140;
  char estado[32] = "not_initialized";
};

void setup_MemoriaTorque();
int baseTorqueParaPolaridad8(int polaridad, bool izquierda = true);
void solicitarGuardarTorque(int pwmPositivo8, int pwmNegativo8,
                            int polaridadPositiva, int polaridadNegativa,
                            int pwmPositivoDerecho8 = 0, int pwmNegativoDerecho8 = 0);
void procesarPersistenciaTorque();
bool persistenciaTorquePendiente();
DiagnosticoMemoriaTorque obtenerDiagnosticoMemoriaTorque();

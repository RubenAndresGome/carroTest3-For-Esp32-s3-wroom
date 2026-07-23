#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <esp_system.h>

enum class TareaDiagnosticada : uint8_t { WEB = 0, CONTROL = 1 };

extern volatile uint32_t stackMinimoWebBytes;
extern volatile uint32_t stackMinimoControlBytes;
extern volatile bool creacionTareasOk;
extern volatile uint32_t periodoControlUltimoUs;
extern volatile uint32_t jitterControlMaxUs;
extern volatile uint32_t duracionControlMaxUs;
extern volatile uint32_t antiguedadMuestraMaxUs;
extern volatile uint32_t deadlinesControlPerdidos;
extern char falloCreacionTarea[32];
extern char motivoResetESP32[32];

void inicializarDiagnosticoRTOS();
void registrarStackLibre(TareaDiagnosticada tarea, UBaseType_t bytesLibres);
void registrarResultadoArquitectura(BaseType_t web, bool controlEnCore1);
void registrarCicloControl(uint32_t periodoUs, uint32_t jitterUs,
                           uint32_t duracionUs, uint32_t antiguedadMuestraUs,
                           uint32_t deadlinesPerdidos);
bool stackRTOSBajo();
bool validarLogicaDiagnosticoRTOS();

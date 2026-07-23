#include "DiagnosticoRTOS.h"
#include "Config.h"

#include <cstring>
#include <limits.h>

volatile uint32_t stackMinimoWebBytes = UINT32_MAX;
volatile uint32_t stackMinimoControlBytes = UINT32_MAX;
volatile bool creacionTareasOk = false;
volatile uint32_t periodoControlUltimoUs = 0;
volatile uint32_t jitterControlMaxUs = 0;
volatile uint32_t duracionControlMaxUs = 0;
volatile uint32_t antiguedadMuestraMaxUs = 0;
volatile uint32_t deadlinesControlPerdidos = 0;
char falloCreacionTarea[32] = {};
char motivoResetESP32[32] = {};

namespace {
const char* textoReset(esp_reset_reason_t motivo) {
    switch (motivo) {
        case ESP_RST_POWERON: return "power_on";
        case ESP_RST_EXT: return "external_pin";
        case ESP_RST_SW: return "software";
        case ESP_RST_PANIC: return "panic";
        case ESP_RST_INT_WDT: return "interrupt_watchdog";
        case ESP_RST_TASK_WDT: return "task_watchdog";
        case ESP_RST_WDT: return "other_watchdog";
        case ESP_RST_DEEPSLEEP: return "deep_sleep";
        case ESP_RST_BROWNOUT: return "brownout";
        case ESP_RST_SDIO: return "sdio";
        default: return "unknown";
    }
}

void actualizarMinimo(volatile uint32_t& destino, UBaseType_t valor) {
    const uint32_t bytes = static_cast<uint32_t>(valor);
    if (bytes < destino) destino = bytes;
}

bool resultadosValidos(BaseType_t web, bool controlEnCore1) {
    return web == pdPASS && controlEnCore1;
}

bool stackBajoValores(uint32_t web, uint32_t control) {
    const auto bajo = [](uint32_t valor) {
        return valor != UINT32_MAX && valor < RTOS_STACK_MIN_ACCEPTABLE_BYTES;
    };
    return bajo(web) || bajo(control);
}
}

void inicializarDiagnosticoRTOS() {
    strncpy(motivoResetESP32, textoReset(esp_reset_reason()), sizeof(motivoResetESP32) - 1);
    motivoResetESP32[sizeof(motivoResetESP32) - 1] = '\0';
}

void registrarStackLibre(TareaDiagnosticada tarea, UBaseType_t bytesLibres) {
    switch (tarea) {
        case TareaDiagnosticada::WEB: actualizarMinimo(stackMinimoWebBytes, bytesLibres); break;
        case TareaDiagnosticada::CONTROL: actualizarMinimo(stackMinimoControlBytes, bytesLibres); break;
    }
}

void registrarResultadoArquitectura(BaseType_t web, bool controlEnCore1) {
    creacionTareasOk = resultadosValidos(web, controlEnCore1);
    falloCreacionTarea[0] = '\0';
    if (web != pdPASS) strncpy(falloCreacionTarea, "task_web_create_failed", sizeof(falloCreacionTarea) - 1);
    else if (!controlEnCore1) strncpy(falloCreacionTarea, "control_not_on_core1", sizeof(falloCreacionTarea) - 1);
    falloCreacionTarea[sizeof(falloCreacionTarea) - 1] = '\0';
}

void registrarCicloControl(uint32_t periodoUs, uint32_t jitterUs,
                           uint32_t duracionUs, uint32_t antiguedadMuestraUs,
                           uint32_t deadlinesPerdidos) {
    periodoControlUltimoUs = periodoUs;
    if (jitterUs > jitterControlMaxUs) jitterControlMaxUs = jitterUs;
    if (duracionUs > duracionControlMaxUs) duracionControlMaxUs = duracionUs;
    if (antiguedadMuestraUs > antiguedadMuestraMaxUs) antiguedadMuestraMaxUs = antiguedadMuestraUs;
    deadlinesControlPerdidos += deadlinesPerdidos;
}

bool stackRTOSBajo() {
    return stackBajoValores(stackMinimoWebBytes, stackMinimoControlBytes);
}

bool validarLogicaDiagnosticoRTOS() {
    if (CONTROL_LOOP_PERIOD_US != 10000UL || CONTROL_LOOP_PRIORITY < 2) return false;
    if (!resultadosValidos(pdPASS, true)) return false;
    if (resultadosValidos(pdFAIL, true)) return false;
    if (resultadosValidos(pdPASS, false)) return false;
    if (stackBajoValores(2048, 2048)) return false;
    if (stackBajoValores(1024, 1024)) return false;
    if (!stackBajoValores(2048, 1023)) return false;
    if (stackBajoValores(UINT32_MAX, UINT32_MAX)) return false;
    return strcmp(textoReset(ESP_RST_PANIC), "panic") == 0 &&
           strcmp(textoReset(ESP_RST_BROWNOUT), "brownout") == 0;
}

#include "Eventos.h"
#include "Estado.h"

QueueHandle_t colaEventosRed = nullptr;

void encolarEvento(TipoEvento tipo, int seq, const char* detalle, float progreso, uint32_t runId) {
    if (!colaEventosRed) return;

    // Compactar y descartar eventos de progreso repetidos con idéntico detalle y porcentaje
    static int ultimoProgresoSeq = -1;
    static char ultimoProgresoDetalle[32] = "";
    static float ultimoProgresoPct = -1.0f;
    static uint32_t ultimoProgresoMs = 0;

    if (tipo == EVT_PROGRESS) {
        const uint32_t ahora = millis();
        if (seq == ultimoProgresoSeq && detalle && strcmp(detalle, ultimoProgresoDetalle) == 0 &&
            fabsf(progreso - ultimoProgresoPct) < 0.015f && (ahora - ultimoProgresoMs < 800)) {
            return; // Descartar duplicado redundante en menos de 800 ms
        }
        ultimoProgresoSeq = seq;
        if (detalle) {
            strncpy(ultimoProgresoDetalle, detalle, sizeof(ultimoProgresoDetalle) - 1);
            ultimoProgresoDetalle[sizeof(ultimoProgresoDetalle) - 1] = '\0';
        }
        ultimoProgresoPct = progreso;
        ultimoProgresoMs = ahora;
    }

    EventoRed evt = {};
    evt.tipo = tipo;
    evt.seq = seq;
    evt.run_id = runId ? runId : pasoEjecucionId;
    evt.progreso = progreso;
    if (detalle) {
        strncpy(evt.detalle, detalle, sizeof(evt.detalle) - 1);
        evt.detalle[sizeof(evt.detalle) - 1] = '\0';
    }
    if (tipo == EVT_COMPLETED || tipo == EVT_FAULT || tipo == EVT_REJECTED) {
        if (tipo == EVT_COMPLETED && seq > ultimoSeqCompletado) {
            ultimoSeqCompletado = seq;
        }
        if (tipo == EVT_FAULT && detalle) {
            strncpy(ultimoFalloDetalle, detalle, sizeof(ultimoFalloDetalle) - 1);
            ultimoFalloDetalle[sizeof(ultimoFalloDetalle) - 1] = '\0';
        }
        if (seq != 0 && seq == seqActivo) {
            seqActivo = 0;
            faseComando[0] = '\0';
            progresoComando = 0.0f;
        }
    }
    xQueueSend(colaEventosRed, &evt, 0);
}

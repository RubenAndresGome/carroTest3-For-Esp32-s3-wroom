#include "Eventos.h"
#include "Estado.h"

QueueHandle_t colaEventosRed = nullptr;

void encolarEvento(TipoEvento tipo, int seq, const char* detalle, float progreso) {
    if (!colaEventosRed) return;
    EventoRed evt = {};
    evt.tipo = tipo;
    evt.seq = seq;
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

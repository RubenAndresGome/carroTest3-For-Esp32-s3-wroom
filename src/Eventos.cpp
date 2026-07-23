#include "Eventos.h"
#include "Estado.h"

QueueHandle_t colaEventosRed = nullptr;

void encolarEvento(TipoEvento tipo, const char* cmd_id, const char* detalle, float progreso) {
    if (!colaEventosRed) return;
    EventoRed evt = {};
    evt.tipo = tipo;
    evt.progreso = progreso;
    if (cmd_id) {
        strncpy(evt.cmd_id, cmd_id, sizeof(evt.cmd_id) - 1);
        evt.cmd_id[sizeof(evt.cmd_id) - 1] = '\0';
    }
    if (detalle) {
        strncpy(evt.detalle, detalle, sizeof(evt.detalle) - 1);
        evt.detalle[sizeof(evt.detalle) - 1] = '\0';
    } else {
        evt.detalle[0] = '\0';
    }
    if (tipo == EVT_COMPLETED || tipo == EVT_REJECTED || tipo == EVT_FAULT) {
        strncpy(ultimoTerminalId, evt.cmd_id, sizeof(ultimoTerminalId) - 1);
        ultimoTerminalId[sizeof(ultimoTerminalId) - 1] = '\0';
        const char* terminal = tipo == EVT_COMPLETED ? "completed" : tipo == EVT_REJECTED ? "rejected" : "fault";
        strncpy(ultimoTerminalTipo, terminal, sizeof(ultimoTerminalTipo) - 1);
        ultimoTerminalTipo[sizeof(ultimoTerminalTipo) - 1] = '\0';
        strncpy(ultimoTerminalDetalle, evt.detalle, sizeof(ultimoTerminalDetalle) - 1);
        ultimoTerminalDetalle[sizeof(ultimoTerminalDetalle) - 1] = '\0';
        if (evt.cmd_id[0] && !strncmp(comandoActivoId, evt.cmd_id, sizeof(comandoActivoId))) {
            comandoActivoId[0] = '\0';
            comandoActivoNombre[0] = '\0';
        }
    }
    xQueueSend(colaEventosRed, &evt, 0);
}

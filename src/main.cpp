#include <Arduino.h>
#include "Config.h"
#include "Estado.h"
#include "Motores.h"
#include "Sensores.h"
#include "Cinematica.h"
#include "Red.h"
#include "Debug.h"
#include "Comandos.h"
#include "PoseEstimator.h"
#include "Seguridad.h"
#include "Eventos.h"

TaskHandle_t TaskControlHandle;
TaskHandle_t TaskWebHandle;
QueueHandle_t colaComandos;
volatile bool flag_ESTOP_ISR = false;

void procesarComandos() {
    if (flag_ESTOP_ISR) {
        flag_ESTOP_ISR = false;
        WatchdogSeguridad.forzarEStop();
        xQueueReset(colaComandos);
        return;
    }
    ComandoRed cmd;
    while (xQueueReceive(colaComandos, &cmd, 0) == pdTRUE) {
        switch (cmd.tipo) {
            case CMD_CALIBRATE:
                if (!iniciarCalibracion(cmd.seq))
                    encolarEvento(EVT_REJECTED, cmd.seq, "cal_unavailable");
                break;
            case CMD_STEP:
                if (!iniciarPaso(cmd.heading, cmd.distanciaCm, cmd.seq))
                    encolarEvento(EVT_REJECTED, cmd.seq, "step_invalid");
                break;
            case CMD_STOP:
                cancelarMovimiento("stopped");
                break;
            case CMD_ESTOP:
                WatchdogSeguridad.forzarEStop();
                break;
            case CMD_CLEAR_FAULT:
                WatchdogSeguridad.resetFallo();
                break;
            case CMD_RESET_POSE:
                if (estadoActual == LISTO || estadoActual == DESARMADO) {
                    PoseGlobal.reset();
                    resetOrientacionIMU();
                    encolarEvento(EVT_COMPLETED, cmd.seq, "pose_reset");
                } else encolarEvento(EVT_REJECTED, cmd.seq, "busy");
                break;
            case CMD_SET_COMP:
                if (cmd.factor >= COMP_FACTOR_MIN && cmd.factor <= COMP_FACTOR_MAX) {
                    factorCompensacionDer = cmd.factor;
                    encolarEvento(EVT_COMPLETED, cmd.seq, "comp_ok");
                } else encolarEvento(EVT_REJECTED, cmd.seq, "comp_range");
                break;
            default: break;
        }
    }
}

void Task_Web(void*) {
    LOG_CORE("Task_Web en Core 0");
    for (;;) {
        procesarWebSockets();
        pushTelemetria();
        LOG_THROTTLED("Heartbeat Web", 20);
        static int c=0; if(++c>=20){LOG_MEMORY();c=0;}
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}

void Task_Control(void*) {
    LOG_CORE("Task_Control en Core 1 (100 Hz)");
    TickType_t last = xTaskGetTickCount();
    const TickType_t periodo = pdMS_TO_TICKS(10);
    uint32_t lastSensorSeq = 0;
    uint32_t lastProgress = 0;
    for (;;) {
        procesarComandos();
        leerSensoresSincrono();
        const SensorSnapshot& snap = snapshotSensoresControl();
        bool fresco = snap.sequence != lastSensorSeq;
        if (fresco) {
            lastSensorSeq = snap.sequence;
            heading360 = normalizar360(anguloZ);
            PoseGlobal.actualizarOdometria(snap.pulsosFL, snap.pulsosFR, snap.pulsosBL, snap.pulsosBR,
                                           (estadoActual == EJECUTANDO && enFaseAvance()));
            if (!snap.mpu_present || snap.mpu_stale) {
                if (estadoActual == EJECUTANDO || estadoActual == CALIBRANDO) {
                    frenarMotores();
                    estadoActual = FALLO;
                    LOG_CORE("FAULT: MPU ausente/obsoleto durante movimiento.");
                    encolarEvento(EVT_FAULT, seqActivo, "mpu_lost");
                }
            }
            WatchdogSeguridad.auditarSalud(snap, pwm_aplicado_L, pwm_aplicado_R);
        }
        controlarMovimiento();
        if (seqActivo && (estadoActual==EJECUTANDO||estadoActual==CALIBRANDO) && millis()-lastProgress>=500) {
            lastProgress=millis();
            encolarEvento(EVT_PROGRESS, seqActivo, faseComando, progresoComando);
        }
        static int cnt=0; if(++cnt>=100){LOG_THROTTLED("Heartbeat Control",100);cnt=0;}
        vTaskDelayUntil(&last, periodo);
    }
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.printf("\n=== ROBOT S3 v2 | reset=%d ===\n", (int)esp_reset_reason());

    setup_MotorPinsLow();
    colaComandos = xQueueCreate(4, sizeof(ComandoRed));
    colaEventosRed = xQueueCreate(8, sizeof(EventoRed));
    PoseGlobal.inicializar(WHEEL_DIAMETER_CM, ENCODER_PPR);
    setup_Motores();
    setup_Red();
    setup_Sensores();
    Serial.println("Sistema listo. Iniciando FreeRTOS...");

    xTaskCreatePinnedToCore(Task_Web,    "Web",    8192, NULL, 1, &TaskWebHandle, 0);
    xTaskCreatePinnedToCore(Task_Control,"Control",8192, NULL,10, &TaskControlHandle, 1);
    vTaskDelete(NULL);
}

void loop() {}

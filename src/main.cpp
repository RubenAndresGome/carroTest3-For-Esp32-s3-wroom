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
#include "DiagnosticoRTOS.h"
#include <esp_timer.h>

TaskHandle_t TaskWebHandle;
QueueHandle_t colaComandos;
volatile bool flag_ESTOP_ISR = false;
volatile int seq_ESTOP_pendiente = 0;

void procesarComandos() {
    if (flag_ESTOP_ISR) {
        const int seq = seq_ESTOP_pendiente;
        flag_ESTOP_ISR = false;
        seq_ESTOP_pendiente = 0;
        WatchdogSeguridad.forzarEStop();
        xQueueReset(colaComandos);
        encolarEvento(EVT_FAULT, seq, "estop");
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
                encolarEvento(EVT_COMPLETED, cmd.seq, "stop_ok");
                break;
            case CMD_ESTOP:
                WatchdogSeguridad.forzarEStop();
                break;
            case CMD_CLEAR_FAULT:
                WatchdogSeguridad.resetFallo();
                encolarEvento(EVT_COMPLETED, cmd.seq, "fault_cleared");
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
        static int c=0; if(++c>=20){
            LOG_MEMORY(); c=0;
            registrarStackLibre(TareaDiagnosticada::WEB,
                uxTaskGetStackHighWaterMark(nullptr));
        }
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}

static void ejecutarCicloControl() {
    static uint32_t ultimoSensorSeq = 0;
    static uint32_t ultimoProgresoMs = 0;

    procesarComandos();
    leerSensoresSincrono();
    const SensorSnapshot& snap = snapshotSensoresControl();
    bool fresco = snap.sequence != ultimoSensorSeq;
    if (fresco) {
        ultimoSensorSeq = snap.sequence;
        heading360 = normalizar360(anguloZ);
        PoseGlobal.actualizarOrientacion(snap.imu_deltaZ_rad);
        if (estadoActual == LISTO || estadoActual == DESARMADO) {
            recentrarYawIMUEnReposo();
        }
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
    if (seqActivo && (estadoActual==EJECUTANDO||estadoActual==CALIBRANDO) && millis()-ultimoProgresoMs>=500) {
        ultimoProgresoMs=millis();
        encolarEvento(EVT_PROGRESS, seqActivo, faseComando, progresoComando);
    }
}

void setup() {
    // Primera operación de hardware: entradas del DRV8833 desenergizadas.
    setup_MotorPinsLow();
    Serial.begin(115200);
    delay(200);
    inicializarDiagnosticoRTOS();
    Serial.printf("\n=== ROBOT S3 v2 | reset=%s ===\n", motivoResetESP32);

    colaComandos = xQueueCreate(4, sizeof(ComandoRed));
    colaEventosRed = xQueueCreate(8, sizeof(EventoRed));
    PoseGlobal.inicializar(WHEEL_DIAMETER_CM, ENCODER_PPR);
    setup_Motores();
    setup_Red();
    setup_Sensores();
    Serial.println("Sistema listo. Iniciando FreeRTOS...");

    const BaseType_t webOk = xTaskCreatePinnedToCore(
        Task_Web, "Web", 8192, nullptr, 1, &TaskWebHandle, 0);
    registrarResultadoArquitectura(webOk, xPortGetCoreID() == 1);
    if (webOk != pdPASS) {
        frenarMotores();
        estadoActual = FALLO;
        Serial.println("FALLO: no se pudo crear Task_Web.");
    }
}

void loop() {
    static TickType_t ultimoDespertar = xTaskGetTickCount();
    static uint64_t inicioAnteriorUs = 0;
    static uint32_t contadorDiagnostico = 0;
    const uint64_t inicioUs = static_cast<uint64_t>(esp_timer_get_time());
    const uint32_t periodoUs = inicioAnteriorUs == 0 ? CONTROL_LOOP_PERIOD_US
        : static_cast<uint32_t>(inicioUs - inicioAnteriorUs);
    inicioAnteriorUs = inicioUs;

    ejecutarCicloControl();

    const uint64_t finUs = static_cast<uint64_t>(esp_timer_get_time());
    const SensorSnapshot& snap = snapshotSensoresControl();
    const uint32_t duracionUs = static_cast<uint32_t>(finUs - inicioUs);
    const uint32_t jitterUs = periodoUs > CONTROL_LOOP_PERIOD_US
        ? periodoUs - CONTROL_LOOP_PERIOD_US : CONTROL_LOOP_PERIOD_US - periodoUs;
    const uint32_t antiguedadUs = finUs >= snap.timestamp_us
        ? static_cast<uint32_t>(finUs - snap.timestamp_us) : 0;
    const uint32_t perdidos = periodoUs > CONTROL_LOOP_PERIOD_US * 2U
        ? (periodoUs / CONTROL_LOOP_PERIOD_US) - 1U : 0U;
    registrarCicloControl(periodoUs, jitterUs, duracionUs, antiguedadUs, perdidos);
    if (++contadorDiagnostico >= 100) {
        contadorDiagnostico = 0;
        registrarStackLibre(TareaDiagnosticada::CONTROL,
            uxTaskGetStackHighWaterMark(nullptr));
        LOG_THROTTLED("Heartbeat Control", 100);
    }
    vTaskDelayUntil(&ultimoDespertar, pdMS_TO_TICKS(10));
}

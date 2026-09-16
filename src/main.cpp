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
#include "ControlSeguridad.h"
#include "ControlManual.h"
#include "MemoriaTorque.h"
#include "ControlConexion.h"
#include <esp_timer.h>

TaskHandle_t TaskWebHandle;
QueueHandle_t colaComandos;
volatile bool flag_ESTOP_ISR = false;
volatile int seq_ESTOP_pendiente = 0;

static portMUX_TYPE manualMux = portMUX_INITIALIZER_UNLOCKED;
static portMUX_TYPE conexionControlMux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint8_t manualSolicitud = 0;
static volatile int manualSolicitudSeq = 0;
static volatile bool desconexionControlPendiente = false;
static ManualDriveFrame manualMailbox = {};
static int manualPwmActualL = 0;
static int manualPwmActualR = 0;
static uint32_t manualBeginSolicitadoMs = 0;
static uint32_t manualActivoDesdeMs = 0;
static bool manualEsperandoPrimerFrame = false;
bool solicitarManualBegin(int seq) { portENTER_CRITICAL(&manualMux); manualSolicitud = 1; manualSolicitudSeq = seq; manualBeginSolicitadoMs = millis(); portEXIT_CRITICAL(&manualMux); return true; }
bool solicitarManualEnd(int seq) { portENTER_CRITICAL(&manualMux); manualSolicitud = 2; manualSolicitudSeq = seq; portEXIT_CRITICAL(&manualMux); return true; }
bool solicitarManualDesconexion() { portENTER_CRITICAL(&manualMux); manualSolicitud = 3; manualSolicitudSeq = 0; portEXIT_CRITICAL(&manualMux); return true; }
bool solicitarDesconexionControl() {
    portENTER_CRITICAL(&conexionControlMux);
    desconexionControlPendiente = true;
    portEXIT_CRITICAL(&conexionControlMux);
    return true;
}
bool tomarDesconexionControl() {
    portENTER_CRITICAL(&conexionControlMux);
    const bool pendiente = desconexionControlPendiente;
    desconexionControlPendiente = false;
    portEXIT_CRITICAL(&conexionControlMux);
    return pendiente;
}
bool publicarManualDrive(float throttle, float steering, uint32_t stream, uint32_t frame) {
    portENTER_CRITICAL(&manualMux);
    if (manualMailbox.recibidoMs != 0 && manualMailbox.stream == stream && frame <= manualMailbox.frame) {
        portEXIT_CRITICAL(&manualMux);
        return false;
    }
    manualMailbox = {constrain(throttle, -1.0f, 1.0f), constrain(steering, -1.0f, 1.0f), stream, frame, millis()};
    portEXIT_CRITICAL(&manualMux); return true;
}
bool tomarSolicitudManual(bool& comenzar, bool& terminar, bool& desconexion, int& seq) {
    portENTER_CRITICAL(&manualMux); const uint8_t s = manualSolicitud; seq = manualSolicitudSeq; manualSolicitud = 0; portEXIT_CRITICAL(&manualMux);
    comenzar = s == 1; terminar = s == 2; desconexion = s == 3; return s != 0;
}
bool leerManualDrive(ManualDriveFrame& trama) { portENTER_CRITICAL(&manualMux); trama = manualMailbox; portEXIT_CRITICAL(&manualMux); return trama.recibidoMs != 0; }
void limpiarManualDrive() { portENTER_CRITICAL(&manualMux); manualMailbox = {}; portEXIT_CRITICAL(&manualMux); }
static void reiniciarEstadoManual() {
    limpiarManualDrive();
    manualEsperandoPrimerFrame = false;
    manualActivoDesdeMs = 0;
    manualPwmActualL = manualPwmActualR = 0;
}
const char* faseManualTexto() {
    if (estadoActual != MANUAL) return "idle";
    return manualEsperandoPrimerFrame ? "waiting_first_frame" : "active";
}
uint32_t inicioManualMs() { return manualActivoDesdeMs; }
static const char* detalleBloqueoMovimiento(const char* fallback) {
    if (!pcntInicializados()) return "pcnt_init_failed";
    if (!ControlSeguridad::fuentesPorLadoValidas(encoderConfiableGlobal)) return "enc_no_side";
    if (persistenciaTorquePendiente()) return "torque_persist_pending";
    return fallback;
}
static void procesarSolicitudesManual() {
    bool comenzar = false, terminar = false, desconexion = false; int seq = 0;
    if (!tomarSolicitudManual(comenzar, terminar, desconexion, seq)) return;
    if (desconexion || terminar) { reiniciarEstadoManual(); if (estadoActual == MANUAL) { frenarMotores(); estadoActual = robotCalibrado ? LISTO : DESARMADO; } if (terminar && seq > 0) encolarEvento(EVT_COMPLETED, seq, "manual_end"); else if (desconexion) encolarEvento(EVT_COMPLETED, 0, "manual_disconnected"); }
    if (comenzar) {
        if (estadoActual == LISTO && robotCalibrado && !persistenciaTorquePendiente() &&
            pcntInicializados() &&
            ControlSeguridad::fuentesPorLadoValidas(encoderConfiableGlobal)) {
            portENTER_CRITICAL(&manualMux);
            if (manualMailbox.recibidoMs == 0 ||
                static_cast<int32_t>(manualMailbox.recibidoMs - manualBeginSolicitadoMs) < 0)
                manualMailbox = {};
            portEXIT_CRITICAL(&manualMux);
            manualPwmActualL = manualPwmActualR = 0;
            manualActivoDesdeMs = millis();
            manualEsperandoPrimerFrame = true;
            estadoActual = MANUAL;
            if (seq > 0) encolarEvento(EVT_COMPLETED, seq, "manual_begin");
        } else if (seq > 0) {
            encolarEvento(EVT_REJECTED, seq, detalleBloqueoMovimiento("manual_unavailable"));
        }
    }
}
static void controlarManual() {
    ManualDriveFrame trama;
    if (estadoActual != MANUAL) return;
    const uint32_t ahora = millis();
    const bool tramaValida = leerManualDrive(trama);
    if (manualEsperandoPrimerFrame && !tramaValida) {
        frenarMotores();
        if (ControlManual::esperaPrimerFrameVigente(
                ahora, manualActivoDesdeMs, MANUAL_FIRST_FRAME_GRACE_MS)) return;
        reiniciarEstadoManual();
        estadoActual = robotCalibrado ? LISTO : DESARMADO;
        encolarEvento(EVT_COMPLETED, 0, "manual_first_frame_timeout");
        return;
    }
    if (tramaValida) manualEsperandoPrimerFrame = false;
    if (!tramaValida || !ControlManual::leaseVigente(ahora, trama.recibidoMs, MANUAL_LEASE_MS)) {
        reiniciarEstadoManual();
        frenarMotores();
        estadoActual = robotCalibrado ? LISTO : DESARMADO;
        encolarEvento(EVT_COMPLETED, 0, "manual_lease_expired");
        return;
    }
    const ControlManual::SalidaPWM salida = ControlManual::mezclar(trama.throttle, trama.steering, PWM_MANUAL_MAX_LIMIT);
    manualPwmActualL = ControlManual::acercar(manualPwmActualL, salida.izquierdo, PWM_MANUAL_RAMP_STEP);
    manualPwmActualR = ControlManual::acercar(manualPwmActualR, salida.derecho, PWM_MANUAL_RAMP_STEP);
    aplicarVelocidades(manualPwmActualL, manualPwmActualR);
}

void procesarComandos() {
    procesarSolicitudesManual();
    if (ControlSeguridad::estopSolicitado(flag_ESTOP_ISR)) {
        const int seq = seq_ESTOP_pendiente;
        const int seqInterrumpido = seqActivo;
        flag_ESTOP_ISR = false;
        seq_ESTOP_pendiente = 0;
        reiniciarEstadoManual();
        WatchdogSeguridad.forzarEStop();
        xQueueReset(colaComandos);
        if (seqInterrumpido > 0 && seqInterrumpido != seq) {
            encolarEvento(EVT_FAULT, seqInterrumpido, "stopped_by_estop");
        }
        encolarEvento(EVT_COMPLETED, seq, "estop_latched");
        return;
    }
    ComandoRed cmd;
    while (xQueueReceive(colaComandos, &cmd, 0) == pdTRUE) {
        switch (cmd.tipo) {
            case CMD_CALIBRATE:
                if (!iniciarCalibracion(cmd.seq))
                    encolarEvento(EVT_REJECTED, cmd.seq,
                        !pcntInicializados() ? "pcnt_init_failed" : "cal_unavailable");
                break;
            case CMD_STEP:
                if (!iniciarPaso(cmd.heading, cmd.distanciaCm, cmd.seq, cmd.targetXCm, cmd.targetYCm,
                                 cmd.tieneObjetivoAbsoluto, cmd.modoPaso))
                    encolarEvento(EVT_REJECTED, cmd.seq, detalleBloqueoMovimiento("step_invalid"));
                break;
            case CMD_TURN_TO:
                if (!iniciarGiroAbsoluto(cmd.heading, cmd.seq))
                    encolarEvento(EVT_REJECTED, cmd.seq, detalleBloqueoMovimiento("turn_unavailable"));
                break;
            case CMD_STOP:
                if (estadoActual == MANUAL) {
                    reiniciarEstadoManual();
                }
                cancelarMovimiento("stopped");
                encolarEvento(EVT_COMPLETED, cmd.seq, "stop_ok");
                break;
            case CMD_ESTOP:
                {
                const int seqInterrumpido = seqActivo;
                reiniciarEstadoManual();
                WatchdogSeguridad.forzarEStop();
                xQueueReset(colaComandos);
                if (seqInterrumpido > 0 && seqInterrumpido != cmd.seq)
                    encolarEvento(EVT_FAULT, seqInterrumpido, "stopped_by_estop");
                encolarEvento(EVT_COMPLETED, cmd.seq, "estop_latched");
                return;
                }
                break;
            case CMD_CLEAR_FAULT:
                {
                const ResultadoRearme resultado = WatchdogSeguridad.resetFallo();
                if (resultado == ResultadoRearme::REARMADO)
                    encolarEvento(EVT_COMPLETED, cmd.seq, "fault_cleared");
                else if (resultado == ResultadoRearme::SIN_FALLO_ACTIVO)
                    encolarEvento(EVT_REJECTED, cmd.seq, "no_fault_active");
                else if (resultado == ResultadoRearme::PCNT_NO_DISPONIBLE)
                    encolarEvento(EVT_REJECTED, cmd.seq, "pcnt_init_failed");
                else
                    encolarEvento(EVT_REJECTED, cmd.seq, "motor_output_unavailable");
                xQueueReset(colaComandos);
                return;
                }
            case CMD_RESET_POSE:
                if (estadoActual == LISTO || estadoActual == DESARMADO) {
                    PoseGlobal.reset();
                    resetOrientacionIMU();
                    encolarEvento(EVT_COMPLETED, cmd.seq, "pose_reset");
                } else encolarEvento(EVT_REJECTED, cmd.seq, "busy");
                break;
            case CMD_SET_COMP:
                if (estadoActual != DESARMADO && estadoActual != LISTO) {
                    encolarEvento(EVT_REJECTED, cmd.seq, "busy");
                } else if (cmd.factor >= COMP_FACTOR_MIN && cmd.factor <= COMP_FACTOR_MAX) {
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
        if (pwm_aplicado_L == 0 && pwm_aplicado_R == 0 &&
            estadoActual != EJECUTANDO && estadoActual != CALIBRANDO && estadoActual != MANUAL)
            procesarPersistenciaTorque();
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
    if (tomarDesconexionControl() &&
        ControlConexion::accionAlPerderWebSocket(estadoActual == CALIBRANDO) ==
            ControlConexion::AccionDesconexion::CANCELAR_CALIBRACION) {
        LOG_CORE("CAL: WebSocket perdido; cancelacion segura en Core 1.");
        cancelarMovimiento("cal_connection_lost");
    }
    leerSensoresSincrono();
    const SensorSnapshot& snap = snapshotSensoresControl();
    bool fresco = snap.sequence != ultimoSensorSeq;
    if (fresco) {
        ultimoSensorSeq = snap.sequence;
        WatchdogSeguridad.actualizarSaludEncoders(snap, pwm_aplicado_L, pwm_aplicado_R);
        heading360 = normalizar360(anguloZ);
        PoseGlobal.actualizarOrientacion(snap.imu_deltaZ_rad);
        if (estadoActual == LISTO || estadoActual == DESARMADO) {
            recentrarYawIMUEnReposo();
        }
        PoseGlobal.actualizarOdometria(snap.pulsosFL, snap.pulsosFR, snap.pulsosBL, snap.pulsosBR,
                                       ((estadoActual == EJECUTANDO && enFaseTraslacion()) || estadoActual == MANUAL));
        if (!ControlSeguridad::imuApta(snap.mpu_present, snap.mpu_stale)) {
            if (estadoActual == EJECUTANDO || estadoActual == CALIBRANDO || estadoActual == MANUAL) {
                frenarMotores();
                reiniciarControlRumbo();
                registrarMotivoFinalizacion("mpu_lost");
                estadoActual = FALLO;
                LOG_CORE("FAULT: MPU ausente/obsoleto durante movimiento.");
                encolarEvento(EVT_FAULT, seqActivo, "mpu_lost");
            }
        }
        WatchdogSeguridad.auditarSalud(snap, pwm_aplicado_L, pwm_aplicado_R);
    }
    controlarMovimiento();
    controlarManual();
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
    Serial.printf("\n=== ROBOT S3 v3 | reset=%s ===\n", motivoResetESP32);

    colaComandos = xQueueCreate(4, sizeof(ComandoRed));
    colaEventosRed = xQueueCreate(8, sizeof(EventoRed));
    PoseGlobal.inicializar(WHEEL_DIAMETER_ODOMETRY_CM, ENCODER_PPR);
    if (!setup_Motores()) {
        estadoActual = FALLO;
        strncpy(ultimoFalloDetalle, "motor_setup_error", sizeof(ultimoFalloDetalle) - 1);
        ultimoFalloDetalle[sizeof(ultimoFalloDetalle) - 1] = '\0';
        Serial.printf("FALLO: salida de motores no inicializada (%s).\n", estadoMotores());
    }
    setup_Sensores();
    if (!pcntInicializados() && estadoActual != FALLO) {
        estadoActual = FALLO;
        strncpy(ultimoFalloDetalle, "pcnt_init_failed", sizeof(ultimoFalloDetalle) - 1);
        ultimoFalloDetalle[sizeof(ultimoFalloDetalle) - 1] = '\0';
        registrarMotivoFinalizacion("pcnt_init_failed");
        Serial.println("FALLO: inicializacion PCNT incompleta; movimiento bloqueado.");
    }
    setup_MemoriaTorque();
    setup_Red();
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

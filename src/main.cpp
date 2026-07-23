#include <Arduino.h>
#include <esp_timer.h>
#include <cstdlib>

#include "Cinematica.h"
#include "Comandos.h"
#include "Config.h"
#include "Debug.h"
#include "DiagnosticoRTOS.h"
#include "Mision.h"
#include "Estado.h"
#include "Eventos.h"
#include "Motores.h"
#include "PoseEstimator.h"
#include "Red.h"
#include "Seguridad.h"
#include "Sensores.h"

TaskHandle_t TaskWebHandle;

QueueHandle_t colaComandos;
QueueHandle_t buzonManual;
volatile bool flag_ESTOP_ISR = false;
static char active_test_id[33] = {};

static void guardarDetalleAutoevaluacion(const char* detalle) {
    strncpy(detalleAutoevaluacion, detalle, sizeof(detalleAutoevaluacion) - 1);
    detalleAutoevaluacion[sizeof(detalleAutoevaluacion) - 1] = '\0';
}

static void ejecutarAutoevaluacion(const char* commandId) {
    frenarMotores();
    estadoAutoevaluacion = SELF_TEST_RUNNING;
    guardarDetalleAutoevaluacion("running");

    const bool motoresOk = validarInterlockMotores();
    const bool giroOk = validarControlGiroSeguro();
    const bool watchdogOk = Seguridad::validarLogicaWatchdog();
    const bool rtosOk = validarLogicaDiagnosticoRTOS();
    if (motoresOk && giroOk && watchdogOk && rtosOk) {
        estadoAutoevaluacion = SELF_TEST_PASSED;
        guardarDetalleAutoevaluacion("all_logic_tests_passed");
        if (estadoActual == FAULT_SENSOR) estadoActual = UNCALIBRATED;
        encolarEvento(EVT_COMPLETED, commandId, "self_test_passed");
        return;
    }

    estadoAutoevaluacion = SELF_TEST_FAILED;
    robotCalibrado = false;
    estadoActual = FAULT_SENSOR;
    if (!motoresOk) guardarDetalleAutoevaluacion("motor_interlock_failed");
    else if (!giroOk) guardarDetalleAutoevaluacion("turn_policy_failed");
    else if (!watchdogOk) guardarDetalleAutoevaluacion("watchdog_logic_failed");
    else guardarDetalleAutoevaluacion("control_diagnostics_failed");
    encolarEvento(EVT_FAULT, commandId, detalleAutoevaluacion);
}

static void crearIdDiagnosticoSerial(const char* prefijo, char* destino,
                                     size_t capacidad) {
    if (!destino || capacidad == 0) return;
    snprintf(destino, capacidad, "%s-%lu", prefijo,
             static_cast<unsigned long>(millis()));
    destino[capacidad - 1] = '\0';
}

static void ejecutarOrdenDiagnosticoSerial(const char* linea) {
    if (!linea || !linea[0]) return;

    if (strcmp(linea, "ESTOP") == 0) {
        WatchdogSeguridad.forzarEStop();
        encolarEvento(EVT_FAULT, comandoActivoId, "serial_estop");
        Serial.println("[SERIAL] E-STOP enclavado.");
        return;
    }
    if (strcmp(linea, "STOP") == 0) {
        cancelarMovimiento("serial_stop");
        Serial.println("[SERIAL] Movimiento detenido.");
        return;
    }
    if (strcmp(linea, "REARMAR") == 0) {
        WatchdogSeguridad.resetFallo();
        Serial.println("[SERIAL] Solicitud de rearme procesada.");
        return;
    }
    if (strcmp(linea, "ESTADO") == 0) {
        const SensorSnapshot &s = snapshotSensoresControl();
        Serial.printf(
            "[SERIAL] estado=%d PWM solicitado=%d/%d aplicado=%d/%d "
            "ticks FL/FR/BL/BR=%lld/%lld/%lld/%lld yaw=%+.2f gyro=%+.4f.\n",
            static_cast<int>(estadoActual), pwm_solicitado_L, pwm_solicitado_R,
            pwm_aplicado_L, pwm_aplicado_R,
            static_cast<long long>(s.pulsosFL), static_cast<long long>(s.pulsosFR),
            static_cast<long long>(s.pulsosBL), static_cast<long long>(s.pulsosBR),
            obtenerYawIMUDeg(), s.gyro_z_filtrado_rad_s);
        return;
    }

    char id[40] = {};
    if (strcmp(linea, "CALIBRAR") == 0) {
        crearIdDiagnosticoSerial("serial-cal", id, sizeof(id));
        const bool aceptada = iniciarCalibracion(id);
        Serial.printf("[SERIAL] CALIBRAR %s (estado=%d).\n",
                      aceptada ? "aceptado" : "rechazado",
                      static_cast<int>(estadoActual));
        return;
    }

    if (strncmp(linea, "GIRO ", 5) == 0) {
        char* fin = nullptr;
        const float angulo = strtof(linea + 5, &fin);
        while (fin && *fin == ' ') ++fin;
        if (!fin || fin == linea + 5 || *fin != '\0' || !isfinite(angulo) ||
            fabsf(angulo) > 360.0f || fabsf(angulo) < 0.001f) {
            Serial.println("[SERIAL] GIRO rechazado: use GIRO <-360..360>.");
            return;
        }
        crearIdDiagnosticoSerial("serial-turn", id, sizeof(id));
        const bool aceptado = iniciarGiroRelativo(
            angulo, id, ModoGiroSolicitado::AUTO);
        Serial.printf("[SERIAL] GIRO %+.2f %s (estado=%d).\n", angulo,
                      aceptado ? "aceptado" : "rechazado",
                      static_cast<int>(estadoActual));
        return;
    }

    if (strncmp(linea, "PWM ", 4) == 0) {
        int izquierda = 0;
        int derecha = 0;
        unsigned long duracion = 0;
        char sobrante = '\0';
        if (sscanf(linea + 4, "%d %d %lu %c", &izquierda, &derecha,
                   &duracion, &sobrante) != 3 ||
            izquierda < -PWM_SAFE_LIMIT_8BIT || izquierda > PWM_SAFE_LIMIT_8BIT ||
            derecha < -PWM_SAFE_LIMIT_8BIT || derecha > PWM_SAFE_LIMIT_8BIT ||
            duracion < 100 || duracion > 1500) {
            Serial.println("[SERIAL] PWM rechazado: use PWM <L -230..230> <R -230..230> <100..1500 ms>.");
            return;
        }
        ComandoRed cmd = {};
        crearIdDiagnosticoSerial("serial-pwm", cmd.id, sizeof(cmd.id));
        cmd.tipo = CMD_TEST_PWM;
        cmd.pwm_l = izquierda;
        cmd.pwm_r = derecha;
        cmd.tiempo_ms = static_cast<uint32_t>(duracion);
        const bool encolado = xQueueSend(colaComandos, &cmd, 0) == pdTRUE;
        Serial.printf("[SERIAL] PWM %d/%d por %lu ms %s.\n", izquierda,
                      derecha, duracion, encolado ? "encolado" : "rechazado");
        return;
    }

    Serial.println("[SERIAL] Orden desconocida. Use ESTADO, CALIBRAR, GIRO <deg>, PWM <L> <R> <ms>, STOP, ESTOP o REARMAR.");
}

static void procesarDiagnosticoSerial() {
    static char linea[64] = {};
    static size_t usados = 0;
    while (Serial.available() > 0) {
        const char recibido = static_cast<char>(Serial.read());
        if (recibido == '\r') continue;
        if (recibido == '\n') {
            linea[usados] = '\0';
            ejecutarOrdenDiagnosticoSerial(linea);
            usados = 0;
            continue;
        }
        if (usados + 1 < sizeof(linea)) linea[usados++] = recibido;
        else usados = 0;
    }
}

void procesarComandos() {
    procesarDiagnosticoSerial();
    if (flag_ESTOP_ISR) {
        flag_ESTOP_ISR = false;
        abortarMovimientoPorSeguridad("estop_latched");
        WatchdogSeguridad.forzarEStop();
        xQueueReset(colaComandos);
        xQueueReset(buzonManual);
        detenerMisionAutonoma(true);
        return;
    }

    ComandoRed cmd;
    if (estadoActual == MANUAL || estadoActual == IDLE) {
        if (xQueueReceive(buzonManual, &cmd, 0) == pdTRUE &&
            cmd.tipo == CMD_MANUAL && estadoActual != FAULT_SENSOR &&
            estadoActual != ESTOP_LATCHED) {
            if (cmd.pwm_l == 0 && cmd.pwm_r == 0) {
                frenarMotores();
                estadoActual = IDLE;
                return;
            }
            estadoActual = MANUAL;
            WatchdogSeguridad.alimentarWatchdogRed();
            aplicarVelocidades(
                static_cast<int>(cmd.pwm_l * PWM_SCALE_8_TO_10),
                static_cast<int>(cmd.pwm_r * PWM_SCALE_8_TO_10));
        }
    }

    if (xQueueReceive(colaComandos, &cmd, 0) != pdTRUE) return;

    if (cmd.tipo == CMD_STOP) {
        detenerMisionAutonoma(true);
        cancelarMovimiento("stopped");
        xQueueReset(colaComandos);
        encolarEvento(EVT_COMPLETED, cmd.id, "stop_ok");
    } else if (cmd.tipo == CMD_CALIBRATE) {
        if (!iniciarCalibracion(cmd.id)) {
            encolarEvento(EVT_REJECTED, cmd.id, "calibration_unavailable");
        }
    } else if (cmd.tipo == CMD_SELF_TEST) {
        ejecutarAutoevaluacion(cmd.id);
    } else if (cmd.tipo == CMD_CLEAR_FAULT) {
        WatchdogSeguridad.resetFallo();
        encolarEvento(EVT_COMPLETED, cmd.id, "fault_cleared");
    } else if (cmd.tipo == CMD_RESET_POSE && estadoActual == IDLE) {
        frenarMotores();
        PoseGlobal.reset();
        resetOrientacionIMU();
        encolarEvento(EVT_COMPLETED, cmd.id, "pose_reset");
    } else if (cmd.tipo == CMD_CLEAR_ROUTE) {
        frenarMotores();
        cancelarMovimiento("mission_cleared");
        liberarMisionAutonoma();
        estadoActual = IDLE;
        xQueueReset(colaComandos);
        encolarEvento(EVT_COMPLETED, cmd.id, "mission_memory_cleared");
    } else if (estadoActual == FAULT_SENSOR || estadoActual == ESTOP_LATCHED) {
        LOG_CORE("Comando ignorado por estado de FALLO.");
    } else if (cmd.tipo == CMD_MOVE_ABS && estadoActual == IDLE) {
        if (!calcularDestino(cmd.arg_x, cmd.arg_y, cmd.id)) {
            encolarEvento(EVT_REJECTED, cmd.id, motivoUltimoRechazoMovimiento());
        }
    } else if (cmd.tipo == CMD_DRIVE && estadoActual == IDLE) {
        if (!iniciarDistancia(cmd.distancia_mm, cmd.id)) {
            encolarEvento(EVT_REJECTED, cmd.id, "drive_invalid");
        }
    } else if (cmd.tipo == CMD_TURN && estadoActual == IDLE) {
        if (!iniciarGiroRelativo(cmd.angulo_deg, cmd.id, cmd.modo_giro)) {
            encolarEvento(EVT_REJECTED, cmd.id, "turn_invalid");
        }
    } else if (cmd.tipo == CMD_TEST_PWM &&
               (estadoActual == IDLE || estadoActual == UNCALIBRATED)) {
        const int safeL = static_cast<int>(
            constrain(cmd.pwm_l, -PWM_SAFE_LIMIT_8BIT, PWM_SAFE_LIMIT_8BIT) * PWM_SCALE_8_TO_10);
        const int safeR = static_cast<int>(
            constrain(cmd.pwm_r, -PWM_SAFE_LIMIT_8BIT, PWM_SAFE_LIMIT_8BIT) * PWM_SCALE_8_TO_10);
        unsigned long duracion = cmd.tiempo_ms == 0 ? 500 : cmd.tiempo_ms;
        if (duracion > 1500) duracion = 1500;
        estadoActual = TESTING;
        strncpy(active_test_id, cmd.id, sizeof(active_test_id) - 1);
        active_test_id[sizeof(active_test_id) - 1] = '\0';
        strncpy(comandoActivoId, cmd.id, sizeof(comandoActivoId) - 1);
        comandoActivoId[sizeof(comandoActivoId) - 1] = '\0';
        strncpy(comandoActivoNombre, "test_pwm", sizeof(comandoActivoNombre) - 1);
        comandoActivoNombre[sizeof(comandoActivoNombre) - 1] = '\0';
        pwm_target_l = safeL;
        pwm_target_r = safeR;
        tiempoTestingInicio = millis();
        duracionTesting = duracion;
        WatchdogSeguridad.alimentarWatchdogRed();
        aplicarVelocidades(pwm_target_l, pwm_target_r);
    }
}

void Task_Web(void*) {
    LOG_CORE("Task_Web iniciada en Core 0");
    for (;;) {
        registrarStackLibre(TareaDiagnosticada::WEB, uxTaskGetStackHighWaterMark(nullptr));
        procesarWebSockets();
        pushTelemetria();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void ejecutarControlFisico(const SensorSnapshot &snap) {
    PoseGlobal.actualizarOdometria(
        snap.pulsosFL, snap.pulsosFR, snap.pulsosBL, snap.pulsosBR, estadoActual);

    if (estadoActual == UNCALIBRATED && snap.mpu_present &&
        snap.mpu_calibrated && !snap.mpu_stale &&
        habilitarControlSimpleSinCalibracion()) {
        Serial.println("[CONTROL] Listo sin calibracion motriz: polaridad fija del ensayo aprobado.");
    }

    if (snap.mpu_present && snap.mpu_calibrated && !snap.mpu_stale) {
        PoseGlobal.actualizarOrientacion(snap.imu_deltaZ_rad);
    } else if (estadoActual == AVANZANDO || estadoActual == GIRANDO ||
               estadoActual == RECOVERING || estadoActual == CALIBRATING) {
        abortarMovimientoPorSeguridad("mpu_lost");
        estadoActual = FAULT_SENSOR;
        LOG_CORE("FAULT: MPU6050 ausente u obsoleto durante navegacion.");
    }

    procesarComandos();
    WatchdogSeguridad.auditarSalud(snap, pwm_aplicado_L, pwm_aplicado_R);

    if (estadoActual == GIRANDO || estadoActual == RECOVERING) {
        controlarGiro();
    } else if (estadoActual == CALIBRATING) {
        controlarCalibracion();
    } else if (estadoActual == ESPERANDO_ESTABILIZACION) {
        if (millis() - tiempoEspera >= ESTABILIZACION_MS) {
            resetOrientacionIMU();
            tiempoAnterior = millis();
            tiempoInicioAvance = millis();
            estadoActual = AVANZANDO;
        }
    } else if (estadoActual == AVANZANDO) {
        controlarAvance();
    } else if (estadoActual == TESTING) {
        if (millis() - tiempoTestingInicio > duracionTesting) {
            frenarMotores();
            estadoActual = robotCalibrado ? IDLE : UNCALIBRATED;
            encolarEvento(EVT_COMPLETED, active_test_id, "test_done");
            active_test_id[0] = '\0';
        } else {
            aplicarVelocidades(pwm_target_l, pwm_target_r);
        }
    }
    procesarMisionAutonoma();

    if ((estadoActual == IDLE || estadoActual == UNCALIBRATED) &&
        !comandoActivoId[0] && pwm_aplicado_L == 0 && pwm_aplicado_R == 0 &&
        recentrarYawIMUEnReposo()) {
        LOG_CORE("Yaw MPU recentrado por multiplos de 360 grados en reposo.");
    }

    static unsigned long ultimoProgresoMs = 0;
    if (comandoActivoId[0] && millis() - ultimoProgresoMs >= 500) {
        ultimoProgresoMs = millis();
        encolarEvento(EVT_PROGRESS, comandoActivoId, "motion_progress", progresoComando);
    }
}

void setup() {
    setup_MotorPinsLow();

    Serial.begin(115200);
    delay(300);
    inicializarDiagnosticoRTOS();
    Serial.println("\n[BOOT] Firmware Robot S3 con super-ciclo determinista.");
    Serial.printf("[BOOT] Motivo de reinicio: %s\n", motivoResetESP32);

    colaComandos = xQueueCreate(10, sizeof(ComandoRed));
    buzonManual = xQueueCreate(1, sizeof(ComandoRed));
    colaEventosRed = xQueueCreate(8, sizeof(EventoRed));

    PoseGlobal.inicializar(WHEEL_DIAMETER_ODOMETRY_CM, ENCODER_PPR);
    inicializarPersistenciaMision();
    setup_Motores();
    setup_Red();
    setup_Sensores();

    const BaseType_t webCreada = xTaskCreatePinnedToCore(
        Task_Web, "Red", TASK_WEB_STACK_BYTES, nullptr, 1, &TaskWebHandle, 0);
    const bool controlEnCore1 = xPortGetCoreID() == 1;
    vTaskPrioritySet(nullptr, CONTROL_LOOP_PRIORITY);
    registrarResultadoArquitectura(webCreada, controlEnCore1);

    if (!creacionTareasOk) {
        frenarMotores();
        robotCalibrado = false;
        estadoActual = FAULT_SENSOR;
        Serial.printf("[CONTROL] FALLO CRITICO: %s. Motores bloqueados.\n", falloCreacionTarea);
        for (;;) {
            frenarMotores();
            delay(1000);
        }
    }

    Serial.printf("[CONTROL] Web Core 0; loop Core %d prioridad %u, periodo %lu us, CPU %u MHz.\n",
                  xPortGetCoreID(), static_cast<unsigned>(uxTaskPriorityGet(nullptr)),
                  static_cast<unsigned long>(CONTROL_LOOP_PERIOD_US), getCpuFrequencyMhz());
}

void loop() {
    static int64_t proximoCicloUs = 0;
    static int64_t inicioAnteriorUs = 0;
    static uint32_t ultimoReporteMs = 0;

    int64_t ahoraUs = esp_timer_get_time();
    if (proximoCicloUs == 0) proximoCicloUs = ahoraUs;
    while (ahoraUs < proximoCicloUs) {
        const int64_t restanteUs = proximoCicloUs - ahoraUs;
        if (restanteUs > 2000) vTaskDelay(pdMS_TO_TICKS(1));
        else if (restanteUs > 0) delayMicroseconds(static_cast<uint32_t>(restanteUs));
        ahoraUs = esp_timer_get_time();
    }

    const int64_t programadoUs = proximoCicloUs;
    const int64_t inicioUs = ahoraUs;
    proximoCicloUs += CONTROL_LOOP_PERIOD_US;
    uint32_t deadlinesPerdidos = 0;
    if (inicioUs >= proximoCicloUs) {
        deadlinesPerdidos = static_cast<uint32_t>(
            (inicioUs - proximoCicloUs) / CONTROL_LOOP_PERIOD_US + 1);
        proximoCicloUs += static_cast<int64_t>(deadlinesPerdidos) * CONTROL_LOOP_PERIOD_US;
    }

    const SensorSnapshot snap = leerSensoresSincrono();
    ejecutarControlFisico(snap);

    const int64_t finUs = esp_timer_get_time();
    const uint32_t periodoUs = inicioAnteriorUs == 0
        ? CONTROL_LOOP_PERIOD_US : static_cast<uint32_t>(inicioUs - inicioAnteriorUs);
    inicioAnteriorUs = inicioUs;
    const uint32_t jitterUs = static_cast<uint32_t>(inicioUs - programadoUs);
    const uint32_t duracionUs = static_cast<uint32_t>(finUs - inicioUs);
    const uint32_t antiguedadUs = static_cast<uint32_t>(finUs - snap.timestamp_us);
    registrarCicloControl(periodoUs, jitterUs, duracionUs, antiguedadUs, deadlinesPerdidos);
    registrarStackLibre(TareaDiagnosticada::CONTROL, uxTaskGetStackHighWaterMark(nullptr));

    if (millis() - ultimoReporteMs >= 2000) {
        ultimoReporteMs = millis();
        Serial.printf("[CONTROL] periodo=%lu us jitterMax=%lu us duracionMax=%lu us muestraMax=%lu us deadlines=%lu stack Web/Control=%lu/%lu%s\n",
                      static_cast<unsigned long>(periodoControlUltimoUs),
                      static_cast<unsigned long>(jitterControlMaxUs),
                      static_cast<unsigned long>(duracionControlMaxUs),
                      static_cast<unsigned long>(antiguedadMuestraMaxUs),
                      static_cast<unsigned long>(deadlinesControlPerdidos),
                      static_cast<unsigned long>(stackMinimoWebBytes),
                      static_cast<unsigned long>(stackMinimoControlBytes),
                      stackRTOSBajo() ? " ALERTA_STACK_BAJO" : "");
    }
}

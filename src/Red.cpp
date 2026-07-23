#include "Red.h"
#include "Config.h"
#include "Estado.h"
#include "Debug.h"
#include "Comandos.h"
#include "Motores.h"
#include "PoseEstimator.h"
#include "Sensores.h"
#include "Eventos.h"
#include "Seguridad.h"
#include "Cinematica.h"
#include "DiagnosticoRTOS.h"
#include "Mision.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <ctype.h>

#define MAX_WS_MSG 4096

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

static char msg_buf[MAX_WS_MSG + 1];
static unsigned long ultimoIntentoApMs = 0;
static bool servidorIniciado = false;

static bool puntoAccesoDisponible() {
  const wifi_mode_t modo = WiFi.getMode();
  const bool modoAp = modo == WIFI_AP || modo == WIFI_AP_STA;
  return modoAp && WiFi.softAPIP() != IPAddress(0, 0, 0, 0);
}

static bool iniciarPuntoAcceso() {
  ultimoIntentoApMs = millis();
  WiFi.setSleep(false);
  if (!WiFi.mode(WIFI_AP)) {
    Serial.println("[RED] ERROR: no fue posible activar WIFI_AP.");
    return false;
  }

  // Canal 6 y dos estaciones coinciden con la configuracion fisica aprobada.
  const bool iniciado = WiFi.softAP(ssid_AP, password_AP, 6, false, 2);
  if (!iniciado || !puntoAccesoDisponible()) {
    Serial.printf("[RED] ERROR: softAP fallo para SSID '%s'. Se reintentara.\n", ssid_AP);
    return false;
  }

  Serial.printf("[RED] AP LISTO | SSID=%s | IP=%s | canal=6 | MAC=%s\n",
                ssid_AP,
                WiFi.softAPIP().toString().c_str(),
                WiFi.softAPmacAddress().c_str());
  return true;
}

static int pwmA8Bits(int valor10Bits) {
    const float convertido = valor10Bits / PWM_SCALE_8_TO_10;
    return static_cast<int>(convertido >= 0.0f ? convertido + 0.5f : convertido - 0.5f);
}

const char* estadoTexto() {
    switch (estadoActual) {
    case UNCALIBRATED: return "UNCALIBRATED";
    case CALIBRATING: return "CALIBRATING";
    case IDLE: return "IDLE";
    case GIRANDO: return "GIRANDO";
    case ESPERANDO_ESTABILIZACION: return "ESPERANDO";
    case AVANZANDO: return "AVANZANDO";
    case MANUAL: return "MANUAL";
    case TESTING: return "TESTING";
    case ESTOP_LATCHED: return "ESTOP_LATCHED";
    case SAFE_STOP_COMMS: return "SAFE_STOP";
    case FAULT_SENSOR: return "FAULT";
    case RECOVERING: return "RECOVERING";
    default: return "UNKNOWN";
  }
}

static const char* autoevaluacionTexto() {
    switch (estadoAutoevaluacion) {
        case SELF_TEST_RUNNING: return "RUNNING";
        case SELF_TEST_PASSED: return "PASSED";
        case SELF_TEST_FAILED: return "FAILED";
        default: return "NOT_RUN";
    }
}

uint32_t active_client_id = 0;
static bool controller_handshake = false;

static AsyncWebSocketClient* clienteControlador() {
    return active_client_id ? ws.client(active_client_id) : nullptr;
}

static bool idValido(const char* id) {
    if (!id || strlen(id) != 32) return false;
    for (size_t i = 0; i < 32; ++i) if (!isxdigit(static_cast<unsigned char>(id[i]))) return false;
    return true;
}

static void copiarId(JsonVariantConst value, char* destino, size_t capacidad) {
    destino[0] = '\0';
    if (value.is<const char*>()) {
        strncpy(destino, value.as<const char*>(), capacidad - 1);
        destino[capacidad - 1] = '\0';
    } else if (value.is<unsigned long>()) {
        snprintf(destino, capacidad, "%lu", value.as<unsigned long>());
    }
}

static ComandoRed parsearComando(JsonObjectConst doc) {
    ComandoRed cmd = {};
    cmd.tipo = CMD_INVALID;
    const bool protocol_v1 = doc["v"] == 1 && doc["type"] == "command";
    if (!protocol_v1) return cmd;
    const char* name = doc["name"];
    if (!name) return cmd;
    copiarId(doc["id"], cmd.id, sizeof(cmd.id));
    if (!idValido(cmd.id)) { cmd.id[0] = '\0'; return cmd; }
    JsonObjectConst args = doc["payload"].as<JsonObjectConst>();

    if (!strcmp(name, "calibrate")) cmd.tipo = CMD_CALIBRATE;
    else if (!strcmp(name, "estop")) cmd.tipo = CMD_ESTOP;
    else if (!strcmp(name, "stop")) cmd.tipo = CMD_STOP;
    else if (!strcmp(name, "reset_pose")) cmd.tipo = CMD_RESET_POSE;
    else if (!strcmp(name, "clear_fault")) cmd.tipo = CMD_CLEAR_FAULT;
    else if (!strcmp(name, "self_test")) cmd.tipo = CMD_SELF_TEST;
    else if (!strcmp(name, "mission_clear")) cmd.tipo = CMD_CLEAR_ROUTE;
    else if (!strcmp(name, "mission_start")) {
        copiarId(args["mission_id"], cmd.mission_id, sizeof(cmd.mission_id));
        cmd.mission_revision = args["revision"] | 0;
        if (idValido(cmd.mission_id) && cmd.mission_revision > 0) cmd.tipo = CMD_MISSION_START;
    }
    else if (!strcmp(name, "manual") || !strcmp(name, "test_pwm")) {
        if (!args["l"].is<int>() || !args["r"].is<int>()) return cmd;
        const int left = args["l"];
        const int right = args["r"];
        if (left < -PWM_SAFE_LIMIT_8BIT || left > PWM_SAFE_LIMIT_8BIT ||
            right < -PWM_SAFE_LIMIT_8BIT || right > PWM_SAFE_LIMIT_8BIT) return cmd;
        cmd.pwm_l = left;
        cmd.pwm_r = right;
        cmd.tipo = !strcmp(name, "manual") ? CMD_MANUAL : CMD_TEST_PWM;
        if (cmd.tipo == CMD_TEST_PWM) {
            cmd.tiempo_ms = args["dur_ms"] | 500;
            if (cmd.tiempo_ms < 10 || cmd.tiempo_ms > 1500) cmd.tipo = CMD_INVALID;
        }
    } else if (!strcmp(name, "move")) {
        if (!args["x_mm"].is<float>() || !args["y_mm"].is<float>()) return cmd;
        cmd.arg_x = args["x_mm"].as<float>() / 10.0f;
        cmd.arg_y = args["y_mm"].as<float>() / 10.0f;
        if (isfinite(cmd.arg_x) && isfinite(cmd.arg_y)) cmd.tipo = CMD_MOVE_ABS;
    } else if (!strcmp(name, "drive")) {
        if (!args["distance_mm"].is<float>()) return cmd;
        cmd.distancia_mm = args["distance_mm"].as<float>();
        if (isfinite(cmd.distancia_mm) && cmd.distancia_mm != 0.0f && fabsf(cmd.distancia_mm) <= MAX_AUTONOMOUS_SEGMENT_MM) cmd.tipo = CMD_DRIVE;
    } else if (!strcmp(name, "turn")) {
        if (!args["angle_deg"].is<float>()) return cmd;
        cmd.angulo_deg = args["angle_deg"].as<float>();
        const char* modo = args["mode"] | "auto";
        if (!strcmp(modo, "auto")) cmd.modo_giro = ModoGiroSolicitado::AUTO;
        else if (!strcmp(modo, "pivot")) cmd.modo_giro = ModoGiroSolicitado::PIVOT;
        else if (!strcmp(modo, "arc_left_active")) cmd.modo_giro = ModoGiroSolicitado::ARC_LEFT_ACTIVE;
        else if (!strcmp(modo, "arc_right_active")) cmd.modo_giro = ModoGiroSolicitado::ARC_RIGHT_ACTIVE;
        else return cmd;
        if (isfinite(cmd.angulo_deg) && fabsf(cmd.angulo_deg) <= 360.0f) cmd.tipo = CMD_TURN;
    }
    return cmd;
}

static void enviarResultado(AsyncWebSocketClient* client, const char* tipo,
                            const char* id = nullptr, const char* cmd = nullptr,
                            const char* reason = nullptr, const char* detail = nullptr) {
    StaticJsonDocument<256> doc;
    doc["v"] = 1;
    doc["type"] = tipo;
    if (id && id[0]) doc["id"] = id;
    if (cmd && cmd[0]) doc["cmd"] = cmd;
    if (reason && reason[0]) doc["reason"] = reason;
    if (detail && detail[0]) doc["detail"] = detail;
    char respuesta[256];
    const size_t escritos = serializeJson(doc, respuesta, sizeof(respuesta));
    if (escritos > 0 && escritos < sizeof(respuesta) - 1) client->text(respuesta);
}

static const char* nombreComando(TipoComando tipo) {
    switch (tipo) {
        case CMD_CALIBRATE: return "calibrate";
        case CMD_ESTOP: return "estop";
        case CMD_STOP: return "stop";
        case CMD_RESET_POSE: return "reset_pose";
        case CMD_CLEAR_FAULT: return "clear_fault";
        case CMD_MOVE_ABS: return "move";
        case CMD_DRIVE: return "drive";
        case CMD_TURN: return "turn";
        case CMD_MANUAL: return "manual";
        case CMD_TEST_PWM: return "test_pwm";
        case CMD_SELF_TEST: return "self_test";
        case CMD_MISSION_START: return "mission_start";
        case CMD_CLEAR_ROUTE: return "mission_clear";
        default: return "";
    }
}

static void procesarEventosRed() {
    EventoRed evt;
    while (xQueueReceive(colaEventosRed, &evt, 0) == pdTRUE) {
        AsyncWebSocketClient* destino = clienteControlador();
        if (destino == nullptr || !destino->canSend()) continue;
        StaticJsonDocument<192> doc;
        doc["v"] = 1;
        if (evt.cmd_id[0]) doc["id"] = evt.cmd_id;
        if (evt.detalle[0]) doc["detail"] = evt.detalle;
        switch (evt.tipo) {
            case EVT_COMPLETED:
                doc["type"] = "completed";
                break;
            case EVT_REJECTED:
                doc["type"] = "rejected";
                doc["reason"] = evt.detalle;
                break;
            case EVT_FAULT:
                doc["type"] = "fault";
                break;
            case EVT_PROGRESS:
                doc["type"] = "progress";
                doc["progress"] = evt.progreso;
                break;
            case EVT_TELEMETRY:
                break;
            default:
                continue;
        }
        char respuesta[192];
        const size_t escritos = serializeJson(doc, respuesta, sizeof(respuesta));
        if (escritos > 0 && escritos < sizeof(respuesta) - 1) destino->text(respuesta);
    }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    // Solo existe una computadora controladora. Si Windows cambia de red y
    // vuelve antes de que AsyncTCP notifique la baja anterior, la conexion
    // vieja queda obsoleta: cerrarla permite una toma de control atomica sin
    // encerrar a Python en una sucesion de resets TCP 10054.
    if (active_client_id != 0 && active_client_id != client->id()) {
      AsyncWebSocketClient* anterior = ws.client(active_client_id);
      Serial.printf("WS cliente #%u sustituye conexion obsoleta #%u.\n",
                    client->id(), active_client_id);
      active_client_id = client->id();
      controller_handshake = false;
      if (anterior != nullptr) anterior->close();
    } else {
      active_client_id = client->id();
      controller_handshake = false;
    }

    Serial.printf("WS cliente #%u conectado. Tomando control.\n", client->id());
    StaticJsonDocument<768> hello;
    hello["v"] = 1; hello["type"] = "hello"; hello["protocol"] = PROTOCOL_NAME; hello["role"] = "robot";
    hello["calibrated"] = robotCalibrado;
    hello["active_command_id"] = comandoActivoId[0] ? comandoActivoId : nullptr;
    JsonObject mission = hello.createNestedObject("mission");
    mission["id"] = idMisionAutonoma();
    mission["revision"] = revisionMisionAutonoma();
    mission["state"] = estadoMisionAutonoma();
    mission["completed_steps"] = pasosMisionCompletados();
    mission["active_step"] = pasoMisionActual();
    mission["total_steps"] = totalPasosMision();
    mission["active_step_id"] = idPasoMisionActual();
    mission["interrupted"] = misionAutonomaInterrumpida();
    JsonObject terminal = hello.createNestedObject("last_terminal");
    if (ultimoTerminalId[0]) { terminal["v"] = 1; terminal["type"] = ultimoTerminalTipo; terminal["id"] = ultimoTerminalId; terminal["detail"] = ultimoTerminalDetalle; }
    char saludo[768]; serializeJson(hello, saludo, sizeof(saludo)); client->text(saludo);
  } else if (type == WS_EVT_DISCONNECT) {
    if (client->id() == active_client_id) {
      Serial.printf("WS cliente principal #%u desconectado.\n", client->id());
      active_client_id = 0;
      controller_handshake = false;
    }
  } else if (type == WS_EVT_DATA) {
    if (client->id() != active_client_id) return;

    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->opcode != WS_TEXT || !info->final || info->index != 0 || info->len != len) {
        enviarResultado(client, "rejected", nullptr, nullptr, "fragmented_message");
        return;
    }

    if (len > MAX_WS_MSG) {
        enviarResultado(client, "rejected", nullptr, nullptr, "msg_too_large");
        return;
    }

    memcpy(msg_buf, data, len);
    msg_buf[len] = '\0';

    StaticJsonDocument<MAX_WS_MSG> doc;
    if (deserializeJson(doc, msg_buf) != DeserializationError::Ok || !doc.is<JsonObject>()) {
        enviarResultado(client, "rejected", nullptr, nullptr, "invalid_json");
        return;
    }
    JsonObjectConst root = doc.as<JsonObjectConst>();
    const char* message_type = root["type"] | "";

    if (root["v"] == 1 && !strcmp(message_type, "heartbeat")) {
        // Heartbeat confirma que el bus Wi-Fi sigue vivo, pero no renueva por
        // si solo la orden manual: esa lease exige mensajes manual repetidos.
        WatchdogSeguridad.registrarHeartbeat();
        StaticJsonDocument<96> heartbeat;
        heartbeat["v"] = 1;
        heartbeat["type"] = "heartbeat_ack";
        if (root.containsKey("ts_ms")) heartbeat["ts_ms"] = root["ts_ms"];
        char respuesta[96];
        serializeJson(heartbeat, respuesta, sizeof(respuesta));
        client->text(respuesta);
        return;
    }

    if (root["v"] == 1 && !strcmp(message_type, "hello")) {
        if (strcmp(root["protocol"] | "", PROTOCOL_NAME)) {
            enviarResultado(client, "rejected", nullptr, nullptr, "protocol_mismatch");
            client->close();
            return;
        }
        controller_handshake = true;
        StaticJsonDocument<384> hello;
        hello["v"] = 1;
        hello["type"] = "hello_ack";
        hello["protocol"] = PROTOCOL_NAME;
        JsonObject mission = hello.createNestedObject("mission");
        mission["id"] = idMisionAutonoma();
        mission["revision"] = revisionMisionAutonoma();
        mission["state"] = estadoMisionAutonoma();
        mission["completed_steps"] = pasosMisionCompletados();
        mission["active_step"] = pasoMisionActual();
        mission["total_steps"] = totalPasosMision();
        mission["active_step_id"] = idPasoMisionActual();
        mission["interrupted"] = misionAutonomaInterrumpida();
        char respuesta[384];
        serializeJson(hello, respuesta, sizeof(respuesta));
        client->text(respuesta);
        return;
    }

    if (!controller_handshake) {
        enviarResultado(client, "rejected", nullptr, nullptr, "handshake_required");
        return;
    }

    if (root["v"] != 1 || strcmp(message_type, "command")) {
        enviarResultado(client, "rejected", nullptr, nullptr, "v1_command_required");
        return;
    }

    const char* requestedName = root["name"] | "";
    if (!strcmp(requestedName, "mission_upload")) {
        const char* incomingId = root["id"] | "";
        JsonObjectConst args = root["payload"].as<JsonObjectConst>();
        const char* missionId = args["mission_id"] | "";
        const uint32_t revision = args["revision"] | 0;
        JsonArrayConst points = args["points"].as<JsonArrayConst>();
        if (!idValido(incomingId) || !idValido(missionId) || revision == 0 ||
            points.isNull() || points.size() == 0 || points.size() > MAX_MISSION_SEGMENTS) {
            enviarResultado(client, "rejected", incomingId, requestedName, "invalid_mission");
            return;
        }
        PuntoMision parsed[MAX_MISSION_SEGMENTS] = {};
        size_t count = 0;
        for (JsonObjectConst point : points) {
            const char* stepId = point["step_id"] | "";
            if (!point["x_mm"].is<float>() || !point["y_mm"].is<float>() || !idValido(stepId)) {
                enviarResultado(client, "rejected", incomingId, requestedName, "invalid_mission_point");
                return;
            }
            parsed[count].x_cm = point["x_mm"].as<float>() / 10.0f;
            parsed[count].y_cm = point["y_mm"].as<float>() / 10.0f;
            strncpy(parsed[count].step_id, stepId, sizeof(parsed[count].step_id) - 1);
            for (size_t previous = 0; previous < count; ++previous) {
                if (!strcmp(parsed[previous].step_id, parsed[count].step_id)) {
                    enviarResultado(client, "rejected", incomingId, requestedName, "duplicate_mission_step_id");
                    return;
                }
            }
            ++count;
        }
        if (misionAutonomaCoincide(missionId, revision, parsed, count)) {
            enviarResultado(client, "accepted", incomingId, requestedName, nullptr, "mission_already_loaded");
            enviarResultado(client, "completed", incomingId, requestedName, nullptr, "mission_upload_idempotent");
            return;
        }
        if (!cargarMisionAutonoma(missionId, revision, parsed, count)) {
            enviarResultado(client, "rejected", incomingId, requestedName, "mission_upload_unavailable");
            return;
        }
        enviarResultado(client, "accepted", incomingId, requestedName);
        enviarResultado(client, "completed", incomingId, requestedName, nullptr, "mission_uploaded");
        return;
    }

    const char* incoming_id = root["id"] | "";
    if (idValido(incoming_id) && !strcmp(incoming_id, comandoActivoId)) {
        enviarResultado(client, "queued", incoming_id, comandoActivoNombre, nullptr, "already_active");
        return;
    }
    if (idValido(incoming_id) && !strcmp(incoming_id, ultimoTerminalId)) {
        enviarResultado(client, ultimoTerminalTipo, incoming_id, nullptr, nullptr, ultimoTerminalDetalle);
        return;
    }

    ComandoRed cmd = parsearComando(root);

    if (cmd.tipo == CMD_INVALID) {
        enviarResultado(client, "rejected", cmd.id, nullptr, "invalid_command");
        return;
    }

    const char* command_name = nombreComando(cmd.tipo);

    const bool siemprePermitido = cmd.tipo == CMD_ESTOP || cmd.tipo == CMD_STOP ||
                                  cmd.tipo == CMD_CLEAR_FAULT || cmd.tipo == CMD_CALIBRATE ||
                                  cmd.tipo == CMD_SELF_TEST || cmd.tipo == CMD_CLEAR_ROUTE;
    if (!robotCalibrado && !siemprePermitido) {
        enviarResultado(client, "rejected", cmd.id, command_name, "calibration_required");
        return;
    }

    if (cmd.tipo == CMD_ESTOP) {
        flag_ESTOP_ISR = true;
        enviarResultado(client, "accepted", cmd.id, command_name);
        return;
    }

    if (cmd.tipo == CMD_STOP) {
        if (xQueueSend(colaComandos, &cmd, 0) == pdTRUE) enviarResultado(client, "accepted", cmd.id, command_name);
        else enviarResultado(client, "rejected", cmd.id, command_name, "command_queue_full");
        return;
    }

    if (cmd.tipo == CMD_SELF_TEST) {
        if (estadoActual == UNCALIBRATED || estadoActual == IDLE || estadoActual == FAULT_SENSOR) {
            if (xQueueSend(colaComandos, &cmd, 0) == pdTRUE) enviarResultado(client, "queued", cmd.id, command_name);
            else enviarResultado(client, "rejected", cmd.id, command_name, "command_queue_full");
        } else enviarResultado(client, "rejected", cmd.id, command_name, "busy");
        return;
    }

    if (cmd.tipo == CMD_CALIBRATE) {
        // La calibracion motriz ya no existe: este comando sólo recentra pose
        // y yaw con PWM cero, por lo que no depende de la autoevaluacion.
        if (estadoActual == UNCALIBRATED || estadoActual == IDLE) {
            if (xQueueSend(colaComandos, &cmd, 0) == pdTRUE) enviarResultado(client, "queued", cmd.id, command_name);
            else enviarResultado(client, "rejected", cmd.id, command_name, "command_queue_full");
        } else enviarResultado(client, "rejected", cmd.id, command_name, "busy");
        return;
    }

    if (cmd.tipo == CMD_CLEAR_FAULT) {
        if (estadoAutoevaluacion == SELF_TEST_FAILED) {
            enviarResultado(client, "rejected", cmd.id, command_name, "self_test_failed");
            return;
        }
        if (estadoActual == ESTOP_LATCHED || estadoActual == FAULT_SENSOR || estadoActual == SAFE_STOP_COMMS) {
            if (xQueueSend(colaComandos, &cmd, 0) == pdTRUE) enviarResultado(client, "accepted", cmd.id, command_name);
            else enviarResultado(client, "rejected", cmd.id, command_name, "command_queue_full");
        } else {
            enviarResultado(client, "rejected", cmd.id, command_name, "no_fault");
        }
        return;
    }

    if (cmd.tipo == CMD_RESET_POSE) {
        if (estadoActual == IDLE) {
            if (xQueueSend(colaComandos, &cmd, 0) == pdTRUE) enviarResultado(client, "accepted", cmd.id, command_name);
            else enviarResultado(client, "rejected", cmd.id, command_name, "command_queue_full");
        } else {
            enviarResultado(client, "rejected", cmd.id, command_name, "busy");
        }
        return;
    }

    if (cmd.tipo == CMD_CLEAR_ROUTE) {
        if (xQueueSend(colaComandos, &cmd, 0) == pdTRUE) enviarResultado(client, "accepted", cmd.id, command_name);
        else enviarResultado(client, "rejected", cmd.id, command_name, "command_queue_full");
        return;
    }


    if (cmd.tipo == CMD_MISSION_START) {
        if (estadoActual == IDLE && iniciarMisionAutonoma(cmd.id, cmd.mission_id, cmd.mission_revision)) {
            enviarResultado(client, "accepted", cmd.id, command_name);
        } else {
            enviarResultado(client, "rejected", cmd.id, command_name, "mission_start_unavailable");
        }
        return;
    }

    if (cmd.tipo == CMD_MANUAL) {
        if (estadoActual == IDLE || estadoActual == MANUAL) {
            if (xQueueOverwrite(buzonManual, &cmd) == pdTRUE) enviarResultado(client, "accepted", cmd.id, command_name);
            else enviarResultado(client, "rejected", cmd.id, command_name, "manual_mailbox_unavailable");
        } else {
            enviarResultado(client, "rejected", cmd.id, command_name, "busy");
        }
        return;
    }

    if (cmd.tipo == CMD_MOVE_ABS || cmd.tipo == CMD_DRIVE || cmd.tipo == CMD_TURN) {
        if (estadoActual == IDLE) {
            if (xQueueSend(colaComandos, &cmd, 0) == pdTRUE) enviarResultado(client, "queued", cmd.id, command_name);
            else enviarResultado(client, "rejected", cmd.id, command_name, "command_queue_full");
        } else {
            enviarResultado(client, "rejected", cmd.id, command_name, "busy");
        }
        return;
    }

    if (cmd.tipo == CMD_TEST_PWM) {
        if (estadoActual == IDLE) {
            if (xQueueSend(colaComandos, &cmd, 0) == pdTRUE) enviarResultado(client, "queued", cmd.id, command_name);
            else enviarResultado(client, "rejected", cmd.id, command_name, "command_queue_full");
        } else {
            enviarResultado(client, "rejected", cmd.id, command_name, "busy");
        }
        return;
    }
  }
}

void pushTelemetria() {
  static uint32_t ultimaTelemetriaMs = 0;
  if (millis() - ultimaTelemetriaMs < 500) return;
  AsyncWebSocketClient* destino = clienteControlador();
  if (destino == nullptr || !destino->canSend()) return;
  ultimaTelemetriaMs = millis();

  SensorSnapshot snap = {};
  if (!obtenerUltimoSnapshotSensores(snap)) return;

  static char robot_id[24] = {};
  if (!robot_id[0]) {
      const unsigned long long chip_id = ESP.getEfuseMac();
      snprintf(robot_id, sizeof(robot_id), "%s-%012llX", ROBOT_ID_PREFIX, chip_id);
  }

  // Incluye diagnóstico de giro continuo, interlocks y cuatro encoders. Mantener
  // holgura evita que ArduinoJson descarte silenciosamente los últimos campos.
  static StaticJsonDocument<8192> doc;
  doc.clear();
  doc["v"] = 1;
  doc["type"] = "telemetry";
  doc["seq"] = snap.sequence;
  JsonObject payload = doc.createNestedObject("payload");
  payload["uptime_ms"] = millis();
  payload["robot_id"] = robot_id;
  payload["firmware_version"] = FIRMWARE_VERSION;
  payload["x_mm"] = PoseGlobal.getX() * 10.0f;
  payload["y_mm"] = PoseGlobal.getY() * 10.0f;
  payload["yaw_deg"] = PoseGlobal.getThetaDeg();
  payload["heading_deg"] = PoseGlobal.getThetaDeg();
  payload["imu_yaw_unwrapped_deg"] = snap.yaw_integrado_deg;
  payload["imu_yaw_recenter_count"] = cantidadRecentradosYawIMU();
  payload["state"] = estadoTexto();
  payload["calibrated"] = robotCalibrado;
  payload["degraded_mode"] = modoDegradado;
  JsonObject mission = payload.createNestedObject("mission");
  mission["id"] = idMisionAutonoma();
  mission["revision"] = revisionMisionAutonoma();
  mission["state"] = estadoMisionAutonoma();
  mission["completed_steps"] = pasosMisionCompletados();
  mission["active_step"] = pasoMisionActual();
  mission["total_steps"] = totalPasosMision();
  mission["active_step_id"] = idPasoMisionActual();
  mission["interrupted"] = misionAutonomaInterrumpida();
  JsonObject rtos = payload.createNestedObject("rtos");
  rtos["architecture"] = "web_core0_control_superloop_core1";
  rtos["reset_reason"] = motivoResetESP32;
  rtos["tasks_created_ok"] = creacionTareasOk;
  if (falloCreacionTarea[0]) rtos["task_creation_failure"] = falloCreacionTarea;
  rtos["stack_warning"] = stackRTOSBajo();
  rtos["stack_min_acceptable_bytes"] = RTOS_STACK_MIN_ACCEPTABLE_BYTES;
  JsonObject stackMin = rtos.createNestedObject("stack_min_free_bytes");
  if (stackMinimoWebBytes != UINT32_MAX) stackMin["web"] = stackMinimoWebBytes;
  if (stackMinimoControlBytes != UINT32_MAX) stackMin["control"] = stackMinimoControlBytes;
  JsonObject controlTiming = rtos.createNestedObject("control_timing");
  controlTiming["target_period_us"] = CONTROL_LOOP_PERIOD_US;
  controlTiming["last_period_us"] = periodoControlUltimoUs;
  controlTiming["max_jitter_us"] = jitterControlMaxUs;
  controlTiming["max_cycle_duration_us"] = duracionControlMaxUs;
  controlTiming["max_sample_age_us"] = antiguedadMuestraMaxUs;
  controlTiming["missed_deadlines"] = deadlinesControlPerdidos;
  controlTiming["sample_sequence"] = snap.sequence;
  JsonObject selfTest = payload.createNestedObject("self_test");
  selfTest["status"] = autoevaluacionTexto();
  selfTest["detail"] = detalleAutoevaluacion;
  if (comandoActivoId[0]) payload["active_command_id"] = comandoActivoId;
  if (comandoActivoNombre[0]) payload["active_command_name"] = comandoActivoNombre;
  payload["command_progress"] = progresoComando;
  payload["turn_attempt"] = intentoGiroActualTelemetria();
  payload["turn_attempt_max"] = AUTONOMOUS_MOTION_MAX_ATTEMPTS;
  payload["retry_pause_remaining_ms"] = pausaReintentoRestanteMs();
  payload["drive_attempt"] = intentoAvanceActualTelemetria();
  payload["drive_attempt_max"] = AUTONOMOUS_MOTION_MAX_ATTEMPTS;
  payload["drive_retry_pause_remaining_ms"] = pausaReintentoAvanceRestanteMs();
  payload["autonomous_recovery_reason"] = motivoRecuperacionAutonomaTelemetria();
  payload["turn_mode"] = modoGiroTexto();
  payload["turn_requested_mode"] = modoGiroSolicitadoTexto();
  payload["turn_mode_reason"] = motivoModoGiroArco();
  payload["turn_fallback_reason"] = motivoModoGiroArco();
  payload["turn_fallback_count"] = transicionesFallbackGiroTelemetria();
  payload["turn_left_energized"] = ladoGiroFisicamenteEnergizado(0);
  payload["turn_right_energized"] = ladoGiroFisicamenteEnergizado(1);
  payload["turn_left_power_started_ms"] = inicioEnergiaFisicaGiroMs(0);
  payload["turn_right_power_started_ms"] = inicioEnergiaFisicaGiroMs(1);
  payload["arc_entry_pause_remaining_ms"] = pausaEntradaArcoRestanteMs();
  payload["turn_fixed_sign"] = signoFijoGiroTelemetria();
  payload["turn_attempt_sign"] = signoFijoGiroTelemetria();
  payload["turn_requested_deg"] = anguloSolicitadoGiroTelemetria();
  payload["turn_physical_target_deg"] = objetivoFisicoGiroTelemetria();
  payload["turn_wrap_count"] = vueltasExtendidasGiroTelemetria();
  payload["turn_progress_deg"] = progresoGiroTelemetria();
  payload["turn_error_deg"] = errorGiroTelemetria();
  payload["turn_predicted_error_deg"] = errorPredichoGiroTelemetria();
  payload["turn_gyro_deg_s"] = snap.gyro_z_filtrado_rad_s * 180.0f / PI;
  payload["turn_pwm_target"] = pwmObjetivoGiroTelemetria();
  payload["turn_pwm_target_8bit"] = static_cast<int>(
      lroundf(pwmObjetivoGiroTelemetria() / PWM_SCALE_8_TO_10));
  payload["turn_arc_path_cm"] = PoseGlobal.getArcoCentroGiroCm();
  payload["turn_translation_x_cm"] = PoseGlobal.getTraslacionGiroXCm();
  payload["turn_translation_y_cm"] = PoseGlobal.getTraslacionGiroYCm();
  payload["turn_braking_phase"] = faseFrenadoGiroTelemetria();
  payload["turn_yaw_authority"] = true;
  payload["turn_imu_no_response_ms"] = tiempoImuSinRespuestaGiroMs();
  payload["turn_direction_mismatch_ms"] = tiempoDireccionIncorrectaGiroMs();
  payload["cardinal_target_deg"] = rumboCardinalObjetivoTelemetria();
  payload["move_phase"] = faseMovimientoTexto();
  payload["alignment_stage"] = etapaAlineacionTexto();
  payload["turn_tolerance_deg"] = toleranciaGiroActualTelemetria();
  payload["heading_target_deg"] = rumboCardinalObjetivoTelemetria();
  payload["heading_error_deg"] = errorRumboCardinalTelemetria();
  payload["heading_predicted_error_deg"] = errorRumboPredichoTelemetria();
  payload["heading_control_pwm"] = correccionRumboPwmTelemetria();
  payload["encoder_control_pwm"] = correccionEncoderPwmTelemetria();
  payload["heading_recovery_attempt"] = intentoRecuperacionRumboTelemetria();
  payload["heading_recovery_attempt_max"] = HEADING_RECOVERY_MAX_ATTEMPTS;
  payload["heading_stable_ms"] = estabilidadRumboActualMs();
  payload["route_coarse_aligned"] = rutaAlineadaGruesaTelemetria();
  payload["route_precise_aligned"] = rutaAlineadaPrecisaTelemetria();
  payload["route_axis_remaining_cm"] = distanciaRestanteEjeRutaTelemetria();
  payload["arc_phase"] = faseArcoTexto();
  payload["arc_pulse_count"] = pulsosArcoTelemetria();
  payload["arc_no_progress_count"] = pulsosArcoSinProgresoTelemetria();
  payload["pfl"] = snap.pulsosFL;
  payload["pfr"] = snap.pulsosFR;
  payload["pbl"] = snap.pulsosBL;
  payload["pbr"] = snap.pulsosBR;
  payload["pwm_l"] = pwm_aplicado_L;
  payload["pwm_r"] = pwm_aplicado_R;
  JsonObject motorControl = payload.createNestedObject("motor_control");
  motorControl["limit_8bit"] = PWM_SAFE_LIMIT_8BIT;
  motorControl["limit_10bit"] = PWM_SAFE_HARD_LIMIT;
  JsonObject requested = motorControl.createNestedObject("requested");
  requested["left_10bit"] = pwm_solicitado_L;
  requested["right_10bit"] = pwm_solicitado_R;
  requested["left_8bit"] = pwmA8Bits(pwm_solicitado_L);
  requested["right_8bit"] = pwmA8Bits(pwm_solicitado_R);
  JsonObject physical = motorControl.createNestedObject("physical");
  physical["left_10bit"] = pwm_aplicado_L;
  physical["right_10bit"] = pwm_aplicado_R;
  physical["left_8bit"] = pwmA8Bits(pwm_aplicado_L);
  physical["right_8bit"] = pwmA8Bits(pwm_aplicado_R);
  JsonObject interlockLeft = motorControl.createNestedObject("interlock_left");
  interlockLeft["state"] = estadoInterlockL();
  interlockLeft["energized_sign"] = signoEnergizadoL();
  interlockLeft["pending_sign"] = signoPendienteL();
  interlockLeft["inversion_pending"] = signoPendienteL() != 0;
  JsonObject interlockRight = motorControl.createNestedObject("interlock_right");
  interlockRight["state"] = estadoInterlockR();
  interlockRight["energized_sign"] = signoEnergizadoR();
  interlockRight["pending_sign"] = signoPendienteR();
  interlockRight["inversion_pending"] = signoPendienteR() != 0;
  JsonObject calibrationDiag = payload.createNestedObject("calibration_diagnostics");
  calibrationDiag["phase"] = faseCalibracionTexto();
  calibrationDiag["pwm_10bit"] = pwmCalibracionActual();
  calibrationDiag["pwm_8bit"] = pwmA8Bits(pwmCalibracionActual());
  calibrationDiag["mode"] = "continuous_approved";
  calibrationDiag["torque_attempt"] = intentoTorqueCalibracionTelemetria();
  calibrationDiag["torque_attempt_max"] = CALIBRATION_TORQUE_MAX_ATTEMPTS;
  JsonObject calibrationTicks = calibrationDiag.createNestedObject("last_pulse_ticks");
  calibrationTicks["fl"] = ticksUltimoPulsoCalibracion(0);
  calibrationTicks["fr"] = ticksUltimoPulsoCalibracion(1);
  calibrationTicks["bl"] = ticksUltimoPulsoCalibracion(2);
  calibrationTicks["br"] = ticksUltimoPulsoCalibracion(3);
  payload["mpu_present"] = snap.mpu_present;
  payload["mpu_stale"] = snap.mpu_stale;
  payload["mpu_calibrated"] = snap.mpu_calibrated;
  payload["i2c_ok"] = snap.mpu_present && snap.mpu_calibrated && !snap.mpu_stale;
  payload["gyro_z_filtered_rad_s"] = snap.gyro_z_filtrado_rad_s;
  payload["gyro_z_offset_rad_s"] = snap.gyro_z_offset_rad_s;
  payload["theta_error_deg"] = THETA_ERROR_DEG;
  JsonObject encoderAvg = payload.createNestedObject("encoder_delta_avg");
  encoderAvg["fl"] = snap.delta_pulsos_filtrado_FL;
  encoderAvg["fr"] = snap.delta_pulsos_filtrado_FR;
  encoderAvg["bl"] = snap.delta_pulsos_filtrado_BL;
  encoderAvg["br"] = snap.delta_pulsos_filtrado_BR;
  JsonObject wheelSpeed = payload.createNestedObject("wheel_speed_cm_s");
  wheelSpeed["left"] = velocidadFiltradaLadoTelemetria(0);
  wheelSpeed["right"] = velocidadFiltradaLadoTelemetria(1);
  JsonObject health = payload.createNestedObject("encoder_health");
  const char* encoderKeys[4] = {"fl", "fr", "bl", "br"};
  for (int i = 0; i < 4; ++i) health[encoderKeys[i]] = saludEncoderTexto(i);
  JsonObject stall = payload.createNestedObject("stall_accumulated_ms");
  for (int i = 0; i < 4; ++i) stall[encoderKeys[i]] = WatchdogSeguridad.tiempoStallMs(i);
  JsonObject fusion = payload.createNestedObject("encoder_fusion");
  fusion["estimator"] = estimadorDistanciaTexto();
  fusion["left_reliable_count"] = encodersConfiablesLado(0);
  fusion["right_reliable_count"] = encodersConfiablesLado(1);
  fusion["left_no_progress_ms"] = WatchdogSeguridad.tiempoStallLadoMs(0);
  fusion["right_no_progress_ms"] = WatchdogSeguridad.tiempoStallLadoMs(1);
  JsonObject target = payload.createNestedObject("target");
  if (!strcmp(comandoActivoNombre, "move")) {
    target["x_mm"] = nuevoDestinoX * 10.0f; target["y_mm"] = nuevoDestinoY * 10.0f;
  } else if (!strcmp(comandoActivoNombre, "drive")) target["distance_mm"] = distanciaObjetivoCM * 10.0f;
  else if (!strcmp(comandoActivoNombre, "turn")) target["angle_deg"] = anguloObjetivoGrados;
  JsonObject terminal = payload.createNestedObject("last_terminal");
  if (ultimoTerminalId[0]) {
    terminal["v"] = 1; terminal["type"] = ultimoTerminalTipo;
    terminal["id"] = ultimoTerminalId; terminal["detail"] = ultimoTerminalDetalle;
  }
  JsonObject pins = payload.createNestedObject("pin_state");
  pins["motor_left_active"] = pwm_aplicado_L != 0;
  pins["motor_right_active"] = pwm_aplicado_R != 0;
  pins["encoder_fl_seen"] = snap.pulsosFL != 0;
  pins["encoder_fr_seen"] = snap.pulsosFR != 0;
  pins["encoder_bl_seen"] = snap.pulsosBL != 0;
  pins["encoder_br_seen"] = snap.pulsosBR != 0;
  pins["i2c_sda"] = PIN_I2C_SDA;
  pins["i2c_scl"] = PIN_I2C_SCL;
  if (doc.overflowed()) {
    Serial.println("[RED] Telemetria omitida: capacidad JSON insuficiente.");
    return;
  }
  static char buf[8192];
  const size_t written = serializeJson(doc, buf, sizeof(buf));
  if (written == 0 || written >= sizeof(buf) - 1) return;
  destino->text(buf);
}

void procesarWebSockets() {
  if (!puntoAccesoDisponible() && millis() - ultimoIntentoApMs >= 2000UL) {
    Serial.println("[RED] AP no disponible; intentando recuperacion...");
    iniciarPuntoAcceso();
  }
  ws.cleanupClients();
  procesarEventosRed();
}

void setup_Red() {
  LOG_CORE("Iniciando configuracion de Red...");
  iniciarPuntoAcceso();

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", "{\"robot\":\"ESP32-S3\",\"websocket\":\"/ws\",\"protocol\":\"robot-s3-json-v1\"}");
  });
  if (!servidorIniciado) {
    server.begin();
    servidorIniciado = true;
    Serial.println("[RED] Servidor HTTP/WebSocket iniciado en puerto 80.");
  }
}

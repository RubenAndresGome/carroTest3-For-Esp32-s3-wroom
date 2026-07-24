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
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>

#define MAX_WS_MSG 4096

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
static AsyncWebSocketClient* clienteActivo = nullptr;
static char msgBuf[MAX_WS_MSG + 1];
static unsigned long ultimoIntentoApMs = 0;
static bool servidorIniciado = false;
static unsigned long ultimaTelemetriaMs = 0;

static bool apDisponible() {
  return WiFi.getMode() == WIFI_AP && WiFi.softAPIP() != IPAddress(0,0,0,0);
}

static bool iniciarAP() {
  ultimoIntentoApMs = millis();
  WiFi.setSleep(false);
  if (!WiFi.mode(WIFI_AP)) { Serial.println("[RED] ERROR: modo AP fallo."); return false; }
  if (!WiFi.softAP(ssid_AP, password_AP, 6, false, 2) || !apDisponible()) {
    Serial.printf("[RED] softAP '%s' fallo.\n", ssid_AP); return false;
  }
  Serial.printf("[RED] AP: %s | IP: %s\n", ssid_AP, WiFi.softAPIP().toString().c_str());
  return true;
}

// ------ envío de JSON por WebSocket ------
static void enviarJSON(const JsonDocument& doc) {
  size_t len = measureJson(doc);
  if (len > MAX_WS_MSG) return;
  serializeJson(doc, msgBuf, MAX_WS_MSG + 1);
  if (clienteActivo && clienteActivo->canSend()) ws.textAll(msgBuf, len);
}

// ------ hello / sesión (se maneja directamente en Red) ------
static void manejarHello(const JsonObject& o) {
  const char* nueva = o["session"] | "";
  if (!nueva || !nueva[0]) return;
  bool misma = strncmp(sessionId, nueva, 16) == 0 && sessionId[0];
  if (!misma) {
    strncpy(sessionId, nueva, 16); sessionId[16] = '\0';
    ultimoSeqCompletado = 0;
  }
  StaticJsonDocument<256> doc;
  doc["evt"] = "hello_ack";
  doc["session"] = sessionId;
  doc["state"] = (estadoActual==DESARMADO?"desarmado":estadoActual==LISTO?"listo":estadoActual==EJECUTANDO?"ejecutando":estadoActual==CALIBRANDO?"calibrando":estadoActual==FALLO?"fallo":"estop");
  doc["last_seq"] = ultimoSeqCompletado;
  doc["calibrated"] = robotCalibrado;
  doc["protocol"] = PROTOCOL_NAME;
  if (ultimoFalloDetalle[0]) doc["fault"] = ultimoFalloDetalle;
  enviarJSON(doc);
}

// ------ parseo de comandos (push a cola, excepto ESTOP inmediato) ------
static bool leerFloatFinito(JsonVariantConst valor, float& destino) {
  if (!valor.is<float>() && !valor.is<double>() && !valor.is<int>() && !valor.is<long>()) return false;
  destino = valor.as<float>();
  return isfinite(destino);
}

static void responderRechazado(int seq, const char* detalle) {
  encolarEvento(EVT_REJECTED, seq, detalle);
}

static void parsearMensaje(const uint8_t* data, size_t len) {
  StaticJsonDocument<768> doc;
  DeserializationError err = deserializeJson(doc, data, len);
  if (err || !doc.is<JsonObject>()) return;
  const char* cmd = doc["cmd"] | "";
  if (!cmd[0]) return;

  if (strcmp(cmd, "hello") == 0) { manejarHello(doc.as<JsonObject>()); return; }

  if (!sessionId[0]) { responderRechazado(0, "hello_required"); return; }
  JsonVariantConst seqVar = doc["seq"];
  if (!seqVar.is<int>() && !seqVar.is<long>()) { responderRechazado(0, "seq_invalid"); return; }
  const int seq = seqVar.as<int>();
  if (seq <= 0) { responderRechazado(seq, "seq_invalid"); return; }
  if (seq <= ultimoSeqCompletado) { encolarEvento(EVT_ALREADY_DONE, seq, "already_done"); return; }
  if (seqActivo == seq) { encolarEvento(EVT_ACCEPTED, seq, "already_active"); return; }

  if (strcmp(cmd, "estop") == 0) {
    seq_ESTOP_pendiente = seq;
    flag_ESTOP_ISR = true;
    encolarEvento(EVT_ACCEPTED, seq, "estop");
    return;
  }

  ComandoRed c = {}; c.seq = seq;
  if (strcmp(cmd,"calibrate")==0)      { c.tipo=CMD_CALIBRATE; }
  else if (strcmp(cmd,"step")==0) {
    c.tipo=CMD_STEP;
    if (!leerFloatFinito(doc["heading"], c.heading) || !leerFloatFinito(doc["cm"], c.distanciaCm)) {
      responderRechazado(seq, "step_payload_invalid"); return;
    }
  }
  else if (strcmp(cmd,"stop")==0)       { c.tipo=CMD_STOP; }
  else if (strcmp(cmd,"clear_fault")==0){ c.tipo=CMD_CLEAR_FAULT; }
  else if (strcmp(cmd,"set_comp")==0) {
    c.tipo=CMD_SET_COMP;
    if (!leerFloatFinito(doc["factor"], c.factor)) { responderRechazado(seq, "comp_invalid"); return; }
  }
  else if (strcmp(cmd,"reset_pose")==0) { c.tipo=CMD_RESET_POSE; }
  else { responderRechazado(seq, "unknown_command"); return; }

  if (xQueueSend(colaComandos, &c, 0) != pdTRUE) responderRechazado(seq, "command_queue_full");
}

// ------ eventos WS ------
static void onWsEvent(AsyncWebSocket* s, AsyncWebSocketClient* c, AwsEventType t, void* arg, uint8_t* data, size_t len) {
  switch (t) {
    case WS_EVT_CONNECT:
      if (clienteActivo && clienteActivo != c) { c->close(4000, "single_client"); return; }
      clienteActivo = c;
      {
        StaticJsonDocument<256> doc;
        doc["evt"] = "welcome";
        doc["session"] = sessionId;
        doc["state"] = (estadoActual==DESARMADO?"desarmado":estadoActual==LISTO?"listo":estadoActual==EJECUTANDO?"ejecutando":estadoActual==CALIBRANDO?"calibrando":estadoActual==FALLO?"fallo":"estop");
        doc["last_seq"] = ultimoSeqCompletado;
        doc["calibrated"] = robotCalibrado;
        doc["protocol"] = PROTOCOL_NAME;
        enviarJSON(doc);
      }
      break;
    case WS_EVT_DISCONNECT:
      if (clienteActivo == c) clienteActivo = nullptr;
      break;
    case WS_EVT_DATA: {
      AwsFrameInfo* info = static_cast<AwsFrameInfo*>(arg);
      if (info && info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT && len <= MAX_WS_MSG)
        parsearMensaje(data, len);
      break;
    }
    default: break;
  }
}

// ------ drenar cola de eventos ------
static void drenarEventos() {
  EventoRed evt;
  while (colaEventosRed && xQueueReceive(colaEventosRed, &evt, 0) == pdTRUE) {
    StaticJsonDocument<256> doc;
    const char* tipo = evt.tipo==EVT_ACCEPTED?"accepted":evt.tipo==EVT_REJECTED?"rejected":evt.tipo==EVT_COMPLETED?"completed":evt.tipo==EVT_ALREADY_DONE?"already_done":evt.tipo==EVT_FAULT?"fault":"progress";
    doc["evt"] = tipo; doc["seq"] = evt.seq;
    if (evt.detalle[0]) doc["detail"] = evt.detalle;
    if (evt.tipo == EVT_PROGRESS) doc["pct"] = evt.progreso;
    enviarJSON(doc);
  }
}

// ------ telemetría 10 Hz ------
static void enviarTelemetria() {
  if (millis() - ultimaTelemetriaMs < 100) return;
  ultimaTelemetriaMs = millis();
  StaticJsonDocument<1024> doc;
  doc["evt"] = "telemetry";
  doc["state"] = (estadoActual==DESARMADO?"desarmado":estadoActual==LISTO?"listo":estadoActual==EJECUTANDO?"ejecutando":estadoActual==CALIBRANDO?"calibrando":estadoActual==FALLO?"fallo":"estop");
  doc["yaw"] = roundf(heading360*10)/10;
  doc["x"] = roundf(PoseGlobal.getX()*10)/10;
  doc["y"] = roundf(PoseGlobal.getY()*10)/10;
  doc["pwm_l"] = pwm_aplicado_L; doc["pwm_r"] = pwm_aplicado_R;
  JsonArray enc = doc.createNestedArray("enc");
  SensorSnapshot s = {};
  obtenerUltimoSnapshotSensores(s);
  enc.add(s.pulsosFL); enc.add(s.pulsosFR); enc.add(s.pulsosBL); enc.add(s.pulsosBR);
  doc["degraded"] = modoDegradado;
  doc["session"] = sessionId;
  doc["last_seq"] = ultimoSeqCompletado;
  doc["seq"] = seqActivo;
  doc["phase"] = faseComando;
  doc["prog"] = roundf(progresoComando*100)/100;
  doc["comp"] = roundf(factorCompensacionDer*100)/100;
  doc["cal"] = robotCalibrado;
  doc["firmware"] = FIRMWARE_VERSION;
  doc["protocol"] = PROTOCOL_NAME;
  doc["reset_reason"] = motivoResetESP32;
  doc["stack_web"] = stackMinimoWebBytes == UINT32_MAX ? 0 : stackMinimoWebBytes;
  doc["stack_control"] = stackMinimoControlBytes == UINT32_MAX ? 0 : stackMinimoControlBytes;
  doc["stack_warning"] = stackRTOSBajo();
  doc["tasks_ok"] = creacionTareasOk;
  doc["control_period_us"] = periodoControlUltimoUs;
  doc["control_jitter_max_us"] = jitterControlMaxUs;
  doc["control_cycle_max_us"] = duracionControlMaxUs;
  doc["control_sample_age_max_us"] = antiguedadMuestraMaxUs;
  doc["control_missed_deadlines"] = deadlinesControlPerdidos;
  enviarJSON(doc);
}

// ------ API pública ------
void setup_Red() {
  for (int intento = 0; intento < 30; ++intento) {
    if (iniciarAP()) { servidorIniciado = true; break; }
    delay(200);
  }
  if (!servidorIniciado) { Serial.println("[RED] CRITICO: AP nunca inicio."); return; }

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.begin();
  Serial.println("[RED] WebSocket /ws listo.");
  ultimaTelemetriaMs = millis();
}

void procesarWebSockets() {
  ws.cleanupClients();
  if (!servidorIniciado && millis() - ultimoIntentoApMs >= 5000) iniciarAP();
  drenarEventos();
}

void pushTelemetria() {
  enviarTelemetria();
}

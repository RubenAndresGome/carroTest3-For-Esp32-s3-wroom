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
  return (WiFi.getMode() & WIFI_AP) && WiFi.softAPIP() != IPAddress(0,0,0,0);
}

static bool iniciarAP() {
  ultimoIntentoApMs = millis();
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  IPAddress local_ip(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  if (!WiFi.softAP(ssid_AP, password_AP, 1, 0, 4)) {
    Serial.printf("[RED] softAP '%s' fallo.\n", ssid_AP); return false;
  }
  delay(100);
  Serial.printf("[RED] AP exitoso: %s | IP: %s\n", ssid_AP, WiFi.softAPIP().toString().c_str());
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

static bool leerModoPaso(JsonVariantConst valor, ModoPaso& destino) {
  // Los clientes anteriores no envían drive_mode y conservan la semántica de
  // avance tradicional. La HMI actual manda auto para habilitar reversa.
  if (valor.isNull()) { destino = PASO_ADELANTE; return true; }
  if (!valor.is<const char*>()) return false;
  const char* modo = valor.as<const char*>();
  if (strcmp(modo, "forward") == 0) { destino = PASO_ADELANTE; return true; }
  if (strcmp(modo, "reverse") == 0) { destino = PASO_REVERSA; return true; }
  if (strcmp(modo, "auto") == 0) { destino = PASO_AUTO; return true; }
  return false;
}

static void responderRechazado(int seq, const char* detalle) {
  encolarEvento(EVT_REJECTED, seq, detalle);
}

static void parsearMensaje(const uint8_t* data, size_t len) {
  StaticJsonDocument<1024> doc;
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
  if (seq == 1) { ultimoSeqCompletado = 0; }
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
    if (!leerModoPaso(doc["drive_mode"], c.modoPaso)) {
      responderRechazado(seq, "step_drive_mode_invalid"); return;
    }
    const bool tieneX = !doc["target_x_mm"].isNull();
    const bool tieneY = !doc["target_y_mm"].isNull();
    if (tieneX != tieneY) {
      responderRechazado(seq, "step_target_pair_required"); return;
    }
    if (tieneX) {
      float targetXmm = 0.0f, targetYmm = 0.0f;
      if (!leerFloatFinito(doc["target_x_mm"], targetXmm) ||
          !leerFloatFinito(doc["target_y_mm"], targetYmm) ||
          fabsf(targetXmm) > STEP_TARGET_MAX_ABS_CM * 10.0f ||
          fabsf(targetYmm) > STEP_TARGET_MAX_ABS_CM * 10.0f) {
        responderRechazado(seq, "step_target_invalid"); return;
      }
      c.targetXCm = targetXmm / 10.0f;
      c.targetYCm = targetYmm / 10.0f;
      c.tieneObjetivoAbsoluto = true;
    }
  }
  else if (strcmp(cmd,"turn_to")==0) {
    c.tipo=CMD_TURN_TO;
    if (!leerFloatFinito(doc["heading"], c.heading)) {
      responderRechazado(seq, "turn_payload_invalid"); return;
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
    doc["evt"] = tipo; doc["seq"] = evt.seq; doc["run_id"] = evt.run_id;
    if (evt.detalle[0]) doc["detail"] = evt.detalle;
    if (evt.tipo == EVT_PROGRESS) doc["pct"] = evt.progreso;
    enviarJSON(doc);
  }
}

// ------ telemetría 10 Hz ------
static void enviarTelemetria() {
  if (millis() - ultimaTelemetriaMs < 100) return;
  ultimaTelemetriaMs = millis();
  StaticJsonDocument<2048> doc;
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
  doc["degraded_mode"] = modoDegradado;
  JsonArray encConfiables = doc.createNestedArray("enc_trusted");
  for (bool confiable : encoderConfiableGlobal) encConfiables.add(confiable);
  doc["encoder_scale_factor"] = FACTOR_ESCALA_ENCODER;
  doc["encoder_error_pct"] = ENCODER_ERROR_PORCENTAJE;
  JsonObject saludEncoders = doc.createNestedObject("encoder_health");
  saludEncoders["fl"] = encoderConfiableGlobal[0] ? "ok" : "excluded";
  saludEncoders["fr"] = encoderConfiableGlobal[1] ? "ok" : "excluded";
  saludEncoders["bl"] = encoderConfiableGlobal[2] ? "ok" : "excluded";
  saludEncoders["br"] = encoderConfiableGlobal[3] ? "ok" : "excluded";
  JsonObject fusionEncoders = doc.createNestedObject("encoder_fusion");
  fusionEncoders["estimator"] = "mediana_robusta_por_lado";
  fusionEncoders["left_reliable_count"] = int(encoderConfiableGlobal[0]) + int(encoderConfiableGlobal[2]);
  fusionEncoders["right_reliable_count"] = int(encoderConfiableGlobal[1]) + int(encoderConfiableGlobal[3]);
  fusionEncoders["distance_scale_factor"] = FACTOR_ESCALA_ENCODER;
  fusionEncoders["distance_error_pct"] = ENCODER_ERROR_PORCENTAJE;
  JsonObject movimiento = doc.createNestedObject("motion");
  movimiento["requested_mode"] = pasoModoSolicitado;
  movimiento["effective_mode"] = pasoModoEfectivo;
  movimiento["travel_heading_deg"] = pasoRumboTrayectoDeg;
  movimiento["body_heading_deg"] = pasoRumboCuerpoDeg;
  movimiento["final_heading_deg"] = pasoRumboFinalDeg;
  movimiento["remaining_cm"] = pasoDistanciaRestanteCm;
  movimiento["brake_prediction_cm"] = pasoFrenoPrevistoCm;
  movimiento["coast_cm"] = pasoArrastreFrenoCm;
  movimiento["settle_elapsed_ms"] = pasoAsentamientoMs;
  movimiento["reverse_ramp_elapsed_ms"] = pasoRampaReversaMs;
  movimiento["reverse_ramp_active"] = pasoEnReversa &&
      pasoRampaReversaMs < RAMPA_REVERSA_MS;
  movimiento["run_id"] = pasoEjecucionId;
  doc["session"] = sessionId;
  doc["last_seq"] = ultimoSeqCompletado;
  doc["seq"] = seqActivo;
  doc["command_run_id"] = pasoEjecucionId;
  doc["phase"] = faseComando;
  doc["prog"] = roundf(progresoComando*100)/100;
  doc["comp"] = roundf(factorCompensacionDer*100)/100;
  JsonObject target = doc.createNestedObject("target");
  target["absolute"] = pasoObjetivoAbsoluto;
  target["x_cm"] = pasoTargetXObjetivoCm;
  target["y_cm"] = pasoTargetYObjetivoCm;
  target["longitudinal_error_cm"] = pasoErrorLongitudinalCm;
  target["lateral_error_cm"] = pasoErrorLateralCm;
  target["distance_error_cm"] = pasoErrorEuclidianoCm;
  target["endpoint_attempt"] = pasoIntentosEndpoint;
  target["finish_reason"] = pasoMotivoFinalizacion;
  JsonObject recuperacion = doc.createNestedObject("recovery");
  recuperacion["decision"] = pasoDecisionRecuperacion;
  recuperacion["distance_cm"] = pasoDistanciaRecuperacionCm;
  recuperacion["direction"] = pasoRecuperacionUsaReversa ? "reverse" : "forward";
  recuperacion["pivot_avoided"] = pasoRecuperacionUsaReversa;
  recuperacion["min_distance_cm"] = DISTANCIA_MINIMA_RECUPERACION_ENDPOINT_CM;
  JsonObject control = doc.createNestedObject("drive_control");
  control["dynamic_heading_deg"] = pasoRumboDinamicoDeg;
  control["heading_error_deg"] = pasoErrorRumboDeg;
  control["pwm"] = pasoControlRumboPwm;
  control["p"] = pasoControlRumboP;
  control["i"] = pasoControlRumboI;
  control["d"] = pasoControlRumboD;
  control["integral_deg_s"] = pasoIntegralRumboGradoS;
  control["encoder_pwm"] = pasoControlEncoderPwm;
  control["lateral_correction_deg"] = pasoControlLateralDeg;
  control["right_compensation"] = factorCompensacionDer;
  control["heading_brake_side"] = pasoLadoFrenoRumbo;
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

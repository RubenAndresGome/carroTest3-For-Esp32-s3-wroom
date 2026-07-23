import { api } from "./api.js";
import { bindControls } from "./controls.js";
import { connectEvents } from "./events.js";
import { store } from "./store.js";
import { renderTelemetry } from "./telemetry.js";

const labels = { connected: "Conectado", connecting: "Conectando", backoff: "Sin enlace", stopped: "Detenido" };

function log(kind, message) {
  const item = document.createElement("li");
  const time = document.createElement("time");
  const text = document.createElement("span");
  time.textContent = new Date().toLocaleTimeString(); text.textContent = `${kind}: ${message}`;
  item.append(time, text);
  const list = document.getElementById("event-log"); list.prepend(item);
  while (list.children.length > 40) list.lastElementChild.remove();
}

function render(state) {
  const connection = state.connection || {};
  const pill = document.getElementById("connection-pill");
  pill.dataset.state = connection.state || "stopped";
  document.getElementById("connection-label").textContent = labels[connection.state] || connection.state;
  document.getElementById("connection-detail").textContent = connection.detail || `ws://${state.robot_host || "192.168.4.1"}/ws`;
  document.getElementById("protocol-badge").textContent = (connection.protocol || "—").toUpperCase();
  document.getElementById("robot-host").value = state.robot_host || "192.168.4.1";
  const connected = connection.state === "connected";
  document.querySelectorAll(".drive, .command").forEach((button) => { button.disabled = !connected; });
  const indicator = document.getElementById("record-indicator");
  indicator.textContent = state.recording ? `GRABANDO #${state.session_id}` : "DETENIDA";
  indicator.classList.toggle("active", Boolean(state.recording));
  document.getElementById("record-toggle").textContent = state.recording ? "Detener grabación" : "Iniciar grabación";
  const exportLink = document.getElementById("export-link");
  exportLink.classList.toggle("disabled", !state.session_id);
  exportLink.href = state.session_id ? `/api/v1/sessions/${state.session_id}/telemetry.csv` : "#";
  renderTelemetry(state.telemetry);
}

async function refresh() {
  try { store.set(await api.status()); } catch (error) { log("App local", error.message); }
}

store.subscribe(render);
bindControls(log, refresh);
connectEvents((kind, payload) => {
  if (kind === "snapshot") store.set(payload);
  else if (kind === "telemetry") store.set({ telemetry: payload });
  else if (kind === "connection") store.set({ connection: { ...store.get().connection, ...payload } });
  else if (kind === "session") store.set(payload);
  else if (kind === "config") store.set(payload);
  else log(kind, payload.detail || payload.reason || payload.status || JSON.stringify(payload));
}, () => log("App local", "Reconectando canal de eventos"));
refresh();

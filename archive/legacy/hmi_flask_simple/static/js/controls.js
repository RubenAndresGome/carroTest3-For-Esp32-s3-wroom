import { api } from "./api.js";

export function bindControls(log, refresh) {
  document.getElementById("robot-config").addEventListener("submit", async (event) => {
    event.preventDefault();
    try { await api.configure(document.getElementById("robot-host").value); log("Configuración", "Reconectando con la nueva dirección"); }
    catch (error) { log("Error", error.message); }
  });
  document.getElementById("estop").addEventListener("click", async () => {
    try { await api.estop(); log("Seguridad", "Paro de emergencia enviado"); } catch (error) { log("Error", error.message); }
  });
  document.querySelectorAll(".command").forEach((button) => button.addEventListener("click", async () => {
    try { await api.command(button.dataset.command); log("Comando", button.dataset.command); } catch (error) { log("Error", error.message); }
  }));
  let manualTimer = null;
  let manualPointer = null;
  const sendManual = async (left, right) => {
    try { await api.command("manual", { l: left, r: right }); } catch (error) { log("Error", error.message); }
  };
  const stopManual = () => {
    if (manualTimer !== null) window.clearInterval(manualTimer);
    manualTimer = null;
    manualPointer = null;
    void sendManual(0, 0);
  };
  document.querySelectorAll(".drive").forEach((button) => {
    button.addEventListener("pointerdown", (event) => {
      if (manualTimer !== null) stopManual();
      manualPointer = event.pointerId;
      button.setPointerCapture(event.pointerId);
      const left = Number(button.dataset.left), right = Number(button.dataset.right);
      void sendManual(left, right);
      manualTimer = window.setInterval(() => { void sendManual(left, right); }, 100);
    });
    button.addEventListener("pointerup", (event) => { if (event.pointerId === manualPointer) stopManual(); });
    button.addEventListener("pointercancel", stopManual);
    button.addEventListener("lostpointercapture", () => { if (manualTimer !== null) stopManual(); });
    button.addEventListener("contextmenu", (event) => event.preventDefault());
  });
  window.addEventListener("blur", () => { if (manualTimer !== null) stopManual(); });
  document.addEventListener("visibilitychange", () => {
    if (document.visibilityState !== "visible" && manualTimer !== null) stopManual();
  });
  document.getElementById("move-form").addEventListener("submit", async (event) => {
    event.preventDefault(); const form = new FormData(event.currentTarget);
    try { await api.command("move", { x: Number(form.get("x")), y: Number(form.get("y")) }); log("Ruta", "Destino enviado"); }
    catch (error) { log("Error", error.message); }
  });
  document.getElementById("record-toggle").addEventListener("click", async () => {
    const action = document.getElementById("record-indicator").classList.contains("active") ? "stop" : "start";
    try { await api.session(action); await refresh(); } catch (error) { log("Error", error.message); }
  });
  document.getElementById("clear-log").addEventListener("click", () => document.getElementById("event-log").replaceChildren());
}

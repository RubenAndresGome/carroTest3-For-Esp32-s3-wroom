const fs = require("node:fs");
const vm = require("node:vm");

const archivo = process.argv[2] || "desktop_app/robot_app/hmi/index.html";
const html = fs.readFileSync(archivo, "utf8");
const inicio = html.lastIndexOf("<script>");
const fin = html.lastIndexOf("</script>");
if (inicio < 0 || fin <= inicio) throw new Error("No se encontró el script principal de la HMI");
new vm.Script(html.slice(inicio + "<script>".length, fin), { filename: archivo });
if (!html.includes("user-scalable=yes") || !html.includes("maximum-scale=3.0")) {
  throw new Error("La HMI móvil debe conservar zoom nativo hasta 3x");
}
if ((html.match(/data-toggle-sign=/g) || []).length < 4) {
  throw new Error("Faltan controles ± para coordenadas negativas");
}
for (const axisLabel of [
  "Coordenada Relativa X (+X Derecha):",
  "Coordenada Relativa Y (+Y Frente):",
  "X (cm) [← Izq / Der →]",
  "Y (cm) [↓ Rev / Fte ↑]",
]) {
  if (!html.includes(axisLabel)) throw new Error(`Falta convención ortogonal visible: ${axisLabel}`);
}
for (const renderGuard of [
  "frameIntervalMs:1000/15",
  "document.hidden || now-this.lastTime < this.frameIntervalMs",
]) {
  if (!html.includes(renderGuard)) throw new Error(`Falta límite de render WebView: ${renderGuard}`);
}
for (const guard of [
  "state.activeTab === 'touch' && id !== 'touch') Touch.end()",
  "visibilitychange',()=>{if(document.hidden)Touch.end()",
  "window.addEventListener('pagehide',()=>Touch.end()",
]) {
  if (!html.includes(guard)) throw new Error(`Falta liberación segura de joystick: ${guard}`);
}
for (const missionGuard of [
  "async stopAndReset()",
  "const mission=await WSControl.api('/api/v1/missions',{method:'DELETE'})",
  "if(previous.blocked)",
]) {
  if (!html.includes(missionGuard)) throw new Error(`Falta reset confirmado de misión: ${missionGuard}`);
}
process.stdout.write("JavaScript de la HMI válido.\n");

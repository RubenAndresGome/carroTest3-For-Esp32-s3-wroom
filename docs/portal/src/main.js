import "./styles.css";
import mermaid from "mermaid";
import cytoscape from "cytoscape";
import Chart from "chart.js/auto";
import { createIcons, Activity, AlertTriangle, BookOpen, Boxes, Braces, CircuitBoard, Database, Download, Film, GitBranch, Maximize2, Menu, Printer, Route, Search, ShieldCheck, X } from "lucide";
import catalog from "../../catalogo_funciones.json";
import sqliteSummary from "../../datos_sqlite_documentales.json";
import { diagrams, evidence, findings, glossary, manuals } from "./content.js";

const iconSet = { Activity, AlertTriangle, BookOpen, Boxes, Braces, CircuitBoard, Database, Download, Film, GitBranch, Maximize2, Menu, Printer, Route, Search, ShieldCheck, X };
const app = document.querySelector("#app");

const esc = (value) => String(value ?? "").replace(/[&<>'"]/g, (char) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", "'": "&#39;", '"': "&quot;" }[char]));
const assetPath = (path) => import.meta.env.DEV ? path : `../../evidencia/${path}`;
const repoPath = (path) => import.meta.env.DEV ? `../../${path}` : `../../${path}`;
const severityClass = (severity) => ["Crítica", "Alta", "Pendiente"].includes(severity) ? "badge-high" : severity === "Media" ? "badge-medium" : "badge-low";
const navItems = [
  ["resumen", "activity", "Estado actual"],
  ["arquitectura", "boxes", "Arquitectura"],
  ["uml", "git-branch", "UML y grafos"],
  ["funciones", "braces", "Funciones"],
  ["api", "database", "Interfaces"],
  ["datos", "activity", "Datos SQLite"],
  ["manual", "book-open", "Manual de uso"],
  ["glosario", "braces", "Glosario"],
  ["seguridad", "shield-check", "Seguridad"],
  ["evidencia", "film", "Evidencia"],
];

app.innerHTML = `
  <div class="shell">
    <aside class="sidebar" aria-label="Navegación documental">
      <div class="flex items-center justify-between gap-4">
        <div>
          <p class="font-mono text-[10px] uppercase tracking-[.28em] text-mint">Robot S3</p>
          <p class="mt-1 text-lg font-semibold text-white">Atlas técnico</p>
        </div>
        <button id="nav-toggle" class="rounded-lg border border-line p-2 text-slate-300 lg:hidden" aria-label="Abrir navegación" aria-expanded="false"><i data-lucide="menu"></i></button>
      </div>
      <p class="mt-4 text-xs leading-5 text-slate-500">Auditoría, UML, evidencia y operación de la arquitectura <span class="text-slate-300">robot-s3-steps-v3</span>.</p>
      <label class="relative mt-5 block"><span class="sr-only">Buscar en todo el portal</span><i data-lucide="search" class="pointer-events-none absolute left-3 top-2.5 h-4 w-4 text-slate-500"></i><input id="global-search" class="filter w-full pl-10" type="search" placeholder="Buscar en todo…"></label>
      <div id="global-results" class="mt-2 hidden max-h-52 overflow-y-auto rounded-lg border border-line bg-ink p-2 text-xs"></div>
      <nav id="nav" class="mt-7 hidden space-y-1 lg:block">
        ${navItems.map(([id, icon, label]) => `<a class="nav-link" href="#${id}"><i data-lucide="${icon}"></i><span>${label}</span></a>`).join("")}
      </nav>
      <div class="mt-8 rounded-xl border border-line bg-white/[.025] p-4 text-xs text-slate-500">
        <p class="font-mono uppercase tracking-wide text-cyan">Corte documental</p>
        <p class="mt-2 text-slate-300">Commit 7a69eac</p>
        <p>${catalog.stats.files} archivos operativos</p>
        <p>Generado ${new Date(catalog.generated_at).toLocaleDateString("es-MX")}</p>
      </div>
      <div class="mt-4 grid grid-cols-2 gap-2 text-xs"><a class="route-link" href="./uml.html">Abrir UML</a><a class="route-link" href="./manual.html">Abrir manual</a><button id="print-page" class="route-link"><i data-lucide="printer"></i> Imprimir</button><a class="route-link" href="../../docs/informe_entrega_documentacion.md">Matriz final</a></div>
    </aside>

    <main id="contenido" class="content">
      <section id="resumen" class="section">
        <p class="eyebrow">Manual vivo · Auditoría reproducible</p>
        <h1 class="max-w-5xl text-4xl font-semibold tracking-[-.04em] text-white sm:text-6xl">Del gesto en la HMI al PWM en las ruedas.</h1>
        <p class="mt-6 max-w-3xl text-lg leading-8 text-slate-400">Una vista verificable del sistema real: quién posee la misión, cómo viaja cada paso, qué protege el movimiento y dónde siguen existiendo riesgos.</p>
        <div class="mt-9 grid gap-4 sm:grid-cols-2 xl:grid-cols-4">
          ${[
            [catalog.stats.functions, "Funciones y prototipos", "braces"],
            [catalog.stats.types, "Tipos activos", "boxes"],
            [catalog.stats.routes, "Rutas HTTP", "route"],
            [findings.filter((item) => ["Alta", "Crítica", "Pendiente"].includes(item.severity)).length, "Hallazgos altos/críticos", "alert-triangle"],
          ].map(([value, label, icon]) => `<article class="metric"><i class="text-mint" data-lucide="${icon}"></i><p class="mt-5 text-4xl font-semibold text-white">${value}</p><p class="mt-1 text-sm text-slate-400">${label}</p></article>`).join("")}
        </div>
        <div class="mt-8 grid gap-6 xl:grid-cols-[1.15fr_.85fr]">
          <article class="card">
            <div class="flex items-center justify-between"><div><p class="eyebrow">Cobertura funcional</p><h2 class="text-xl font-semibold">Distribución por subsistema</h2></div><i data-lucide="circuit-board" class="text-cyan"></i></div>
            <div class="mt-5 h-80"><canvas id="coverage-chart" aria-label="Gráfica de funciones por subsistema"></canvas></div>
          </article>
          <article class="card">
            <p class="eyebrow">Dictamen</p>
            <h2 class="text-xl font-semibold text-white">Arquitectura correcta; resiliencia incompleta</h2>
            <p class="mt-4 leading-7 text-slate-400">Python y ESP32 tienen responsabilidades claras, pero una caída prolongada deja el gateway detenido y un reboot del robot borra la memoria necesaria para reanudar un paso con certeza.</p>
            <div class="mt-6 space-y-3">
              <div class="rounded-xl border border-mint/20 bg-mint/5 p-4"><strong class="text-mint">Fortaleza:</strong> <span class="text-slate-300">control en tiempo real y parada permanecen en el ESP32.</span></div>
              <div class="rounded-xl border border-coral/20 bg-coral/5 p-4"><strong class="text-coral">Bloqueo:</strong> <span class="text-slate-300">calibración física reciente terminó con encoders en cero.</span></div>
            </div>
          </article>
        </div>
        <div class="mt-8">
          <div class="flex items-end justify-between gap-4"><div><p class="eyebrow">Hallazgos trazables</p><h2 class="text-2xl font-semibold">Riesgos que condicionan la aceptación</h2></div><span class="text-sm text-slate-500">${findings.length} registros</span></div>
          <div class="mt-5 grid gap-4 md:grid-cols-2 xl:grid-cols-3">
            ${findings.map((item) => `<article class="card"><div class="flex items-center justify-between"><span class="badge ${severityClass(item.severity)}">${item.severity}</span><span class="font-mono text-xs text-slate-600">${item.id}</span></div><h3 class="mt-4 font-semibold text-white">${item.title}</h3><p class="mt-2 text-sm leading-6 text-slate-400">${item.detail}</p><p class="mt-4 break-words font-mono text-[11px] text-slate-600">${item.source}</p></article>`).join("")}
          </div>
        </div>
      </section>

      <section id="arquitectura" class="section">
        <p class="eyebrow">Vista de sistema</p><h2 class="section-title">Arquitectura operativa</h2>
        <p class="section-copy">El controlador puede ser Windows o Android, nunca ambos. Python conserva misión e historial; el ESP32 conserva los lazos de 100 Hz y la autoridad de seguridad.</p>
        <div class="diagram mermaid">${diagrams.context}</div>
        <div class="diagram mermaid">${diagrams.runtime}</div>
      </section>

      <section id="uml" class="section">
        <p class="eyebrow">Comportamiento coordinado</p><h2 class="section-title">UML navegable y grafo de dependencias</h2>
        <p class="section-copy">Selecciona un flujo para ver la secuencia real. Debajo, el grafo interactivo muestra las dependencias principales entre carpetas.</p>
        <div id="diagram-tabs" class="no-print mt-6 flex flex-wrap gap-2">
          ${Object.entries({ mission: "Misión", calibration: "Calibración", returnHome: "Ockham", reconnect: "Corte de red", robotReboot: "Reinicio ESP32", pythonRestart: "Reinicio Python", close: "Cierre", states: "Estados", database: "SQLite", android: "Android", safety: "Seguridad", validation: "Validación" }).map(([key, label], index) => `<button data-diagram="${key}" class="diagram-tab rounded-full border px-4 py-2 text-sm ${index === 0 ? "border-mint bg-mint/10 text-mint" : "border-line text-slate-400"}">${label}</button>`).join("")}
        </div>
        <div id="dynamic-diagram" class="diagram mermaid">${diagrams.mission}</div>
        <div class="no-print mt-7 flex flex-wrap items-center gap-3"><label class="text-sm text-slate-400">Subsistema <select id="graph-folder" class="filter ml-2">${Object.keys(catalog.stats.by_folder).map((folder) => `<option>${folder}</option>`).join("")}</select></label><button id="graph-fullscreen" class="route-link"><i data-lucide="maximize-2"></i> Pantalla completa</button><span id="graph-count" class="font-mono text-xs text-slate-500"></span></div>
        <div id="call-graph" class="call-graph" role="img" aria-label="Grafo interactivo de todas las funciones del subsistema"></div>
        <div id="graph-detail" class="mt-3 rounded-lg border border-line bg-black/10 p-3 text-xs text-slate-400">Arrastra nodos, usa la rueda para zoom y selecciona una función para ver su ruta, riesgo, complejidad y vecindario.</div>
      </section>

      <section id="funciones" class="section">
        <p class="eyebrow">Catálogo generado</p><h2 class="section-title">Todas las funciones activas por carpeta</h2>
        <p class="section-copy">Inventario producido con Universal Ctags y análisis estático. Se excluyen archivos históricos, pruebas, dependencias y salidas generadas.</p>
        <div class="no-print mt-6 grid gap-3 md:grid-cols-2 xl:grid-cols-5">
          <label class="relative"><i data-lucide="search" class="pointer-events-none absolute left-3 top-2.5 h-4 w-4 text-slate-500"></i><input id="function-search" class="filter w-full pl-10" type="search" placeholder="Buscar función, archivo o llamada…" /></label>
          <select id="folder-filter" class="filter" aria-label="Filtrar por carpeta"><option value="">Todas las carpetas</option>${Object.keys(catalog.stats.by_folder).map((folder) => `<option>${folder}</option>`).join("")}</select>
          <select id="platform-filter" class="filter" aria-label="Filtrar por plataforma"><option value="">Todas las plataformas</option>${[...new Set(catalog.functions.map((item) => item.platform))].sort().map((value) => `<option>${value}</option>`).join("")}</select>
          <select id="risk-filter" class="filter" aria-label="Filtrar por riesgo"><option value="">Todos los riesgos</option><option>Alto</option><option>Medio</option><option>Bajo</option></select>
          <select id="status-filter" class="filter" aria-label="Filtrar por estado"><option value="">Todos los estados</option>${[...new Set(catalog.functions.map((item) => item.status))].sort().map((value) => `<option>${value}</option>`).join("")}</select>
        </div>
        <div class="mt-3 flex flex-wrap items-center justify-between gap-3"><p id="function-count" class="font-mono text-xs text-slate-500"></p><div class="no-print flex gap-2"><a class="route-link" href="../../docs/catalogo_funciones.csv" download><i data-lucide="download"></i> CSV</a><a class="route-link" href="../../docs/catalogo_funciones.json" download><i data-lucide="download"></i> JSON</a></div></div>
        <div class="table-wrap mt-4 max-h-[42rem] overflow-y-auto"><table><thead><tr><th>Función</th><th>Propietario/plataforma</th><th>Riesgo/estado/CC</th><th>Archivo/línea</th><th>Entra desde</th><th>Sale hacia</th></tr></thead><tbody id="function-rows"></tbody></table></div>
      </section>

      <section id="api" class="section">
        <p class="eyebrow">Fronteras del sistema</p><h2 class="section-title">REST, SSE, WebSocket y SQLite</h2>
        <p class="section-copy">Las mutaciones REST requieren X-App-Token. La telemetría y los eventos viajan por SSE hacia la HMI y por WebSocket entre Python y el ESP32.</p>
        <div class="mt-7 grid gap-6 xl:grid-cols-2">
          <article class="card"><h3 class="text-lg font-semibold">Rutas HTTP detectadas</h3><div class="table-wrap mt-4 max-h-[34rem] overflow-y-auto"><table><thead><tr><th>Método</th><th>Ruta</th><th>Función</th></tr></thead><tbody>${catalog.http_routes.map((route) => `<tr><td><span class="badge badge-low">${esc(route.methods)}</span></td><td class="font-mono text-xs">${esc(route.path)}</td><td>${esc(route.function)}</td></tr>`).join("")}</tbody></table></div></article>
          <article class="card"><h3 class="text-lg font-semibold">Contrato de paso v3</h3><pre class="mt-4 overflow-x-auto rounded-xl bg-black/30 p-4 font-mono text-xs leading-6 text-cyan"><code>{
  "cmd": "step",
  "heading": 90.0,
  "cm": 150.0,
  "seq": 7
}</code></pre><div class="diagram mermaid">${diagrams.close}</div></article>
        </div>
      </section>

      <section id="datos" class="section">
        <p class="eyebrow">SQLite seudonimizada</p><h2 class="section-title">Sesiones, comandos, eventos y telemetría</h2>
        <p class="section-copy">Resumen documental sin IDs, tiempos absolutos, payloads, SSID, IP ni tokens. La base cruda no forma parte del portal.</p>
        <div class="mt-8 grid gap-6 xl:grid-cols-2"><article class="card"><h3 class="text-lg font-semibold">Sesiones y volumen</h3><div class="mt-4 h-80"><canvas id="sessions-chart"></canvas></div></article><article class="card"><h3 class="text-lg font-semibold">Estados de telemetría</h3><div class="mt-4 h-80"><canvas id="states-chart"></canvas></div></article></div>
        <article class="card mt-6"><h3 class="text-lg font-semibold">Pose, yaw y PWM de una sesión de ruta</h3><p class="mt-2 text-sm text-slate-500">Serie reducida para documentación; no sustituye el archivo CSV exportable.</p><div class="mt-4 h-[28rem]"><canvas id="telemetry-chart"></canvas></div></article>
        <div class="mt-6 flex flex-wrap gap-3"><a class="route-link" href="../../docs/datos_sqlite_documentales.json" download><i data-lucide="download"></i> Descargar resumen JSON</a><a class="route-link" href="../../docs/datos_y_privacidad.md">Privacidad, exportación y borrado</a></div>
      </section>

      <section id="manual" class="section">
        <p class="eyebrow">Procedimientos operativos</p><h2 class="section-title">Manual de uso</h2>
        <p class="section-copy">Secuencias concebidas para ejecución supervisada. Ninguna instrucción autoriza operar en suelo antes de la prueba eléctrica.</p>
        <div class="mt-8 grid gap-6 lg:grid-cols-2">
          ${manuals.map((manual) => `<article class="card"><h3 class="text-xl font-semibold text-white">${manual.title}</h3><ol class="mt-6">${manual.steps.map((step, index) => `<li class="manual-step"><span class="font-mono text-xs text-mint">${String(index + 1).padStart(2, "0")}</span><p class="mt-1 leading-6 text-slate-300">${step}</p></li>`).join("")}</ol></article>`).join("")}
        </div>
      </section>

      <section id="glosario" class="section">
        <p class="eyebrow">Lectura académica</p><h2 class="section-title">Glosario esencial</h2>
        <p class="section-copy">Definiciones cortas para leer diagramas, código y procedimientos sin asumir experiencia previa.</p>
        <div class="mt-7 grid gap-4 md:grid-cols-2 xl:grid-cols-3">${glossary.map(([term, definition]) => `<article class="card"><h3 class="font-mono text-mint">${term}</h3><p class="mt-2 text-sm leading-6 text-slate-400">${definition}</p></article>`).join("")}</div>
        <a class="route-link mt-6 inline-flex" href="../../docs/glosario_y_referencias.md">Abrir glosario completo y bibliografía primaria</a>
      </section>

      <section id="seguridad" class="section">
        <p class="eyebrow">Puerta física obligatoria</p><h2 class="section-title">Seguridad antes del suelo</h2>
        <p class="section-copy">El software limita energía y detecta fallos, pero no puede sustituir fusibles, capacitores ni medición de corriente.</p>
        <div class="mt-8 grid gap-6 xl:grid-cols-[.85fr_1.15fr]">
          <article class="card border-coral/30"><i data-lucide="alert-triangle" class="text-coral"></i><h3 class="mt-4 text-xl font-semibold">No aceptado todavía</h3><p class="mt-3 leading-7 text-slate-400">Los videos no demuestran corriente de arranque o rotor bloqueado. La prueba reciente además mostró giro sin ticks de encoder.</p></article>
          <article class="card"><h3 class="text-xl font-semibold">Lista de comprobación</h3><div class="mt-5 grid gap-3 sm:grid-cols-2">${["Ruedas elevadas", "Fuente limitada a 0.5 A", "Fusible de 1 A en prueba", "≥100 µF en VMOT–GND", "0.1 µF por motor", "PWM cero antes de VMOT", "Corriente sostenida <1 A", "Sólo un controlador conectado"].map((item) => `<label class="flex gap-3 rounded-lg border border-line bg-black/10 p-3 text-sm text-slate-300"><input type="checkbox" class="mt-1 accent-[#39e6aa]" />${item}</label>`).join("")}</div></article>
        </div>
      </section>

      <section id="evidencia" class="section">
        <p class="eyebrow">Tutorial audiovisual</p><h2 class="section-title">Evidencia y fotogramas guiados</h2>
        <p class="section-copy">Las versiones comprimidas y normalizadas cuentan con SHA-256 y Git LFS. Sus 24 fotogramas por video sirven como guía visual, no como sustituto de telemetría o medición física.</p>
        <article class="card mt-8 border-amber/30">
          <div class="flex items-start gap-4"><i data-lucide="activity" class="mt-1 shrink-0 text-amber"></i><div><h3 class="text-lg font-semibold">Caso trazable de reconexión</h3><p class="mt-2 leading-7 text-slate-400">La sesión Python se conservó, pero el ESP32 reapareció desarmado, sin calibración y con <code>last_seq=0</code>. La ruta fue rechazada antes de crear un <code>step</code>; la recalibración posterior terminó en <code>cal_stall_left</code> con PWM 0/0.</p><a class="mt-3 inline-flex font-mono text-xs text-mint hover:underline" href="../../evidencia/incidentes/2026-07-30_reinicio_y_calibracion.md">Abrir extracto SQLite y límites de interpretación →</a></div></div>
        </article>
        <div class="mt-8 grid gap-8 xl:grid-cols-2">
          ${evidence.map((item) => `<article class="video-card card"><div class="flex items-start justify-between gap-4"><div><h3 class="text-xl font-semibold">${item.title}</h3><p class="mt-1 font-mono text-xs text-slate-500">${item.duration} · SHA ${item.hash}</p></div><img class="h-28 w-20 rounded-lg object-cover" src="${assetPath(item.thumbnail)}" alt="Miniatura animada de ${item.title}"></div><video class="mt-5" controls preload="metadata"><source src="${encodeURI(assetPath(item.src))}" type="video/mp4" />Tu navegador no puede reproducir este video.</video><div class="mt-5 grid grid-cols-2 gap-3 sm:grid-cols-3 xl:grid-cols-4">${item.frames.map(([time, src, caption]) => `<figure><img class="timeline-img" loading="lazy" src="${assetPath(src)}" alt="${caption} a ${time}" /><figcaption class="mt-2 text-xs text-slate-500"><span class="font-mono text-cyan">${time}</span> · ${caption}</figcaption></figure>`).join("")}</div></article>`).join("")}
        </div>
      </section>

      <footer class="py-10 text-sm text-slate-600"><p>Robot S3 · documentación local reproducible · No publicar evidencia sin revisión de privacidad.</p></footer>
    </main>
  </div>`;

createIcons({ icons: iconSet, attrs: { width: 18, height: 18, "aria-hidden": "true" } });

mermaid.initialize({ startOnLoad: false, theme: "neutral", securityLevel: "strict", flowchart: { htmlLabels: true, curve: "basis" }, sequence: { useMaxWidth: false } });
const mermaidRenderTasks = [];
for (const node of document.querySelectorAll(".mermaid")) {
  mermaidRenderTasks.push(mermaid.run({ nodes: [node] }).catch((error) => {
    node.innerHTML = `<p class="p-4 text-sm text-red-700">No se pudo renderizar este diagrama: ${esc(error.message)}</p>`;
  }));
}

const chartLabels = Object.keys(catalog.stats.by_folder);
new Chart(document.querySelector("#coverage-chart"), {
  type: "doughnut",
  data: { labels: chartLabels, datasets: [{ data: chartLabels.map((label) => catalog.stats.by_folder[label]), backgroundColor: ["#39e6aa", "#5bd8ff", "#ffca67", "#ff7a7a", "#8b9cff", "#67e8f9", "#a7f3d0", "#c4b5fd"], borderColor: "#0c1929", borderWidth: 3 }] },
  options: { maintainAspectRatio: false, plugins: { legend: { position: "bottom", labels: { color: "#94a3b8", boxWidth: 10, padding: 16 } } } }
});

const chartText = { color: "#94a3b8" };
new Chart(document.querySelector("#sessions-chart"), {
  type: "bar",
  data: { labels: sqliteSummary.sessions.map((item) => item.label), datasets: [
    { label: "Comandos", data: sqliteSummary.sessions.map((item) => item.commands), backgroundColor: "#39e6aa" },
    { label: "Eventos", data: sqliteSummary.sessions.map((item) => item.events), backgroundColor: "#5bd8ff" },
    { label: "Telemetría", data: sqliteSummary.sessions.map((item) => item.samples), backgroundColor: "#ffca67" },
  ] },
  options: { maintainAspectRatio: false, scales: { x: { ticks: chartText }, y: { type: "logarithmic", ticks: chartText } }, plugins: { legend: { labels: chartText } } },
});
new Chart(document.querySelector("#states-chart"), {
  type: "doughnut",
  data: { labels: Object.keys(sqliteSummary.telemetry_by_state), datasets: [{ data: Object.values(sqliteSummary.telemetry_by_state), backgroundColor: ["#39e6aa", "#5bd8ff", "#ffca67", "#ff7a7a", "#8b9cff"] }] },
  options: { maintainAspectRatio: false, plugins: { legend: { position: "bottom", labels: chartText } } },
});
new Chart(document.querySelector("#telemetry-chart"), {
  type: "line",
  data: { labels: sqliteSummary.telemetry_series.map((item) => item.t), datasets: [
    { label: "X cm", data: sqliteSummary.telemetry_series.map((item) => item.x_cm), borderColor: "#5bd8ff", pointRadius: 0 },
    { label: "Y cm", data: sqliteSummary.telemetry_series.map((item) => item.y_cm), borderColor: "#39e6aa", pointRadius: 0 },
    { label: "Yaw °", data: sqliteSummary.telemetry_series.map((item) => item.yaw), borderColor: "#8b9cff", pointRadius: 0 },
    { label: "PWM L", data: sqliteSummary.telemetry_series.map((item) => item.pwm_l), borderColor: "#ffca67", pointRadius: 0 },
    { label: "PWM R", data: sqliteSummary.telemetry_series.map((item) => item.pwm_r), borderColor: "#ff7a7a", pointRadius: 0 },
  ] },
  options: { maintainAspectRatio: false, interaction: { mode: "index", intersect: false }, scales: { x: { title: { display: true, text: "segundos", color: "#94a3b8" }, ticks: chartText }, y: { ticks: chartText } }, plugins: { legend: { labels: chartText } } },
});

const cy = cytoscape({
  container: document.querySelector("#call-graph"),
  elements: [],
  style: [
    { selector: "node", style: { "background-color": "#15344a", "border-color": "#39e6aa", "border-width": 1, label: "data(label)", color: "#dbeafe", "font-size": 8, "text-valign": "bottom", "text-margin-y": 6, width: 24, height: 24 } },
    { selector: 'node[risk="Medio"]', style: { "background-color": "#4a3717", "border-color": "#ffca67" } },
    { selector: 'node[risk="Alto"]', style: { "background-color": "#5b1f2a", "border-color": "#ff7a7a" } },
    { selector: "edge", style: { width: 1, "line-color": "#35536b", "target-arrow-color": "#5bd8ff", "target-arrow-shape": "triangle", "curve-style": "bezier" } },
    { selector: 'edge[interaction!="síncrona"]', style: { "line-style": "dashed", "line-color": "#ffca67" } },
    { selector: ".faded", style: { opacity: 0.14 } },
    { selector: ".focused", style: { "border-width": 3, "border-color": "#5bd8ff" } },
  ],
});
const graphFolder = document.querySelector("#graph-folder");
const graphDetail = document.querySelector("#graph-detail");
function renderGraph() {
  const selected = catalog.functions.filter((item) => item.folder === graphFolder.value);
  const byName = new Map();
  selected.forEach((item, index) => { if (!byName.has(item.name)) byName.set(item.name, `f${index}`); });
  const nodes = selected.map((item, index) => ({ data: { id: `f${index}`, label: item.name, ...item } }));
  const edges = [];
  selected.forEach((item, index) => item.calls.forEach((called, callIndex) => { const target = byName.get(called); if (target && target !== `f${index}`) edges.push({ data: { id: `e${index}_${callIndex}`, source: `f${index}`, target, interaction: item.interaction } }); }));
  cy.elements().remove();
  cy.add([...nodes, ...edges]);
  cy.layout({ name: "cose", animate: false, padding: 40, idealEdgeLength: 70, nodeRepulsion: 5000 }).run();
  document.querySelector("#graph-count").textContent = `${nodes.length} nodos · ${edges.length} aristas`;
}
graphFolder.addEventListener("change", renderGraph);
renderGraph();
cy.on("tap", "node", (event) => { const node = event.target; const data = node.data(); cy.elements().addClass("faded"); node.closedNeighborhood().removeClass("faded"); node.addClass("focused"); graphDetail.innerHTML = `<strong class="text-white">${esc(data.qualified_name)}</strong> · CC ${data.complexity} · ${esc(data.risk)} · ${esc(data.status)}<br><span>${esc(data.owner)} · ${esc(data.interaction)}</span> · <a class="text-mint hover:underline" href="${repoPath(data.path)}#L${data.line}">${esc(data.path)}:${data.line}</a>`; });
cy.on("tap", (event) => { if (event.target === cy) cy.elements().removeClass("faded focused"); });
document.querySelector("#graph-fullscreen").addEventListener("click", () => document.querySelector("#call-graph").requestFullscreen?.());

const rows = document.querySelector("#function-rows");
const count = document.querySelector("#function-count");
const search = document.querySelector("#function-search");
const folderFilter = document.querySelector("#folder-filter");
const platformFilter = document.querySelector("#platform-filter");
const riskFilter = document.querySelector("#risk-filter");
const statusFilter = document.querySelector("#status-filter");
function renderFunctions() {
  const term = search.value.trim().toLowerCase();
  const folder = folderFilter.value;
  const filtered = catalog.functions.filter((item) => (!folder || item.folder === folder) && (!platformFilter.value || item.platform === platformFilter.value) && (!riskFilter.value || item.risk === riskFilter.value) && (!statusFilter.value || item.status === statusFilter.value) && (!term || `${item.qualified_name} ${item.path} ${item.owner} ${item.status} ${item.callers.join(" ")} ${item.calls.join(" ")}`.toLowerCase().includes(term))).sort((a, b) => a.qualified_name.localeCompare(b.qualified_name, "es"));
  count.textContent = `${filtered.length} de ${catalog.functions.length} funciones/prototipos`;
  rows.innerHTML = filtered.slice(0, 250).map((item) => `<tr><td><code>${esc(item.qualified_name)}</code><div class="mt-1 text-xs text-slate-600">${esc(item.signature)}</div></td><td class="text-xs">${esc(item.owner)}<br><span class="text-slate-600">${esc(item.platform)} · ${esc(item.folder)}</span></td><td><span class="badge ${item.risk === "Alto" ? "badge-high" : item.risk === "Medio" ? "badge-medium" : "badge-low"}">${esc(item.risk)}</span><div class="mt-1 text-xs text-slate-500">CC ${item.complexity} · ${esc(item.status)}</div></td><td class="font-mono text-xs"><a class="text-mint hover:underline" href="${repoPath(item.path)}#L${item.line}">${esc(item.path)}:${item.line}</a></td><td class="text-xs">${item.callers.length ? item.callers.slice(0, 8).map((call) => `<code>${esc(call)}</code>`).join(", ") : "—"}</td><td class="text-xs">${item.calls.length ? item.calls.slice(0, 8).map((call) => `<code>${esc(call)}</code>`).join(", ") : "—"}</td></tr>`).join("");
  if (filtered.length > 250) rows.insertAdjacentHTML("beforeend", `<tr><td colspan="6" class="text-center text-slate-500">Refina la búsqueda para ver los ${filtered.length - 250} resultados restantes.</td></tr>`);
}
search.addEventListener("input", renderFunctions);
[folderFilter, platformFilter, riskFilter, statusFilter].forEach((filter) => filter.addEventListener("change", renderFunctions));
renderFunctions();

document.querySelectorAll(".diagram-tab").forEach((button) => button.addEventListener("click", async () => {
  document.querySelectorAll(".diagram-tab").forEach((item) => item.className = "diagram-tab rounded-full border border-line px-4 py-2 text-sm text-slate-400");
  button.className = "diagram-tab rounded-full border border-mint bg-mint/10 px-4 py-2 text-sm text-mint";
  const target = document.querySelector("#dynamic-diagram");
  target.removeAttribute("data-processed");
  target.innerHTML = diagrams[button.dataset.diagram];
  await mermaid.run({ nodes: [target] });
}));

const navToggle = document.querySelector("#nav-toggle");
const nav = document.querySelector("#nav");
navToggle.addEventListener("click", () => { const open = nav.classList.toggle("hidden"); navToggle.setAttribute("aria-expanded", String(!open)); });
const links = [...document.querySelectorAll(".nav-link")];
const observer = new IntersectionObserver((entries) => entries.forEach((entry) => { if (entry.isIntersecting) { links.forEach((link) => link.classList.toggle("active", link.hash === `#${entry.target.id}`)); } }), { rootMargin: "-20% 0px -70%", threshold: 0 });
document.querySelectorAll("main section[id]").forEach((section) => observer.observe(section));

const globalSearch = document.querySelector("#global-search");
const globalResults = document.querySelector("#global-results");
const searchIndex = [
  ...catalog.functions.map((item) => ({ section: "funciones", title: item.qualified_name, text: `${item.path} ${item.owner} ${item.risk} ${item.status}` })),
  ...findings.map((item) => ({ section: "resumen", title: `${item.id} · ${item.title}`, text: item.detail })),
  ...manuals.map((item) => ({ section: "manual", title: item.title, text: item.steps.join(" ") })),
  ...glossary.map(([title, text]) => ({ section: "glosario", title, text })),
];
globalSearch.addEventListener("input", () => {
  const term = globalSearch.value.trim().toLocaleLowerCase("es");
  if (term.length < 2) { globalResults.classList.add("hidden"); globalResults.innerHTML = ""; return; }
  const matches = searchIndex.filter((item) => `${item.title} ${item.text}`.toLocaleLowerCase("es").includes(term)).slice(0, 15);
  globalResults.innerHTML = matches.length ? matches.map((item) => `<a class="block rounded px-2 py-2 text-slate-300 hover:bg-white/5 hover:text-mint" href="#${item.section}"><strong>${esc(item.title)}</strong><br><span class="text-slate-600">${esc(item.text).slice(0, 90)}</span></a>`).join("") : `<p class="p-2 text-slate-600">Sin coincidencias.</p>`;
  globalResults.classList.remove("hidden");
});
globalResults.addEventListener("click", () => globalResults.classList.add("hidden"));
document.querySelector("#print-page").addEventListener("click", () => window.print());
document.addEventListener("keydown", (event) => {
  if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === "k") { event.preventDefault(); globalSearch.focus(); }
  if (event.key === "Escape") { globalSearch.value = ""; globalResults.classList.add("hidden"); }
});

const initialSection = document.body.dataset.section;
if (initialSection && initialSection !== "resumen" && !location.hash) {
  Promise.allSettled(mermaidRenderTasks).then(() => {
    requestAnimationFrame(() => document.querySelector(`#${initialSection}`)?.scrollIntoView());
  });
}

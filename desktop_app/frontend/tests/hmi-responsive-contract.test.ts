import { existsSync, readdirSync, readFileSync } from "node:fs";
import { join } from "node:path";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";

const hmiDir = fileURLToPath(new URL("../../robot_app/hmi", import.meta.url));
const hmiPath = join(hmiDir, "index.html");

function loadCompleteHmi(): string {
  let content = readFileSync(hmiPath, "utf8");
  const cardsDir = join(hmiDir, "cards");
  const cssDir = join(hmiDir, "css");
  const jsDir = join(hmiDir, "js");

  if (existsSync(cardsDir)) {
    const cardFiles = readdirSync(cardsDir).filter(f => f.endsWith(".html"));
    const cardsHtml = cardFiles.map(f => readFileSync(join(cardsDir, f), "utf8")).join("\n");
    content = content.replace("<!-- <!-- INCLUDE_CARDS --> -->", cardsHtml);
  }
  if (existsSync(cssDir)) {
    const cssFiles = readdirSync(cssDir).filter(f => f.endsWith(".css"));
    const allCss = cssFiles.map(f => readFileSync(join(cssDir, f), "utf8")).join("\n");
    content = content.replace("/* <!-- INCLUDE_CSS --> */", allCss);
  }
  if (existsSync(jsDir)) {
    const jsFiles = readdirSync(jsDir).filter(f => f.endsWith(".js"));
    const allJs = jsFiles.map(f => readFileSync(join(jsDir, f), "utf8")).join("\n");
    content = content.replace("/* <!-- INCLUDE_JS --> */", allJs);
  }
  return content;
}

const html = loadCompleteHmi();

function occurrences(pattern: RegExp): number {
  return html.match(pattern)?.length ?? 0;
}

describe("contrato responsive del HMI canónico", () => {
  it("conserva un viewport accesible y compatible con áreas seguras", () => {
    const viewport = html.match(/<meta\s+name=["']viewport["']\s+content=["']([^"']+)["']/i)?.[1];

    expect(viewport).toBeDefined();
    expect(viewport).toContain("width=device-width");
    expect(viewport).toContain("viewport-fit=cover");
    expect(viewport).toContain("user-scalable=yes");
    expect(viewport).not.toMatch(/maximum-scale\s*=\s*1(?:\.0)?(?:,|$)/);
    expect(html).not.toMatch(/user-scalable\s*=\s*no/i);

    for (const edge of ["top", "right", "bottom", "left"]) {
      const safeArea = new RegExp(`env\\(\\s*safe-area-inset-${edge}(?:\\s*,|\\s*\\))`);
      expect(safeArea.test(html), `falta safe-area-inset-${edge}`).toBe(true);
    }
  });

  it("declara y consume los tokens mínimos de tacto y tipografía", () => {
    expect(/--touch-target-min\s*:\s*44px\s*;/.test(html), "falta --touch-target-min: 44px").toBe(true);
    expect(/--touch-target-critical\s*:\s*48px\s*;/.test(html), "falta --touch-target-critical: 48px").toBe(true);
    expect(/--font-size-min\s*:\s*12px\s*;/.test(html), "falta --font-size-min: 12px").toBe(true);
    expect(/--font-size-operational\s*:\s*14px\s*;/.test(html), "falta --font-size-operational: 14px").toBe(true);

    expect(occurrences(/var\(--touch-target-min\)/g)).toBeGreaterThanOrEqual(2);
    expect(occurrences(/var\(--touch-target-critical\)/g)).toBeGreaterThanOrEqual(1);
    expect(occurrences(/var\(--font-size-min\)/g)).toBeGreaterThanOrEqual(1);
    expect(occurrences(/var\(--font-size-operational\)/g)).toBeGreaterThanOrEqual(1);
  });

  it("mantiene los rangos teléfono, tablet, escritorio y landscape compacto", () => {
    for (const width of [480, 768, 1100]) {
      const breakpoint = new RegExp(`@media\\s*\\(max-width\\s*:\\s*${width}px\\)`);
      expect(breakpoint.test(html), `falta breakpoint de ${width}px`).toBe(true);
    }
    expect(html).toMatch(
      /@media\s*\(max-height\s*:\s*500px\)\s*and\s*\(orientation\s*:\s*landscape\)/,
    );
  });

  it("expone toda la navegación sin recortar ni deshabilitar pestañas", () => {
    const tablist = html.match(/<nav\b[^>]*role=["']tablist["'][^>]*>([\s\S]*?)<\/nav>/i)?.[1];
    const tabs = tablist?.match(/<button\b[^>]*class=["'][^"']*\btab-btn\b[^"']*["'][^>]*>/gi) ?? [];

    expect(tablist).toBeDefined();
    expect(tabs).toHaveLength(5);
    for (const tab of tabs) {
      expect(tab).toMatch(/\brole=["']tab["']/i);
      expect(tab).toMatch(/\bid=["']tab-control-[^"']+["']/i);
      expect(tab).toMatch(/\baria-controls=["']tab-[^"']+["']/i);
      expect(tab).toMatch(/\baria-selected=["'](?:true|false)["']/i);
      expect(tab).not.toMatch(/\sdisabled(?:\s|=|>)/i);
    }

    const touchTab = tabs.find((tab) => /data-tab=["']touch["']/i.test(tab));
    expect(touchTab).toMatch(/aria-disabled=["']true["']/i);
    expect(touchTab).toMatch(/aria-describedby=["'][^"']+["']/i);

    expect(
      /\.tab-bar\s*\{[^}]*overflow-x\s*:\s*auto\s*;/s.test(html),
      "la barra de pestañas debe desplazarse horizontalmente",
    ).toBe(true);
    for (const tabName of ["conexion", "mapa", "testing", "touch", "rutas"]) {
      const panelRelation = new RegExp(
        `<div\\b[^>]*id=["']tab-${tabName}["'][^>]*aria-labelledby=["']tab-control-${tabName}["']`,
        "i",
      );
      expect(panelRelation.test(html), `falta relación ARIA para ${tabName}`).toBe(true);
    }
    for (const key of ["ArrowRight", "ArrowLeft", "Home", "End"]) {
      expect(html).toContain(`event.key==='${key}'`);
    }
  });

  it("evita patrones que ocultan problemas de reflow o bloquean el zoom", () => {
    expect(/\b(?:body|html)\s*\{[^}]*\bmin-width\s*:\s*\d+px\s*;/s.test(html)).toBe(false);
    expect(/\b(?:body|html)\s*\{[^}]*\boverflow-x\s*:\s*(?:hidden|clip)\s*;/s.test(html)).toBe(false);
    expect(/\b(?:body|html)\s*\{[^}]*\b(?:zoom|transform)\s*:/s.test(html)).toBe(false);
  });

  it("mantiene el panel técnico glass con logs jerárquicos y acciones seguras", () => {
    for (const token of ["--glass-surface", "--glass-border", "--shadow-elevated"]) {
      expect(html).toContain(token);
    }
    expect(html).toMatch(/class=["']log-filters["'][^>]*role=["']group["']/i);
    expect(occurrences(/class=["'][^"']*log-filter-btn[^"']*["'][^>]*aria-pressed=["']true["']/gi)).toBe(4);
    expect(html).toMatch(/class=["']log-actions["'][^>]*aria-label=["']Acciones de registros["']/i);
    expect(html).toMatch(/id=["']btn-purge-db-logs["'][^>]*>[\s\S]*?requiere confirmación/i);
    expect(html).toContain("document.createElement('time')");
    expect(html).toContain("detail.textContent = message");
    expect(html).not.toMatch(/id=["']select-purge-days["'][^>]*\sstyle=/i);
  });

  it("expone rectangular, angular descompuesto y angular vectorial experimental", () => {
    expect(html).toContain('<option value="polar">Angular por descomposición (X→Y)</option>');
    expect(html).toContain('<option value="vectorial">Angular vectorial (directa)</option>');
    expect(html).toContain("class RoutePlanningMode");
    expect(html).toContain("class RectangularRouteMode extends RoutePlanningMode");
    expect(html).toContain("class AngularRouteMode extends RoutePlanningMode");
    expect(html).toContain("class AngularDecompositionRouteMode extends AngularRouteMode");
    expect(html).toContain("class AngularVectorialRouteMode extends AngularRouteMode");
    expect(html).toContain("logicalSteps: []");
    expect(html).toContain("routeAngularOverlay");
    expect(html).toContain("Hipotenusa / vector");
    expect(html).toContain("Componente X ejecutable");
    expect(html).toContain("Componente Y ejecutable");
    expect(html).toContain("Ángulo relativo");
    expect(html).toContain("experimental_vectorial_routes");
    expect(html).toContain("vectorialReadiness");
  });

  it("dimensiona hora y nivel del log por contenido antes del reflow móvil", () => {
    expect(html).toMatch(/\.log-entry\s*\{[^}]*grid-template-columns:\s*max-content\s+max-content\s+minmax\(0,1fr\)/s);
    expect(html).toMatch(/@media\s*\(max-width:\s*480px\)[\s\S]*?\.log-entry\s*\{[^}]*grid-template-columns:\s*1fr\s+auto/s);
  });

  it("gobierna Touch por manual_drive_v1 y declara el ciclo seguro local", () => {
    expect(html).toContain("manual_drive_v1");
    expect(html).toContain("/api/v1/manual/${path}");
    for (const action of ["start", "drive", "stop"]) expect(html).toContain(`manual('${action}'`);
    for (const event of ["touchend", "touchcancel", "visibilitychange", "pagehide", "blur"]) {
      expect(html).toContain(event);
    }
    expect(html).toContain("Programar recorrido");
    expect(html).toContain("Previsualizar");
    expect(html).toContain("Descartar");
    expect(html).toContain("Confirma que el área está despejada");
    expect(html).toContain("Traza medida");
    expect(html).toContain("Vector simplificado");
    expect(html).toContain("Segmentos ≤256");
  });

  it("implementa el modo de ventanas con selector de espacios y catálogo de forks", () => {
    expect(html).toContain('id="ws-mode-tabs"');
    expect(html).toContain('id="ws-mode-tiling"');
    expect(html).toContain('id="ws-mode-floating"');
    expect(html).toContain('id="ws-btn-add-fork"');
    expect(html).toContain('id="glass-fork-menu"');
    expect(html).toContain("const WindowManager = {");
    expect(html).toContain("const HMI_FORK_REGISTRY = {");
    expect(html).toContain("renderForkMenu()");
  });

  it("define tokens de glassmorphism de alto rendimiento y salvaguardas de GPU", () => {
    for (const token of [
      "--glass-surface-translucent",
      "--glass-surface-card",
      "--glass-surface-dock",
      "--glass-border-glow-plan",
      "--glass-border-glow-boost",
      "--glass-specular-edge",
      "--glass-blur-safe",
      "--glass-blur-dock",
      "--glass-blur-boost",
    ]) {
      expect(html).toContain(token);
    }

    expect(html).toMatch(/\.is-interacting[\s\S]*?backdrop-filter:\s*none\s*!important/);
    expect(html).toMatch(/body\.is-touch-interacting[\s\S]*?backdrop-filter:\s*none\s*!important/);
  });

  it("incorpora la barra de bifurcaciones de ruta y superposición A/B", () => {
    expect(html).toContain('id="btn-fork-route"');
    expect(html).toContain('id="route-branch-select"');
    expect(html).toContain('id="btn-compare-branches"');
    expect(html).toContain('id="btn-delete-branch"');
    expect(html).toContain('id="route-fork-metrics"');
    expect(html).toContain("forkCurrentRoute(");
    expect(html).toContain("switchBranch(");
    expect(html).toContain("toggleCompareAB(");
    expect(html).toContain("Ruta Fork [B]");
  });

  it("incluye instrumentación visual para el HUD cinético y térmico Boost DRV8833", () => {
    expect(html).toContain('id="tab-boost"');
    expect(html).toContain('id="boost-burst-bar"');
    expect(html).toContain('id="boost-burst-time"');
    expect(html).toContain('id="boost-cooldown-status"');
    expect(html).toContain('id="boost-diff-val"');
    expect(html).toContain('id="boost-left-bar"');
    expect(html).toContain('id="boost-right-bar"');
    expect(html).toContain('id="boost-phase-1"');
    expect(html).toContain('id="boost-phase-2"');
    expect(html).toContain('id="boost-phase-3"');
    expect(html).toContain("const BoostHUD = {");
  });

  it("implementa la disposición Master-Detail de 2 columnas en el planeador de rutas", () => {
    expect(html).toContain("route-planner-layout");
    expect(html).toContain("route-planner-col-control");
    expect(html).toContain("route-planner-col-view");
    expect(html).toContain("route-queue-scroll");
    expect(html).toContain('id="card-route-queue"');
    expect(html).toContain('id="route-list"');
  });

  it("implementa barra de seguridad superior con rearme contextual y recuperación de doble nivel", () => {
    expect(html).toContain('id="btn-reset-hmi"');
    expect(html).toContain('id="btn-reboot-robot"');
    expect(html).toContain('id="btn-clear-fault" hidden disabled');
    expect(html).toContain('id="btn-estop"');
    expect(html).toContain("Reset FSM");
    expect(html).toContain("Reiniciar Robot");
    expect(html).toContain("Rearmar sistema");
  });

  it("incorpora el cockpit dedicado de Retorno Ockham con simulación y badges", () => {
    expect(html).toContain('id="card-route-ockham"');
    expect(html).toContain('id="ockham-badge-settling"');
    expect(html).toContain('id="ockham-badge-realigning"');
    expect(html).toContain('id="ockham-current-stage"');
    expect(html).toContain('id="ockham-val-lat"');
    expect(html).toContain('id="ockham-val-ang"');
    expect(html).toContain('id="ockham-val-long"');
    expect(html).toContain('id="btn-route-ockham"');
    expect(html).toContain('id="route-ockham-confirm-box"');
    expect(html).toContain('id="btn-confirm-ockham"');
    expect(html).toContain('id="btn-cancel-ockham"');
    expect(html).toContain("Simulación Ockham");
  });

  it("incorpora el panel de configuracion glassmorphic, persistencia y boton engranaje", () => {
    expect(html).toContain('id="btn-open-settings"');
    expect(html).toContain('id="modal-settings"');
    expect(html).toContain('id="setting-telemetry-mode"');
    expect(html).toContain('id="setting-theme"');
    expect(html).toContain('id="setting-language"');
    expect(html).toContain('id="setting-default-mode"');
    expect(html).toContain("const SettingsManager = {");
    expect(html).toContain("hmi_user_settings");
  });

  it("expone etiquetas amigables para guardar ruta y crear nueva ruta planificada", () => {
    expect(html).toContain("Crear nueva ruta planificada");
    expect(html).toContain("Guardar la Ruta");
    expect(html).toContain("header-recovery-group");
  });

  it("habilita el redimensionamiento de mosaicos en workspace-tiling", () => {
    expect(html).toMatch(/#hmi-workspace\.workspace-tiling\s+\.tab-content\s*\{[^}]*resize:\s*vertical/);
    expect(html).toMatch(/#hmi-workspace\.workspace-tiling\s+\.tab-content\s*\{[^}]*overflow:\s*hidden/);
  });

  it("garantiza que las tarjetas críticas no contengan caracteres emoji", () => {
    const emojiRegex = /[\u{1F300}-\u{1FAFF}\u{26A1}\u{2705}\u{274C}\u{1F6E0}\u{1F4D0}\u{1F4F1}\u{1F500}]/u;
    for (const card of ["header.html", "tab_conexion.html", "tab_rutas.html", "hud_boost.html", "footer_panels.html"]) {
      const cardPath = join(hmiDir, "cards", card);
      if (existsSync(cardPath)) {
        const cardContent = readFileSync(cardPath, "utf8");
        expect(emojiRegex.test(cardContent)).toBe(false);
      }
    }
  });

  it("garantiza alternancia de signo (+/-) agregando signo menos solitario en vacio o cero", () => {
    expect(html).toContain("data-toggle-sign");
    expect(html).toContain("inputmode=\"decimal\"");
    expect(html).toContain("val === '' || val === '0' || val === '0.0'");
    expect(html).toContain("input.value = '-'");
  });

  it("garantiza soporte de traducciones i18n y cambio dinamico de modo en SettingsManager", () => {
    expect(html).toContain("translations: {");
    expect(html).toContain("applyTranslations(");
    expect(html).toContain("WindowManager.setMode(this.settings.default_mode)");
    expect(html).toContain("id=\"btn-clear-fault-label\" class=\"btn-text\"");
  });

  it("preserva dimensiones de mosaico al cambiar entre modos de espacio de trabajo", () => {
    expect(html).toContain("tilingWidth");
    expect(html).toContain("tilingHeight");
    expect(html).toContain("ResizeObserver");
  });

  it("unifica la gestion de ventanas en la barra superior eliminando el dock inferior y anadiendo tres puntos", () => {
    expect(html).not.toContain('id="glass-dock"');
    expect(html).not.toContain('id="dock-items"');
    expect(html).toContain('id="ws-btn-add-fork"');
    expect(html).toContain('<span class="ws-label">···</span>');
    expect(html).toContain(".tab-btn.is-minimized");
    expect(html).toContain("ensureTaskbarButton(");
    expect(html).toContain("openWindow(");
  });

  it("implementa scroll universal en ventanas flotantes, mosaico vertical con scroll general y tirador tactil de 28px", () => {
    expect(html).toMatch(/\.window-resize-handle\s*\{[^}]*width:\s*28px/);
    expect(html).toMatch(/\.window-resize-handle\s*\{[^}]*height:\s*28px/);
    expect(html).toMatch(/\.window-resize-handle\s*\{[^}]*touch-action:\s*none/);
    expect(html).toMatch(/#hmi-workspace\.workspace-tiling\s*\{[^}]*flex-direction:\s*column/);
    expect(html).toMatch(/#hmi-workspace\.workspace-tiling\s*\{[^}]*overflow-y:\s*auto/);
    expect(html).toMatch(/#hmi-workspace\.workspace-tiling\s+\.window-resize-handle\s*\{[^}]*cursor:\s*ns-resize/);
    expect(html).toContain("connection-layout");
    expect(html).toContain("testing-grid");
    expect(html).toContain("touch-layout");
    expect(html).toContain("Otras ventanas, submódulos y alertas");
  });

  it("desduplica los botones de modo cine en charts de rutas y asegura maximizacion fija en mosaico", () => {
    expect(html).toContain("Cine (Plan)");
    expect(html).toContain("Cine (Telemetría)");
    expect(html).toMatch(/#hmi-workspace\.workspace-tiling\s+\.tab-content\.is-maximized\s*\{[^}]*position:\s*fixed/);
  });
});

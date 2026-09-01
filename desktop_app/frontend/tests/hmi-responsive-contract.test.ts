import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";

const hmiPath = fileURLToPath(new URL("../../robot_app/hmi/index.html", import.meta.url));
const html = readFileSync(hmiPath, "utf8");

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
});

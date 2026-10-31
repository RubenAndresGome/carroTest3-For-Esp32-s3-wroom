import { readFileSync } from "node:fs";
import { join } from "node:path";
import { fileURLToPath } from "node:url";
import { describe, expect, it } from "vitest";

const cssDir = fileURLToPath(new URL("../../robot_app/hmi/css", import.meta.url));
const read = (f: string) => readFileSync(join(cssDir, f), "utf8");

function bloqueTema(css: string, tema: string): string {
  const inicio = css.indexOf(`:root[data-theme="${tema}"]`);
  expect(inicio).toBeGreaterThan(-1);
  return css.slice(inicio, css.indexOf("}", inicio));
}

describe("Tokens --ui-* del tema claro", () => {
  const usados = new Set<string>();
  for (const f of ["layout.css", "components.css", "responsive.css"]) {
    for (const m of read(f).matchAll(/var\((--ui-[a-z-]+),\s*[^)]/g)) usados.add(m[1]);
  }

  it("cada token --ui-* usado lleva valor de respaldo y está definido en light-glass", () => {
    const claro = bloqueTema(read("theme.css"), "light-glass");
    expect(usados.size).toBeGreaterThan(5);
    for (const token of usados) expect(claro).toContain(`${token}:`);
  });

  it("el tema oscuro y el de alto contraste no definen --ui-* (usan el valor original)", () => {
    const theme = read("theme.css");
    const raiz = theme.slice(theme.indexOf(":root {"), theme.indexOf("}", theme.indexOf(":root {")));
    expect(raiz).not.toMatch(/--ui-[a-z-]+\s*:/);
    expect(bloqueTema(theme, "high-contrast")).not.toMatch(/--ui-[a-z-]+\s*:/);
  });

  it("la cadena FSM no usa tokens --ui-* (se conserva tal cual)", () => {
    const comp = read("components.css").replace(/\r\n/g, "\n");
    for (const [, sel, body] of comp.matchAll(/([^{}]+)\{([^{}]*)\}/g)) {
      if (/fsm/i.test(sel)) expect(body).not.toContain("--ui-");
    }
  });
});

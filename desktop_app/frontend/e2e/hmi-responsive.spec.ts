import AxeBuilder from "@axe-core/playwright";
import { expect, test, type Page } from "@playwright/test";

const viewports = [
  { name: "telefono-compacto-320", width: 320, height: 568 },
  { name: "telefono-compacto-360", width: 360, height: 640 },
  { name: "telefono-390", width: 390, height: 844 },
  { name: "telefono-412", width: 412, height: 915 },
  { name: "telefono-480", width: 480, height: 800 },
  { name: "telefono-landscape-640", width: 640, height: 360 },
  { name: "telefono-landscape-844", width: 844, height: 390 },
  { name: "tablet-600", width: 600, height: 960 },
  { name: "tablet-768", width: 768, height: 1024 },
  { name: "tablet-800", width: 800, height: 1280 },
  { name: "tablet-landscape-1024", width: 1024, height: 768 },
  { name: "escritorio-1280", width: 1280, height: 720 },
  { name: "escritorio-1440", width: 1440, height: 900 },
] as const;

const operationalPanels = ["conexion", "mapa", "testing", "rutas"] as const;

async function isolateEventStream(page: Page): Promise<void> {
  await page.route("**/api/v1/events", async (route) => {
    await route.fulfill({
      status: 200,
      contentType: "text/event-stream",
      body: "retry: 60000\n\n",
    });
  });
}

async function openPanel(page: Page, panel: (typeof operationalPanels)[number]): Promise<void> {
  if (panel === "conexion") return;
  await page.evaluate("Calibration.calibrated=true");
  await page.evaluate(`TabManager.switch(${JSON.stringify(panel)})`);
  await expect(page.locator(`#tab-${panel}`)).toBeVisible();
}

test.describe("HMI responsive en navegador real", () => {
  for (const viewport of viewports) {
    test(`${viewport.name}: reflow, targets y opciones`, async ({ page }) => {
      await page.setViewportSize({ width: viewport.width, height: viewport.height });
      await isolateEventStream(page);
      await page.goto("/", { waitUntil: "domcontentloaded" });
      await expect(page.locator(".tab-btn")).toHaveCount(5);
      await expect(page.locator('[data-tab="touch"]')).toHaveAttribute("aria-disabled", "true");

      for (const panel of operationalPanels) {
        await openPanel(page, panel);
        const metrics = await page.evaluate(() => {
          const visible = (element: Element): boolean => {
            const style = getComputedStyle(element);
            const rect = element.getBoundingClientRect();
            return style.display !== "none" && style.visibility !== "hidden" && rect.width > 0 && rect.height > 0;
          };
          const documentElement = document.documentElement;
          const controls = [...document.querySelectorAll("button, input, select, .route-axis-toggle label")]
            .filter((element) => !element.matches(".route-axis-toggle input"))
            .filter(visible);
          const localScrollSelector = ".tab-bar, .session-table-wrap, .data-table-wrap, .fsm-panel";
          const undersized = controls.flatMap((element) => {
            const rect = element.getBoundingClientRect();
            if (rect.width >= 44 && rect.height >= 44) return [];
            return [{ id: (element as HTMLElement).id || (element as HTMLElement).dataset.tab || element.tagName, width: rect.width, height: rect.height }];
          });
          const outsideViewport = controls.flatMap((element) => {
            if (element.closest(localScrollSelector)) return [];
            const rect = element.getBoundingClientRect();
            if (rect.left >= -0.5 && rect.right <= documentElement.clientWidth + 0.5) return [];
            return [{ id: (element as HTMLElement).id || element.tagName, left: rect.left, right: rect.right }];
          });
          const header = document.querySelector(".app-header")?.getBoundingClientRect();
          const tabBar = document.querySelector(".tab-bar")?.getBoundingClientRect();
          const estop = document.querySelector("#btn-estop")?.getBoundingClientRect();
          return {
            globalOverflow: documentElement.scrollWidth > documentElement.clientWidth,
            undersized,
            outsideViewport,
            headerOverlap: Boolean(header && tabBar && tabBar.top < header.bottom - 1),
            estop: estop ? { width: estop.width, height: estop.height } : null,
            clippedTabs: [...document.querySelectorAll<HTMLElement>(".tab-btn")]
              .filter((tab) => tab.scrollWidth > tab.clientWidth)
              .map((tab) => tab.dataset.tab),
          };
        });

        expect(metrics.globalOverflow, `${viewport.name}/${panel}: overflow global`).toBe(false);
        expect(metrics.undersized, `${viewport.name}/${panel}: targets menores de 44 px`).toEqual([]);
        expect(metrics.outsideViewport, `${viewport.name}/${panel}: controles fuera del viewport`).toEqual([]);
        expect(metrics.headerOverlap, `${viewport.name}/${panel}: cabecera solapada`).toBe(false);
        expect(metrics.estop?.height, `${viewport.name}/${panel}: E-STOP menor de 48 px`).toBeGreaterThanOrEqual(48);
        expect(metrics.clippedTabs, `${viewport.name}/${panel}: pestañas truncadas`).toEqual([]);
      }
    });
  }

  test("preserva bloqueos de Touch, calibración y ruta y permite flechas", async ({ page }) => {
    await page.setViewportSize({ width: 320, height: 568 });
    await isolateEventStream(page);
    await page.goto("/", { waitUntil: "domcontentloaded" });

    await page.locator('[data-tab="touch"]').click({ force: true });
    await expect(page.locator('[data-tab="conexion"]')).toHaveAttribute("aria-selected", "true");
    await page.evaluate("TabManager.switch('touch')");
    await expect(page.locator('[data-tab="conexion"]')).toHaveAttribute("aria-selected", "true");

    await page.locator('[data-tab="conexion"]').press("ArrowRight");
    await expect(page.locator('[data-tab="mapa"]')).toBeFocused();

    await page.evaluate("Calibration.calibrated=false; TabManager.switch('testing')");
    await expect(page.locator('[data-tab="conexion"]')).toHaveAttribute("aria-selected", "true");

    await page.evaluate("Calibration.calibrated=true; TabManager.switch('rutas'); RouteManager.isRunning=true; TabManager.switch('testing')");
    await expect(page.locator('[data-tab="rutas"]')).toHaveAttribute("aria-selected", "true");
  });

  test("la rotación conserva vista y formulario sin overflow", async ({ page }) => {
    await page.setViewportSize({ width: 390, height: 844 });
    await isolateEventStream(page);
    await page.goto("/", { waitUntil: "domcontentloaded" });
    await openPanel(page, "rutas");
    await page.locator("#route-input-1").fill("12.5");

    await page.setViewportSize({ width: 844, height: 390 });
    await expect(page.locator('[data-tab="rutas"]')).toHaveAttribute("aria-selected", "true");
    await expect(page.locator("#route-input-1")).toHaveValue("12.5");
    const state = await page.evaluate(() => {
      const documentElement = document.documentElement;
      const estop = document.querySelector("#btn-estop")?.getBoundingClientRect();
      const header = document.querySelector(".app-header")?.getBoundingClientRect();
      const tabBar = document.querySelector(".tab-bar")?.getBoundingClientRect();
      return {
        globalOverflow: documentElement.scrollWidth > documentElement.clientWidth,
        estopVisible: Boolean(estop && estop.width >= 48 && estop.height >= 48),
        headerOverlap: Boolean(header && tabBar && tabBar.top < header.bottom - 1),
      };
    });
    expect(state).toEqual({ globalOverflow: false, estopVisible: true, headerOverlap: false });
  });

  test("cumple presupuestos locales de arranque y cambio de pestaña", async ({ page }) => {
    await page.setViewportSize({ width: 390, height: 844 });
    await isolateEventStream(page);
    const loadSamples: number[] = [];
    for (let attempt = 0; attempt < 5; attempt += 1) {
      const started = performance.now();
      await page.goto("/", { waitUntil: "domcontentloaded" });
      await expect(page.locator(".tab-bar")).toBeVisible();
      loadSamples.push(performance.now() - started);
    }
    const performanceMetrics = await page.evaluate<number[]>(`(() => {
      Calibration.calibrated = true;
      const samples = [];
      const panels = ["mapa", "testing", "rutas", "conexion"];
      for (let index = 0; index < 12; index += 1) {
        const started = performance.now();
        TabManager.switch(panels[index % panels.length]);
        samples.push(performance.now() - started);
      }
      return samples;
    })()`);
    const percentile95 = (samples: number[]): number => {
      const sorted = [...samples].sort((left, right) => left - right);
      return sorted[Math.ceil(sorted.length * .95) - 1] ?? Number.POSITIVE_INFINITY;
    };
    expect(percentile95(loadSamples), "arranque local p95 mayor de 2 s").toBeLessThanOrEqual(2_000);
    expect(percentile95(performanceMetrics), "cambio de pestaña p95 mayor de 150 ms").toBeLessThanOrEqual(150);
  });

  for (const viewport of [
    { name: "telefono", width: 390, height: 844 },
    { name: "escritorio", width: 1280, height: 720 },
  ]) {
    test(`${viewport.name}: sin infracciones axe serias o críticas`, async ({ page }) => {
      await page.setViewportSize({ width: viewport.width, height: viewport.height });
      await isolateEventStream(page);
      await page.goto("/", { waitUntil: "domcontentloaded" });
      for (const panel of operationalPanels) {
        await openPanel(page, panel);
        const results = await new AxeBuilder({ page })
          .withTags(["wcag2a", "wcag2aa", "wcag21a", "wcag21aa"])
          .analyze();
        const severe = results.violations.filter((violation) => violation.impact === "critical" || violation.impact === "serious");
        expect(severe.map((violation) => ({
          id: violation.id,
          targets: violation.nodes.map((node) => node.target),
        })), `${viewport.name}/${panel}`).toEqual([]);
      }
    });
  }
});

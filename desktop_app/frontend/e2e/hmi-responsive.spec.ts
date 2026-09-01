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

  test("el modo angular encadena θ, conserva L=0 visual y envía sólo X→Y", async ({ page }) => {
    await page.setViewportSize({ width: 1280, height: 720 });
    await isolateEventStream(page);
    let missionPayload: unknown = null;
    await page.route("**/api/v1/missions", async (route) => {
      const request = route.request();
      if (request.method() === "POST") {
        missionPayload = request.postDataJSON();
        await route.fulfill({
          status: 200,
          contentType: "application/json",
          body: JSON.stringify({ id: "angular-test", current_index: 0, active_command_id: "cmd-1", total_segments: 2 }),
        });
        return;
      }
      await route.fulfill({ status: 200, contentType: "application/json", body: JSON.stringify({ blocked: false, id: null }) });
    });
    await page.goto("/", { waitUntil: "domcontentloaded" });
    await openPanel(page, "rutas");
    await page.locator("#route-mode").selectOption("polar");

    const addVector = async (length: string, theta: string): Promise<void> => {
      await page.locator("#route-input-1").fill(length);
      await page.locator("#route-input-2").fill(theta);
      await page.locator("#route-form button[type='submit']").click();
    };
    await addVector("10", "0");
    await addVector("0", "90");
    await addVector("10", "0");

    await expect(page.locator("#route-mode")).toBeDisabled();
    await expect(page.locator("#route-angular-meta")).toBeVisible();
    await expect(page.locator("#route-angular-summary")).toContainText("punto final (10.00, 10.00) cm");
    const model = await page.evaluate(`(() => ({
      logical: RouteManager.logicalSteps.map(step => ({
        length: step.length,
        theta: step.theta,
        absolute: step.absoluteAngleDeg,
        end: step.end
      })),
      queue: RouteManager.queue.map(point => ({x: point.x, y: point.y, component: point.component})),
      geometryCount: RouteManager.planChart.$angularGeometry.length,
      datasetLabel: RouteManager.planChart.data.datasets[0].label,
      datasetColor: RouteManager.planChart.data.datasets[0].borderColor
    }))()`);
    expect(model).toEqual({
      logical: [
        { length: 10, theta: 0, absolute: 0, end: { x: 10, y: 0 } },
        { length: 0, theta: 90, absolute: 90, end: { x: 10, y: 0 } },
        { length: 10, theta: 0, absolute: 90, end: { x: 10, y: 10 } },
      ],
      queue: [
        { x: 10, y: 0, component: "x" },
        { x: 10, y: 10, component: "y" },
      ],
      geometryCount: 3,
      datasetLabel: "Hipotenusas angulares",
      datasetColor: "#c56cff",
    });

    await page.locator("#btn-route-start").click();
    await expect.poll(() => missionPayload).not.toBeNull();
    expect(missionPayload).toEqual({
      mode: "angular_decomposition",
      vectors: [
        { length_cm: 10, relative_angle_deg: 0 },
        { length_cm: 0, relative_angle_deg: 90 },
        { length_cm: 10, relative_angle_deg: 0 },
      ],
    });
  });

  test("Angular vectorial dibuja y envía una hipotenusa directa sólo con salud válida", async ({ page }) => {
    await page.setViewportSize({ width: 1280, height: 800 });
    await isolateEventStream(page);
    let missionPayload: unknown = null;
    await page.route("**/api/v1/config/robot", async (route) => {
      await route.fulfill({
        status: 200, contentType: "application/json",
        body: JSON.stringify({ robot_host: "192.168.4.1", experimental_vectorial_routes: true }),
      });
    });
    await page.route("**/api/v1/missions", async (route) => {
      if (route.request().method() === "POST") {
        missionPayload = route.request().postDataJSON();
        await route.fulfill({
          status: 200, contentType: "application/json",
          body: JSON.stringify({ id: "vectorial-test", mode: "angular_vectorial", current_index: 0, active_command_id: "cmd-v", total_segments: 1 }),
        });
        return;
      }
      await route.fulfill({ status: 200, contentType: "application/json", body: JSON.stringify({ blocked: false, id: null }) });
    });
    await page.goto("/", { waitUntil: "domcontentloaded" });
    await openPanel(page, "rutas");
    await page.locator("#route-mode").selectOption("vectorial");
    await page.locator("#route-vectorial-enabled").check();
    await page.evaluate(`RouteManager.applyVectorialTelemetry({
      state:'listo', degraded_mode:false,
      mpu:{present:true,calibrated:true,stale:false},
      encoder_health:{fl:'healthy',fr:'healthy',bl:'healthy',br:'healthy'}
    })`);
    await expect(page.locator("#route-vectorial-status")).toHaveClass(/ready/);
    await page.locator("#route-input-1").fill("50");
    await page.locator("#route-input-2").fill("135");
    await page.locator("#route-form button[type='submit']").click();
    expect(await page.evaluate("RouteManager.queue.map(point=>({component:point.component,x:point.x,y:point.y}))"))
      .toEqual([{ component: "vector", x: -35.35533905932737, y: 35.35533905932738 }]);
    await expect(page.locator("#route-angular-x-label")).toHaveText("Proyección matemática X");
    await page.locator("#btn-route-start").click();
    await expect.poll(() => missionPayload).not.toBeNull();
    expect(missionPayload).toEqual({
      mode: "angular_vectorial",
      vectors: [{ length_cm: 50, relative_angle_deg: 135 }],
    });
  });

  test("deshacer restaura la orientación angular anterior y desbloquea el modo al vaciar", async ({ page }) => {
    await page.setViewportSize({ width: 390, height: 844 });
    await isolateEventStream(page);
    await page.goto("/", { waitUntil: "domcontentloaded" });
    await openPanel(page, "rutas");
    await page.locator("#route-mode").selectOption("polar");
    for (const [length, theta] of [["5", "170"], ["5", "30"]]) {
      await page.locator("#route-input-1").fill(length);
      await page.locator("#route-input-2").fill(theta);
      await page.locator("#route-form button[type='submit']").click();
    }
    expect(await page.evaluate("RouteManager.logicalSteps.map(step => step.absoluteAngleDeg)")).toEqual([170, -160]);
    await page.locator("#btn-route-pop").click();
    expect(await page.evaluate("RouteManager.logicalSteps.map(step => step.absoluteAngleDeg)")).toEqual([170]);
    await page.locator("#btn-route-pop").click();
    await expect(page.locator("#route-mode")).toBeEnabled();
    expect(await page.evaluate("RouteManager.queue.length")).toBe(0);
  });

  test("conserva Rectangular X→Y y bloquea una ruta formada sólo por L=0", async ({ page }) => {
    await page.setViewportSize({ width: 800, height: 900 });
    await isolateEventStream(page);
    await page.goto("/", { waitUntil: "domcontentloaded" });
    await openPanel(page, "rutas");
    await page.locator("#route-input-1").fill("12");
    await page.locator("#route-input-2").fill("-4");
    await page.locator("#route-form button[type='submit']").click();
    expect(await page.evaluate("RouteManager.queue.map(point => ({x:point.x,y:point.y,component:point.component}))")).toEqual([
      { x: 12, y: 0, component: "x" },
      { x: 12, y: -4, component: "y" },
    ]);
    await page.locator("#btn-route-pop").click();
    await page.locator("#route-mode").selectOption("polar");
    await page.locator("#route-input-1").fill("0");
    await page.locator("#route-input-2").fill("90");
    await page.locator("#route-form button[type='submit']").click();
    await page.locator("#btn-route-start").click();
    expect(await page.evaluate("({logical:RouteManager.logicalSteps.length,queue:RouteManager.queue.length,running:RouteManager.isRunning})")).toEqual({ logical: 1, queue: 0, running: false });
    await expect(page.locator("#log-container")).toContainText("sólo contiene orientaciones visuales");
  });

  test("hora, nivel y mensaje del log no se enciman en los viewports aceptados", async ({ page }) => {
    await isolateEventStream(page);
    await page.goto("/", { waitUntil: "domcontentloaded" });
    await page.evaluate("LogSystem.add('INFO','Mensaje de prueba para verificar separación responsive.'); LogSystem.add('ERROR','Segundo mensaje técnico.');");
    for (const viewport of [
      { width: 320, height: 568 },
      { width: 390, height: 844 },
      { width: 768, height: 1024 },
      { width: 1024, height: 768 },
      { width: 1280, height: 720 },
    ]) {
      await page.setViewportSize(viewport);
      const metrics = await page.evaluate(() => {
        const entry = document.querySelector<HTMLElement>(".log-entry");
        const time = entry?.querySelector(".log-time")?.getBoundingClientRect();
        const level = entry?.querySelector(".log-level")?.getBoundingClientRect();
        const message = entry?.querySelector(".log-msg")?.getBoundingClientRect();
        const intersects = (left?: DOMRect, right?: DOMRect): boolean => Boolean(left && right && left.left < right.right && left.right > right.left && left.top < right.bottom && left.bottom > right.top);
        return {
          timeLevel: intersects(time, level),
          timeMessage: intersects(time, message),
          levelMessage: intersects(level, message),
          overflow: document.documentElement.scrollWidth > document.documentElement.clientWidth,
        };
      });
      expect(metrics, `${viewport.width}x${viewport.height}`).toEqual({ timeLevel: false, timeMessage: false, levelMessage: false, overflow: false });
    }
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

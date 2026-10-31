import { readdirSync, readFileSync } from "node:fs";
import { join } from "node:path";
import { fileURLToPath } from "node:url";
import vm from "node:vm";
import { describe, expect, it } from "vitest";

const hmiDir = fileURLToPath(new URL("../../robot_app/hmi", import.meta.url));
const compilarHmiPath = join(hmiDir, "compilar_hmi.py");
const jsDir = join(hmiDir, "js");
const cardsDir = join(hmiDir, "cards");
const cssDir = join(hmiDir, "css");

function extractJsOrderFromCompiler(): string[] {
  const content = readFileSync(compilarHmiPath, "utf8");
  const match = content.match(/JS_ORDER\s*=\s*\[([\s\S]*?)\]/);
  if (!match) {
    throw new Error("No se encontró la lista JS_ORDER en compilar_hmi.py");
  }
  const body = match[1];
  const items: string[] = [];
  const regex = /["']([^"']+\.js)["']/g;
  let m: RegExpExecArray | null;
  while ((m = regex.exec(body)) !== null) {
    items.push(m[1]);
  }
  return items;
}

describe("Guarda de ensamblado modular del HMI (compilar_hmi.py vs js/*.js)", () => {
  it("JS_ORDER contiene exactamente todos los archivos .js existentes en hmi/js/", () => {
    const jsOrder = extractJsOrderFromCompiler();
    const diskJsFiles = readdirSync(jsDir)
      .filter(f => f.endsWith(".js"))
      .sort();

    const sortedOrder = [...jsOrder].sort();

    expect(jsOrder.length).toBeGreaterThan(0);
    // Ningún archivo duplicado en JS_ORDER
    expect(new Set(jsOrder).size).toBe(jsOrder.length);
    // Exacta coincidencia biyectiva entre archivos en disco y JS_ORDER
    expect(sortedOrder).toEqual(diskJsFiles);
  });

  it("posiciona operator_feedback.js inmediatamente después de config.js", () => {
    const jsOrder = extractJsOrderFromCompiler();
    const configIdx = jsOrder.indexOf("config.js");
    const feedbackIdx = jsOrder.indexOf("operator_feedback.js");

    expect(configIdx).toBeGreaterThanOrEqual(0);
    expect(feedbackIdx).toBe(configIdx + 1);
  });

  it("posiciona route_lifecycle.js inmediatamente antes de route_manager.js", () => {
    const jsOrder = extractJsOrderFromCompiler();
    const lifecycleIdx = jsOrder.indexOf("route_lifecycle.js");
    const managerIdx = jsOrder.indexOf("route_manager.js");

    expect(lifecycleIdx).toBeGreaterThanOrEqual(0);
    expect(managerIdx).toBe(lifecycleIdx + 1);
  });
});

describe("Contrato de infraestructura Fase 1 (Huecos de alerta y estilos)", () => {
  it("los 5 contenedores de alerta existen con atributo hidden y roles de accesibilidad correctos", () => {
    const headerHtml = readFileSync(join(cardsDir, "header.html"), "utf8");
    const rutasHtml = readFileSync(join(cardsDir, "tab_rutas.html"), "utf8");
    const testingHtml = readFileSync(join(cardsDir, "tab_testing.html"), "utf8");

    // 1. #estop-status en header.html
    expect(headerHtml).toMatch(/id="estop-status"[^>]*class="[^"]*estop-status[^"]*"[^>]*role="status"[^>]*hidden/);

    // 2. #route-form-alert en tab_rutas.html
    expect(rutasHtml).toMatch(/id="route-form-alert"[^>]*class="[^"]*inline-alert[^"]*"[^>]*role="alert"[^>]*hidden/);

    // 3. #route-start-alert en tab_rutas.html
    expect(rutasHtml).toMatch(/id="route-start-alert"[^>]*class="[^"]*inline-alert[^"]*"[^>]*role="alert"[^>]*hidden/);

    // 4. #route-ockham-error en tab_rutas.html
    expect(rutasHtml).toMatch(/id="route-ockham-error"[^>]*class="[^"]*inline-alert[^"]*"[^>]*role="alert"[^>]*hidden/);

    // 5. #testing-cmd-alert en tab_testing.html
    expect(testingHtml).toMatch(/id="testing-cmd-alert"[^>]*class="[^"]*inline-alert[^"]*"[^>]*role="alert"[^>]*hidden/);
  });

  it("components.css declara los estilos de inline-alert y estop-status", () => {
    const css = readFileSync(join(cssDir, "components.css"), "utf8");

    expect(css).toMatch(/\.inline-alert\s*\{[^}]*border-left:[^}]*var\(--danger\)/);
    expect(css).toMatch(/\.inline-alert\[data-level="warn"\]\s*\{[^}]*border-left-color:[^}]*var\(--accent\)/);
    expect(css).toMatch(/\.estop-status\s*\{[^}]*font-weight:\s*700/);
    expect(css).toMatch(/\.estop-status\[data-stage="failed"\]\s*\{[^}]*background:[^}]*var\(--danger\)/);
  });
});

describe("Contrato y modularidad de RouteLifecycle y RouteManager (Fase 1.5)", () => {
  it("route_manager.js se mantiene por debajo de 450 líneas", () => {
    const rmContent = readFileSync(join(jsDir, "route_manager.js"), "utf8");
    const lines = rmContent.split(/\r?\n/).length;
    expect(lines).toBeLessThan(450);
  });

  it("route_manager.js conserva la cadena literal 'logicalSteps: []'", () => {
    const rmContent = readFileSync(join(jsDir, "route_manager.js"), "utf8");
    expect(rmContent).toContain("logicalSteps: []");
  });

  it("route_manager.js delega en RouteLifecycle mediante métodos fachada", () => {
    const rmContent = readFileSync(join(jsDir, "route_manager.js"), "utf8");
    expect(rmContent).toMatch(/lockTabs\(\)\s*\{\s*RouteLifecycle\.lockTabs\(this\);\s*\}/);
    expect(rmContent).toMatch(/unlockTabs\(\)\s*\{\s*RouteLifecycle\.unlockTabs\(this\);\s*\}/);
    expect(rmContent).toMatch(/start\(\)\s*\{\s*return RouteLifecycle\.start\(this\);\s*\}/);
    expect(rmContent).toMatch(/failRoute\(reason\)\s*\{\s*return RouteLifecycle\.failRoute\(this,\s*reason\);\s*\}/);
    expect(rmContent).toMatch(/stopAndReset\(\)\s*\{\s*return RouteLifecycle\.stopAndReset\(this\);\s*\}/);
    expect(rmContent).toMatch(/clearRobotMemory\(\)\s*\{\s*return RouteLifecycle\.clearRobotMemory\(this\);\s*\}/);
  });
});

describe("Comportamiento funcional de OperatorFeedback (Fase 1.1)", () => {
  function createFeedbackSandbox() {
    const dom: Record<string, any> = {};
    const sandbox = {
      window: {
        confirm: (msg: string) => msg.includes("OK"),
      },
      document: {
        getElementById: (id: string) => dom[id] || null,
      },
      OperatorFeedback: undefined as any,
    };
    vm.createContext(sandbox);

    const feedbackCode = readFileSync(join(jsDir, "operator_feedback.js"), "utf8");
    vm.runInContext(`${feedbackCode}; this.OperatorFeedback = OperatorFeedback;`, sandbox);

    return { sandbox, dom };
  }

  it("show() y clear() gestionan el contenido, nivel y visibilidad", () => {
    const { sandbox, dom } = createFeedbackSandbox();
    dom["test-slot"] = { textContent: "", dataset: {}, hidden: true };

    sandbox.OperatorFeedback.show("test-slot", "Error crítico", "error");
    expect(dom["test-slot"].textContent).toBe("Error crítico");
    expect(dom["test-slot"].dataset.level).toBe("error");
    expect(dom["test-slot"].hidden).toBe(false);

    sandbox.OperatorFeedback.clear("test-slot");
    expect(dom["test-slot"].textContent).toBe("");
    expect(dom["test-slot"].hidden).toBe(true);
  });

  it("readBoundedNumber() valida correctamente números, límites y valores inválidos", () => {
    const { sandbox } = createFeedbackSandbox();

    function mockInput(val: string, min = "1", max = "100") {
      const attrs: Record<string, string> = {};
      return {
        value: val,
        min,
        max,
        setAttribute: (k: string, v: string) => { attrs[k] = v; },
        getAttribute: (k: string) => attrs[k] ?? null,
      };
    }

    // Válido
    const inp1 = mockInput("50");
    expect(sandbox.OperatorFeedback.readBoundedNumber(inp1)).toBe(50);
    expect(inp1.getAttribute("aria-invalid")).toBe("false");

    // Límite exacto
    const inpMin = mockInput("1");
    expect(sandbox.OperatorFeedback.readBoundedNumber(inpMin)).toBe(1);
    expect(inpMin.getAttribute("aria-invalid")).toBe("false");

    // Fuera de rango inferior
    const inpUnder = mockInput("0");
    expect(sandbox.OperatorFeedback.readBoundedNumber(inpUnder)).toBeNull();
    expect(inpUnder.getAttribute("aria-invalid")).toBe("true");

    // Fuera de rango superior
    const inpOver = mockInput("150");
    expect(sandbox.OperatorFeedback.readBoundedNumber(inpOver)).toBeNull();
    expect(inpOver.getAttribute("aria-invalid")).toBe("true");

    // Entrada no numérica o vacía
    const inpEmpty = mockInput("   ");
    expect(sandbox.OperatorFeedback.readBoundedNumber(inpEmpty)).toBeNull();
    expect(inpEmpty.getAttribute("aria-invalid")).toBe("true");

    const inpNan = mockInput("abc");
    expect(sandbox.OperatorFeedback.readBoundedNumber(inpNan)).toBeNull();
    expect(inpNan.getAttribute("aria-invalid")).toBe("true");

    // Input null
    expect(sandbox.OperatorFeedback.readBoundedNumber(null)).toBeNull();
  });

  it("estopStatus() actualiza el estado y visibilidad según la etapa", () => {
    const { sandbox, dom } = createFeedbackSandbox();
    dom["estop-status"] = { textContent: "", dataset: {}, hidden: true };

    sandbox.OperatorFeedback.estopStatus("sent");
    expect(dom["estop-status"].textContent).toContain("E-STOP enviado");
    expect(dom["estop-status"].dataset.stage).toBe("sent");
    expect(dom["estop-status"].hidden).toBe(false);

    sandbox.OperatorFeedback.estopStatus("failed", "timeout 3s");
    expect(dom["estop-status"].textContent).toContain("E-STOP NO CONFIRMADO");
    expect(dom["estop-status"].textContent).toContain("timeout 3s");
    expect(dom["estop-status"].dataset.stage).toBe("failed");
    expect(dom["estop-status"].hidden).toBe(false);

    sandbox.OperatorFeedback.estopStatus("idle");
    expect(dom["estop-status"].textContent).toBe("");
    expect(dom["estop-status"].hidden).toBe(true);
  });
});

describe("Comportamiento funcional de humanizeError() y ERROR_MESSAGES (Fase 1.2)", () => {
  function createConfigSandbox() {
    const sandbox = {
      window: {},
      document: { getElementById: () => null },
      Chart: function () {},
      ChartDataLabels: {},
      WSControl: { sendMove: () => {} },
      actualizarBarra: () => {},
      humanizeError: undefined as any,
      ERROR_MESSAGES: undefined as any,
    };
    vm.createContext(sandbox);

    const configCode = readFileSync(join(jsDir, "config.js"), "utf8");
    vm.runInContext(`${configCode}; this.humanizeError = humanizeError; this.ERROR_MESSAGES = ERROR_MESSAGES;`, sandbox);

    return sandbox;
  }

  it("traduce códigos canónicos a español conservando el identificador para diagnóstico", () => {
    const sandbox = createConfigSandbox();

    expect(sandbox.humanizeError("robot_restarted_mid_mission")).toContain("El ESP32 se reinició durante la misión");
    expect(sandbox.humanizeError("robot_restarted_mid_mission")).toContain("(código: robot_restarted_mid_mission)");

    expect(sandbox.humanizeError("operator_stop")).toContain("Detenida por el operador");
    expect(sandbox.humanizeError("5v_fuente_desconectada")).toContain("Alimentación de 5V o USB desconectada");
  });

  it("resuelve códigos con metadatos de firmware por prefijo", () => {
    const sandbox = createConfigSandbox();

    const translated = sandbox.humanizeError("drive_stall_left[Cinematica.cpp:503]");
    expect(translated).toContain("Bloqueo detectado en ruedas izquierdas durante avance");
    expect(translated).toContain("(código: drive_stall_left[Cinematica.cpp:503])");

    const calDivergence = sandbox.humanizeError("cal_yaw_divergence: delta=3.2");
    expect(calDivergence).toContain("Divergencia angular en calibración MPU");
  });

  it("maneja de forma segura valores nulos, vacíos o desconocidos", () => {
    const sandbox = createConfigSandbox();

    expect(sandbox.humanizeError(null)).toBe("error desconocido");
    expect(sandbox.humanizeError(undefined)).toBe("error desconocido");
    expect(sandbox.humanizeError("")).toBe("error desconocido");
    expect(sandbox.humanizeError("unknown_error_xyz")).toBe("unknown_error_xyz");
  });
});

describe("Contrato de modularidad y seguridad operativa Etapa 1 (C1-C5, A6-A10)", () => {
  it("todos los módulos .js en hmi/js/ tienen <= 450 líneas", () => {
    const diskJsFiles = readdirSync(jsDir).filter(f => f.endsWith(".js"));
    for (const file of diskJsFiles) {
      const content = readFileSync(join(jsDir, file), "utf8");
      const lineCount = content.split(/\r?\n/).length;
      expect(lineCount, `${file} excede 450 líneas (${lineCount})`).toBeLessThanOrEqual(450);
    }
  });

  it("touch.js se mantiene por debajo de 200 líneas tras limpieza de código legacy", () => {
    const touchContent = readFileSync(join(jsDir, "touch.js"), "utf8");
    const lines = touchContent.split(/\r?\n/).length;
    expect(lines).toBeLessThanOrEqual(200);
  });

  it("touch_routine.js se mantiene por debajo de 380 líneas", () => {
    const routineContent = readFileSync(join(jsDir, "touch_routine.js"), "utf8");
    const lines = routineContent.split(/\r?\n/).length;
    expect(lines).toBeLessThanOrEqual(380);
  });

  it("separa stopRoute (Detener preservando cola) de discardRoute (Descartar con confirmación) (C2)", () => {
    const rlContent = readFileSync(join(jsDir, "route_lifecycle.js"), "utf8");
    const rmContent = readFileSync(join(jsDir, "route_manager.js"), "utf8");

    expect(rlContent).toContain("stopRoute(manager)");
    expect(rlContent).toContain("discardRoute(manager)");
    expect(rlContent).toContain("manager.stoppedAtIndex");
    expect(rmContent).toContain("stoppedAtIndex: null");
    expect(rmContent).toContain("'Detenido'");
    expect(rmContent).toContain("'stopped'");
  });

  it("exige confirmación física OperatorFeedback.confirmMotion antes de iniciar ruta o calibrar (C3)", () => {
    const rlContent = readFileSync(join(jsDir, "route_lifecycle.js"), "utf8");
    const calContent = readFileSync(join(jsDir, "calibration.js"), "utf8");

    expect(rlContent).toContain("OperatorFeedback.confirmMotion");
    expect(calContent).toContain("OperatorFeedback.confirmMotion");
  });

  it("elimina fallbacks silenciosos || 50 y || 90 en _executeQuickCommand y valida con readBoundedNumber (C4)", () => {
    const mainContent = readFileSync(join(jsDir, "main.js"), "utf8");
    const testingHtml = readFileSync(join(cardsDir, "tab_testing.html"), "utf8");

    expect(mainContent).not.toMatch(/parseFloat\(paramInput\?\.value\)\s*\|\|\s*50/);
    expect(mainContent).not.toMatch(/parseFloat\(paramInput\?\.value\)\s*\|\|\s*90/);
    expect(mainContent).toContain("OperatorFeedback.readBoundedNumber");
    expect(testingHtml).toContain('aria-describedby="cmd-param-hint"');
  });

  it("etiqueta #btn-reset-hmi como 'Limpiar vista HMI' y condiciona clear_fault al estado de fallo (C5)", () => {
    const headerHtml = readFileSync(join(cardsDir, "header.html"), "utf8");
    const conexionHtml = readFileSync(join(cardsDir, "tab_conexion.html"), "utf8");
    const mainContent = readFileSync(join(jsDir, "main.js"), "utf8");

    expect(headerHtml).toContain("Limpiar vista HMI");
    expect(conexionHtml).toContain('id="surface-alert"');
    expect(mainContent).toContain("willRearm");
    expect(mainContent).not.toContain("FSMDisplay.update('IDLE')");
  });

  it("persiste historial tras POST /api/v1/missions (A6), retiene caja Ockham hasta éxito (A7) y reintenta E-STOP (A9)", () => {
    const rlContent = readFileSync(join(jsDir, "route_lifecycle.js"), "utf8");
    const ockhamContent = readFileSync(join(jsDir, "route_ockham.js"), "utf8");
    const wsContent = readFileSync(join(jsDir, "ws_control.js"), "utf8");

    expect(rlContent).toContain("this._persistRouteHistory(manager, curName, mode)");
    expect(ockhamContent).toContain("confirmBtn.setAttribute('aria-busy', 'true')");
    expect(ockhamContent).toContain("OperatorFeedback.show('route-ockham-error'");
    expect(wsContent).toContain("async estop()");
    expect(wsContent).toContain("OperatorFeedback.estopStatus('confirmed')");
    expect(wsContent).toContain("OperatorFeedback.estopStatus('failed'");
  });
});



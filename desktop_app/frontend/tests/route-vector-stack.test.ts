import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import vm from "node:vm";
import { describe, expect, it } from "vitest";

const stackPath = fileURLToPath(new URL("../../robot_app/hmi/js/route_vector_stack.js", import.meta.url));

function cargarPilaVectorial() {
  const sandbox: Record<string, any> = {};
  sandbox.window = sandbox;
  vm.createContext(sandbox);
  vm.runInContext(readFileSync(stackPath, "utf8"), sandbox);
  return sandbox.RouteVectorStack as {
    VectorRouteStack: new () => any;
  };
}

describe("Pila vectorial: referencia angular tras deshacer pasos", () => {
  it("vuelve a +X (0°) cuando se deshace el único paso de la ruta", () => {
    const { VectorRouteStack } = cargarPilaVectorial();
    const stack = new VectorRouteStack();
    const logicalSteps: any[] = [];

    logicalSteps.push(stack.createNode({ index: 1, id: 1, length: 30, thetaRealDeg: 45, prevChartAngleDeg: 0, start: { x: 0, y: 0 } }));
    expect(stack.lastNode(logicalSteps).thetaChartDeg).toBe(45);

    logicalSteps.pop(); // Igual que RouteManager.removeLastStep()

    expect(stack.lastNode(logicalSteps)).toBeNull();
    const preview = stack.updatePreview("20", "0", logicalSteps);
    expect(preview.thetaChart).toBe(0);
  });

  it("toma la referencia del último paso vigente tras deshacer uno intermedio", () => {
    const { VectorRouteStack } = cargarPilaVectorial();
    const stack = new VectorRouteStack();
    const steps: any[] = [];
    steps.push(stack.createNode({ index: 1, id: 1, length: 60, thetaRealDeg: 90, prevChartAngleDeg: 0, start: { x: 0, y: 0 } }));
    steps.push(stack.createNode({ index: 2, id: 2, length: 40, thetaRealDeg: -45, prevChartAngleDeg: 90, start: steps[0].end }));

    steps.pop();

    expect(stack.lastNode(steps).thetaChartDeg).toBe(90);
    expect(stack.updatePreview("10", "0", steps).thetaChart).toBe(90);
  });

  it("sin lista lógica explícita conserva la compatibilidad con la pila interna", () => {
    const { VectorRouteStack } = cargarPilaVectorial();
    const stack = new VectorRouteStack();
    stack.createNode({ index: 1, id: 1, length: 10, thetaRealDeg: 30, prevChartAngleDeg: 0, start: { x: 0, y: 0 } });
    expect(stack.lastNode().thetaChartDeg).toBe(30);
  });
});

const rosePath = fileURLToPath(new URL("../../robot_app/hmi/js/route_angle_rose.js", import.meta.url));

describe("Rosa de ángulos: referencia y valores absolutos", () => {
  function cargarRosa() {
    const sandbox: Record<string, any> = {};
    sandbox.window = sandbox;
    vm.createContext(sandbox);
    vm.runInContext(readFileSync(rosePath, "utf8"), sandbox);
    return sandbox.RouteAngleRose;
  }

  it("usa +X (0°) cuando no hay pasos y la prolongación del último paso vectorial si los hay", () => {
    const rose = cargarRosa();
    expect(rose.reference({ logicalSteps: [] })).toEqual({ refDeg: 0, refIndex: 0 });
    const steps = [{ mode: "vectorial", thetaChartDeg: 90 }, { mode: "vectorial", thetaChartDeg: 45 }];
    expect(rose.reference({ logicalSteps: steps })).toEqual({ refDeg: 45, refIndex: 2 });
  });

  it("normaliza el valor absoluto mostrado y presenta la reversa como 180°", () => {
    const rose = cargarRosa();
    expect(rose.norm(45 + 135)).toBe(180);
    expect(rose.norm(45 - 90)).toBe(-45);
    expect(rose.fmt(rose.norm(90 + 0))).toBe("+90°");
  });
});

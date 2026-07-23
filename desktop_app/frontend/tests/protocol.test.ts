import { describe, expect, it } from "vitest";
import { isRobotStatus } from "../src/protocol";

describe("isRobotStatus", () => {
  it("acepta el contrato mínimo de Flask", () => {
    expect(isRobotStatus({ robot_host: "192.168.4.1", connection: { state: "connected" } })).toBe(true);
  });

  it("rechaza valores que no son objetos de estado", () => {
    expect(isRobotStatus(null)).toBe(false);
    expect(isRobotStatus({ connection: {} })).toBe(false);
  });
});

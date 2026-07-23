import { isRobotStatus, type RobotStatus } from "./protocol";

export class RobotApi {
  public constructor(private readonly baseUrl = "") {}

  public async status(signal?: AbortSignal): Promise<RobotStatus> {
    const response = await fetch(`${this.baseUrl}/api/v1/status`, { signal });
    if (!response.ok) throw new Error(`Estado HTTP ${response.status}`);
    const payload: unknown = await response.json();
    if (!isRobotStatus(payload)) throw new Error("Respuesta de estado inválida");
    return payload;
  }
}

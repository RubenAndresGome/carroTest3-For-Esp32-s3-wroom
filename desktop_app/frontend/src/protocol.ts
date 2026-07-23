export const PROTOCOL_VERSION = 1 as const;

export type CommandName =
  | "calibrate"
  | "estop"
  | "stop"
  | "reset_pose"
  | "clear_fault"
  | "move"
  | "drive"
  | "turn"
  | "manual"
  | "test_pwm";

export interface ConnectionStatus {
  readonly state: "stopped" | "connecting" | "connected" | "backoff";
  readonly detail: string | null;
  readonly protocol: "negotiating" | "v1";
  readonly protocol_name?: string;
  readonly heartbeat_age_ms?: number | null;
}

export interface Telemetry {
  readonly seq: number;
  readonly state: string;
  readonly x_mm: number;
  readonly y_mm: number;
  readonly yaw_deg: number;
  readonly heading_deg: number;
  readonly imu_yaw_unwrapped_deg: number;
  readonly imu_yaw_recenter_count: number;
  readonly pulses: Readonly<Record<"fl" | "fr" | "bl" | "br", number>>;
  readonly pwm: Readonly<Record<"left" | "right", number>>;
  readonly wheel_speed_cm_s: Readonly<Record<"left" | "right", number>>;
  readonly encoder_delta_avg: Readonly<Record<"fl" | "fr" | "bl" | "br", number>>;
  readonly calibrated: boolean;
  readonly degraded_mode: boolean;
  readonly active_command_id: string | null;
  readonly active_command_name: string | null;
  readonly command_progress: number;
  readonly turn_requested_mode: "AUTO" | "PIVOT" | string;
  readonly turn_mode: "PIVOT_CONTINUOUS" | string;
  readonly turn_mode_reason: string;
  readonly turn_fallback_reason: string;
  readonly turn_fallback_count: number;
  readonly turn_attempt: number;
  readonly turn_attempt_max: number;
  readonly turn_attempt_sign: number;
  readonly turn_progress_deg: number;
  readonly turn_error_deg: number;
  readonly turn_predicted_error_deg: number;
  readonly turn_gyro_deg_s: number;
  readonly turn_pwm_target: number;
  readonly turn_pwm_target_8bit: number;
  readonly turn_braking_phase: string;
  readonly cardinal_target_deg: number;
  readonly move_phase: string;
  readonly alignment_stage: string;
  readonly turn_tolerance_deg: number;
  readonly heading_target_deg: number;
  readonly heading_error_deg: number;
  readonly heading_predicted_error_deg: number;
  readonly heading_control_pwm: number;
  readonly encoder_control_pwm: number;
  readonly heading_recovery_attempt: number;
  readonly heading_recovery_attempt_max: number;
  readonly heading_stable_ms: number;
  readonly route_coarse_aligned: boolean;
  readonly route_precise_aligned: boolean;
  readonly turn_left_energized: boolean;
  readonly turn_right_energized: boolean;
  readonly turn_left_power_started_ms: number;
  readonly turn_right_power_started_ms: number;
  readonly rtos: {
    readonly architecture: string;
    readonly reset_reason: string;
    readonly tasks_created_ok: boolean;
    readonly task_creation_failure?: string;
    readonly stack_warning: boolean;
    readonly stack_min_acceptable_bytes: number;
    readonly stack_min_free_bytes: Partial<Readonly<Record<"web" | "control", number>>>;
    readonly control_timing: {
      readonly target_period_us: number;
      readonly last_period_us: number;
      readonly max_jitter_us: number;
      readonly max_cycle_duration_us: number;
      readonly max_sample_age_us: number;
      readonly missed_deadlines: number;
      readonly sample_sequence: number;
    };
  };
  readonly encoder_health: Readonly<Record<"fl" | "fr" | "bl" | "br", string>>;
  readonly encoder_fusion: {
    readonly estimator: string;
    readonly left_reliable_count: number;
    readonly right_reliable_count: number;
    readonly left_no_progress_ms: number;
    readonly right_no_progress_ms: number;
  };
  readonly mpu: {
    readonly present: boolean | null;
    readonly stale: boolean | null;
    readonly calibrated: boolean | null;
    readonly i2c_ok: boolean | null;
    readonly gyro_z_filtered_rad_s: number;
    readonly gyro_z_offset_rad_s: number;
    readonly theta_error_deg: number;
  };
}

export interface RobotStatus {
  readonly connection: ConnectionStatus;
  readonly robot_host: string;
  readonly websocket_url: string;
  readonly recording: boolean;
  readonly session_id: number | null;
  readonly telemetry: Telemetry | null;
}

export function isRobotStatus(value: unknown): value is RobotStatus {
  if (typeof value !== "object" || value === null) return false;
  const candidate = value as Partial<RobotStatus>;
  return typeof candidate.robot_host === "string" && typeof candidate.connection?.state === "string";
}

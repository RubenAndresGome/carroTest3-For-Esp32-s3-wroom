export const PROTOCOL_VERSION = 1 as const;

export type CommandName =
  | "calibrate"
  | "estop"
  | "stop"
  | "reset_pose"
  | "clear_fault"
  | "step"
  | "turn_to"
  | "set_comp"
  | "move"
  | "drive"
  | "turn"
  | "manual_begin"
  | "manual_drive"
  | "manual_end"
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
  readonly allowed_commands: readonly CommandName[];
  readonly capabilities: readonly string[];
  readonly fault: {
    readonly active?: boolean;
    readonly state?: "none" | "fallo" | "estop" | string;
    readonly detail?: string;
    readonly rearm_required?: boolean;
  };
  readonly calibration_diagnostics: {
    readonly active?: boolean;
    readonly phase?: string;
    readonly ramp_level?: number;
    readonly ramp_level_count?: number;
    readonly pwm_8bit?: number;
    readonly pwm_10bit?: number;
    readonly attempt?: number;
    readonly attempt_max?: number;
    readonly direction_candidate?: number;
    readonly encoder_delta?: readonly [number, number, number, number];
    readonly encoder_responding?: readonly [boolean, boolean, boolean, boolean];
    readonly encoder_isolated?: readonly [boolean, boolean, boolean, boolean];
    readonly sides?: Readonly<Record<"left" | "right", {
      readonly average?: number;
      readonly ok?: boolean;
      readonly stall_ms?: number;
    }>>;
    readonly ticks_required?: number;
    readonly stall_limit_ms?: number;
  };
  readonly target: {
    readonly absolute?: boolean;
    readonly x_cm?: number;
    readonly y_cm?: number;
    readonly longitudinal_error_cm?: number;
    readonly lateral_error_cm?: number;
    readonly distance_error_cm?: number;
    readonly endpoint_attempt?: number;
    readonly finish_reason?: string;
  };
  readonly drive_control: {
    readonly dynamic_heading_deg?: number;
    readonly heading_error_deg?: number;
    readonly pwm?: number;
    readonly p?: number;
    readonly i?: number;
    readonly d?: number;
    readonly integral_deg_s?: number;
    readonly encoder_pwm?: number;
    readonly lateral_correction_deg?: number;
    readonly right_compensation?: number;
  };
  readonly recovery: {
    readonly decision?: string;
    readonly distance_cm?: number;
    readonly direction?: "forward" | "reverse" | string;
    readonly pivot_avoided?: boolean;
    readonly min_distance_cm?: number;
    readonly phase?: "inactive" | "pause" | "turn_to_axis" | "drive_to_axis" |
      "verify" | "restore_heading" | "resumed" | string;
    readonly trigger?: string;
    readonly attempt?: number;
    readonly attempt_max?: number;
    readonly rejoin_x_cm?: number;
    readonly rejoin_y_cm?: number;
    readonly initial_lateral_cm?: number;
    readonly improvement_cm?: number;
  };
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
    readonly left_delta?: number;
    readonly right_delta?: number;
    readonly left_sources?: string;
    readonly right_sources?: string;
    readonly warning?: boolean;
  };
  readonly anti_friction: {
    readonly active: boolean;
    readonly pulse_on: boolean;
    readonly pulse_index: number;
    readonly pulse_total: number;
    readonly target_pwm: number;
    readonly target_8bit: number;
    readonly target_percent: number;
    readonly pulse_on_ms: number;
    readonly pulse_off_ms: number;
    readonly movement_confirmed: boolean;
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

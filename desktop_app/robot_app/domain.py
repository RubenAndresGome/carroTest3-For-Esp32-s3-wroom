"""Tipos de dominio pequeños, inmutables y validados."""

from __future__ import annotations

import math
import time
import uuid
from dataclasses import dataclass, field
from enum import StrEnum
from typing import Any, Mapping


class ConnectionState(StrEnum):
    STOPPED = "stopped"
    CONNECTING = "connecting"
    CONNECTED = "connected"
    BACKOFF = "backoff"


class CommandStatus(StrEnum):
    QUEUED = "queued"
    SENT = "sent"
    ACKNOWLEDGED = "acknowledged"
    COMPLETED = "completed"
    REJECTED = "rejected"
    FAILED = "failed"


class Severity(StrEnum):
    INFO = "info"
    WARNING = "warning"
    ERROR = "error"
    CRITICAL = "critical"


ALLOWED_COMMANDS = frozenset(
    {"calibrate", "estop", "stop", "reset_pose", "clear_fault", "move", "drive", "turn", "manual", "test_pwm", "self_test",
     "mission_upload", "mission_start", "mission_clear"}
)

MAX_SEGMENT_MM = 2_000.0
PWM_SAFE_LIMIT = 230
TURN_MODES = frozenset({"auto", "pivot", "arc_left_active", "arc_right_active"})


def _finite_number(value: Any, name: str, low: float, high: float) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ValueError(f"{name} debe ser numérico")
    result = float(value)
    if not math.isfinite(result) or not low <= result <= high:
        raise ValueError(f"{name} debe estar entre {low:g} y {high:g}")
    return result


def _heading_degrees(value: Any) -> float:
    return _finite_number(value, "heading", -1e9, 1e9) % 360.0


def validate_command_payload(name: str, payload: Mapping[str, Any] | None) -> dict[str, Any]:
    if name not in ALLOWED_COMMANDS:
        raise ValueError("Comando no permitido")
    source = dict(payload or {})
    if name == "mission_upload":
        mission_id = str(source.get("mission_id", "")).strip().lower()
        if len(mission_id) != 32 or any(character not in "0123456789abcdef" for character in mission_id):
            raise ValueError("mission_id debe ser hexadecimal de 32 caracteres")
        revision = int(_finite_number(source.get("revision", 1), "revision", 1, 2_147_483_647))
        points = source.get("points")
        if not isinstance(points, list) or not 1 <= len(points) <= 32:
            raise ValueError("La misión cargada requiere entre 1 y 32 segmentos")
        validated_points = []
        for point in points:
            validated = validate_command_payload("move", point)
            step_id = str(point.get("step_id", "")).strip().lower() if isinstance(point, Mapping) else ""
            if len(step_id) != 32 or any(character not in "0123456789abcdef" for character in step_id):
                raise ValueError("Cada segmento requiere step_id hexadecimal de 32 caracteres")
            validated["step_id"] = step_id
            validated_points.append(validated)
        if len({point["step_id"] for point in validated_points}) != len(validated_points):
            raise ValueError("Los step_id de la misión deben ser únicos")
        return {"mission_id": mission_id, "revision": revision, "points": validated_points}
    if name == "mission_start":
        mission_id = str(source.get("mission_id", "")).strip().lower()
        if len(mission_id) != 32 or any(character not in "0123456789abcdef" for character in mission_id):
            raise ValueError("mission_id debe ser hexadecimal de 32 caracteres")
        return {
            "mission_id": mission_id,
            "revision": int(_finite_number(source.get("revision", 1), "revision", 1, 2_147_483_647)),
        }
    if name == "move":
        return {
            "x_mm": _finite_number(source.get("x_mm"), "x_mm", -100_000, 100_000),
            "y_mm": _finite_number(source.get("y_mm"), "y_mm", -100_000, 100_000),
        }
    if name == "drive":
        distance = _finite_number(source.get("distance_mm"), "distance_mm", -MAX_SEGMENT_MM, MAX_SEGMENT_MM)
        if distance == 0:
            raise ValueError("distance_mm no puede ser cero")
        return {"distance_mm": distance}
    if name == "turn":
        angle = _finite_number(source.get("angle_deg"), "angle_deg", -360, 360)
        result = {"angle_deg": angle}
        if "mode" in source:
            mode = str(source.get("mode", "")).strip().lower()
            if mode not in TURN_MODES:
                raise ValueError("mode de giro no permitido")
            result["mode"] = mode
        return result
    if name in {"manual", "test_pwm"}:
        result: dict[str, Any] = {
            "l": int(_finite_number(source.get("l"), "l", -PWM_SAFE_LIMIT, PWM_SAFE_LIMIT)),
            "r": int(_finite_number(source.get("r"), "r", -PWM_SAFE_LIMIT, PWM_SAFE_LIMIT)),
        }
        if name == "test_pwm":
            result["dur_ms"] = int(_finite_number(source.get("dur_ms", 500), "dur_ms", 10, 1_500))
        return result
    return {}


@dataclass(frozen=True, slots=True)
class RobotCommand:
    name: str
    payload: dict[str, Any] = field(default_factory=dict)
    command_id: str = field(default_factory=lambda: uuid.uuid4().hex)
    created_at: float = field(default_factory=time.time)

    @classmethod
    def create(cls, name: object, payload: Mapping[str, Any] | None = None) -> "RobotCommand":
        normalized_name = str(name or "").strip().lower()
        return cls(name=normalized_name, payload=validate_command_payload(normalized_name, payload))

    def protocol_envelope(self) -> dict[str, Any]:
        return {"v": 1, "type": "command", "id": self.command_id, "name": self.name, "payload": dict(self.payload)}


def split_segment_mm(
    start_x_mm: float, start_y_mm: float, end_x_mm: float, end_y_mm: float,
    max_segment_mm: float = MAX_SEGMENT_MM,
) -> list[dict[str, float]]:
    """Divide un tramo absoluto sin cambiar su dirección ni perder el residual."""
    start_x = _finite_number(start_x_mm, "start_x_mm", -1e9, 1e9)
    start_y = _finite_number(start_y_mm, "start_y_mm", -1e9, 1e9)
    end_x = _finite_number(end_x_mm, "end_x_mm", -1e9, 1e9)
    end_y = _finite_number(end_y_mm, "end_y_mm", -1e9, 1e9)
    limit = _finite_number(max_segment_mm, "max_segment_mm", 1, MAX_SEGMENT_MM)
    dx, dy = end_x - start_x, end_y - start_y
    distance = math.hypot(dx, dy)
    if distance == 0:
        return []
    count = max(1, math.ceil(distance / limit))
    return [
        {"x_mm": start_x + dx * index / count, "y_mm": start_y + dy * index / count}
        for index in range(1, count + 1)
    ]


@dataclass(frozen=True, slots=True)
class TelemetrySnapshot:
    sequence: int
    received_at: float
    uptime_ms: int | None
    state: str
    x_mm: float
    y_mm: float
    yaw_deg: float
    pulses: tuple[int, int, int, int]
    pwm: tuple[int, int]
    wheel_speed_cm_s: tuple[float, float]
    encoder_delta_avg: tuple[float, float, float, float]
    gyro_z_filtered_rad_s: float
    gyro_z_offset_rad_s: float
    theta_error_deg: float
    mpu_present: bool | None
    mpu_stale: bool | None
    mpu_calibrated: bool | None
    i2c_ok: bool | None
    robot_id: str | None
    firmware_version: str | None
    heading_deg: float = 0.0
    imu_yaw_unwrapped_deg: float = 0.0
    imu_yaw_recenter_count: int = 0
    calibrated: bool = False
    degraded_mode: bool = False
    active_command_id: str | None = None
    active_command_name: str | None = None
    command_progress: float = 0.0
    turn_attempt: int = 0
    turn_attempt_max: int = 0
    retry_pause_remaining_ms: int = 0
    drive_attempt: int = 0
    drive_attempt_max: int = 0
    drive_retry_pause_remaining_ms: int = 0
    autonomous_recovery_reason: str = "none"
    turn_requested_mode: str = "AUTO"
    turn_mode: str = "PIVOT"
    turn_mode_reason: str = ""
    turn_fallback_reason: str = ""
    turn_fallback_count: int = 0
    turn_left_energized: bool = False
    turn_right_energized: bool = False
    turn_left_power_started_ms: int = 0
    turn_right_power_started_ms: int = 0
    arc_entry_pause_remaining_ms: int = 0
    turn_fixed_sign: int = 0
    turn_attempt_sign: int = 0
    turn_requested_deg: float = 0.0
    turn_physical_target_deg: float = 0.0
    turn_wrap_count: int = 0
    turn_progress_deg: float = 0.0
    turn_error_deg: float = 0.0
    turn_predicted_error_deg: float = 0.0
    turn_gyro_deg_s: float = 0.0
    turn_pwm_target: int = 0
    turn_pwm_target_8bit: int = 0
    turn_arc_path_cm: float = 0.0
    turn_translation_x_cm: float = 0.0
    turn_translation_y_cm: float = 0.0
    turn_braking_phase: str = "OFF"
    turn_yaw_authority: bool = False
    turn_imu_no_response_ms: int = 0
    turn_direction_mismatch_ms: int = 0
    cardinal_target_deg: float = 0.0
    move_phase: str = "OFF"
    alignment_stage: str = "OFF"
    turn_tolerance_deg: float = 0.0
    heading_target_deg: float = 0.0
    heading_error_deg: float = 0.0
    heading_predicted_error_deg: float = 0.0
    heading_control_pwm: float = 0.0
    encoder_control_pwm: float = 0.0
    heading_recovery_attempt: int = 0
    heading_recovery_attempt_max: int = 0
    heading_stable_ms: int = 0
    route_coarse_aligned: bool = False
    route_precise_aligned: bool = False
    route_axis_remaining_cm: float = 0.0
    arc_phase: str = "OFF"
    arc_pulse_count: int = 0
    arc_no_progress_count: int = 0
    stall_accumulated_ms: tuple[int, int, int, int] = (0, 0, 0, 0)
    rtos_diagnostics: dict[str, Any] = field(default_factory=dict)
    motor_control: dict[str, Any] = field(default_factory=dict)
    calibration_diagnostics: dict[str, Any] = field(default_factory=dict)
    self_test: dict[str, Any] = field(default_factory=dict)
    target: dict[str, Any] = field(default_factory=dict)
    last_terminal: dict[str, Any] = field(default_factory=dict)
    mission: dict[str, Any] = field(default_factory=dict)
    encoder_health: tuple[str, str, str, str] = ("unknown", "unknown", "unknown", "unknown")
    encoder_fusion: dict[str, Any] = field(default_factory=dict)
    pin_state: dict[str, Any] = field(default_factory=dict)
    raw: dict[str, Any] = field(default_factory=dict)

    @classmethod
    def from_message(cls, message: Mapping[str, Any], fallback_sequence: int) -> "TelemetrySnapshot":
        payload = message.get("payload") if message.get("type") == "telemetry" else message
        if not isinstance(payload, Mapping):
            raise ValueError("Telemetría sin objeto payload")
        sequence = int(message.get("seq", payload.get("seq", fallback_sequence)))
        return cls(
            sequence=sequence,
            received_at=time.time(),
            uptime_ms=int(payload["uptime_ms"]) if "uptime_ms" in payload else None,
            state=str(payload.get("state", payload.get("s", "UNKNOWN")))[:32],
            x_mm=_finite_number(payload.get("x_mm", payload.get("x", 0)), "x", -1e9, 1e9),
            y_mm=_finite_number(payload.get("y_mm", payload.get("y", 0)), "y", -1e9, 1e9),
            yaw_deg=_finite_number(payload.get("yaw_deg", payload.get("a", 0)), "yaw", -1e9, 1e9),
            heading_deg=_heading_degrees(payload.get("heading_deg", payload.get("yaw_deg", 0))),
            imu_yaw_unwrapped_deg=_finite_number(payload.get("imu_yaw_unwrapped_deg", 0), "imu_yaw_unwrapped", -1e9, 1e9),
            imu_yaw_recenter_count=int(payload.get("imu_yaw_recenter_count", 0) or 0),
            pulses=(int(payload.get("pfl", 0)), int(payload.get("pfr", 0)), int(payload.get("pbl", 0)), int(payload.get("pbr", 0))),
            pwm=(int(payload.get("pwm_l", payload.get("pwmL", 0))), int(payload.get("pwm_r", payload.get("pwmR", 0)))),
            wheel_speed_cm_s=(
                _finite_number(
                    payload.get("wheel_speed_cm_s", {}).get("left", 0)
                    if isinstance(payload.get("wheel_speed_cm_s", {}), Mapping) else 0,
                    "wheel_speed_left", -1e6, 1e6,
                ),
                _finite_number(
                    payload.get("wheel_speed_cm_s", {}).get("right", 0)
                    if isinstance(payload.get("wheel_speed_cm_s", {}), Mapping) else 0,
                    "wheel_speed_right", -1e6, 1e6,
                ),
            ),
            encoder_delta_avg=tuple(
                _finite_number(
                    payload.get("encoder_delta_avg", {}).get(key, 0)
                    if isinstance(payload.get("encoder_delta_avg", {}), Mapping) else 0,
                    f"encoder_delta_{key}", -1e6, 1e6,
                )
                for key in ("fl", "fr", "bl", "br")
            ),
            gyro_z_filtered_rad_s=_finite_number(payload.get("gyro_z_filtered_rad_s", 0), "gyro_z_filtered", -1e6, 1e6),
            gyro_z_offset_rad_s=_finite_number(payload.get("gyro_z_offset_rad_s", 0), "gyro_z_offset", -1e6, 1e6),
            theta_error_deg=_finite_number(payload.get("theta_error_deg", 0), "theta_error", -360, 360),
            mpu_present=bool(payload["mpu_present"]) if "mpu_present" in payload else None,
            mpu_stale=bool(payload["mpu_stale"]) if "mpu_stale" in payload else None,
            mpu_calibrated=bool(payload["mpu_calibrated"]) if "mpu_calibrated" in payload else None,
            i2c_ok=bool(payload["i2c_ok"]) if "i2c_ok" in payload else None,
            robot_id=str(payload["robot_id"])[:64] if payload.get("robot_id") else None,
            firmware_version=str(payload["firmware_version"])[:32] if payload.get("firmware_version") else None,
            calibrated=bool(payload.get("calibrated", False)),
            degraded_mode=bool(payload.get("degraded_mode", False)),
            active_command_id=str(payload["active_command_id"])[:32] if payload.get("active_command_id") else None,
            active_command_name=str(payload["active_command_name"])[:24] if payload.get("active_command_name") else None,
            command_progress=_finite_number(payload.get("command_progress", 0), "command_progress", 0, 1),
            turn_attempt=int(payload.get("turn_attempt", 0) or 0),
            turn_attempt_max=int(payload.get("turn_attempt_max", 0) or 0),
            retry_pause_remaining_ms=int(payload.get("retry_pause_remaining_ms", 0) or 0),
            drive_attempt=int(payload.get("drive_attempt", 0) or 0),
            drive_attempt_max=int(payload.get("drive_attempt_max", 0) or 0),
            drive_retry_pause_remaining_ms=int(payload.get("drive_retry_pause_remaining_ms", 0) or 0),
            autonomous_recovery_reason=str(payload.get("autonomous_recovery_reason", "none"))[:48],
            turn_requested_mode=str(payload.get("turn_requested_mode", "AUTO"))[:24],
            turn_mode=str(payload.get("turn_mode", "PIVOT"))[:24],
            turn_mode_reason=str(payload.get("turn_mode_reason", ""))[:96],
            turn_fallback_reason=str(payload.get("turn_fallback_reason", ""))[:96],
            turn_fallback_count=int(payload.get("turn_fallback_count", 0) or 0),
            turn_left_energized=bool(payload.get("turn_left_energized", False)),
            turn_right_energized=bool(payload.get("turn_right_energized", False)),
            turn_left_power_started_ms=int(payload.get("turn_left_power_started_ms", 0) or 0),
            turn_right_power_started_ms=int(payload.get("turn_right_power_started_ms", 0) or 0),
            arc_entry_pause_remaining_ms=int(payload.get("arc_entry_pause_remaining_ms", 0) or 0),
            turn_fixed_sign=int(payload.get("turn_fixed_sign", 0) or 0),
            turn_attempt_sign=int(payload.get("turn_attempt_sign", payload.get("turn_fixed_sign", 0)) or 0),
            turn_requested_deg=_finite_number(payload.get("turn_requested_deg", 0), "turn_requested_deg", -1e6, 1e6),
            turn_physical_target_deg=_finite_number(payload.get("turn_physical_target_deg", 0), "turn_physical_target_deg", -1e6, 1e6),
            turn_wrap_count=int(payload.get("turn_wrap_count", 0) or 0),
            turn_progress_deg=_finite_number(payload.get("turn_progress_deg", 0), "turn_progress_deg", -1e6, 1e6),
            turn_error_deg=_finite_number(payload.get("turn_error_deg", 0), "turn_error_deg", -1e6, 1e6),
            turn_predicted_error_deg=_finite_number(payload.get("turn_predicted_error_deg", 0), "turn_predicted_error_deg", -1e6, 1e6),
            turn_gyro_deg_s=_finite_number(payload.get("turn_gyro_deg_s", 0), "turn_gyro_deg_s", -1e6, 1e6),
            turn_pwm_target=int(payload.get("turn_pwm_target", 0) or 0),
            turn_pwm_target_8bit=int(payload.get("turn_pwm_target_8bit", 0) or 0),
            turn_arc_path_cm=_finite_number(payload.get("turn_arc_path_cm", 0), "turn_arc_path_cm", 0, 1e6),
            turn_translation_x_cm=_finite_number(payload.get("turn_translation_x_cm", 0), "turn_translation_x_cm", -1e6, 1e6),
            turn_translation_y_cm=_finite_number(payload.get("turn_translation_y_cm", 0), "turn_translation_y_cm", -1e6, 1e6),
            turn_braking_phase=str(payload.get("turn_braking_phase", "OFF"))[:24],
            turn_yaw_authority=bool(payload.get("turn_yaw_authority", False)),
            turn_imu_no_response_ms=int(payload.get("turn_imu_no_response_ms", 0) or 0),
            turn_direction_mismatch_ms=int(payload.get("turn_direction_mismatch_ms", 0) or 0),
            cardinal_target_deg=_finite_number(payload.get("cardinal_target_deg", 0), "cardinal_target_deg", -360, 360),
            move_phase=str(payload.get("move_phase", "OFF"))[:32],
            alignment_stage=str(payload.get("alignment_stage", "OFF"))[:32],
            turn_tolerance_deg=_finite_number(payload.get("turn_tolerance_deg", 0), "turn_tolerance_deg", 0, 180),
            heading_target_deg=_finite_number(payload.get("heading_target_deg", 0), "heading_target_deg", -360, 360),
            heading_error_deg=_finite_number(payload.get("heading_error_deg", 0), "heading_error_deg", -180, 180),
            heading_predicted_error_deg=_finite_number(
                payload.get("heading_predicted_error_deg", 0), "heading_predicted_error_deg", -180, 180,
            ),
            heading_control_pwm=_finite_number(payload.get("heading_control_pwm", 0), "heading_control_pwm", -1023, 1023),
            encoder_control_pwm=_finite_number(payload.get("encoder_control_pwm", 0), "encoder_control_pwm", -1023, 1023),
            heading_recovery_attempt=int(payload.get("heading_recovery_attempt", 0) or 0),
            heading_recovery_attempt_max=int(payload.get("heading_recovery_attempt_max", 0) or 0),
            heading_stable_ms=int(payload.get("heading_stable_ms", 0) or 0),
            route_coarse_aligned=bool(payload.get("route_coarse_aligned", False)),
            route_precise_aligned=bool(payload.get("route_precise_aligned", False)),
            route_axis_remaining_cm=_finite_number(
                payload.get("route_axis_remaining_cm", 0), "route_axis_remaining_cm", 0, 1e6,
            ),
            arc_phase=str(payload.get("arc_phase", "OFF"))[:24],
            arc_pulse_count=int(payload.get("arc_pulse_count", 0) or 0),
            arc_no_progress_count=int(payload.get("arc_no_progress_count", 0) or 0),
            stall_accumulated_ms=tuple(
                int(payload.get("stall_accumulated_ms", {}).get(key, 0) or 0)
                if isinstance(payload.get("stall_accumulated_ms", {}), Mapping) else 0
                for key in ("fl", "fr", "bl", "br")
            ),
            rtos_diagnostics=dict(payload.get("rtos", {}))
            if isinstance(payload.get("rtos", {}), Mapping) else {},
            motor_control=dict(payload.get("motor_control", {}))
            if isinstance(payload.get("motor_control", {}), Mapping) else {},
            calibration_diagnostics=dict(payload.get("calibration_diagnostics", {}))
            if isinstance(payload.get("calibration_diagnostics", {}), Mapping) else {},
            self_test=dict(payload.get("self_test", {}))
            if isinstance(payload.get("self_test", {}), Mapping) else {},
            target=dict(payload.get("target", {})) if isinstance(payload.get("target", {}), Mapping) else {},
            last_terminal=dict(payload.get("last_terminal", {})) if isinstance(payload.get("last_terminal", {}), Mapping) else {},
            mission=dict(payload.get("mission", {})) if isinstance(payload.get("mission", {}), Mapping) else {},
            encoder_health=tuple(
                str(payload.get("encoder_health", {}).get(key, "unknown"))[:16]
                if isinstance(payload.get("encoder_health", {}), Mapping) else "unknown"
                for key in ("fl", "fr", "bl", "br")
            ),
            encoder_fusion=dict(payload.get("encoder_fusion", {}))
            if isinstance(payload.get("encoder_fusion", {}), Mapping) else {},
            pin_state=dict(payload.get("pin_state", {})) if isinstance(payload.get("pin_state", {}), Mapping) else {},
            raw=dict(message),
        )

    def public_dict(self) -> dict[str, Any]:
        return {
            "seq": self.sequence, "received_at": self.received_at, "uptime_ms": self.uptime_ms,
            "state": self.state, "x_mm": self.x_mm, "y_mm": self.y_mm, "yaw_deg": self.yaw_deg,
            "heading_deg": self.heading_deg, "imu_yaw_unwrapped_deg": self.imu_yaw_unwrapped_deg,
            "imu_yaw_recenter_count": self.imu_yaw_recenter_count,
            "pulses": {"fl": self.pulses[0], "fr": self.pulses[1], "bl": self.pulses[2], "br": self.pulses[3]},
            "pwm": {"left": self.pwm[0], "right": self.pwm[1]},
            "wheel_speed_cm_s": {"left": self.wheel_speed_cm_s[0], "right": self.wheel_speed_cm_s[1]},
            "encoder_delta_avg": {
                "fl": self.encoder_delta_avg[0], "fr": self.encoder_delta_avg[1],
                "bl": self.encoder_delta_avg[2], "br": self.encoder_delta_avg[3],
            },
            "mpu": {
                "present": self.mpu_present, "stale": self.mpu_stale, "calibrated": self.mpu_calibrated,
                "i2c_ok": self.i2c_ok, "gyro_z_filtered_rad_s": self.gyro_z_filtered_rad_s,
                "gyro_z_offset_rad_s": self.gyro_z_offset_rad_s, "theta_error_deg": self.theta_error_deg,
            },
            "robot_id": self.robot_id, "firmware_version": self.firmware_version, "pin_state": self.pin_state,
            "calibrated": self.calibrated, "degraded_mode": self.degraded_mode,
            "active_command_id": self.active_command_id, "active_command_name": self.active_command_name,
            "command_progress": self.command_progress, "target": self.target, "mission": self.mission,
            "turn_attempt": self.turn_attempt, "turn_attempt_max": self.turn_attempt_max,
            "retry_pause_remaining_ms": self.retry_pause_remaining_ms,
            "drive_attempt": self.drive_attempt,
            "drive_attempt_max": self.drive_attempt_max,
            "drive_retry_pause_remaining_ms": self.drive_retry_pause_remaining_ms,
            "autonomous_recovery_reason": self.autonomous_recovery_reason,
            "turn_requested_mode": self.turn_requested_mode,
            "turn_mode": self.turn_mode, "turn_mode_reason": self.turn_mode_reason,
            "turn_fallback_reason": self.turn_fallback_reason,
            "turn_fallback_count": self.turn_fallback_count,
            "turn_left_energized": self.turn_left_energized,
            "turn_right_energized": self.turn_right_energized,
            "turn_left_power_started_ms": self.turn_left_power_started_ms,
            "turn_right_power_started_ms": self.turn_right_power_started_ms,
            "arc_entry_pause_remaining_ms": self.arc_entry_pause_remaining_ms,
            "turn_fixed_sign": self.turn_fixed_sign,
            "turn_attempt_sign": self.turn_attempt_sign,
            "turn_requested_deg": self.turn_requested_deg,
            "turn_physical_target_deg": self.turn_physical_target_deg,
            "turn_wrap_count": self.turn_wrap_count,
            "turn_progress_deg": self.turn_progress_deg,
            "turn_error_deg": self.turn_error_deg,
            "turn_predicted_error_deg": self.turn_predicted_error_deg,
            "turn_gyro_deg_s": self.turn_gyro_deg_s,
            "turn_pwm_target": self.turn_pwm_target,
            "turn_pwm_target_8bit": self.turn_pwm_target_8bit,
            "turn_arc_path_cm": self.turn_arc_path_cm,
            "turn_translation_x_cm": self.turn_translation_x_cm,
            "turn_translation_y_cm": self.turn_translation_y_cm,
            "turn_braking_phase": self.turn_braking_phase,
            "turn_yaw_authority": self.turn_yaw_authority,
            "turn_imu_no_response_ms": self.turn_imu_no_response_ms,
            "turn_direction_mismatch_ms": self.turn_direction_mismatch_ms,
            "cardinal_target_deg": self.cardinal_target_deg,
            "move_phase": self.move_phase,
            "alignment_stage": self.alignment_stage,
            "turn_tolerance_deg": self.turn_tolerance_deg,
            "heading_target_deg": self.heading_target_deg,
            "heading_error_deg": self.heading_error_deg,
            "heading_predicted_error_deg": self.heading_predicted_error_deg,
            "heading_control_pwm": self.heading_control_pwm,
            "encoder_control_pwm": self.encoder_control_pwm,
            "heading_recovery_attempt": self.heading_recovery_attempt,
            "heading_recovery_attempt_max": self.heading_recovery_attempt_max,
            "heading_stable_ms": self.heading_stable_ms,
            "route_coarse_aligned": self.route_coarse_aligned,
            "route_precise_aligned": self.route_precise_aligned,
            "route_axis_remaining_cm": self.route_axis_remaining_cm,
            "arc_phase": self.arc_phase,
            "arc_pulse_count": self.arc_pulse_count,
            "arc_no_progress_count": self.arc_no_progress_count,
            "stall_accumulated_ms": {
                "fl": self.stall_accumulated_ms[0], "fr": self.stall_accumulated_ms[1],
                "bl": self.stall_accumulated_ms[2], "br": self.stall_accumulated_ms[3],
            },
            "rtos": self.rtos_diagnostics,
            "motor_control": self.motor_control,
            "calibration_diagnostics": self.calibration_diagnostics,
            "self_test": self.self_test,
            "last_terminal": self.last_terminal,
            "encoder_health": {
                "fl": self.encoder_health[0], "fr": self.encoder_health[1],
                "bl": self.encoder_health[2], "br": self.encoder_health[3],
            },
            "encoder_fusion": self.encoder_fusion,
        }

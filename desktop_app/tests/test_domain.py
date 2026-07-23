import unittest

from robot_app.domain import RobotCommand, TelemetrySnapshot, split_segment_mm


class DomainTests(unittest.TestCase):
    def test_mission_upload_requires_explicit_unique_step_identity(self) -> None:
        mission_id = "a" * 32
        command = RobotCommand.create("mission_upload", {
            "mission_id": mission_id, "revision": 1,
            "points": [{"x_mm": 500, "y_mm": 0, "step_id": f"{mission_id[:24]}00000001"}],
        })
        self.assertEqual(command.payload["points"][0]["step_id"], f"{mission_id[:24]}00000001")

    def test_manual_command_is_validated(self) -> None:
        command = RobotCommand.create("manual", {"l": 100, "r": -100})
        self.assertEqual(command.payload, {"l": 100, "r": -100})

    def test_self_test_has_no_motor_payload(self) -> None:
        self.assertEqual(RobotCommand.create("self_test", {}).payload, {})

    def test_out_of_range_pwm_is_rejected(self) -> None:
        self.assertEqual(RobotCommand.create("manual", {"l": 230, "r": -230}).payload,
                         {"l": 230, "r": -230})
        with self.assertRaises(ValueError):
            RobotCommand.create("manual", {"l": 231, "r": 0})

    def test_v1_and_legacy_keep_the_same_command_id(self) -> None:
        command = RobotCommand.create("move", {"x_mm": 100, "y_mm": 200})
        self.assertEqual(command.protocol_envelope()["id"], command.command_id)
        self.assertEqual(command.protocol_envelope()["payload"], {"x_mm": 100.0, "y_mm": 200.0})

    def test_drive_and_turn_use_explicit_units_and_limits(self) -> None:
        self.assertEqual(RobotCommand.create("drive", {"distance_mm": -500}).payload, {"distance_mm": -500.0})
        self.assertEqual(RobotCommand.create("turn", {"angle_deg": 90}).payload, {"angle_deg": 90.0})
        self.assertEqual(RobotCommand.create("turn", {"angle_deg": 0}).payload, {"angle_deg": 0.0})
        self.assertEqual(
            RobotCommand.create("turn", {"angle_deg": -25, "mode": " ARC_LEFT_ACTIVE "}).payload,
            {"angle_deg": -25.0, "mode": "arc_left_active"},
        )
        with self.assertRaisesRegex(ValueError, "mode de giro no permitido"):
            RobotCommand.create("turn", {"angle_deg": 90, "mode": "arc_unknown"})
        with self.assertRaises(ValueError):
            RobotCommand.create("drive", {"distance_mm": 2001})

    def test_long_segment_is_split_without_losing_residual(self) -> None:
        points = split_segment_mm(0, 0, 4_500, 0)
        self.assertEqual(points, [
            {"x_mm": 1500.0, "y_mm": 0.0},
            {"x_mm": 3000.0, "y_mm": 0.0},
            {"x_mm": 4500.0, "y_mm": 0.0},
        ])

    def test_pwm_test_matches_firmware_duration_limit(self) -> None:
        with self.assertRaises(ValueError):
            RobotCommand.create("test_pwm", {"l": 10, "r": 10, "dur_ms": 1501})

    def test_legacy_telemetry_is_supported(self) -> None:
        snapshot = TelemetrySnapshot.from_message({"s": "IDLE", "x": 2, "y": 3, "a": 4}, 7)
        self.assertEqual(snapshot.sequence, 7)
        self.assertEqual(snapshot.state, "IDLE")

    def test_v1_diagnostics_are_normalized(self) -> None:
        snapshot = TelemetrySnapshot.from_message({
            "v": 1, "type": "telemetry", "seq": 9, "payload": {
                "state": "MANUAL", "x_mm": 10, "y_mm": 20, "yaw_deg": 30,
                "heading_deg": 30, "imu_yaw_unwrapped_deg": 390, "imu_yaw_recenter_count": 2,
                "pfl": 1, "pfr": 2, "pbl": 3, "pbr": 4, "pwm_l": 50, "pwm_r": -50,
                "mpu_present": True, "mpu_stale": False, "i2c_ok": True,
                "mpu_calibrated": True, "gyro_z_filtered_rad_s": 0.2,
                "gyro_z_offset_rad_s": 0.01, "theta_error_deg": 2.5,
                "wheel_speed_cm_s": {"left": 12.5, "right": 13.5},
                "encoder_delta_avg": {"fl": 1, "fr": 2, "bl": 3, "br": 4},
                "robot_id": "ESP32S3-AABBCCDDEEFF", "firmware_version": "robot-s3-v1",
                "pin_state": {"i2c_sda": 1},
                "calibrated": True, "degraded_mode": True,
                "active_command_id": "a" * 32, "active_command_name": "move",
                "command_progress": 0.5,
                "turn_attempt": 3, "turn_attempt_max": 21, "retry_pause_remaining_ms": 1250,
                "drive_attempt": 2, "drive_attempt_max": 7,
                "drive_retry_pause_remaining_ms": 500,
                "autonomous_recovery_reason": "drive_left_side_stalled_retry",
                "turn_requested_mode": "AUTO",
                "turn_mode": "ARC_RIGHT_ACTIVE", "turn_mode_reason": "stall_fl",
                "turn_fallback_reason": "stall_fl",
                "turn_fallback_count": 1,
                "turn_left_energized": False, "turn_right_energized": True,
                "turn_left_power_started_ms": 0, "turn_right_power_started_ms": 12345,
                "arc_entry_pause_remaining_ms": 500,
                "turn_fixed_sign": 1, "turn_attempt_sign": -1, "turn_requested_deg": 90,
                "turn_physical_target_deg": 90, "turn_wrap_count": 0,
                "turn_progress_deg": 92.5, "turn_error_deg": -2.5,
                "turn_predicted_error_deg": -1.2, "turn_gyro_deg_s": 8.0,
                "turn_pwm_target": 600, "turn_pwm_target_8bit": 150,
                "turn_braking_phase": "BRAKING",
                "turn_yaw_authority": True, "turn_imu_no_response_ms": 120,
                "turn_direction_mismatch_ms": 0,
                "cardinal_target_deg": 90, "move_phase": "TURN_CARDINAL",
                "alignment_stage": "CONTINUOUS", "turn_tolerance_deg": 2,
                "heading_target_deg": 90, "heading_error_deg": 6.5,
                "heading_predicted_error_deg": 5.25,
                "heading_control_pwm": 42.0, "encoder_control_pwm": -3.0,
                "heading_recovery_attempt": 2, "heading_recovery_attempt_max": 7,
                "heading_stable_ms": 120, "route_coarse_aligned": True,
                "route_precise_aligned": False, "route_axis_remaining_cm": 34.5,
                "arc_phase": "PULSE_OFF", "arc_pulse_count": 4,
                "arc_no_progress_count": 2,
                "stall_accumulated_ms": {"fl": 10, "fr": 20, "bl": 30, "br": 40},
                "rtos": {
                    "architecture": "web_core0_control_superloop_core1",
                    "reset_reason": "power_on", "tasks_created_ok": True,
                    "stack_warning": False, "stack_min_acceptable_bytes": 1024,
                    "stack_min_free_bytes": {"web": 4096, "control": 3072},
                    "control_timing": {
                        "target_period_us": 10000, "last_period_us": 10001,
                        "max_jitter_us": 18, "max_cycle_duration_us": 1620,
                        "max_sample_age_us": 1610, "missed_deadlines": 0,
                        "sample_sequence": 123,
                    },
                },
                "motor_control": {
                    "limit_8bit": 230,
                    "requested": {"left_8bit": 200, "right_8bit": -200},
                    "interlock_left": {"state": "ACTIVE", "energized_sign": 1, "pending_sign": 0},
                },
                "calibration_diagnostics": {
                    "phase": "TORQUE_FIRST_DIRECTION", "pwm_8bit": 170,
                    "last_pulse_ticks": {"fl": 1, "fr": 2, "bl": 1, "br": 2},
                },
                "self_test": {"status": "PASSED", "detail": "all_logic_tests_passed"},
                "last_terminal": {"v": 1, "type": "completed", "id": "b" * 32},
                "encoder_health": {"fl": "ok", "fr": "ok", "bl": "excluded", "br": "ok"},
                "encoder_fusion": {
                    "estimator": "fusion_lados_degradada",
                    "left_reliable_count": 1, "right_reliable_count": 2,
                    "left_no_progress_ms": 0, "right_no_progress_ms": 0,
                },
            }
        }, 1)
        self.assertEqual(snapshot.sequence, 9)
        self.assertEqual(snapshot.pulses, (1, 2, 3, 4))
        self.assertEqual(snapshot.pwm, (50, -50))
        self.assertTrue(snapshot.i2c_ok)
        self.assertTrue(snapshot.mpu_calibrated)
        self.assertEqual(snapshot.wheel_speed_cm_s, (12.5, 13.5))
        self.assertEqual(snapshot.theta_error_deg, 2.5)
        self.assertEqual(snapshot.robot_id, "ESP32S3-AABBCCDDEEFF")
        self.assertTrue(snapshot.calibrated)
        self.assertTrue(snapshot.degraded_mode)
        self.assertEqual(snapshot.command_progress, 0.5)
        self.assertEqual(snapshot.turn_attempt, 3)
        self.assertEqual(snapshot.turn_attempt_max, 21)
        self.assertEqual(snapshot.retry_pause_remaining_ms, 1250)
        self.assertEqual(snapshot.drive_attempt, 2)
        self.assertEqual(snapshot.drive_attempt_max, 7)
        self.assertEqual(snapshot.drive_retry_pause_remaining_ms, 500)
        self.assertEqual(snapshot.autonomous_recovery_reason, "drive_left_side_stalled_retry")
        self.assertEqual(snapshot.turn_requested_mode, "AUTO")
        self.assertEqual(snapshot.turn_mode, "ARC_RIGHT_ACTIVE")
        self.assertEqual(snapshot.turn_mode_reason, "stall_fl")
        self.assertEqual(snapshot.turn_fallback_reason, "stall_fl")
        self.assertEqual(snapshot.turn_fallback_count, 1)
        self.assertFalse(snapshot.turn_left_energized)
        self.assertTrue(snapshot.turn_right_energized)
        self.assertEqual(snapshot.turn_right_power_started_ms, 12345)
        self.assertEqual(snapshot.arc_entry_pause_remaining_ms, 500)
        self.assertEqual(snapshot.turn_fixed_sign, 1)
        self.assertEqual(snapshot.turn_requested_deg, 90)
        self.assertEqual(snapshot.turn_attempt_sign, -1)
        self.assertEqual(snapshot.turn_physical_target_deg, 90)
        self.assertEqual(snapshot.turn_wrap_count, 0)
        self.assertEqual(snapshot.turn_progress_deg, 92.5)
        self.assertEqual(snapshot.turn_error_deg, -2.5)
        self.assertEqual(snapshot.turn_braking_phase, "BRAKING")
        self.assertTrue(snapshot.turn_yaw_authority)
        self.assertEqual(snapshot.turn_imu_no_response_ms, 120)
        self.assertEqual(snapshot.turn_direction_mismatch_ms, 0)
        self.assertEqual(snapshot.cardinal_target_deg, 90)
        self.assertEqual(snapshot.move_phase, "TURN_CARDINAL")
        self.assertEqual(snapshot.alignment_stage, "CONTINUOUS")
        self.assertEqual(snapshot.turn_tolerance_deg, 2)
        self.assertEqual(snapshot.heading_error_deg, 6.5)
        self.assertEqual(snapshot.heading_predicted_error_deg, 5.25)
        self.assertEqual(snapshot.heading_control_pwm, 42.0)
        self.assertEqual(snapshot.encoder_control_pwm, -3.0)
        self.assertEqual(snapshot.heading_recovery_attempt, 2)
        self.assertEqual(snapshot.heading_recovery_attempt_max, 7)
        self.assertEqual(snapshot.encoder_fusion["left_reliable_count"], 1)
        self.assertTrue(snapshot.route_coarse_aligned)
        self.assertFalse(snapshot.route_precise_aligned)
        self.assertEqual(snapshot.route_axis_remaining_cm, 34.5)
        self.assertEqual(snapshot.arc_phase, "PULSE_OFF")
        self.assertEqual(snapshot.arc_pulse_count, 4)
        self.assertEqual(snapshot.arc_no_progress_count, 2)
        self.assertEqual(snapshot.public_dict()["turn_mode"], "ARC_RIGHT_ACTIVE")
        self.assertEqual(snapshot.public_dict()["turn_requested_mode"], "AUTO")
        self.assertEqual(snapshot.public_dict()["turn_physical_target_deg"], 90)
        self.assertEqual(snapshot.public_dict()["heading_deg"], 30)
        self.assertEqual(snapshot.public_dict()["imu_yaw_unwrapped_deg"], 390)
        self.assertEqual(snapshot.public_dict()["imu_yaw_recenter_count"], 2)
        self.assertEqual(snapshot.stall_accumulated_ms, (10, 20, 30, 40))
        self.assertEqual(snapshot.rtos_diagnostics["reset_reason"], "power_on")
        self.assertEqual(snapshot.public_dict()["rtos"]["stack_min_free_bytes"]["control"], 3072)
        self.assertEqual(snapshot.public_dict()["rtos"]["control_timing"]["missed_deadlines"], 0)
        self.assertEqual(snapshot.motor_control["limit_8bit"], 230)
        self.assertEqual(snapshot.calibration_diagnostics["pwm_8bit"], 170)
        self.assertEqual(snapshot.self_test["status"], "PASSED")

        legacy = TelemetrySnapshot.from_message({"yaw_deg": -1}, 2)
        self.assertEqual(legacy.heading_deg, 359)


if __name__ == "__main__":
    unittest.main()

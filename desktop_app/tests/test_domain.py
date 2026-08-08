import unittest

from robot_app.domain import RobotCommand, TelemetrySnapshot, split_segment_mm


class DomainTests(unittest.TestCase):
    def test_step_uses_integer_sequence_and_direct_wire_contract(self) -> None:
        command = RobotCommand.create("step", {"heading": 450, "cm": 50}, seq=7)
        self.assertEqual(command.payload, {"heading": 90.0, "cm": 50.0})
        self.assertEqual(command.protocol_envelope(), {"cmd": "step", "heading": 90.0, "cm": 50.0, "seq": 7})

    def test_turn_to_is_atomic_and_has_no_distance(self) -> None:
        command = RobotCommand.create("turn_to", {"heading": -90}, seq=8)
        self.assertEqual(command.payload, {"heading": 270.0})
        self.assertEqual(command.protocol_envelope(), {"cmd": "turn_to", "heading": 270.0, "seq": 8})

    def test_only_short_memory_commands_are_accepted(self) -> None:
        for legacy in ("manual", "test_pwm", "drive", "turn", "mission_upload"):
            with self.subTest(legacy=legacy), self.assertRaises(ValueError):
                RobotCommand.create(legacy, {})
        self.assertEqual(RobotCommand.create("set_comp", {"factor": 0.95}, seq=2).payload,
                         {"factor": 0.95})

    def test_step_and_compensation_limits_are_enforced(self) -> None:
        with self.assertRaises(ValueError):
            RobotCommand.create("step", {"heading": 0, "cm": 201})
        with self.assertRaises(ValueError):
            RobotCommand.create("step", {"heading": 0, "cm": 20, "target_x_mm": 0})
        for target_x in (float("nan"), 100_001):
            with self.subTest(target_x=target_x), self.assertRaises(ValueError):
                RobotCommand.create("step", {
                    "heading": 0, "cm": 20, "target_x_mm": target_x, "target_y_mm": 0,
                })
        with self.assertRaises(ValueError):
            RobotCommand.create("set_comp", {"factor": 1.01})

    def test_step_accepts_an_additive_absolute_target(self) -> None:
        command = RobotCommand.create("step", {
            "heading": 90, "cm": 50, "drive_mode": "auto", "target_x_mm": 500, "target_y_mm": 0,
        }, seq=11)
        self.assertEqual(command.protocol_envelope(), {
            "cmd": "step", "heading": 90.0, "cm": 50.0, "drive_mode": "auto",
            "target_x_mm": 500.0, "target_y_mm": 0.0, "seq": 11,
        })

    def test_step_validates_forward_reverse_or_auto(self) -> None:
        self.assertEqual(RobotCommand.create("step", {
            "heading": 180, "cm": 20, "drive_mode": "reverse",
        }).payload["drive_mode"], "reverse")
        with self.assertRaises(ValueError):
            RobotCommand.create("step", {"heading": 0, "cm": 20, "drive_mode": "sideways"})

    def test_long_segment_is_split_without_losing_residual(self) -> None:
        pieces = split_segment_mm(0, 0, 4500, 0)
        self.assertEqual(len(pieces), 3)
        self.assertEqual(pieces[-1], {"x_mm": 4500.0, "y_mm": 0.0})

    def test_steps_v3_telemetry_is_normalized(self) -> None:
        snapshot = TelemetrySnapshot.from_message({
            "evt": "telemetry", "state": "listo", "yaw": 359.5,
            "x": 12.3, "y": -4.5, "enc": [1, 2, 3, 4],
            "pwm_l": 10, "pwm_r": 20, "cal": True, "degraded": False,
            "seq": 9, "phase": "avance", "prog": 0.4,
            "target": {"absolute": True, "lateral_error_cm": 2.5, "distance_error_cm": 4.0,
                       "finish_reason": "endpoint_not_reached"},
            "drive_control": {"p": 12, "i": 3, "d": -2, "right_compensation": 1.0},
            "motion": {"effective_mode": "reverse", "coast_cm": 2.4, "settle_elapsed_ms": 320},
            "recovery": {"decision": "reverse_no_pivot", "distance_cm": 24.0,
                         "pivot_avoided": True},
            "firmware": "robot-s3-v2", "reset_reason": "power_on",
            "stack_web": 2048, "stack_control": 3072,
        }, 17)
        public = snapshot.public_dict()
        self.assertEqual(snapshot.sequence, 17)
        self.assertEqual((snapshot.x_mm, snapshot.y_mm), (123.0, -45.0))
        self.assertEqual(snapshot.pulses, (1, 2, 3, 4))
        self.assertEqual(snapshot.active_command_id, "9")
        self.assertTrue(snapshot.calibrated)
        self.assertTrue(public["target"]["absolute"])
        self.assertEqual(public["target"]["finish_reason"], "endpoint_not_reached")
        self.assertEqual(public["drive_control"]["i"], 3)
        self.assertEqual(public["motion"]["effective_mode"], "reverse")
        self.assertTrue(public["recovery"]["pivot_avoided"])
        self.assertEqual(public["rtos"]["stack_min_free_bytes"]["control"], 3072)

    def test_legacy_telemetry_remains_readable_for_exports(self) -> None:
        snapshot = TelemetrySnapshot.from_message({"yaw_deg": -1}, 2)
        self.assertEqual(snapshot.heading_deg, 359)


if __name__ == "__main__":
    unittest.main()

import json
import re
import time
import unittest
from pathlib import Path
from unittest.mock import patch

from robot_app.domain import RobotCommand
from robot_app.config import MAX_ROBOT_MESSAGE_BYTES
from robot_app.gateway import RobotGateway


class _Connection:
    def __init__(self) -> None:
        self.messages: list[str] = []

    def send(self, message: str) -> None:
        self.messages.append(message)


class _OversizedConnection(_Connection):
    def settimeout(self, _timeout: float) -> None:
        pass

    def recv(self) -> bytes:
        return b"x" * (MAX_ROBOT_MESSAGE_BYTES + 1)

    def close(self) -> None:
        pass


class GatewayTests(unittest.TestCase):
    def setUp(self) -> None:
        self.received: list[dict] = []
        self.gateway = RobotGateway(
            lambda: "192.168.4.1", self.received.append,
            lambda state, detail: None, lambda command: None,
            session_getter=lambda: "0123456789abcdef",
        )

    def test_commands_wait_for_steps_v3_handshake(self) -> None:
        connection = _Connection()
        command = RobotCommand.create("step", {"heading": 90, "cm": 50}, seq=3)
        self.assertTrue(self.gateway.enqueue(command))
        self.gateway._drain_one(connection)
        self.assertEqual(connection.messages, [])
        self.gateway._receive(json.dumps({
            "evt": "hello_ack", "protocol": "robot-s3-steps-v3",
            "session": "0123456789abcdef", "last_seq": 0,
        }))
        self.gateway._drain_one(connection)
        self.assertEqual(json.loads(connection.messages[0]),
                         {"cmd": "step", "heading": 90.0, "cm": 50.0, "seq": 3})

    def test_incompatible_protocol_or_session_is_rejected(self) -> None:
        with self.assertRaises(ValueError):
            self.gateway._receive(json.dumps({"evt": "hello_ack", "protocol": "otro",
                                              "session": "0123456789abcdef"}))
        with self.assertRaises(ValueError):
            self.gateway._receive(json.dumps({"evt": "hello_ack", "protocol": "robot-s3-steps-v3",
                                              "session": "otra"}))

    def test_accepts_firmware_limit_and_rejects_only_larger_frames(self) -> None:
        prefix = '{"evt":"telemetry","pad":"'
        suffix = '"}'
        valid = prefix + "x" * (MAX_ROBOT_MESSAGE_BYTES - len(prefix) - len(suffix)) + suffix
        self.assertEqual(len(valid.encode("utf-8")), MAX_ROBOT_MESSAGE_BYTES)
        self.assertEqual(self.gateway._receive(valid), "telemetry")
        with self.assertRaisesRegex(
            ValueError,
            rf"{MAX_ROBOT_MESSAGE_BYTES + 1} bytes > limite {MAX_ROBOT_MESSAGE_BYTES} bytes",
        ):
            self.gateway._receive(valid + " ")

    def test_python_limit_matches_firmware_contract(self) -> None:
        header = Path(__file__).resolve().parents[2] / "include" / "ProtocolLimits.h"
        match = re.search(
            r"WS_MESSAGE_MAX_BYTES\s*=\s*(\d+)",
            header.read_text(encoding="utf-8"),
        )
        self.assertIsNotNone(match)
        self.assertEqual(int(match.group(1)), MAX_ROBOT_MESSAGE_BYTES)

    def test_session_231_sized_calibration_telemetry_is_accepted(self) -> None:
        message = {
            "evt": "telemetry",
            "state": "calibrando",
            "progress": 0.099860005,
            "pwm": [-702, 702],
            "reset_reason": "power_on",
            "torque_history": {"mounted": False, "status": "mount_failed"},
            "calibration_diagnostics": {
                "phase": "cal_a",
                "authority": "imu_primary_bilateral_pcnt",
                "encoder_delta": [0, 0, 0, 0],
                "pivot_pwm": {"logical_left": -702, "logical_right": 702},
            },
            "diagnostic_padding": "x" * 5200,
        }
        encoded = json.dumps(message, separators=(",", ":"))
        self.assertGreater(len(encoded.encode("utf-8")), 5120)
        self.assertLessEqual(len(encoded.encode("utf-8")), MAX_ROBOT_MESSAGE_BYTES)
        self.assertEqual(self.gateway._receive(encoded), "telemetry")

    def test_cancelled_command_is_not_sent_after_reconnect(self) -> None:
        connection = _Connection()
        command = RobotCommand.create("reset_pose", {}, seq=4)
        self.gateway._protocol_v1 = True
        self.assertTrue(self.gateway.enqueue(command))
        self.gateway.cancel(command.command_id)
        self.gateway._drain_one(connection)
        self.assertEqual(connection.messages, [])

    def test_stop_has_priority_over_a_pending_maneuver(self) -> None:
        connection = _Connection()
        step = RobotCommand.create("step", {"heading": 0, "cm": 50}, seq=5)
        stop = RobotCommand.create("stop", {}, seq=6)
        self.gateway._protocol_v1 = True
        self.assertTrue(self.gateway.enqueue(step))
        self.assertTrue(self.gateway.enqueue(stop))
        self.gateway._drain_one(connection)
        self.assertEqual(json.loads(connection.messages[0]), {"cmd": "stop", "seq": 6})

    def test_manual_drive_is_latest_wins_and_manual_end_clears_it(self) -> None:
        connection = _Connection()
        self.gateway._protocol_v1 = True
        first = RobotCommand.create("manual_drive", {
            "throttle": .2, "steering": 0, "stream": 7, "frame": 1,
        }, seq=0)
        latest = RobotCommand.create("manual_drive", {
            "throttle": .6, "steering": -.1, "stream": 7, "frame": 2,
        }, seq=0)
        self.assertTrue(self.gateway.enqueue(first))
        self.assertTrue(self.gateway.enqueue(latest))
        self.gateway._drain_one(connection)
        self.assertEqual(len(connection.messages), 1)
        self.assertEqual(json.loads(connection.messages[0])["frame"], 2)
        self.gateway.enqueue(first)
        self.gateway.enqueue(RobotCommand.create("manual_end", {}, seq=8))
        self.gateway._drain_one(connection)
        self.assertEqual(json.loads(connection.messages[-1]), {"cmd": "manual_end", "seq": 8})

    def test_manual_begin_flushes_latest_drive_before_receive_wait(self) -> None:
        connection = _Connection()
        self.gateway._protocol_v1 = True
        begin = RobotCommand.create("manual_begin", {}, seq=7)
        drive = RobotCommand.create("manual_drive", {
            "throttle": .5, "steering": .1, "stream": 3, "frame": 4,
        }, seq=0)
        self.assertTrue(self.gateway.enqueue(begin))
        self.assertTrue(self.gateway.enqueue(drive))
        self.gateway._drain_one(connection)
        self.assertEqual([json.loads(message)["cmd"] for message in connection.messages],
                         ["manual_begin", "manual_drive"])

    def test_connection_attempts_stop_instead_of_looping_forever(self) -> None:
        self.gateway.MAX_CONNECT_ATTEMPTS = 1
        with patch("robot_app.gateway.websocket.create_connection", side_effect=TimeoutError("sin ruta")):
            self.gateway.start()
            deadline = time.monotonic() + 1.0
            snapshot = self.gateway.snapshot()
            while snapshot["state"] != "stopped" and time.monotonic() < deadline:
                time.sleep(0.01)
                snapshot = self.gateway.snapshot()
            self.assertEqual(snapshot["state"], "stopped")
            self.gateway.stop()

    def test_oversized_frames_consume_attempts_instead_of_reconnecting_forever(self) -> None:
        self.gateway.MAX_CONNECT_ATTEMPTS = 1
        connection = _OversizedConnection()
        with patch("robot_app.gateway.websocket.create_connection", return_value=connection) as connect:
            self.gateway.start()
            deadline = time.monotonic() + 1.0
            snapshot = self.gateway.snapshot()
            while snapshot["state"] != "stopped" and time.monotonic() < deadline:
                time.sleep(0.01)
                snapshot = self.gateway.snapshot()
            self.assertEqual(snapshot["state"], "stopped")
            self.assertEqual(snapshot["connect_attempt"], 1)
            self.assertEqual(connect.call_count, 1)
            self.gateway.stop()


if __name__ == "__main__":
    unittest.main()

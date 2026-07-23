import json
import time
import unittest
from unittest.mock import patch

from robot_app.domain import RobotCommand
from robot_app.gateway import RobotGateway


class _Connection:
    def __init__(self) -> None:
        self.messages: list[str] = []

    def send(self, message: str) -> None:
        self.messages.append(message)


class GatewayTests(unittest.TestCase):
    def setUp(self) -> None:
        self.received: list[dict] = []
        self.gateway = RobotGateway(
            lambda: "192.168.4.1", self.received.append,
            lambda state, detail: None, lambda command: None,
        )

    def test_commands_wait_for_v1_handshake(self) -> None:
        connection = _Connection()
        command = RobotCommand.create("turn", {"angle_deg": 90})
        self.assertTrue(self.gateway.enqueue(command))
        self.gateway._drain_one(connection)
        self.assertEqual(connection.messages, [])
        self.gateway._receive(json.dumps({
            "v": 1, "type": "hello", "protocol": "robot-s3-json-v1", "role": "robot",
        }))
        self.gateway._drain_one(connection)
        envelope = json.loads(connection.messages[0])
        self.assertEqual(envelope["id"], command.command_id)
        self.assertEqual(envelope["payload"], {"angle_deg": 90.0})

    def test_incompatible_protocol_is_rejected(self) -> None:
        with self.assertRaises(ValueError):
            self.gateway._receive(json.dumps({"v": 1, "type": "hello", "protocol": "otro"}))

    def test_cancelled_command_is_not_sent_after_reconnect(self) -> None:
        connection = _Connection()
        command = RobotCommand.create("move", {"x_mm": 100, "y_mm": 0})
        self.gateway._protocol_v1 = True
        self.assertTrue(self.gateway.enqueue(command))
        self.gateway.cancel(command.command_id)
        self.gateway._drain_one(connection)
        self.assertEqual(connection.messages, [])

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
            self.assertIn("ROBOT_S3_LOCAL", snapshot["detail"])
            self.gateway.stop()


if __name__ == "__main__":
    unittest.main()

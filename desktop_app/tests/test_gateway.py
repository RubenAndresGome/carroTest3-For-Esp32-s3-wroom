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
            session_getter=lambda: "0123456789abcdef",
        )

    def test_commands_wait_for_steps_v2_handshake(self) -> None:
        connection = _Connection()
        command = RobotCommand.create("step", {"heading": 90, "cm": 50}, seq=3)
        self.assertTrue(self.gateway.enqueue(command))
        self.gateway._drain_one(connection)
        self.assertEqual(connection.messages, [])
        self.gateway._receive(json.dumps({
            "evt": "hello_ack", "protocol": "robot-s3-steps-v2",
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
            self.gateway._receive(json.dumps({"evt": "hello_ack", "protocol": "robot-s3-steps-v2",
                                              "session": "otra"}))

    def test_cancelled_command_is_not_sent_after_reconnect(self) -> None:
        connection = _Connection()
        command = RobotCommand.create("reset_pose", {}, seq=4)
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
            self.gateway.stop()


if __name__ == "__main__":
    unittest.main()

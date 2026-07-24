import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from robot_app.app_factory import create_app
from robot_app.config import AppConfig
from robot_app.domain import ConnectionState, TelemetrySnapshot


class ApiTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.app = create_app(AppConfig(self.root, self.root / "test.sqlite3", start_gateway=False))
        self.client = self.app.test_client()
        self.token = self.app.extensions["app_token"]
        self.service = self.app.extensions["robot_service"]
        self.service.gateway._state = ConnectionState.CONNECTED
        self.service.gateway._protocol_v1 = True

    def tearDown(self) -> None:
        self.service.close()
        self.temp.cleanup()

    def _ready(self, x_mm: float = 0, y_mm: float = 0) -> None:
        self.service._last_telemetry = TelemetrySnapshot.from_message({
            "evt": "telemetry", "state": "listo", "x": x_mm / 10,
            "y": y_mm / 10, "yaw": 0, "cal": True, "enc": [0, 0, 0, 0],
        }, 1)

    def test_status_and_mutation_security(self) -> None:
        self.assertEqual(self.client.get("/api/v1/status").status_code, 200)
        denied = self.client.post("/api/v1/commands", json={"name": "stop"})
        allowed = self.client.post("/api/v1/commands", json={"name": "stop"},
                                   headers={"X-App-Token": self.token})
        self.assertEqual(denied.status_code, 403)
        self.assertEqual(allowed.status_code, 202)

    def test_hmi_describes_physical_calibration_and_steps_v2(self) -> None:
        html = self.client.get("/").get_data(as_text=True)
        self.assertIn("La calibración moverá el robot", html)
        self.assertIn("robot-s3-steps-v2", html)
        self.assertNotIn("Este botón sólo recentra la pose", html)

    def test_event_seq_is_mapped_back_to_python_command_id(self) -> None:
        self.service.start_session()
        response = self.client.post(
            "/api/v1/commands", json={"name": "reset_pose"},
            headers={"X-App-Token": self.token},
        )
        command = self.service.gateway._outgoing.queue[0].command
        self.service._on_robot_message({"evt": "accepted", "seq": command.seq})
        row = self.service.database.command_rows(self.service._session_id)[0]
        self.assertEqual(row["id"], response.json["id"])
        self.assertEqual(row["status"], "acknowledged")

    def test_route_is_split_and_sent_one_atomic_step_at_a_time(self) -> None:
        self._ready()
        mission = self.service.start_mission([{"x_mm": 4500, "y_mm": 0}])
        self.assertEqual(mission["total_segments"], 3)
        first = self.service.gateway._outgoing.queue[0].command
        self.assertEqual(first.name, "step")
        self.assertEqual(first.payload, {"heading": 90.0, "cm": 150.0})
        self.service._on_robot_message({"evt": "completed", "seq": first.seq, "detail": "step_ok"})
        status = self.service.mission_status()
        self.assertEqual(status["current_index"], 1)
        self.assertNotEqual(status["active_seq"], first.seq)

    def test_orthogonal_headings_follow_plus_y_zero_convention(self) -> None:
        self._ready()
        self.service.start_mission([{"x_mm": 0, "y_mm": 1000}, {"x_mm": 1000, "y_mm": 1000}])
        first = self.service.gateway._outgoing.queue[0].command
        self.assertEqual(first.payload["heading"], 0.0)
        self.service._on_robot_message({"evt": "completed", "seq": first.seq})
        commands = [item.command for item in self.service.gateway._outgoing.queue]
        self.assertEqual(commands[-1].payload["heading"], 90.0)

    def test_diagonal_route_is_rejected_but_one_mm_tolerance_is_allowed(self) -> None:
        self._ready()
        bad = self.client.post("/api/v1/missions", json={"points": [{"x_mm": 1000, "y_mm": 1000}]},
                               headers={"X-App-Token": self.token})
        self.assertEqual(bad.status_code, 202)
        good = self.client.post("/api/v1/missions", json={"points": [{"x_mm": 1000, "y_mm": 1}]},
                                headers={"X-App-Token": self.token})
        self.assertEqual(good.status_code, 202)

    def test_fault_blocks_current_waypoint(self) -> None:
        self._ready()
        mission = self.service.start_mission([{"x_mm": 1000, "y_mm": 0}])
        self.service._on_robot_message({"evt": "fault", "seq": mission["active_seq"], "detail": "stall_FR"})
        status = self.service.mission_status()
        self.assertTrue(status["blocked"])
        self.assertEqual(status["current_index"], 0)
        self.assertEqual(status["error"], "stall_FR")

    def test_hello_reconciles_completed_step_without_repeating_it(self) -> None:
        self._ready()
        mission = self.service.start_mission([{"x_mm": 1000, "y_mm": 0}])
        self.service._on_robot_message({
            "evt": "hello_ack", "state": "listo", "last_seq": mission["active_seq"],
            "session": self.service._controller_session, "protocol": "robot-s3-steps-v2",
        })
        status = self.service.mission_status()
        self.assertEqual(status["current_index"], 1)
        self.assertFalse(status["running"])

    def test_restart_mid_step_is_blocked(self) -> None:
        self._ready()
        self.service.start_mission([{"x_mm": 1000, "y_mm": 0}])
        self.service._on_robot_message({
            "evt": "hello_ack", "state": "desarmado", "last_seq": 0,
            "session": self.service._controller_session, "protocol": "robot-s3-steps-v2",
        })
        self.assertEqual(self.service.mission_status()["error"], "robot_restarted_mid_step")

    def test_stalled_coordination_timeout_blocks_route(self) -> None:
        self._ready()
        with patch("robot_app.services.time.monotonic", return_value=100.0):
            self.service.start_mission([{"x_mm": 1000, "y_mm": 0}])
            command = self.service.gateway._outgoing.queue[0].command
            self.service._on_command_sent(command)
        with patch("robot_app.services.time.monotonic", return_value=341.0):
            self.service._expire_stalled_mission()
        self.assertEqual(self.service.mission_status()["error"], "mission_segment_timeout")

    def test_session_export_keeps_commands_and_events(self) -> None:
        session_id = self.service.start_session()
        self.service.send_command("stop", {})
        self.service.stop_session("test")
        export = self.client.get(f"/api/v1/sessions/{session_id}.json")
        self.assertEqual(export.status_code, 200)
        self.assertIn("commands", export.json)
        self.assertIn("events", export.json)

    def test_cleanup_sessions_endpoint(self) -> None:
        session_id = self.service.start_session()
        self.service.stop_session("test")
        response = self.client.post(
            "/api/v1/sessions/cleanup", json={"days": 0},
            headers={"X-App-Token": self.token},
        )
        self.assertEqual(response.status_code, 200)
        self.assertIn("deleted_sessions", response.json)


if __name__ == "__main__":
    unittest.main()


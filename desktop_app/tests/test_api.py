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
        service = self.app.extensions["robot_service"]
        service.gateway._state = ConnectionState.CONNECTED
        service.gateway._protocol_v1 = True

    def tearDown(self) -> None:
        self.app.extensions["robot_service"].close()
        self.temp.cleanup()

    def test_status_is_readable(self) -> None:
        response = self.client.get("/api/v1/status")
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.json["robot_host"], "192.168.4.1")

    def test_environment_does_not_autoconnect_by_default(self) -> None:
        with patch.dict("os.environ", {"ROBOT_APP_DATA_DIR": str(self.root)}, clear=True):
            config = AppConfig.from_environment()
        self.assertFalse(config.start_gateway)

    def test_same_robot_host_does_not_force_reconnect(self) -> None:
        service = self.app.extensions["robot_service"]
        with patch.object(service.gateway, "request_reconnect") as reconnect:
            service.set_robot_host("192.168.4.1")
            reconnect.assert_not_called()
            service.set_robot_host("192.168.4.2")
            reconnect.assert_called_once()

    def test_mutation_requires_local_token(self) -> None:
        denied = self.client.post("/api/v1/commands", json={"name": "stop"})
        allowed = self.client.post(
            "/api/v1/commands", json={"name": "stop"}, headers={"X-App-Token": self.token}
        )
        self.assertEqual(denied.status_code, 403)
        self.assertEqual(allowed.status_code, 202)

    def test_index_uses_migrated_ui_and_local_token(self) -> None:
        response = self.client.get("/")
        html = response.get_data(as_text=True)
        self.assertEqual(response.status_code, 200)
        self.assertIn('data-tab="conexion"', html)
        self.assertIn('id="btn-calibrate"', html)
        self.assertIn('id="connection-panel-detail"', html)
        self.assertIn("ROBOT_S3_LOCAL en el Wi‑Fi", html)
        self.assertIn("calibration-locked", html)
        self.assertIn("sólo recentra la pose y el rumbo", html)
        self.assertNotIn("girará +25°", html)
        self.assertIn("turn_physical_target_deg", html)
        self.assertIn('id="turn-mode"', html)
        self.assertNotIn('value="arc_left_active"', html)
        self.assertIn("turn_requested_mode", html)
        self.assertIn('id="telem-rtos"', html)
        self.assertIn("stack_min_free_bytes", html)
        self.assertIn("Automático — pivot continuo", html)
        self.assertIn("pausa por sobrepaso 300 ms", html)
        self.assertIn("cada paso termina a 0°", html)
        self.assertIn("CAÍDA DE VOLTAJE", html)
        self.assertIn("/api/v1/missions", html)
        self.assertIn(self.token, html)
        self.assertIn("Content-Security-Policy", response.headers)

    def test_session_exports_include_diagnostics(self) -> None:
        service = self.app.extensions["robot_service"]
        session_id = service.start_session()
        service.stop_session("test")
        listing = self.client.get("/api/v1/sessions")
        export = self.client.get(f"/api/v1/sessions/{session_id}.json")
        self.assertEqual(listing.status_code, 200)
        self.assertEqual(listing.json[0]["disconnect_reason"], "test")
        self.assertEqual(export.status_code, 200)
        self.assertIn("commands", export.json)
        self.assertIn("events", export.json)

    def test_robot_ack_uses_python_command_id_and_heartbeats_are_not_persisted(self) -> None:
        service = self.app.extensions["robot_service"]
        session_id = service.start_session()
        response = self.client.post(
            "/api/v1/commands", json={"name": "stop"}, headers={"X-App-Token": self.token}
        )
        command_id = response.json["id"]
        service._on_robot_message({"v": 1, "type": "accepted", "id": command_id, "cmd": "stop"})
        service._on_robot_message({"v": 1, "type": "heartbeat_ack", "ts_ms": 123})
        commands = service.database.command_rows(session_id)
        events = service.database.event_rows(session_id)
        self.assertEqual(commands[0]["id"], command_id)
        self.assertEqual(commands[0]["status"], "acknowledged")
        self.assertEqual(events[0]["kind"], "accepted")
        self.assertNotIn("heartbeat_ack", {event["kind"] for event in events})

    def test_mission_is_uploaded_once_and_advances_from_firmware_checkpoint(self) -> None:
        service = self.app.extensions["robot_service"]
        service._last_telemetry = TelemetrySnapshot.from_message({
            "v": 1, "type": "telemetry", "payload": {
                "state": "IDLE", "x_mm": 0, "y_mm": 0, "yaw_deg": 0, "calibrated": True,
            },
        }, 1)
        response = self.client.post(
            "/api/v1/missions",
            json={"points": [{"x_mm": 4500, "y_mm": 0}]},
            headers={"X-App-Token": self.token},
        )
        self.assertEqual(response.status_code, 202)
        self.assertEqual(response.json["total_segments"], 3)
        upload = service.gateway._outgoing.queue[0].command
        step_ids = [point["step_id"] for point in upload.payload["points"]]
        self.assertEqual(len(step_ids), len(set(step_ids)))
        self.assertTrue(all(len(step_id) == 32 for step_id in step_ids))
        first_id = response.json["active_command_id"]
        service._on_robot_message({"v": 1, "type": "completed", "id": "f" * 32})
        self.assertEqual(service.mission_status()["current_index"], 0)
        service._on_robot_message({"v": 1, "type": "completed", "id": first_id})
        self.assertEqual(service.mission_status()["current_index"], 0)
        self.assertNotEqual(service.mission_status()["active_command_id"], first_id)
        service._on_robot_message({"v": 1, "type": "telemetry", "payload": {
            "state": "IDLE", "calibrated": True,
            "mission": {"id": response.json["id"], "revision": 1, "state": "EXECUTING", "completed_steps": 1},
        }})
        self.assertEqual(service.mission_status()["current_index"], 1)

    def test_interrupted_firmware_step_is_blocked_and_never_reuploaded(self) -> None:
        service = self.app.extensions["robot_service"]
        service._last_telemetry = TelemetrySnapshot.from_message({
            "v": 1, "type": "telemetry", "payload": {
                "state": "IDLE", "x_mm": 0, "y_mm": 0, "yaw_deg": 0, "calibrated": True,
            },
        }, 1)
        mission = service.start_mission([{"x_mm": 500, "y_mm": 0}])
        queued_before = service.gateway._outgoing.qsize()
        service._on_robot_message({"v": 1, "type": "hello_ack", "protocol": "robot-s3-json-v1", "mission": {
            "id": mission["id"], "revision": 1, "state": "INTERRUPTED",
            "completed_steps": 0, "active_step_id": f"{mission['id'][:24]}00000001",
        }})
        status = service.mission_status()
        self.assertTrue(status["blocked"])
        self.assertEqual(status["stage"], "interrupted")
        self.assertEqual(status["error"], "robot_restarted_mid_mission")
        self.assertEqual(service.gateway._outgoing.qsize(), queued_before)

    def test_mission_requires_live_v1_connection(self) -> None:
        service = self.app.extensions["robot_service"]
        service._last_telemetry = TelemetrySnapshot.from_message({
            "v": 1, "type": "telemetry", "payload": {
                "state": "IDLE", "x_mm": 0, "y_mm": 0, "yaw_deg": 0, "calibrated": True,
            },
        }, 1)
        service.gateway._state = ConnectionState.STOPPED
        response = self.client.post(
            "/api/v1/missions", json={"points": [{"x_mm": 100, "y_mm": 0}]},
            headers={"X-App-Token": self.token},
        )
        self.assertEqual(response.status_code, 409)
        self.assertIn("protocolo v1", response.json["error"])

    def test_reconciliation_waits_until_gateway_sends_command(self) -> None:
        service = self.app.extensions["robot_service"]
        service._last_telemetry = TelemetrySnapshot.from_message({
            "v": 1, "type": "telemetry", "payload": {
                "state": "IDLE", "x_mm": 0, "y_mm": 0, "yaw_deg": 0, "calibrated": True,
            },
        }, 1)
        with patch("robot_app.services.time.monotonic", return_value=100.0):
            service.start_mission([{"x_mm": 100, "y_mm": 0}])
        with patch("robot_app.services.time.monotonic", return_value=103.0):
            service._on_robot_message({
                "v": 1, "type": "telemetry", "payload": {
                    "state": "IDLE", "x_mm": 0, "y_mm": 0, "yaw_deg": 0, "calibrated": True,
                },
            })
        self.assertFalse(service.mission_status()["blocked"])

    def test_orthogonal_route_is_executed_by_esp32_after_single_upload(self) -> None:
        service = self.app.extensions["robot_service"]
        service._last_telemetry = TelemetrySnapshot.from_message({
            "v": 1, "type": "telemetry", "payload": {
                "state": "IDLE", "x_mm": 0, "y_mm": 0, "yaw_deg": 0, "calibrated": True,
            },
        }, 1)
        mission = service.start_mission([
            {"x_mm": 1000, "y_mm": 0},
            {"x_mm": 1000, "y_mm": 1000},
        ])
        first_id = mission["active_command_id"]

        service._on_robot_message({"v": 1, "type": "completed", "id": "a" * 32})
        self.assertEqual(service.mission_status()["current_index"], 0)
        service._on_robot_message({"v": 1, "type": "completed", "id": first_id})

        second = service.mission_status()
        self.assertEqual(second["current_index"], 0)
        self.assertEqual(second["stage"], "executing")
        self.assertNotEqual(second["active_command_id"], first_id)
        service._on_robot_message({
            "v": 1, "type": "telemetry", "payload": {"state": "IDLE", "calibrated": True,
                "mission": {"id": mission["id"], "revision": 1, "state": "COMPLETED", "completed_steps": 2}},
        })
        self.assertEqual(service.mission_status()["current_index"], 2)
        self.assertFalse(service.mission_status()["running"])

    def test_diagonal_route_is_rejected_with_waypoint_index(self) -> None:
        service = self.app.extensions["robot_service"]
        service._last_telemetry = TelemetrySnapshot.from_message({
            "v": 1, "type": "telemetry", "payload": {
                "state": "IDLE", "x_mm": 0, "y_mm": 0,
                "yaw_deg": 0, "calibrated": True,
            },
        }, 1)
        response = self.client.post(
            "/api/v1/missions",
            json={"points": [{"x_mm": 1000, "y_mm": 1000}]},
            headers={"X-App-Token": self.token},
        )
        self.assertEqual(response.status_code, 400)
        self.assertIn("waypoint 1", response.json["error"])
        self.assertIn("no ortogonal", response.json["error"])

    def test_one_millimeter_cross_axis_tolerance_is_accepted(self) -> None:
        service = self.app.extensions["robot_service"]
        service._last_telemetry = TelemetrySnapshot.from_message({
            "v": 1, "type": "telemetry", "payload": {
                "state": "IDLE", "x_mm": 0, "y_mm": 0,
                "yaw_deg": 0, "calibrated": True,
            },
        }, 1)
        mission = service.start_mission([{"x_mm": 1000, "y_mm": 1}])
        self.assertEqual(mission["total_segments"], 1)

    def test_stalled_segment_stops_mission_after_safety_timeout(self) -> None:
        service = self.app.extensions["robot_service"]
        service._last_telemetry = TelemetrySnapshot.from_message({
            "v": 1, "type": "telemetry", "payload": {
                "state": "IDLE", "x_mm": 0, "y_mm": 0, "yaw_deg": 0, "calibrated": True,
            },
        }, 1)
        with patch("robot_app.services.time.monotonic", return_value=100.0):
            service.start_mission([{"x_mm": 1000, "y_mm": 0}])
            queued_command = service.gateway._outgoing.queue[0].command
            service._on_command_sent(queued_command)
        with patch("robot_app.services.time.monotonic", return_value=341.0):
            service._expire_stalled_mission()
        status = service.mission_status()
        self.assertFalse(status["running"])
        self.assertTrue(status["blocked"])
        self.assertEqual(status["current_index"], 0)
        self.assertEqual(status["error"], "mission_segment_timeout")
        self.assertIsNone(status["active_command_id"])

    def test_fault_blocks_route_and_preserves_failed_waypoint(self) -> None:
        service = self.app.extensions["robot_service"]
        service._last_telemetry = TelemetrySnapshot.from_message({
            "v": 1, "type": "telemetry", "payload": {
                "state": "IDLE", "x_mm": 0, "y_mm": 0, "yaw_deg": 0, "calibrated": True,
            },
        }, 1)
        mission = service.start_mission([{"x_mm": 2500, "y_mm": 0}])
        command_id = mission["active_command_id"]
        service._on_robot_message({"v": 1, "type": "fault", "id": command_id, "detail": "stall_fr"})
        status = service.mission_status()
        self.assertFalse(status["running"])
        self.assertTrue(status["blocked"])
        self.assertEqual(status["current_index"], 0)
        self.assertEqual(status["error"], "stall_fr")

    def test_operator_reset_clears_blocked_route_error(self) -> None:
        service = self.app.extensions["robot_service"]
        service._last_telemetry = TelemetrySnapshot.from_message({
            "v": 1, "type": "telemetry", "payload": {
                "state": "IDLE", "x_mm": 0, "y_mm": 0, "yaw_deg": 0, "calibrated": True,
            },
        }, 1)
        mission = service.start_mission([{"x_mm": 100, "y_mm": 0}])
        service._on_robot_message({
            "v": 1, "type": "rejected", "id": mission["active_command_id"],
            "detail": "non_orthogonal_segment",
        })
        self.assertTrue(service.mission_status()["blocked"])

        status = service.stop_mission()
        self.assertFalse(status["blocked"])
        self.assertIsNone(status["error"])
        self.assertIsNone(status["id"])
        self.assertEqual(status["total_segments"], 0)

    def test_reconnect_terminal_is_processed_before_timeout(self) -> None:
        service = self.app.extensions["robot_service"]
        service._last_telemetry = TelemetrySnapshot.from_message({
            "v": 1, "type": "telemetry", "payload": {
                "state": "IDLE", "x_mm": 0, "y_mm": 0, "yaw_deg": 0, "calibrated": True,
            },
        }, 1)
        with patch("robot_app.services.time.monotonic", return_value=100.0):
            mission = service.start_mission([{"x_mm": 1000, "y_mm": 0}])
        command_id = mission["active_command_id"]
        with patch("robot_app.services.time.monotonic", return_value=400.0):
            service._on_robot_message({
                "v": 1, "type": "telemetry", "payload": {
                    "state": "IDLE", "x_mm": 1000, "y_mm": 0, "yaw_deg": 0,
                    "calibrated": True,
                    "mission": {"id": mission["id"], "revision": 1, "state": "EXECUTING", "completed_steps": 1},
                },
            })
        status = service.mission_status()
        self.assertFalse(status["blocked"])
        self.assertEqual(status["current_index"], 1)
        self.assertIsNone(status["error"])


if __name__ == "__main__":
    unittest.main()

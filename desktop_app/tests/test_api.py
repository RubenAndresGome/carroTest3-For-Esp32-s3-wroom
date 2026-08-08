import tempfile
import threading
import time
import unittest
from pathlib import Path
from unittest.mock import patch

from robot_app.app_factory import create_app
from robot_app.config import AppConfig
from robot_app.database import Database
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

    def test_hmi_describes_physical_calibration_and_steps_v3(self) -> None:
        html = self.client.get("/").get_data(as_text=True)
        self.assertIn("La calibración moverá el robot", html)
        self.assertIn("robot-s3-steps-v3", html)
        self.assertIn("Cerrar aplicación", html)
        self.assertNotIn("Este botón sólo recentra la pose", html)
        for diagnostic in (
            "longitudinal_error_cm", "lateral_error_cm", "distance_error_cm",
            "dynamic_heading_deg", "heading_error_deg", "encoder_pwm",
            "right_compensation", "finish_reason", "route-live-heading",
            "route-live-endpoint", "route-live-recovery", "pivot_avoided",
            "route-axis-default", "Eje único", "route-program-header",
        ):
            with self.subTest(diagnostic=diagnostic):
                self.assertIn(diagnostic, html)

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
        self.assertEqual(first.payload, {
            "heading": 90.0, "cm": 150.0, "drive_mode": "auto",
            "target_x_mm": 1500.0, "target_y_mm": 0.0,
        })
        self.service._on_robot_message({"evt": "completed", "seq": first.seq, "detail": "step_ok"})
        status = self.service.mission_status()
        self.assertEqual(status["current_index"], 1)
        self.assertNotEqual(status["active_seq"], first.seq)

    def test_orthogonal_headings_follow_plus_y_zero_convention(self) -> None:
        self._ready()
        self.service.start_mission([{"x_mm": 0, "y_mm": 1000}, {"x_mm": 1000, "y_mm": 1000}])
        first = self.service.gateway._outgoing.queue[0].command
        self.assertEqual(first.payload["heading"], 0.0)
        self.assertEqual(first.payload["drive_mode"], "auto")
        self.assertEqual((first.payload["target_x_mm"], first.payload["target_y_mm"]), (0.0, 1000.0))
        self.service._on_robot_message({"evt": "completed", "seq": first.seq})
        commands = [item.command for item in self.service.gateway._outgoing.queue]
        self.assertEqual(commands[-1].payload["heading"], 90.0)
        self.assertEqual((commands[-1].payload["target_x_mm"], commands[-1].payload["target_y_mm"]),
                         (1000.0, 1000.0))

    def test_diagonal_route_is_rejected_but_one_mm_tolerance_is_allowed(self) -> None:
        self._ready()
        bad = self.client.post("/api/v1/missions", json={"points": [{"x_mm": 1000, "y_mm": 1000}]},
                               headers={"X-App-Token": self.token})
        self.assertEqual(bad.status_code, 202)
        self.service.stop_mission("test_replace")
        self._ready()
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

    def test_endpoint_not_reached_blocks_current_waypoint(self) -> None:
        self._ready()
        mission = self.service.start_mission([{"x_mm": 1000, "y_mm": 0}])
        self.service._on_robot_message({
            "evt": "fault", "seq": mission["active_seq"], "detail": "endpoint_not_reached",
        })
        status = self.service.mission_status()
        self.assertTrue(status["blocked"])
        self.assertEqual(status["current_index"], 0)
        self.assertEqual(status["error"], "endpoint_not_reached")

    def test_hello_reconciles_completed_step_without_repeating_it(self) -> None:
        self._ready()
        mission = self.service.start_mission([{"x_mm": 1000, "y_mm": 0}])
        self.service._on_robot_message({
            "evt": "hello_ack", "state": "listo", "last_seq": mission["active_seq"],
            "session": self.service._controller_session, "protocol": "robot-s3-steps-v3",
        })
        status = self.service.mission_status()
        self.assertEqual(status["current_index"], 1)
        self.assertFalse(status["running"])

    def test_same_process_reconnect_reuses_python_command_identity(self) -> None:
        self._ready()
        self.service.start_session()
        mission = self.service.start_mission([{"x_mm": 1000, "y_mm": 0}])
        original = self.service.gateway._outgoing.get_nowait().command
        self.service._on_command_sent(original)
        self.service._on_robot_message({
            "evt": "hello_ack", "state": "listo", "last_seq": 0,
            "session": self.service._controller_session, "protocol": "robot-s3-steps-v3",
        })
        retried = self.service.gateway._outgoing.get_nowait().command
        self.assertEqual((retried.command_id, retried.seq), (original.command_id, mission["active_seq"]))
        rows = self.service.database.command_rows(self.service._session_id)
        self.assertEqual([row["id"] for row in rows].count(original.command_id), 1)

    def test_restart_mid_step_is_blocked(self) -> None:
        self._ready()
        self.service.start_mission([{"x_mm": 1000, "y_mm": 0}])
        self.service._on_robot_message({
            "evt": "hello_ack", "state": "desarmado", "last_seq": 0,
            "session": self.service._controller_session, "protocol": "robot-s3-steps-v3",
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

    def test_blocked_mission_is_persisted(self) -> None:
        self._ready()
        mission = self.service.start_mission([{"x_mm": 1000, "y_mm": 0}])
        self.service._on_robot_message({"evt": "fault", "seq": mission["active_seq"], "detail": "stall_FR"})
        saved = self.service.database.get_setting("active_mission")
        self.assertTrue(saved["blocked"])
        self.assertEqual(saved["error"], "stall_FR")
        self.assertEqual(saved["stage"], "blocked")

    def test_completed_route_returns_by_inverse_vectors_and_aligns_zero(self) -> None:
        self._ready()
        mission = self.service.start_mission([{"x_mm": 0, "y_mm": 1000}, {"x_mm": 1000, "y_mm": 1000}])
        self.service._on_robot_message({"evt": "completed", "seq": mission["active_seq"], "detail": "step_ok"})
        self.service._on_robot_message({"evt": "completed", "seq": self.service.mission_status()["active_seq"],
                                        "detail": "step_ok"})
        self.assertEqual(self.service.database.get_setting("last_completed_route")["return_state"], "available")
        self._ready(1000, 1000)
        response = self.client.post("/api/v1/missions/return-home", json={},
                                    headers={"X-App-Token": self.token})
        self.assertEqual(response.status_code, 202)
        self.assertEqual(response.json["planned_points"], [{"x_mm": 0.0, "y_mm": 1000.0},
                                                            {"x_mm": 0.0, "y_mm": 0.0}])
        self.service._on_robot_message({"evt": "completed", "seq": response.json["active_seq"],
                                        "detail": "step_ok"})
        self.service._on_robot_message({"evt": "completed", "seq": self.service.mission_status()["active_seq"],
                                        "detail": "step_ok"})
        aligning = self.service.mission_status()
        self.assertEqual(aligning["stage"], "aligning_final")
        turn = next(item.command for item in self.service.gateway._outgoing.queue
                    if item.command.seq == aligning["active_seq"])
        self.assertEqual((turn.name, turn.payload), ("turn_to", {"heading": 0.0}))
        self.service._on_robot_message({"evt": "completed", "seq": turn.seq, "detail": "turn_ok"})
        self.assertEqual(self.service.mission_status()["stage"], "completed")
        self.assertEqual(self.service.database.get_setting("last_completed_route")["return_state"], "completed")

    def test_corrupt_completed_route_is_rejected_without_consuming_it(self) -> None:
        self._ready()
        route = {"origin": {"x_mm": 0, "y_mm": 0}, "points": [{}], "return_state": "available"}
        self.service.database.set_setting("last_completed_route", route)
        response = self.client.post("/api/v1/missions/return-home", json={},
                                    headers={"X-App-Token": self.token})
        self.assertEqual(response.status_code, 409)
        self.assertEqual(self.service.database.get_setting("last_completed_route")["return_state"], "available")

    def test_close_waits_for_exact_stop_terminal_event(self) -> None:
        self._ready()
        self.service.start_mission([{"x_mm": 1000, "y_mm": 0}])

        def complete_stop() -> None:
            deadline = time.monotonic() + 1.0
            while time.monotonic() < deadline:
                stop = next((item.command for item in list(self.service.gateway._outgoing.queue)
                             if item.command.name == "stop"), None)
                if stop is not None:
                    self.service._on_robot_message({"evt": "completed", "seq": stop.seq, "detail": "stop_ok"})
                    return
                time.sleep(0.005)

        worker = threading.Thread(target=complete_stop)
        worker.start()
        result = self.service.prepare_close(timeout_s=1.0)
        worker.join()
        self.assertTrue(result["safe_to_close"])
        self.assertTrue(result["stop_confirmed"])
        self.assertEqual(self.service.mission_status()["stage"], "abandoned")

    def test_close_timeout_requires_explicit_force(self) -> None:
        self._ready()
        self.service.start_mission([{"x_mm": 1000, "y_mm": 0}])
        blocked = self.service.prepare_close(timeout_s=0)
        self.assertFalse(blocked["safe_to_close"])
        forced = self.client.post("/api/v1/app/close", json={"force": True},
                                  headers={"X-App-Token": self.token})
        self.assertEqual(forced.status_code, 200)
        self.assertTrue(forced.json["forced"])

    def test_process_restart_quarantines_pending_mission_without_requeue(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            database = Database(root / "restart.sqlite3")
            database.initialize()
            session_id = database.create_session()
            database.insert_command("old-command", session_id, "step", {"heading": 0, "cm": 10}, "acknowledged")
            database.set_setting("controller_session", "oldsession000000")
            database.set_setting("active_mission", {
                "id": "old-mission", "revision": 1, "points": [{"x_mm": 0, "y_mm": 100}],
                "current_index": 0, "stage": "executing", "origin": {"x_mm": 0, "y_mm": 0},
                "active_seq": 1, "active_command_id": "old-command", "kind": "outbound",
            })
            app = create_app(AppConfig(root, root / "restart.sqlite3", start_gateway=False))
            service = app.extensions["robot_service"]
            try:
                status = service.mission_status()
                self.assertEqual((status["stage"], status["error"]),
                                 ("abandoned", "app_restarted_with_pending_mission"))
                self.assertTrue(status["blocked"])
                self.assertTrue(service._startup_stop_required)
                self.assertEqual(service.gateway._outgoing.qsize(), 0)
                self.assertEqual(database.command_rows(session_id)[0]["status"], "failed")
                self.assertEqual(database.session_row(session_id)["disconnect_reason"],
                                 "app_restarted_uncleanly")
                self.assertNotEqual(service._controller_session, "oldsession000000")
                service.gateway._state = ConnectionState.CONNECTED
                service.gateway._protocol_v1 = True
                service._on_robot_message({
                    "evt": "hello_ack", "state": "ejecutando", "last_seq": 0,
                    "session": service._controller_session, "protocol": "robot-s3-steps-v3",
                })
                queued = [item.command for item in service.gateway._outgoing.queue]
                self.assertEqual([command.name for command in queued], ["stop"])
                service._on_robot_message({"evt": "completed", "seq": queued[0].seq, "detail": "stop_ok"})
                self.assertFalse(service._startup_stop_required)
                self.assertEqual(service.mission_status()["stage"], "abandoned")
            finally:
                service.close()

    def test_restart_reconciliation_does_not_unlock_on_wrong_stop_detail(self) -> None:
        self.service._startup_stop_required = True
        self.service._reconcile_short_memory_hello({"last_seq": 0, "state": "listo"})
        stop = self.service.gateway._outgoing.queue[0].command
        self.service._on_robot_message({"evt": "completed", "seq": stop.seq, "detail": "turn_ok"})
        self.assertTrue(self.service._startup_stop_required)
        self.assertIsNone(self.service._startup_stop_command_id)
        self.assertFalse(self.service.status()["controls_ready"])

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

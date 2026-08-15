import tempfile
import unittest
import time
from dataclasses import replace
from pathlib import Path

from robot_app.database import Database
from robot_app.domain import TelemetrySnapshot


class DatabaseTests(unittest.TestCase):
    def test_settings_and_session_round_trip(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            database = Database(Path(directory) / "test.sqlite3")
            database.initialize()
            database.set_setting("robot_host", "192.168.4.1")
            self.assertEqual(database.get_setting("robot_host"), "192.168.4.1")
            session_id = database.create_session()
            database.stop_session(session_id)
            self.assertGreater(session_id, 0)

    def test_diagnostic_telemetry_and_identity_are_persisted(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            database = Database(Path(directory) / "test.sqlite3")
            database.initialize()
            session_id = database.create_session()
            snapshot = TelemetrySnapshot(
                sequence=1, received_at=time.time(), uptime_ms=100, state="IDLE",
                x_mm=1.0, y_mm=2.0, yaw_deg=3.0, pulses=(4, 5, 6, 7), pwm=(8, 9),
                wheel_speed_cm_s=(10.0, 11.0), encoder_delta_avg=(1.0, 2.0, 3.0, 4.0),
                gyro_z_filtered_rad_s=0.1, gyro_z_offset_rad_s=0.01, theta_error_deg=0.0,
                mpu_present=True, mpu_stale=False, mpu_calibrated=True, i2c_ok=True,
                robot_id="ESP32S3-001122334455", firmware_version="robot-s3-v1",
                pin_state={"encoder_fl_seen": True}, raw={"type": "telemetry"},
            )
            database.insert_telemetry(session_id, snapshot)
            database.update_session_identity(session_id, snapshot.robot_id, snapshot.firmware_version, "v1")
            row = database.telemetry_rows(session_id)[0]
            session = database.session_row(session_id)
            self.assertEqual((row["pfl"], row["pbr"], row["pwm_l"]), (4, 7, 8))
            self.assertEqual(row["i2c_ok"], 1)
            self.assertIn("encoder_fl_seen", row["pin_state_json"])
            self.assertEqual(session["robot_id"], "ESP32S3-001122334455")

    def test_restart_cleanup_closes_sessions_and_commands(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            database = Database(Path(directory) / "test.sqlite3")
            database.initialize()
            session_id = database.create_session()
            database.insert_command("cmd-1", session_id, "step", {"heading": 0, "cm": 10}, "acknowledged")
            self.assertEqual(database.fail_nonterminal_commands("restart"), 1)
            self.assertEqual(database.close_orphan_sessions("restart"), 1)
            self.assertEqual(database.command_rows(session_id)[0]["status"], "failed")
            self.assertEqual(database.session_row(session_id)["disconnect_reason"], "restart")

    def test_consecutive_telemetry_is_compacted_with_time_and_sequence_range(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            database = Database(Path(directory) / "test.sqlite3")
            database.initialize()
            session_id = database.create_session()
            first = TelemetrySnapshot(
                sequence=1, received_at=time.time(), uptime_ms=100, state="listo",
                x_mm=10.0, y_mm=20.0, yaw_deg=3.0, pulses=(1, 1, 1, 1), pwm=(0, 0),
                wheel_speed_cm_s=(0.0, 0.0), encoder_delta_avg=(0.0, 0.0, 0.0, 0.0),
                gyro_z_filtered_rad_s=0.0, gyro_z_offset_rad_s=0.0, theta_error_deg=0.0,
                mpu_present=True, mpu_stale=False, mpu_calibrated=True, i2c_ok=True,
                robot_id="robot", firmware_version="v1", raw={"evt": "telemetry", "seq": 1, "x": 1},
            )
            second = replace(first, sequence=2, received_at=first.received_at + 1.0,
                             uptime_ms=1100, raw={"evt": "telemetry", "seq": 2, "x": 1})
            database.insert_telemetry(session_id, first)
            database.insert_telemetry(session_id, second)
            rows = database.telemetry_rows(session_id)
            self.assertEqual(len(rows), 1)
            self.assertEqual(rows[0]["repeat_count"], 2)
            self.assertEqual(rows[0]["source_seq_end"], 2)
            self.assertNotEqual(rows[0]["received_at"], rows[0]["last_received_at"])

            database.insert_telemetry(session_id, replace(second, sequence=3, pwm=(180, 180)))
            self.assertEqual(len(database.telemetry_rows(session_id)), 2)

    def test_repeated_nonterminal_events_are_aggregated(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            database = Database(Path(directory) / "test.sqlite3")
            database.initialize()
            session_id = database.create_session()
            database.insert_event(session_id, "connection", "warning", {"detail": "timeout"})
            database.insert_event(session_id, "connection", "warning", {"detail": "timeout"})
            rows = database.event_rows(session_id)
            self.assertEqual(len(rows), 1)
            self.assertEqual(rows[0]["repeat_count"], 2)

            database.insert_event(session_id, "fault", "error", {"reason": "stall"})
            database.insert_event(session_id, "fault", "error", {"reason": "stall"})
            self.assertEqual(len(database.event_rows(session_id)), 3)

    def test_progress_is_saved_at_two_percent_boundaries(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            database = Database(Path(directory) / "test.sqlite3")
            database.initialize()
            session_id = database.create_session()
            base = {"evt": "progress", "seq": 7, "run_id": 2, "detail": "avance"}
            database.insert_event(session_id, "progress", "info", {**base, "pct": 0.10})
            database.insert_event(session_id, "progress", "info", {**base, "pct": 0.115})
            database.insert_event(session_id, "progress", "info", {**base, "pct": 0.121})
            rows = database.event_rows(session_id)
            self.assertEqual(len(rows), 2)
            self.assertEqual(rows[0]["repeat_count"], 2)


if __name__ == "__main__":
    unittest.main()

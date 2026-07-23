import tempfile
import unittest
import time
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


if __name__ == "__main__":
    unittest.main()

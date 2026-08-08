import contextlib
import importlib.util
import io
import tempfile
import time
import unittest
from pathlib import Path

from robot_app.database import Database
from robot_app.domain import TelemetrySnapshot


ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("consultar_db_under_test", ROOT / "consultar_db.py")
assert SPEC and SPEC.loader
CONSULTAR_DB = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CONSULTAR_DB)


def snapshot(sequence: int, x_mm: float, y_mm: float, yaw_deg: float,
             pulses: tuple[int, int, int, int], raw: dict[str, object]) -> TelemetrySnapshot:
    return TelemetrySnapshot(
        sequence=sequence, received_at=time.time(), uptime_ms=sequence * 100, state="ejecutando",
        x_mm=x_mm, y_mm=y_mm, yaw_deg=yaw_deg, pulses=pulses, pwm=(180, 180),
        wheel_speed_cm_s=(10.0, 10.0), encoder_delta_avg=(1.0, 1.0, 1.0, 1.0),
        gyro_z_filtered_rad_s=0.0, gyro_z_offset_rad_s=0.0, theta_error_deg=0.0,
        mpu_present=True, mpu_stale=False, mpu_calibrated=True, i2c_ok=True,
        robot_id="ESP32S3-TEST", firmware_version="test", pin_state={}, raw=raw,
    )


class SegmentReportTests(unittest.TestCase):
    def test_segment_report_includes_positions_encoders_errors_and_result(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            database = Database(Path(directory) / "test.sqlite3")
            database.initialize()
            session_id = database.create_session()
            database.insert_command("segment-1", session_id, "step", {
                "heading": 0, "cm": 100, "target_x_mm": 0, "target_y_mm": 1000,
            }, "sent")
            database.insert_telemetry(session_id, snapshot(
                1, 0.0, 0.0, 0.0, (10, 20, 30, 40),
                {"target": {"absolute": True, "lateral_error_cm": 5.0, "distance_error_cm": 7.0}},
            ))
            database.insert_telemetry(session_id, snapshot(
                2, 100.0, 950.0, 4.0, (110, 120, 130, 140),
                {"target": {"absolute": True, "lateral_error_cm": 5.0, "distance_error_cm": 7.0},
                 "recovery": {"decision": "soft_complete", "distance_cm": 7.0,
                              "direction": "reverse", "pivot_avoided": True}},
            ))
            database.update_command("segment-1", "failed", "endpoint_not_reached")
            output = io.StringIO()
            connection = database.connect()
            try:
                with contextlib.redirect_stdout(output):
                    CONSULTAR_DB.list_segments(connection, 20)
            finally:
                connection.close()
            report = output.getvalue()
            self.assertIn("pos=(0.0,0.0)→(10.0,95.0)cm", report)
            self.assertIn("Δenc FL/FR/BL/BR=100/100/100/100", report)
            self.assertIn("yaw máx=4.0°", report)
            self.assertIn("failed (endpoint_not_reached)", report)
            self.assertIn("lateral=5.0cm, euclidiano=7.0cm", report)
            self.assertIn("endpoint=soft_complete 7.0cm reverse/sin_pivote", report)

    def test_trace_uses_run_id_before_reused_firmware_sequence(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            database = Database(Path(directory) / "test.sqlite3")
            database.initialize()
            session_id = database.create_session()
            database.insert_command("segment-run", session_id, "step", {"heading": 270, "cm": 200}, "sent")
            database.insert_event(session_id, "accepted", "info", {"seq": 1, "run_id": 42})
            for sequence, x_mm in ((1, 0.0), (2, -2000.0)):
                database.insert_telemetry(session_id, snapshot(
                    sequence, x_mm, 0.0, 90.0, (sequence, sequence, sequence, sequence),
                    {"seq": 1, "command_run_id": 42, "phase": "avance",
                     "motion": {"effective_mode": "reverse"},
                     "drive_control": {"heading_error_deg": 1.0}},
                ))
            output = io.StringIO()
            connection = database.connect()
            try:
                with contextlib.redirect_stdout(output):
                    CONSULTAR_DB.trace_command(connection, "segment-run")
            finally:
                connection.close()
            report = output.getvalue()
            self.assertIn("run 42", report)
            self.assertNotIn("base histórica", report)
            self.assertIn("efectivo=reverse", report)


if __name__ == "__main__":
    unittest.main()

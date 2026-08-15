import sqlite3
import tempfile
import unittest
from contextlib import closing
from pathlib import Path

from robot_app.compaction import compact_database
from robot_app.database import Database


class HistoricalCompactionTests(unittest.TestCase):
    def test_dry_run_does_not_modify_and_apply_creates_backup(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "robot.sqlite3"
            database = Database(path)
            database.initialize()
            session_id = database.create_session()
            with database.transaction() as connection:
                for seq in (1, 2, 3):
                    connection.execute(
                        "INSERT INTO telemetry(session_id,seq,received_at,robot_uptime_ms,state,x_mm,y_mm,"
                        "yaw_deg,pfl,pfr,pbl,pbr,pwm_l,pwm_r,mpu_present,mpu_stale,i2c_ok,pin_state_json,"
                        "payload_json) VALUES(?,?,CURRENT_TIMESTAMP,?,'listo',0,0,0,0,0,0,0,0,0,1,0,1,'{}',?)",
                        (session_id, seq, seq * 100, f'{{"evt":"telemetry","seq":{seq}}}'),
                    )
                for _ in range(3):
                    connection.execute(
                        "INSERT INTO events(session_id,kind,severity,payload_json,created_at) "
                        "VALUES(?,'connection','warning','{\"detail\":\"timeout\"}',CURRENT_TIMESTAMP)",
                        (session_id,),
                    )

            dry_run = compact_database(path)
            self.assertEqual(dry_run["telemetry_removed"], 2)
            self.assertEqual(dry_run["events_removed"], 2)
            with closing(sqlite3.connect(path)) as connection:
                self.assertEqual(connection.execute("SELECT COUNT(*) FROM telemetry").fetchone()[0], 3)

            applied = compact_database(path, apply=True)
            self.assertEqual(applied["telemetry_removed"], 2)
            self.assertEqual(applied["events_removed"], 2)
            self.assertTrue(Path(applied["backup"]).exists())
            with closing(sqlite3.connect(path)) as connection:
                row = connection.execute(
                    "SELECT COUNT(*),repeat_count,source_seq_end FROM telemetry"
                ).fetchone()
            self.assertEqual(row, (1, 3, 3))
            with closing(sqlite3.connect(path)) as connection:
                event_row = connection.execute("SELECT COUNT(*),repeat_count FROM events").fetchone()
            self.assertEqual(event_row, (1, 3))


if __name__ == "__main__":
    unittest.main()

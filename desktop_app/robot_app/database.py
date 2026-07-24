"""Persistencia SQLite con transacciones explícitas y migraciones."""

from __future__ import annotations

import contextlib
import json
import sqlite3
from pathlib import Path
from typing import Any, Iterator


class Database:
    def __init__(self, path: Path) -> None:
        self.path = Path(path)
        self.path.parent.mkdir(parents=True, exist_ok=True)

    def connect(self) -> sqlite3.Connection:
        connection = sqlite3.connect(self.path, timeout=5.0, isolation_level=None)
        connection.row_factory = sqlite3.Row
        connection.execute("PRAGMA foreign_keys=ON")
        connection.execute("PRAGMA busy_timeout=5000")
        connection.execute("PRAGMA journal_mode=WAL")
        connection.execute("PRAGMA synchronous=FULL")
        return connection

    @contextlib.contextmanager
    def transaction(self) -> Iterator[sqlite3.Connection]:
        connection = self.connect()
        try:
            connection.execute("BEGIN IMMEDIATE")
            yield connection
            connection.execute("COMMIT")
        except BaseException:
            if connection.in_transaction:
                connection.execute("ROLLBACK")
            raise
        finally:
            connection.close()

    def initialize(self) -> None:
        migration = Path(__file__).resolve().parents[1] / "migrations" / "001_initial.sql"
        sql = migration.read_text(encoding="utf-8")
        connection = self.connect()
        try:
            connection.executescript(sql)
            additions = {
                "sessions": {
                    "protocol": "TEXT", "disconnect_reason": "TEXT"
                },
                "telemetry": {
                    "pfl": "INTEGER", "pfr": "INTEGER", "pbl": "INTEGER", "pbr": "INTEGER",
                    "pwm_l": "INTEGER", "pwm_r": "INTEGER", "mpu_present": "INTEGER",
                    "mpu_stale": "INTEGER", "i2c_ok": "INTEGER", "pin_state_json": "TEXT"
                },
            }
            for table, columns in additions.items():
                existing = {row[1] for row in connection.execute(f"PRAGMA table_info({table})")}
                for name, declaration in columns.items():
                    if name not in existing:
                        connection.execute(f"ALTER TABLE {table} ADD COLUMN {name} {declaration}")
            connection.execute("PRAGMA user_version=2")
        finally:
            connection.close()

    def get_setting(self, key: str, default: Any = None) -> Any:
        with contextlib.closing(self.connect()) as connection:
            row = connection.execute("SELECT value_json FROM settings WHERE key=?", (key,)).fetchone()
        return default if row is None else json.loads(row["value_json"])

    def set_setting(self, key: str, value: Any) -> None:
        encoded = json.dumps(value, ensure_ascii=False, separators=(",", ":"))
        with self.transaction() as connection:
            connection.execute(
                "INSERT INTO settings(key,value_json,updated_at) VALUES(?,?,CURRENT_TIMESTAMP) "
                "ON CONFLICT(key) DO UPDATE SET value_json=excluded.value_json,updated_at=CURRENT_TIMESTAMP",
                (key, encoded),
            )

    def create_session(self) -> int:
        with self.transaction() as connection:
            cursor = connection.execute("INSERT INTO sessions(started_at) VALUES(CURRENT_TIMESTAMP)")
            return int(cursor.lastrowid)

    def update_session_identity(self, session_id: int, robot_id: str | None, firmware_version: str | None, protocol: str | None) -> None:
        with self.transaction() as connection:
            connection.execute(
                "UPDATE sessions SET robot_id=COALESCE(?,robot_id),firmware_version=COALESCE(?,firmware_version),protocol=COALESCE(?,protocol) WHERE id=?",
                (robot_id, firmware_version, protocol, session_id),
            )

    def stop_session(self, session_id: int, reason: str | None = None) -> None:
        with self.transaction() as connection:
            connection.execute(
                "UPDATE sessions SET ended_at=CURRENT_TIMESTAMP,disconnect_reason=COALESCE(?,disconnect_reason) WHERE id=? AND ended_at IS NULL",
                (reason, session_id),
            )

    def insert_command(self, command_id: str, session_id: int | None, name: str, payload: dict[str, Any], status: str) -> None:
        with self.transaction() as connection:
            connection.execute(
                "INSERT INTO commands(id,session_id,command_type,payload_json,status,created_at) VALUES(?,?,?,?,?,CURRENT_TIMESTAMP)",
                (command_id, session_id, name, json.dumps(payload, separators=(",", ":")), status),
            )

    def update_command(self, command_id: str, status: str, error: str | None = None) -> None:
        timestamp_column = (
            "completed_at" if status in {"completed", "rejected", "failed"}
            else "sent_at" if status == "sent"
            else "ack_at"
        )
        with self.transaction() as connection:
            connection.execute(
                f"UPDATE commands SET status=?,error=?,{timestamp_column}=CURRENT_TIMESTAMP WHERE id=?",
                (status, error, command_id),
            )

    def insert_event(self, session_id: int | None, kind: str, severity: str, payload: dict[str, Any]) -> None:
        with self.transaction() as connection:
            connection.execute(
                "INSERT INTO events(session_id,kind,severity,payload_json,created_at) VALUES(?,?,?,?,CURRENT_TIMESTAMP)",
                (session_id, kind, severity, json.dumps(payload, ensure_ascii=False, separators=(",", ":"))),
            )

    def insert_telemetry(self, session_id: int, snapshot: Any) -> None:
        with self.transaction() as connection:
            connection.execute(
                "INSERT OR IGNORE INTO telemetry(session_id,seq,received_at,robot_uptime_ms,state,x_mm,y_mm,yaw_deg,pfl,pfr,pbl,pbr,pwm_l,pwm_r,mpu_present,mpu_stale,i2c_ok,pin_state_json,payload_json) "
                "VALUES (?, ?, datetime(?,'unixepoch'), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
                (session_id, snapshot.sequence, snapshot.received_at, snapshot.uptime_ms, snapshot.state,
                 snapshot.x_mm, snapshot.y_mm, snapshot.yaw_deg, *snapshot.pulses, *snapshot.pwm,
                 snapshot.mpu_present, snapshot.mpu_stale, snapshot.i2c_ok,
                 json.dumps(snapshot.pin_state, ensure_ascii=False, separators=(",", ":")),
                 json.dumps(snapshot.raw, ensure_ascii=False, separators=(",", ":"))),
            )

    def telemetry_rows(self, session_id: int) -> list[sqlite3.Row]:
        with contextlib.closing(self.connect()) as connection:
            return list(connection.execute(
                "SELECT seq,received_at,robot_uptime_ms,state,x_mm,y_mm,yaw_deg,pfl,pfr,pbl,pbr,pwm_l,pwm_r,mpu_present,mpu_stale,i2c_ok,pin_state_json,payload_json FROM telemetry WHERE session_id=? ORDER BY seq",
                (session_id,),
            ))

    def session_rows(self) -> list[sqlite3.Row]:
        with contextlib.closing(self.connect()) as connection:
            return list(connection.execute(
                "SELECT s.*," 
                "(SELECT COUNT(*) FROM telemetry t WHERE t.session_id=s.id) AS samples," 
                "(SELECT COUNT(*) FROM commands c WHERE c.session_id=s.id) AS commands," 
                "(SELECT COUNT(*) FROM events e WHERE e.session_id=s.id) AS events "
                "FROM sessions s ORDER BY s.id DESC"
            ))

    def session_row(self, session_id: int) -> sqlite3.Row | None:
        with contextlib.closing(self.connect()) as connection:
            return connection.execute("SELECT * FROM sessions WHERE id=?", (session_id,)).fetchone()

    def command_rows(self, session_id: int) -> list[sqlite3.Row]:
        with contextlib.closing(self.connect()) as connection:
            return list(connection.execute(
                "SELECT id,command_type,payload_json,status,created_at,sent_at,ack_at,completed_at,error "
                "FROM commands WHERE session_id=? ORDER BY created_at", (session_id,)
            ))

    def event_rows(self, session_id: int) -> list[sqlite3.Row]:
        with contextlib.closing(self.connect()) as connection:
            return list(connection.execute(
                "SELECT id,kind,severity,payload_json,created_at FROM events WHERE session_id=? ORDER BY id",
                (session_id,),
            ))

    def purge_sessions(self, days: int) -> int:
        with self.transaction() as connection:
            if days <= 0:
                target_ids = [row["id"] for row in connection.execute("SELECT id FROM sessions").fetchall()]
            else:
                target_ids = [
                    row["id"]
                    for row in connection.execute(
                        "SELECT id FROM sessions WHERE started_at < datetime('now', '-' || ? || ' days')",
                        (days,),
                    ).fetchall()
                ]
            if not target_ids:
                return 0
            placeholders = ",".join("?" * len(target_ids))
            connection.execute(f"DELETE FROM telemetry WHERE session_id IN ({placeholders})", target_ids)
            connection.execute(f"DELETE FROM commands WHERE session_id IN ({placeholders})", target_ids)
            connection.execute(f"DELETE FROM events WHERE session_id IN ({placeholders})", target_ids)
            cursor = connection.execute(f"DELETE FROM sessions WHERE id IN ({placeholders})", target_ids)
            return cursor.rowcount


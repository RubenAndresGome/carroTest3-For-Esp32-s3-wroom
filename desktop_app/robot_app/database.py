"""Persistencia SQLite con transacciones explícitas y migraciones."""

from __future__ import annotations

import contextlib
import hashlib
import json
import sqlite3
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Callable, Iterator


class Database:
    def __init__(self, path: Path, clock: Callable[[], datetime] | None = None) -> None:
        self.path = Path(path)
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self._clock = clock or (lambda: datetime.now().astimezone())

    def _session_timestamp(self) -> tuple[str, str, int, str]:
        """Captura UTC y calendario local a partir del mismo instante."""
        local_now = self._clock()
        if local_now.tzinfo is None or local_now.utcoffset() is None:
            local_now = local_now.astimezone()
        offset = local_now.utcoffset()
        offset_minutes = int(offset.total_seconds() // 60) if offset is not None else 0
        timezone_name = getattr(local_now.tzinfo, "key", None) or local_now.tzname() or "local"
        utc_text = local_now.astimezone(timezone.utc).strftime("%Y-%m-%d %H:%M:%S")
        return utc_text, local_now.date().isoformat(), offset_minutes, timezone_name

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
                    "protocol": "TEXT", "disconnect_reason": "TEXT",
                    "started_local_day": "TEXT", "ended_local_day": "TEXT",
                    "started_utc_offset_min": "INTEGER", "ended_utc_offset_min": "INTEGER",
                    "timezone_name": "TEXT",
                },
                "telemetry": {
                    "pfl": "INTEGER", "pfr": "INTEGER", "pbl": "INTEGER", "pbr": "INTEGER",
                    "pwm_l": "INTEGER", "pwm_r": "INTEGER", "mpu_present": "INTEGER",
                    "mpu_stale": "INTEGER", "i2c_ok": "INTEGER", "pin_state_json": "TEXT",
                    "last_received_at": "TEXT", "source_seq_end": "INTEGER",
                    "repeat_count": "INTEGER NOT NULL DEFAULT 1", "fingerprint": "TEXT"
                },
                "events": {
                    "last_seen_at": "TEXT", "repeat_count": "INTEGER NOT NULL DEFAULT 1",
                    "dedupe_key": "TEXT"
                },
            }
            for table, columns in additions.items():
                existing = {row[1] for row in connection.execute(f"PRAGMA table_info({table})")}
                for name, declaration in columns.items():
                    if name not in existing:
                        connection.execute(f"ALTER TABLE {table} ADD COLUMN {name} {declaration}")
            connection.execute(
                "CREATE INDEX IF NOT EXISTS idx_telemetry_session_last "
                "ON telemetry(session_id, source_seq_end)"
            )
            connection.execute("PRAGMA user_version=5")
            connection.execute("""CREATE TABLE IF NOT EXISTS touch_recordings (
                id TEXT PRIMARY KEY, created_at TEXT NOT NULL, updated_at TEXT NOT NULL,
                status TEXT NOT NULL, origin_json TEXT NOT NULL, final_json TEXT,
                yaw_initial REAL, yaw_final REAL, samples_json TEXT NOT NULL,
                points_json TEXT, logical_steps_json TEXT, compiled_segments_json TEXT,
                simplification_tolerance_mm REAL, invalid_reason TEXT,
                open_area_ack INTEGER NOT NULL DEFAULT 0
            )""")
            touch_columns = {row[1] for row in connection.execute("PRAGMA table_info(touch_recordings)")}
            for column, declaration in (
                ("logical_steps_json", "TEXT"),
                ("compiled_segments_json", "TEXT"),
                ("simplification_tolerance_mm", "REAL"),
            ):
                if column not in touch_columns:
                    connection.execute(f"ALTER TABLE touch_recordings ADD COLUMN {column} {declaration}")
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
        utc_text, local_day, offset_minutes, timezone_name = self._session_timestamp()
        with self.transaction() as connection:
            cursor = connection.execute(
                "INSERT INTO sessions(started_at,started_local_day,started_utc_offset_min,timezone_name) "
                "VALUES(?,?,?,?)",
                (utc_text, local_day, offset_minutes, timezone_name),
            )
            return int(cursor.lastrowid)

    def update_session_identity(self, session_id: int, robot_id: str | None, firmware_version: str | None, protocol: str | None) -> None:
        with self.transaction() as connection:
            connection.execute(
                "UPDATE sessions SET robot_id=COALESCE(?,robot_id),firmware_version=COALESCE(?,firmware_version),protocol=COALESCE(?,protocol) WHERE id=?",
                (robot_id, firmware_version, protocol, session_id),
            )

    def stop_session(self, session_id: int, reason: str | None = None) -> None:
        utc_text, local_day, offset_minutes, timezone_name = self._session_timestamp()
        with self.transaction() as connection:
            connection.execute(
                "UPDATE sessions SET ended_at=?,ended_local_day=?,ended_utc_offset_min=?,"
                "timezone_name=COALESCE(timezone_name,?),disconnect_reason=COALESCE(?,disconnect_reason) "
                "WHERE id=? AND ended_at IS NULL",
                (utc_text, local_day, offset_minutes, timezone_name, reason, session_id),
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

    def fail_nonterminal_commands(self, reason: str) -> int:
        """Finaliza comandos que no pueden sobrevivir al proceso propietario."""
        with self.transaction() as connection:
            cursor = connection.execute(
                "UPDATE commands SET status='failed',error=?,completed_at=CURRENT_TIMESTAMP "
                "WHERE status IN ('queued','sent','acknowledged')",
                (reason,),
            )
            return int(cursor.rowcount)

    def close_orphan_sessions(self, reason: str) -> int:
        utc_text, local_day, offset_minutes, timezone_name = self._session_timestamp()
        with self.transaction() as connection:
            cursor = connection.execute(
                "UPDATE sessions SET ended_at=?,ended_local_day=?,ended_utc_offset_min=?,"
                "timezone_name=COALESCE(timezone_name,?),disconnect_reason=? WHERE ended_at IS NULL",
                (utc_text, local_day, offset_minutes, timezone_name, reason),
            )
            return int(cursor.rowcount)

    def insert_event(self, session_id: int | None, kind: str, severity: str, payload: dict[str, Any]) -> None:
        encoded = json.dumps(payload, ensure_ascii=False, separators=(",", ":"), sort_keys=True)
        dedupe_key = hashlib.sha256(f"{kind}\0{severity}\0{encoded}".encode("utf-8")).hexdigest()[:24]
        terminal_kinds = {"accepted", "completed", "already_done", "rejected", "fault", "estop"}
        with self.transaction() as connection:
            if severity in {"info", "warning"} and kind not in terminal_kinds:
                last = connection.execute(
                    "SELECT id,dedupe_key,payload_json FROM events WHERE session_id IS ? ORDER BY id DESC LIMIT 1",
                    (session_id,),
                ).fetchone()
                if kind == "progress" and last is not None:
                    try:
                        previous = json.loads(last["payload_json"])
                        same_progress = all(previous.get(key) == payload.get(key)
                                            for key in ("seq", "run_id", "detail"))
                        current_pct = float(payload.get("pct"))
                        previous_pct = float(previous.get("pct"))
                        threshold = 2.0 if max(abs(current_pct), abs(previous_pct)) > 1.0 else 0.02
                    except (TypeError, ValueError, json.JSONDecodeError):
                        same_progress = False
                        current_pct = previous_pct = threshold = 0.0
                    if same_progress and abs(current_pct - previous_pct) < threshold:
                        connection.execute(
                            "UPDATE events SET repeat_count=COALESCE(repeat_count,1)+1,"
                            "last_seen_at=CURRENT_TIMESTAMP WHERE id=?",
                            (last["id"],),
                        )
                        return
                if last is not None and (
                    last["dedupe_key"] == dedupe_key
                    or (last["dedupe_key"] is None and last["payload_json"] == encoded)
                ):
                    connection.execute(
                        "UPDATE events SET repeat_count=COALESCE(repeat_count,1)+1,"
                        "last_seen_at=CURRENT_TIMESTAMP,dedupe_key=? WHERE id=?",
                        (dedupe_key, last["id"]),
                    )
                    return
            connection.execute(
                "INSERT INTO events(session_id,kind,severity,payload_json,created_at,last_seen_at,repeat_count,dedupe_key) "
                "VALUES(?,?,?,?,CURRENT_TIMESTAMP,CURRENT_TIMESTAMP,1,?)",
                (session_id, kind, severity, encoded, dedupe_key),
            )

    @staticmethod
    def _quantize(value: Any, quantum: float) -> float | None:
        try:
            return round(float(value) / quantum) * quantum
        except (TypeError, ValueError):
            return None

    @classmethod
    def telemetry_priority_signature(cls, snapshot: Any) -> str:
        """Firma de cambios que deben persistirse inmediatamente."""
        data = {
            "state": str(snapshot.state).lower(),
            "pwm": tuple(int(value) for value in snapshot.pwm),
            "command": (snapshot.active_command_id, snapshot.active_command_name),
            "phases": (snapshot.move_phase, snapshot.alignment_stage, snapshot.arc_phase,
                       snapshot.turn_braking_phase),
            "encoder_health": tuple(snapshot.encoder_health),
            "fusion": snapshot.encoder_fusion,
            "recovery": (snapshot.autonomous_recovery_reason, snapshot.recovery,
                         snapshot.anti_friction, tuple(snapshot.stall_accumulated_ms)),
            "terminal": snapshot.last_terminal,
            "manual": snapshot.manual_phase,
            "pcnt_init": snapshot.pcnt_init,
            "torque_history": snapshot.torque_history,
        }
        encoded = json.dumps(data, sort_keys=True, default=str, separators=(",", ":"))
        return hashlib.sha256(encoded.encode("utf-8")).hexdigest()

    @classmethod
    def telemetry_fingerprint(cls, snapshot: Any) -> str:
        """Firma diagnóstica; ignora reloj, secuencia y ruido submilimétrico."""
        data = {
            "priority": cls.telemetry_priority_signature(snapshot),
            "pose": (cls._quantize(snapshot.x_mm, 5.0), cls._quantize(snapshot.y_mm, 5.0),
                     cls._quantize(snapshot.yaw_deg, 0.5)),
            "encoder_delta": tuple(cls._quantize(value, 0.5) for value in snapshot.encoder_delta_avg),
            "speed": tuple(cls._quantize(value, 0.5) for value in snapshot.wheel_speed_cm_s),
            "imu": (snapshot.mpu_present, snapshot.mpu_stale, snapshot.i2c_ok),
            "target": snapshot.target,
            "control": (snapshot.turn_attempt, snapshot.drive_attempt, snapshot.turn_pwm_target_8bit,
                        cls._quantize(snapshot.command_progress, 1.0)),
        }
        encoded = json.dumps(data, sort_keys=True, default=str, separators=(",", ":"))
        return hashlib.sha256(encoded.encode("utf-8")).hexdigest()

    @staticmethod
    def _compact_payload(snapshot: Any) -> dict[str, Any]:
        """Quita del JSON campos ya normalizados en columnas SQL."""
        payload = dict(snapshot.raw)
        nested = payload.get("payload") if payload.get("type") == "telemetry" else payload
        if isinstance(nested, dict):
            nested = dict(nested)
            for key in (
                "state", "x", "x_mm", "y", "y_mm", "yaw", "yaw_deg", "enc", "pwm",
                "uptime_ms", "robot_uptime_ms", "mpu_present", "mpu_stale", "i2c_ok", "pin_state",
            ):
                nested.pop(key, None)
            if payload.get("type") == "telemetry":
                payload["payload"] = nested
            else:
                payload = nested
        return payload

    def insert_telemetry(self, session_id: int, snapshot: Any) -> None:
        fingerprint = self.telemetry_fingerprint(snapshot)
        compact_payload = json.dumps(
            self._compact_payload(snapshot), ensure_ascii=False, separators=(",", ":")
        )
        with self.transaction() as connection:
            last = connection.execute(
                "SELECT id,fingerprint FROM telemetry WHERE session_id=? ORDER BY seq DESC LIMIT 1",
                (session_id,),
            ).fetchone()
            if last is not None and last["fingerprint"] == fingerprint:
                connection.execute(
                    "UPDATE telemetry SET last_received_at=datetime(?,'unixepoch'),source_seq_end=?,"
                    "repeat_count=COALESCE(repeat_count,1)+1,robot_uptime_ms=?,payload_json=? WHERE id=?",
                    (snapshot.received_at, snapshot.sequence, snapshot.uptime_ms, compact_payload, last["id"]),
                )
                return
            connection.execute(
                "INSERT OR IGNORE INTO telemetry("
                "session_id,seq,received_at,last_received_at,source_seq_end,repeat_count,fingerprint,"
                "robot_uptime_ms,state,x_mm,y_mm,yaw_deg,pfl,pfr,pbl,pbr,pwm_l,pwm_r,"
                "mpu_present,mpu_stale,i2c_ok,pin_state_json,payload_json) "
                "VALUES(?,?,datetime(?,'unixepoch'),datetime(?,'unixepoch'),?,1,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
                (session_id, snapshot.sequence, snapshot.received_at, snapshot.received_at,
                 snapshot.sequence, fingerprint, snapshot.uptime_ms, snapshot.state,
                 snapshot.x_mm, snapshot.y_mm, snapshot.yaw_deg, *snapshot.pulses, *snapshot.pwm,
                 snapshot.mpu_present, snapshot.mpu_stale, snapshot.i2c_ok,
                 json.dumps(snapshot.pin_state, ensure_ascii=False, separators=(",", ":")),
                 compact_payload),
            )

    def telemetry_rows(self, session_id: int) -> list[sqlite3.Row]:
        with contextlib.closing(self.connect()) as connection:
            return list(connection.execute(
                "SELECT seq,COALESCE(source_seq_end,seq) AS source_seq_end,received_at,"
                "COALESCE(last_received_at,received_at) AS last_received_at,"
                "COALESCE(repeat_count,1) AS repeat_count,robot_uptime_ms,state,x_mm,y_mm,yaw_deg,"
                "pfl,pfr,pbl,pbr,pwm_l,pwm_r,mpu_present,mpu_stale,i2c_ok,pin_state_json,payload_json "
                "FROM telemetry WHERE session_id=? ORDER BY seq",
                (session_id,),
            ))

    def session_rows(self) -> list[sqlite3.Row]:
        with contextlib.closing(self.connect()) as connection:
            return list(connection.execute(
                "SELECT s.*," 
                "(SELECT COALESCE(SUM(COALESCE(t.repeat_count,1)),0) FROM telemetry t "
                "WHERE t.session_id=s.id) AS samples,"
                "(SELECT COUNT(*) FROM telemetry t WHERE t.session_id=s.id) AS stored_samples,"
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
                "SELECT id,kind,severity,payload_json,created_at,"
                "COALESCE(last_seen_at,created_at) AS last_seen_at,"
                "COALESCE(repeat_count,1) AS repeat_count FROM events WHERE session_id=? ORDER BY id",
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

    def save_touch_recording(self, recording: dict[str, Any]) -> None:
        with self.transaction() as connection:
            connection.execute(
                """INSERT INTO touch_recordings
                (id,created_at,updated_at,status,origin_json,final_json,yaw_initial,yaw_final,
                 samples_json,points_json,logical_steps_json,compiled_segments_json,
                 simplification_tolerance_mm,invalid_reason,open_area_ack)
                VALUES(?,CURRENT_TIMESTAMP,CURRENT_TIMESTAMP,?,?,?,?,?,?,?,?,?,?,?,?)
                ON CONFLICT(id) DO UPDATE SET
                  updated_at=CURRENT_TIMESTAMP,status=excluded.status,origin_json=excluded.origin_json,
                  final_json=excluded.final_json,yaw_initial=excluded.yaw_initial,yaw_final=excluded.yaw_final,
                  samples_json=excluded.samples_json,points_json=excluded.points_json,
                  logical_steps_json=excluded.logical_steps_json,
                  compiled_segments_json=excluded.compiled_segments_json,
                  simplification_tolerance_mm=excluded.simplification_tolerance_mm,
                  invalid_reason=excluded.invalid_reason,open_area_ack=excluded.open_area_ack""",
                (recording["id"], recording["status"], json.dumps(recording["origin"], separators=(",", ":")),
                 json.dumps(recording.get("final"), separators=(",", ":")) if recording.get("final") is not None else None,
                 recording.get("yaw_initial"), recording.get("yaw_final"),
                 json.dumps(recording.get("samples", []), separators=(",", ":")),
                 json.dumps(recording.get("points"), separators=(",", ":")) if recording.get("points") is not None else None,
                 json.dumps(recording.get("logical_steps"), separators=(",", ":")) if recording.get("logical_steps") is not None else None,
                 json.dumps(recording.get("compiled_segments"), separators=(",", ":")) if recording.get("compiled_segments") is not None else None,
                 recording.get("simplification_tolerance_mm"), recording.get("invalid_reason"),
                 int(bool(recording.get("open_area_ack"))))
            )

    def touch_recordings(self) -> list[sqlite3.Row]:
        with contextlib.closing(self.connect()) as connection:
            return list(connection.execute("SELECT * FROM touch_recordings ORDER BY created_at DESC"))

    def touch_recording(self, recording_id: str) -> sqlite3.Row | None:
        with contextlib.closing(self.connect()) as connection:
            return connection.execute("SELECT * FROM touch_recordings WHERE id=?", (recording_id,)).fetchone()

    def delete_touch_recording(self, recording_id: str) -> bool:
        with self.transaction() as connection:
            cursor = connection.execute("DELETE FROM touch_recordings WHERE id=?", (recording_id,))
            return cursor.rowcount == 1

    def acknowledge_touch_recording(self, recording_id: str) -> bool:
        with self.transaction() as connection:
            cursor = connection.execute("UPDATE touch_recordings SET open_area_ack=1,updated_at=CURRENT_TIMESTAMP WHERE id=?", (recording_id,))
            return cursor.rowcount == 1

    def invalidate_open_touch_recordings(self, reason: str) -> int:
        with self.transaction() as connection:
            cursor = connection.execute(
                """UPDATE touch_recordings SET status='invalid', invalid_reason=?,
                   updated_at=CURRENT_TIMESTAMP WHERE status='recording'""",
                (reason[:160],),
            )
            return cursor.rowcount

    def optimize_storage(self, full: bool = False) -> dict[str, int]:
        """Recupera páginas libres; el VACUUM completo debe ejecutarse sin sesión activa."""
        before = self.path.stat().st_size if self.path.exists() else 0
        connection = self.connect()
        try:
            connection.execute("PRAGMA wal_checkpoint(TRUNCATE)")
            connection.execute("PRAGMA optimize")
            if full:
                connection.execute("PRAGMA auto_vacuum=INCREMENTAL")
                connection.execute("VACUUM")
            else:
                connection.execute("PRAGMA incremental_vacuum(2048)")
        finally:
            connection.close()
        after = self.path.stat().st_size if self.path.exists() else 0
        return {
            "before_bytes": before,
            "after_bytes": after,
            "reclaimed_bytes": max(0, before - after),
        }

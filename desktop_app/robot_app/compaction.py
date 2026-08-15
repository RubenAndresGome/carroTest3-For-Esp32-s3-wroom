"""Compactación conservadora de bases históricas de telemetría."""

from __future__ import annotations

import hashlib
import json
import sqlite3
from datetime import datetime
from pathlib import Path
from typing import Any

from .database import Database


def _quantize(value: Any, quantum: float) -> float | None:
    try:
        return round(float(value) / quantum) * quantum
    except (TypeError, ValueError):
        return None


def _normalized_payload(raw: str) -> Any:
    try:
        payload = json.loads(raw or "{}")
    except (TypeError, json.JSONDecodeError):
        return raw

    volatile = {
        "seq", "received_at", "timestamp", "uptime", "uptime_ms", "robot_uptime_ms",
        "state", "x", "x_mm", "y", "y_mm", "yaw", "yaw_deg", "enc", "pwm",
    }

    def clean(value: Any) -> Any:
        if isinstance(value, dict):
            return {key: clean(item) for key, item in sorted(value.items()) if key not in volatile}
        if isinstance(value, list):
            return [clean(item) for item in value]
        if isinstance(value, float):
            return round(value, 3)
        return value

    return clean(payload)


def _telemetry_key(row: sqlite3.Row) -> str:
    data = {
        "state": row["state"],
        "pose": (_quantize(row["x_mm"], 5.0), _quantize(row["y_mm"], 5.0),
                 _quantize(row["yaw_deg"], 0.5)),
        "pulses": tuple(row[name] for name in ("pfl", "pfr", "pbl", "pbr")),
        "pwm": (row["pwm_l"], row["pwm_r"]),
        "imu": (row["mpu_present"], row["mpu_stale"], row["i2c_ok"]),
        "pins": _normalized_payload(row["pin_state_json"] or "{}"),
        "diagnostics": _normalized_payload(row["payload_json"]),
    }
    encoded = json.dumps(data, sort_keys=True, default=str, separators=(",", ":"))
    return hashlib.sha256(encoded.encode("utf-8")).hexdigest()


def compact_database(path: Path, apply: bool = False, vacuum: bool = False) -> dict[str, Any]:
    """Analiza o compacta repeticiones consecutivas sin cruzar sesiones."""
    path = Path(path)
    if not path.exists():
        raise FileNotFoundError(path)

    before_bytes = path.stat().st_size
    backup: Path | None = None
    if apply:
        suffix = datetime.now().strftime("%Y%m%d_%H%M%S")
        backup = path.with_suffix(path.suffix + f".backup_{suffix}")
        source = sqlite3.connect(path, timeout=30.0)
        destination = sqlite3.connect(backup)
        try:
            source.backup(destination)
        finally:
            destination.close()
            source.close()
        Database(path).initialize()

    connection = sqlite3.connect(path, timeout=30.0)
    connection.row_factory = sqlite3.Row
    columns = {row[1] for row in connection.execute("PRAGMA table_info(telemetry)")}
    optional = {
        name: name if name in columns else f"NULL AS {name}"
        for name in ("pfl", "pfr", "pbl", "pbr", "pwm_l", "pwm_r", "mpu_present",
                     "mpu_stale", "i2c_ok", "pin_state_json", "repeat_count", "source_seq_end",
                     "last_received_at")
    }
    select = (
        "SELECT id,session_id,seq,received_at,robot_uptime_ms,state,x_mm,y_mm,yaw_deg,payload_json,"
        + ",".join(optional.values()) + " FROM telemetry ORDER BY session_id,seq"
    )
    rows = connection.execute(select).fetchall()
    removable: list[int] = []
    updates: list[tuple[str, int, int, int, str, int]] = []
    keeper: sqlite3.Row | None = None
    keeper_key: str | None = None
    merged_count = 1

    for row in rows:
        key = _telemetry_key(row)
        if keeper is not None and row["session_id"] == keeper["session_id"] and key == keeper_key:
            removable.append(row["id"])
            merged_count += int(row["repeat_count"] or 1)
            end_time = row["last_received_at"] or row["received_at"]
            end_seq = int(row["source_seq_end"] or row["seq"])
            updates[-1] = (end_time, end_seq, merged_count, keeper["id"], key, row["id"])
            continue
        keeper = row
        keeper_key = key
        merged_count = int(row["repeat_count"] or 1)
        end_time = row["last_received_at"] or row["received_at"]
        end_seq = int(row["source_seq_end"] or row["seq"])
        updates.append((end_time, end_seq, merged_count, row["id"], key, row["id"]))

    event_columns = {row[1] for row in connection.execute("PRAGMA table_info(events)")}
    event_repeat = "repeat_count" if "repeat_count" in event_columns else "NULL AS repeat_count"
    events = connection.execute(
        f"SELECT id,session_id,kind,severity,payload_json,created_at,{event_repeat} "
        "FROM events ORDER BY id"
    ).fetchall()
    terminal_kinds = {"accepted", "completed", "already_done", "rejected", "fault", "estop"}
    event_removable: list[int] = []
    event_updates: list[tuple[int, str, int]] = []
    event_keeper: sqlite3.Row | None = None
    event_key: tuple[Any, ...] | None = None
    event_count = 1
    for event in events:
        normalized = json.dumps(_normalized_payload(event["payload_json"]), sort_keys=True,
                                default=str, separators=(",", ":"))
        key = (event["session_id"], event["kind"], event["severity"], normalized)
        aggregatable = event["severity"] in {"info", "warning"} and event["kind"] not in terminal_kinds
        if aggregatable and event_keeper is not None and key == event_key:
            event_removable.append(event["id"])
            event_count += int(event["repeat_count"] or 1)
            event_updates[-1] = (event_count, event["created_at"], event_keeper["id"])
            continue
        event_keeper = event
        event_key = key
        event_count = int(event["repeat_count"] or 1)
        event_updates.append((event_count, event["created_at"], event["id"]))

    result: dict[str, Any] = {
        "database": str(path),
        "apply": apply,
        "telemetry_before": len(rows),
        "telemetry_after": len(rows) - len(removable),
        "telemetry_removed": len(removable),
        "events_before": len(events),
        "events_after": len(events) - len(event_removable),
        "events_removed": len(event_removable),
        "before_bytes": before_bytes,
        "backup": str(backup) if backup else None,
    }

    if apply:
        try:
            connection.execute("BEGIN IMMEDIATE")
            for end_time, end_seq, count, keeper_id, key, _ in updates:
                connection.execute(
                    "UPDATE telemetry SET last_received_at=?,source_seq_end=?,repeat_count=?,fingerprint=? "
                    "WHERE id=?",
                    (end_time, end_seq, count, key, keeper_id),
                )
            for start in range(0, len(removable), 500):
                chunk = removable[start:start + 500]
                placeholders = ",".join("?" for _ in chunk)
                connection.execute(f"DELETE FROM telemetry WHERE id IN ({placeholders})", chunk)
            for count, last_seen_at, keeper_id in event_updates:
                connection.execute(
                    "UPDATE events SET repeat_count=?,last_seen_at=? WHERE id=?",
                    (count, last_seen_at, keeper_id),
                )
            for start in range(0, len(event_removable), 500):
                chunk = event_removable[start:start + 500]
                placeholders = ",".join("?" for _ in chunk)
                connection.execute(f"DELETE FROM events WHERE id IN ({placeholders})", chunk)
            connection.commit()
        except BaseException:
            connection.rollback()
            raise
        finally:
            connection.close()
        storage = Database(path).optimize_storage(full=vacuum)
        result.update(storage)
    else:
        connection.close()
        result["estimated_row_reduction_percent"] = round(
            100.0 * len(removable) / len(rows), 1
        ) if rows else 0.0
    return result

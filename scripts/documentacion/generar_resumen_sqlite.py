"""Genera datos documentales agregados sin publicar identificadores o logs."""

from __future__ import annotations

import argparse
import json
import sqlite3
from collections import Counter
from datetime import datetime
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_DB = ROOT / "tmp_db" / "robot-live.sqlite3"
OUTPUT = ROOT / "docs" / "datos_sqlite_documentales.json"


def rows(connection: sqlite3.Connection, query: str, parameters: tuple = ()) -> list[sqlite3.Row]:
    return list(connection.execute(query, parameters))


def elapsed_seconds(value: str, origin: datetime) -> float:
    parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
    return round((parsed - origin).total_seconds(), 2)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--db", type=Path, default=DEFAULT_DB)
    args = parser.parse_args()
    connection = sqlite3.connect(args.db)
    connection.row_factory = sqlite3.Row
    try:
        sessions = rows(connection, """
            SELECT id, started_at, ended_at,
                   (SELECT COUNT(*) FROM commands c WHERE c.session_id=s.id) commands,
                   (SELECT COUNT(*) FROM events e WHERE e.session_id=s.id) events,
                   (SELECT COUNT(*) FROM telemetry t WHERE t.session_id=s.id) samples
            FROM sessions s ORDER BY id DESC LIMIT 8
        """)
        session_summaries = []
        for index, session in enumerate(reversed(sessions), start=1):
            start = datetime.fromisoformat(session["started_at"])
            end = datetime.fromisoformat(session["ended_at"]) if session["ended_at"] else start
            session_summaries.append({
                "label": f"S{index}",
                "duration_s": round(max(0.0, (end - start).total_seconds()), 1),
                "commands": session["commands"],
                "events": session["events"],
                "samples": session["samples"],
                "closed": session["ended_at"] is not None,
            })

        command_counts = Counter(row["status"] for row in rows(connection, "SELECT status FROM commands"))
        event_counts = Counter(row["kind"] for row in rows(connection, "SELECT kind FROM events"))
        severity_counts = Counter(row["severity"] for row in rows(connection, "SELECT severity FROM events"))
        state_counts = Counter(row["state"] for row in rows(connection, "SELECT state FROM telemetry"))

        selected = connection.execute("""
            SELECT session_id, COUNT(*) score FROM telemetry
            WHERE session_id IN (SELECT DISTINCT session_id FROM commands WHERE command_type='step')
            GROUP BY session_id ORDER BY score DESC LIMIT 1
        """).fetchone()
        series: list[dict[str, float | int]] = []
        if selected:
            telemetry = rows(connection, """
                SELECT received_at,x_mm,y_mm,yaw_deg,pwm_l,pwm_r
                FROM telemetry WHERE session_id=? ORDER BY id
            """, (selected["session_id"],))
            if telemetry:
                origin = datetime.fromisoformat(telemetry[0]["received_at"])
                stride = max(1, len(telemetry) // 120)
                for sample in telemetry[::stride]:
                    series.append({
                        "t": elapsed_seconds(sample["received_at"], origin),
                        "x_cm": round((sample["x_mm"] or 0) / 10, 2),
                        "y_cm": round((sample["y_mm"] or 0) / 10, 2),
                        "yaw": round(sample["yaw_deg"] or 0, 2),
                        "pwm_l": sample["pwm_l"] or 0,
                        "pwm_r": sample["pwm_r"] or 0,
                    })

        payload = {
            "scope": "Resumen agregado y seudonimizado; no contiene IDs, timestamps absolutos, payloads, SSID, IP ni tokens.",
            "sessions": session_summaries,
            "commands_by_status": dict(sorted(command_counts.items())),
            "events_by_kind": dict(event_counts.most_common(12)),
            "events_by_severity": dict(sorted(severity_counts.items())),
            "telemetry_by_state": dict(state_counts.most_common()),
            "telemetry_series": series,
        }
        OUTPUT.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        print(f"Resumen SQLite: {len(session_summaries)} sesiones seudónimas, {len(series)} muestras reducidas")
    finally:
        connection.close()


if __name__ == "__main__":
    main()

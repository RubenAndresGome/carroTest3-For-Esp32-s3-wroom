#!/usr/bin/env python3
"""
Herramienta CLI para consultar e inspeccionar la base de datos SQLite del robot (robot.sqlite3).
Uso básico:
  python consultar_db.py
  python consultar_db.py --commands 10
  python consultar_db.py --events 15
  python consultar_db.py --segments 20
  python consultar_db.py --search "calib"
  python consultar_db.py --db tmp_db/robot.sqlite3
"""

import argparse
import json
import os
import sqlite3
import sys

DEFAULT_DB_PATH = os.path.join("tmp_db", "robot.sqlite3")


def connect_db(db_path: str) -> sqlite3.Connection:
    if not os.path.exists(db_path):
        print(f"[ERROR] No se encontró el archivo de base de datos en: {db_path}")
        print("Sugerencia: Extrae la base de datos de la tablet con ADB primero:")
        print("  adb pull /sdcard/Android/data/mx.ik.robots3/files/debug/robot.sqlite3 tmp_db/robot.sqlite3")
        sys.exit(1)
    conn = sqlite3.connect(db_path)
    conn.row_factory = sqlite3.Row
    return conn


def show_summary(conn: sqlite3.Connection) -> None:
    cursor = conn.cursor()
    print("==================================================")
    print(" RESUMEN DE BASE DE DATOS DEL ROBOT (robot.sqlite3)")
    print("==================================================")
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table';")
    tables = [row["name"] for row in cursor.fetchall()]
    for t in tables:
        cursor.execute(f"SELECT COUNT(*) as count FROM {t};")
        count = cursor.fetchone()["count"]
        print(f"  • Tabla '{t}': {count} registros")
    print("\n--- ÚLTIMAS 5 SESIONES ---")
    cursor.execute("SELECT id, started_at, ended_at, disconnect_reason FROM sessions ORDER BY id DESC LIMIT 5;")
    sessions = cursor.fetchall()
    if not sessions:
        print("  (No hay sesiones registradas)")
    for s in sessions:
        estado = f"Finalizada ({s['disconnect_reason']})" if s['ended_at'] else "Activa / Incompleta"
        print(f"  [Sesión #{s['id']}] Inicio: {s['started_at']} | Fin: {s['ended_at'] or '---'} | {estado}")
    print("==================================================")


def list_sessions(conn: sqlite3.Connection, limit: int) -> None:
    cursor = conn.cursor()
    print(f"\n=== ÚLTIMAS {limit} SESIONES ===")
    cursor.execute(f"SELECT id, started_at, ended_at, robot_id, firmware_version, disconnect_reason FROM sessions ORDER BY id DESC LIMIT {limit};")
    for row in cursor.fetchall():
        end = row['ended_at'] or 'ACTIVA'
        reason = f" ({row['disconnect_reason']})" if row['disconnect_reason'] else ""
        print(f"  • #{row['id']} | Inicio: {row['started_at']} | Fin: {end}{reason} | Robot: {row['robot_id'] or '---'}")


def list_commands(conn: sqlite3.Connection, limit: int) -> None:
    cursor = conn.cursor()
    print(f"\n=== ÚLTIMOS {limit} COMANDOS ===")
    cursor.execute(f"SELECT id, session_id, command_type, status, payload_json, error, created_at FROM commands ORDER BY created_at DESC LIMIT {limit};")
    for row in cursor.fetchall():
        err_str = f" -> Err: {row['error']}" if row['error'] else ""
        print(f"  [{row['created_at']}] Cmd {row['id'][:8]}... (Sesh #{row['session_id']}): {row['command_type']} | Estado: {row['status']}{err_str}")
        if row['payload_json']:
            print(f"      Payload: {row['payload_json']}")


def list_events(conn: sqlite3.Connection, limit: int) -> None:
    cursor = conn.cursor()
    print(f"\n=== ÚLTIMOS {limit} EVENTOS ===")
    cursor.execute(f"SELECT id, session_id, kind, severity, payload_json, created_at FROM events ORDER BY id DESC LIMIT {limit};")
    for row in cursor.fetchall():
        print(f"  [{row['created_at']}] Evt #{row['id']} ({row['severity']}): {row['kind']} | {row['payload_json']}")


def list_telemetry(conn: sqlite3.Connection, limit: int) -> None:
    cursor = conn.cursor()
    print(f"\n=== DISTRIBUCIÓN DE ESTADOS EN TELEMETRÍA ===")
    cursor.execute("SELECT state, COUNT(*) as count FROM telemetry GROUP BY state ORDER BY count DESC;")
    for row in cursor.fetchall():
        print(f"  • Estado '{row['state']}': {row['count']} muestras")

    print(f"\n=== ÚLTIMAS {limit} MUESTRAS DE TELEMETRÍA ===")
    cursor.execute(f"SELECT session_id, received_at, state, x_mm, y_mm, yaw_deg, pwm_l, pwm_r FROM telemetry ORDER BY id DESC LIMIT {limit};")
    for row in cursor.fetchall():
        print(f"  [Sesh #{row['session_id']}] {row['state']} | Pos: ({row['x_mm']/10:.1f}, {row['y_mm']/10:.1f}) cm | Yaw: {row['yaw_deg']:.1f}° | PWM L/R: {row['pwm_l']}/{row['pwm_r']}")


def _angular_delta_deg(reference: float, value: float) -> float:
    return (value - reference + 180.0) % 360.0 - 180.0


def list_segments(conn: sqlite3.Connection, limit: int) -> None:
    """Resume los pasos rectos sin depender de una migración de SQLite.

    Los campos de diagnóstico nuevos viven en telemetry.payload_json; las
    columnas tradicionales siguen siendo suficientes para comparar encoders y
    posición cuando se abre una base histórica.
    """
    cursor = conn.cursor()
    print(f"\n=== ÚLTIMOS {limit} TRAMOS RECTOS ===")
    cursor.execute(
        "SELECT id, session_id, payload_json, status, error, created_at, completed_at "
        "FROM commands WHERE command_type='step' ORDER BY created_at DESC LIMIT ?;", (limit,)
    )
    segments = cursor.fetchall()
    if not segments:
        print("  (No hay comandos step registrados)")
        return
    for segment in reversed(segments):
        payload = json.loads(segment["payload_json"] or "{}")
        end_time = segment["completed_at"] or "9999-12-31 23:59:59"
        samples = conn.execute(
            "SELECT received_at,x_mm,y_mm,yaw_deg,pfl,pfr,pbl,pbr,payload_json FROM telemetry "
            "WHERE session_id=? AND received_at>=? AND received_at<=? ORDER BY received_at;",
            (segment["session_id"], segment["created_at"], end_time),
        ).fetchall()
        if not samples:
            print(f"  • {segment['id'][:8]}: sin telemetría en la ventana ({segment['status']})")
            continue
        first, last = samples[0], samples[-1]
        dx_cm = (last["x_mm"] - first["x_mm"]) / 10.0
        dy_cm = (last["y_mm"] - first["y_mm"]) / 10.0
        max_yaw = max(abs(_angular_delta_deg(first["yaw_deg"], row["yaw_deg"])) for row in samples)
        encoder_delta = tuple(last[key] - first[key] for key in ("pfl", "pfr", "pbl", "pbr"))
        final_payload = json.loads(last["payload_json"] or "{}")
        target = final_payload.get("target", {}) if isinstance(final_payload, dict) else {}
        recovery = final_payload.get("recovery", {}) if isinstance(final_payload, dict) else {}
        lateral = target.get("lateral_error_cm") if isinstance(target, dict) else None
        distance_error = target.get("distance_error_cm") if isinstance(target, dict) else None
        target_text = "relativo"
        if payload.get("target_x_mm") is not None and payload.get("target_y_mm") is not None:
            target_text = f"obj=({payload['target_x_mm']/10:.1f},{payload['target_y_mm']/10:.1f})cm"
        errors = ""
        if isinstance(lateral, (int, float)) and isinstance(distance_error, (int, float)):
            errors = f" | lateral={lateral:.1f}cm, euclidiano={distance_error:.1f}cm"
        recovery_text = ""
        if isinstance(recovery, dict) and recovery.get("decision") not in (None, "", "none"):
            distance = recovery.get("distance_cm", 0.0)
            direction = recovery.get("direction", "forward")
            pivot = "sin_pivote" if recovery.get("pivot_avoided") else "pivote_si_necesario"
            if isinstance(distance, (int, float)):
                recovery_text = (
                    f" | endpoint={recovery.get('decision')} {distance:.1f}cm"
                    f" {direction}/{pivot}"
                )
            else:
                recovery_text = f" | endpoint={recovery.get('decision')} {direction}/{pivot}"
        result = segment["status"]
        if segment["error"]:
            result += f" ({segment['error']})"
        print(
            f"  • {segment['id'][:8]} {target_text} | pos=({first['x_mm']/10:.1f},{first['y_mm']/10:.1f})"
            f"→({last['x_mm']/10:.1f},{last['y_mm']/10:.1f})cm | Δpos=({dx_cm:.1f},{dy_cm:.1f})cm "
            f"| Δenc FL/FR/BL/BR={encoder_delta[0]}/{encoder_delta[1]}/{encoder_delta[2]}/{encoder_delta[3]} "
            f"| yaw máx={max_yaw:.1f}° | {result}{errors}{recovery_text}"
        )


def trace_command(conn: sqlite3.Connection, command_prefix: str) -> None:
    """Muestra una traza de un comando sin mezclar muestras del mismo segundo.

    La tabla `commands` no persiste el seq del ESP32, pero los eventos accepted
    sí. Ambos se insertan en el mismo orden de una sesión; una vez obtenido el
    seq se filtra `payload_json.seq` de telemetría, que es la autoridad de fase.
    """
    commands = conn.execute(
        "SELECT rowid,id,session_id,command_type,payload_json,status,error "
        "FROM commands WHERE id LIKE ? ORDER BY rowid LIMIT 2;", (f"{command_prefix}%",)
    ).fetchall()
    if not commands:
        print(f"\n=== TRAZA {command_prefix}: comando no encontrado ===")
        return
    if len(commands) > 1:
        print(f"\n=== TRAZA {command_prefix}: prefijo ambiguo ===")
        for command in commands:
            print(f"  • {command['id']} ({command['command_type']})")
        return
    command = commands[0]
    if command["session_id"] is None:
        print(f"\n=== TRAZA {command['id'][:8]}: sin sesión SQLite ===")
        return
    session_commands = conn.execute(
        "SELECT rowid,id FROM commands WHERE session_id=? ORDER BY rowid;",
        (command["session_id"],),
    ).fetchall()
    command_index = next((i for i, row in enumerate(session_commands)
                          if row["id"] == command["id"]), None)
    accepted = conn.execute(
        "SELECT id,payload_json FROM events WHERE session_id=? AND kind='accepted' ORDER BY id;",
        (command["session_id"],),
    ).fetchall()
    if command_index is None or command_index >= len(accepted):
        print(f"\n=== TRAZA {command['id'][:8]}: no se pudo enlazar seq ===")
        return
    accepted_payload = json.loads(accepted[command_index]["payload_json"] or "{}")
    firmware_seq = accepted_payload.get("seq")
    run_id = accepted_payload.get("run_id")
    if not isinstance(firmware_seq, int):
        print(f"\n=== TRAZA {command['id'][:8]}: accepted sin seq ===")
        return
    rows = conn.execute(
        "SELECT robot_uptime_ms,x_mm,y_mm,yaw_deg,pwm_l,pwm_r,payload_json "
        "FROM telemetry WHERE session_id=? ORDER BY seq;", (command["session_id"],)
    ).fetchall()
    samples = []
    for row in rows:
        payload = json.loads(row["payload_json"] or "{}")
        if isinstance(run_id, int) and payload.get("command_run_id") == run_id:
            samples.append((row, payload))
    filter_text = f"run {run_id}" if samples else f"seq {firmware_seq} (base histórica)"
    if not samples:
        for row in rows:
            payload = json.loads(row["payload_json"] or "{}")
            if payload.get("seq") == firmware_seq:
                samples.append((row, payload))
    print(f"\n=== TRAZA {command['id'][:8]} · {filter_text} · {command['command_type']} ===")
    result_suffix = f" ({command['error']})" if command["error"] else ""
    print(f"  Resultado: {command['status']}{result_suffix}")
    if not samples:
        print("  (No hay telemetría con el seq activo; posiblemente base histórica)")
        return
    first_row, first_payload = samples[0]
    last_row, last_payload = samples[-1]
    yaw_delta = [_angular_delta_deg(first_row["yaw_deg"], row["yaw_deg"]) for row, _ in samples]
    heading_errors = [abs(float(payload.get("drive_control", {}).get("heading_error_deg", 0) or 0))
                      for _, payload in samples if isinstance(payload.get("drive_control"), dict)]
    motion = last_payload.get("motion", {}) if isinstance(last_payload.get("motion"), dict) else {}
    recovery = last_payload.get("recovery", {}) if isinstance(last_payload.get("recovery"), dict) else {}
    phases: list[str] = []
    for _, payload in samples:
        phase = str(payload.get("phase", "unknown"))
        if not phases or phases[-1] != phase:
            phases.append(phase)
    print(
        f"  Posición: ({first_row['x_mm']/10:.1f},{first_row['y_mm']/10:.1f}) → "
        f"({last_row['x_mm']/10:.1f},{last_row['y_mm']/10:.1f}) cm | "
        f"yaw Δmáx={max(abs(value) for value in yaw_delta):.1f}° | "
        f"eθ máx={max(heading_errors, default=0.0):.1f}°"
    )
    print(
        f"  Movimiento: solicitado={motion.get('requested_mode', '—')} "
        f"efectivo={motion.get('effective_mode', '—')} | trayecto="
        f"{motion.get('travel_heading_deg', '—')}° chasis={motion.get('body_heading_deg', '—')}° "
        f"final={motion.get('final_heading_deg', '—')}°"
    )
    print(
        f"  PWM L/R: {min(row['pwm_l'] for row, _ in samples)}/"
        f"{max(row['pwm_l'] for row, _ in samples)} · "
        f"{min(row['pwm_r'] for row, _ in samples)}/{max(row['pwm_r'] for row, _ in samples)} "
        f"| fases: {' → '.join(phases)}"
    )
    if recovery.get("decision") not in (None, "", "none"):
        print(f"  Endpoint: {recovery}")


def search_all(conn: sqlite3.Connection, term: str) -> None:
    cursor = conn.cursor()
    term_pattern = f"%{term}%"
    print(f"\n=== BÚSQUEDA DE '{term}' EN BASE DE DATOS ===")
    
    print("\n--- COMANDOS COINCIDENTES ---")
    cursor.execute("SELECT id, session_id, command_type, status, payload_json, error, created_at FROM commands WHERE command_type LIKE ? OR status LIKE ? OR payload_json LIKE ? OR error LIKE ? ORDER BY created_at DESC LIMIT 20;", (term_pattern, term_pattern, term_pattern, term_pattern))
    cmds = cursor.fetchall()
    if not cmds:
        print("  (Ningún comando coincide)")
    for row in cmds:
        print(f"  [{row['created_at']}] Cmd #{row['id']} ({row['command_type']}): status={row['status']} | error={row['error']} | payload={row['payload_json']}")

    print("\n--- EVENTOS COINCIDENTES ---")
    cursor.execute("SELECT id, session_id, kind, severity, payload_json, created_at FROM events WHERE kind LIKE ? OR severity LIKE ? OR payload_json LIKE ? ORDER BY id DESC LIMIT 20;", (term_pattern, term_pattern, term_pattern))
    evts = cursor.fetchall()
    if not evts:
        print("  (Ningún evento coincide)")
    for row in evts:
        print(f"  [{row['created_at']}] Evt #{row['id']} [{row['kind']}]: {row['payload_json']}")


def main():
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    parser = argparse.ArgumentParser(description="Inspección rápida de robot.sqlite3")
    parser.add_argument("--db", default=DEFAULT_DB_PATH, help="Ruta a la base de datos (por defecto: tmp_db/robot.sqlite3)")
    parser.add_argument("--sessions", type=int, nargs="?", const=10, help="Listar últimas N sesiones (por defecto: 10)")
    parser.add_argument("--commands", type=int, nargs="?", const=15, help="Listar últimos N comandos (por defecto: 15)")
    parser.add_argument("--events", type=int, nargs="?", const=20, help="Listar últimos N eventos (por defecto: 20)")
    parser.add_argument("--telemetry", type=int, nargs="?", const=10, help="Listar últimas N telemetrías y resumen")
    parser.add_argument("--segments", type=int, nargs="?", const=20, help="Resumir últimos N tramos rectos")
    parser.add_argument("--trace-command", type=str, help="Trazar un comando por prefijo UUID usando su seq de firmware")
    parser.add_argument("--search", type=str, help="Buscar término en comandos y eventos")

    args = parser.parse_args()
    conn = connect_db(args.db)

    has_filter = any([args.sessions, args.commands, args.events, args.telemetry, args.segments,
                      args.trace_command, args.search])

    if not has_filter:
        show_summary(conn)
        return

    if args.sessions:
        list_sessions(conn, args.sessions)
    if args.commands:
        list_commands(conn, args.commands)
    if args.events:
        list_events(conn, args.events)
    if args.telemetry:
        list_telemetry(conn, args.telemetry)
    if args.segments:
        list_segments(conn, args.segments)
    if args.trace_command:
        trace_command(conn, args.trace_command)
    if args.search:
        search_all(conn, args.search)

    conn.close()


if __name__ == "__main__":
    main()

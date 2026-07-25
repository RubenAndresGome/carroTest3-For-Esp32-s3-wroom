#!/usr/bin/env python3
"""
Herramienta CLI para consultar e inspeccionar la base de datos SQLite del robot (robot.sqlite3).
Uso básico:
  python consultar_db.py
  python consultar_db.py --commands 10
  python consultar_db.py --events 15
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
    parser = argparse.ArgumentParser(description="Inspección rápida de robot.sqlite3")
    parser.add_argument("--db", default=DEFAULT_DB_PATH, help="Ruta a la base de datos (por defecto: tmp_db/robot.sqlite3)")
    parser.add_argument("--sessions", type=int, nargs="?", const=10, help="Listar últimas N sesiones (por defecto: 10)")
    parser.add_argument("--commands", type=int, nargs="?", const=15, help="Listar últimos N comandos (por defecto: 15)")
    parser.add_argument("--events", type=int, nargs="?", const=20, help="Listar últimos N eventos (por defecto: 20)")
    parser.add_argument("--telemetry", type=int, nargs="?", const=10, help="Listar últimas N telemetrías y resumen")
    parser.add_argument("--search", type=str, help="Buscar término en comandos y eventos")

    args = parser.parse_args()
    conn = connect_db(args.db)

    has_filter = any([args.sessions, args.commands, args.events, args.telemetry, args.search])

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
    if args.search:
        search_all(conn, args.search)

    conn.close()


if __name__ == "__main__":
    main()

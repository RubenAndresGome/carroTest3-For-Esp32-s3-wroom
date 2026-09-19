import sqlite3, json

conn = sqlite3.connect('tmp_db/robot.sqlite3')
cur = conn.cursor()

# Ver eventos de cambio de comando o finalizacion de tramos en sesion 15823
rows = cur.execute("""
    SELECT created_at, kind, payload_json 
    FROM events 
    WHERE session_id = 15823
    ORDER BY id
""").fetchall()
print("=== EVENTOS SESION 15823 ===")
for r in rows:
    print(r[0], r[1], r[2])

# Ver telemetria justo antes del inicio de Seq 3
rows_tel = cur.execute("""
    SELECT received_at, seq, state, x_mm, y_mm, yaw_deg 
    FROM telemetry 
    WHERE session_id = 15823 AND received_at >= '2026-09-19 14:37:00' AND received_at <= '2026-09-19 14:37:08'
    ORDER BY received_at, seq
""").fetchall()
print("\n=== TELEMETRIA 14:37:00 - 14:37:08 ===")
for r in rows_tel:
    print(r)

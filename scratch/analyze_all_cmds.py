import sqlite3
import json

conn = sqlite3.connect('tmp_db/robot.sqlite3')
cur = conn.cursor()

print("=== TODOS LOS COMANDOS EN SESION 15823 ===")
for r in cur.execute("SELECT id, command_type, payload_json, status, created_at, completed_at FROM commands WHERE session_id = 15823 ORDER BY created_at"):
    print(f"ID: {r[0][:8]} | Type: {r[1]} | Status: {r[3]} | Created: {r[4]} | Completed: {r[5]}")
    print(f"   Payload: {r[2]}")

print("\n=== TELEMETRIA DESPUES DE 14:37:25 ===")
for r in cur.execute("SELECT seq, received_at, state, x_mm, y_mm, yaw_deg, pwm_l, pwm_r, pfl, pfr, pbl, pbr, payload_json FROM telemetry WHERE session_id = 15823 AND received_at >= '2026-09-19 14:37:24' ORDER BY received_at, seq LIMIT 60"):
    print(f"{r[1]}.{r[0]:03d} | st:{r[2]} | yaw:{r[5]:6.1f} | x:{r[3]:6.0f} y:{r[4]:6.0f} | pwm:[{r[6]:5},{r[7]:5}] | ticks:[{r[8]:3},{r[9]:3},{r[10]:3},{r[11]:3}]")

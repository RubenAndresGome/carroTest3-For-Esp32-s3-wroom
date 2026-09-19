import sqlite3
import json
import sys

conn = sqlite3.connect('tmp_db/robot.sqlite3')
cur = conn.cursor()

print("=== SESIONES DE HOY 19 SEP 2026 ===")
for r in cur.execute("SELECT id, started_at, ended_at, disconnect_reason FROM sessions WHERE id >= 15818 ORDER BY id"):
    print(f"Session {r[0]} | Started: {r[1]} | Ended: {r[2]} | Reason: {r[3]}")

for sess_id in [15821, 15822, 15823]:
    print(f"\n=======================================================")
    print(f"=== SESION {sess_id} ===")
    print(f"=======================================================")
    print("--- COMANDOS ---")
    for r in cur.execute("SELECT id, command_type, payload_json, status, created_at, completed_at FROM commands WHERE session_id = ? ORDER BY created_at", (sess_id,)):
        print(f"Cmd {r[0][:8]} | Type: {r[1]} | Status: {r[3]} | Created: {r[4]} | Completed: {r[5]}")
        print(f"   Payload: {r[2]}")

    print("\n--- RESUMEN TELEMETRIA ---")
    tele = cur.execute("SELECT seq, received_at, state, x_mm, y_mm, yaw_deg, pwm_l, pwm_r, payload_json FROM telemetry WHERE session_id = ? ORDER BY received_at", (sess_id,)).fetchall()
    print(f"Total muestras de telemetria: {len(tele)}")
    for r in tele:
        seq, rec_at, state, x_mm, y_mm, yaw_deg, pwm_l, pwm_r, payload_str = r
        try:
            p = json.loads(payload_str)
        except Exception:
            p = {}
        phase = p.get('phase', '')
        dc = p.get('drive_control', {})
        motion = p.get('motion', {})
        target = p.get('target', {})
        recov = p.get('recovery', {})
        
        tgt_hdg = target.get('heading', '-')
        err_hdg = dc.get('heading_error', '-')
        dyn_hdg = dc.get('dynamic_heading', '-')
        eff_mode = motion.get('effective_mode', '-')
        rem_cm = motion.get('remaining_cm', '-')
        gyro_z = p.get('gyro_z', '-')
        
        print(f"{rec_at} | seq:{seq} | st:{state:10} | ph:{phase:15} | yaw:{yaw_deg:6.1f} | x:{x_mm:6.0f} y:{y_mm:6.0f} | pwm:[{pwm_l:4},{pwm_r:4}] | tgt_h:{tgt_hdg} | dyn_h:{dyn_hdg} | err_h:{err_hdg} | mode:{eff_mode} | rem:{rem_cm}")

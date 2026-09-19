import sqlite3
import json

conn = sqlite3.connect('tmp_db/robot.sqlite3')
conn.row_factory = sqlite3.Row

rows = conn.execute(
    "SELECT received_at, state, x_mm, y_mm, yaw_deg, pwm_l, pwm_r, pfl, pfr, pbl, pbr, payload_json FROM telemetry WHERE session_id=15824 AND received_at >= '2026-09-19 15:44:23' AND received_at <= '2026-09-19 15:44:34' ORDER BY received_at ASC"
).fetchall()

print(f"Muestras en Paso 4 (6bdb53bf): {len(rows)}")
for r in rows:
    p = json.loads(r['payload_json'] or '{}')
    m = p.get('motion', {})
    t = p.get('target', {})
    ctrl = p.get('drive_control', {})
    print(f"{r['received_at']} | st={r['state']:<11} | pos=({r['x_mm']/10:5.1f},{r['y_mm']/10:5.1f}) | yaw={r['yaw_deg']:5.1f} | pwm=[{r['pwm_l']:5d},{r['pwm_r']:5d}] | rem={m.get('remaining_cm'):5.1f} | lon_err={t.get('longitudinal_error_cm',0):5.1f} | eucl={t.get('distance_error_cm',0):5.1f} | brk={m.get('brake_prediction_cm')} | enc=[{r['pfl']},{r['pfr']},{r['pbl']},{r['pbr']}]")

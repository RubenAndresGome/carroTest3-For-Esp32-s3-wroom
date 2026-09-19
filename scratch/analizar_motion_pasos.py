import sqlite3
import json

conn = sqlite3.connect('tmp_db/robot.sqlite3')
conn.row_factory = sqlite3.Row

# Obtener comandos de la sesión 15824
cmds = conn.execute("SELECT * FROM commands WHERE session_id=15824 AND command_type='step' ORDER BY created_at ASC").fetchall()

for cmd in cmds:
    print("=" * 90)
    print(f"STEP: {cmd['id'][:8]} | Req: {cmd['payload_json']}")
    t_start = cmd['created_at']
    t_end = cmd['completed_at']
    rows = conn.execute(
        "SELECT received_at, state, x_mm, y_mm, yaw_deg, pwm_l, pwm_r, pfl, pfr, pbl, pbr, payload_json FROM telemetry WHERE session_id=15824 AND received_at >= ? AND received_at <= ? ORDER BY received_at ASC",
        (t_start, t_end)
    ).fetchall()
    print(f"Total telemetrias: {len(rows)}")
    for r in rows:
        p = json.loads(r['payload_json'] or '{}')
        targ = p.get('target', {})
        m = p.get('motion', {})
        enc_f = p.get('encoder_fusion', {})
        print(f"{r['received_at']} | st={r['state']:<11} | pos=({r['x_mm']/10:5.1f},{r['y_mm']/10:5.1f}) | yaw={r['yaw_deg']:5.1f} | pwm=[{r['pwm_l']:4d},{r['pwm_r']:4d}] | rem_cm={m.get('remaining_cm'):5.1f} | brk_cm={m.get('brake_prediction_cm'):4.1f} | coast_cm={m.get('coast_cm'):4.1f} | lon_err={targ.get('longitudinal_error_cm', 0):5.1f} | fin={targ.get('finish_reason')} | dir={m.get('effective_mode')}")

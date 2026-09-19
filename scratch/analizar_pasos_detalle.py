import sqlite3
import json

conn = sqlite3.connect('tmp_db/robot.sqlite3')
conn.row_factory = sqlite3.Row

# Obtener comandos de la sesión 15824
cmds = conn.execute("SELECT * FROM commands WHERE session_id=15824 AND command_type='step' ORDER BY created_at ASC").fetchall()

for cmd in cmds:
    print("=" * 80)
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
        rec = p.get('recovery', {})
        route = p.get('route', {})
        # Buscar campos relevantes
        print(f"{r['received_at']} | st={r['state']:<12} | pos=({r['x_mm']/10:6.1f}, {r['y_mm']/10:6.1f}) | yaw={r['yaw_deg']:5.1f} | pwm=[{r['pwm_l']:5d},{r['pwm_r']:5d}] | enc=[{r['pfl']:4d},{r['pfr']:4d},{r['pbl']:4d},{r['pbr']:4d}] | err_lon={targ.get('longitudinal_error_cm')} | fin={targ.get('finish_reason')} | p_dist={route.get('distance_cm')}")

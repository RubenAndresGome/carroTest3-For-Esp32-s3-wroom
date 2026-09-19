import sqlite3
import json

conn = sqlite3.connect('tmp_db/robot.sqlite3')
conn.row_factory = sqlite3.Row

cm_per_tick = (3.14159265359 * 6.6 * 1.015) / 40.0  # 0.5261 cm/tick

cmds = conn.execute("SELECT * FROM commands WHERE session_id=15824 AND command_type='step' ORDER BY created_at ASC").fetchall()

print(f"Constante cm/tick = {cm_per_tick:.4f}")
print("=" * 100)

for c in cmds:
    p = json.loads(c['payload_json'] or '{}')
    req_cm = p.get('cm', 0.0)
    req_h = p.get('heading', 0.0)
    target_x = p.get('target_x_mm', 0.0) / 10.0
    target_y = p.get('target_y_mm', 0.0) / 10.0
    expected_ticks = req_cm / cm_per_tick
    
    rows = conn.execute(
        "SELECT received_at, state, x_mm, y_mm, yaw_deg, pfl, pfr, pbl, pbr, payload_json FROM telemetry WHERE session_id=15824 AND received_at >= ? AND received_at <= ? ORDER BY received_at ASC",
        (c['created_at'], c['completed_at'] or '9999-12-31')
    ).fetchall()
    
    if not rows:
        continue
    r0 = rows[0]
    rN = rows[-1]
    
    d_fl = rN['pfl'] - r0['pfl']
    d_fr = rN['pfr'] - r0['pfr']
    d_bl = rN['pbl'] - r0['pbl']
    d_br = rN['pbr'] - r0['pbr']
    
    p0 = json.loads(r0['payload_json'] or '{}')
    pN = json.loads(rN['payload_json'] or '{}')
    
    mN = pN.get('motion', {})
    targN = pN.get('target', {})
    
    dx = (rN['x_mm'] - r0['x_mm']) / 10.0
    dy = (rN['y_mm'] - r0['y_mm']) / 10.0
    dist_odom = (dx**2 + dy**2)**0.5
    
    # Ver cuantas muestras y qué pasó al frenar
    print(f"STEP: {c['id'][:8]} | Cmd req: {req_cm:.1f} cm @ {req_h:.1f}° -> Target: ({target_x:.1f}, {target_y:.1f}) cm")
    print(f"  Pose ini: ({r0['x_mm']/10:.1f}, {r0['y_mm']/10:.1f}) -> Pose fin: ({rN['x_mm']/10:.1f}, {rN['y_mm']/10:.1f}) | Delta Odom: ({dx:.1f}, {dy:.1f}) cm [Dist={dist_odom:.1f} cm]")
    print(f"  Ticks esperados para {req_cm:.1f} cm: {expected_ticks:.1f} ticks")
    print(f"  Delta Encoders medidos: FL={d_fl}, FR={d_fr}, BL={d_bl}, BR={d_br}")
    print(f"  Equivalente cm por rueda: FL={d_fl*cm_per_tick:.1f} cm, BL={d_bl*cm_per_tick:.1f} cm, BR={d_br*cm_per_tick:.1f} cm")
    print(f"  Motion fin: rem_cm={mN.get('remaining_cm')}, brk_cm={mN.get('brake_prediction_cm')}, coast_cm={mN.get('coast_cm')}, mode={mN.get('effective_mode')}")
    print(f"  Target fin: lon_err={targN.get('longitudinal_error_cm')}, lat_err={targN.get('lateral_error_cm')}, dist_err={targN.get('distance_error_cm')}, finish_reason={targN.get('finish_reason')}")
    print("-" * 100)

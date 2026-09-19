import sqlite3
import json

conn = sqlite3.connect('tmp_db/robot.sqlite3')
conn.row_factory = sqlite3.Row

# Obtener comandos de la sesion 15824
cmds = conn.execute('SELECT * FROM commands WHERE session_id=15824 ORDER BY id ASC').fetchall()
print(f"=== COMANDOS SESION 15824 ({len(cmds)}) ===")
for c in cmds:
    print(f"[{c['created_at']} -> {c['completed_at']}] Cmd {c['id'][:8]} | Type: {c['command_type']} | Status: {c['status']} | Err: {c['error']}")
    print(f"   Payload: {c['payload_json']}")
    if 'result_json' in c.keys():
        print(f"   Result: {c['result_json']}")

print("\n=== DETALLE DE CADA PASO (STEP) ===")
for c in cmds:
    if c['command_type'] != 'step':
        continue
    p = json.loads(c['payload_json'] or '{}')
    t_start = c['created_at']
    t_end = c['completed_at'] or '9999-12-31'
    rows = conn.execute(
        "SELECT * FROM telemetry WHERE session_id=15824 AND received_at >= ? AND received_at <= ? ORDER BY received_at ASC",
        (t_start, t_end)
    ).fetchall()
    print(f"\n--- STEP {c['id'][:8]} | Req: {p.get('cm')} cm @ {p.get('heading')}° | Target: ({p.get('target_x_mm',0)/10:.1f}, {p.get('target_y_mm',0)/10:.1f}) cm ---")
    print(f"   Muestras de telemetría: {len(rows)}")
    if not rows:
        continue
    r0 = rows[0]
    rN = rows[-1]
    dx = (rN['x_mm'] - r0['x_mm']) / 10.0
    dy = (rN['y_mm'] - r0['y_mm']) / 10.0
    dist_recorrida_odom = (dx**2 + dy**2)**0.5
    d_fl = rN['pfl'] - r0['pfl']
    d_fr = rN['pfr'] - r0['pfr']
    d_bl = rN['pbl'] - r0['pbl']
    d_br = rN['pbr'] - r0['pbr']
    print(f"   Inicio: ({r0['x_mm']/10:.1f}, {r0['y_mm']/10:.1f}) cm, Yaw: {r0['yaw_deg']:.1f}°")
    print(f"   Fin:    ({rN['x_mm']/10:.1f}, {rN['y_mm']/10:.1f}) cm, Yaw: {rN['yaw_deg']:.1f}°")
    print(f"   Delta Pos Odom: dx={dx:.1f} cm, dy={dy:.1f} cm, dist_odom={dist_recorrida_odom:.1f} cm (Pedido: {p.get('cm')} cm)")
    print(f"   Delta Encoders: FL={d_fl}, FR={d_fr}, BL={d_bl}, BR={d_br}")
    
    # Revisemos los ultimos 5 estados de telemetria del paso
    print("   Últimas muestras de telemetría del paso:")
    for r in rows[-5:]:
        pay = json.loads(r['payload_json'] or '{}')
        targ = pay.get('target', {})
        rec = pay.get('recovery', {})
        print(f"     [{r['received_at']}] State: {r['state']} | Pos: ({r['x_mm']/10:.1f}, {r['y_mm']/10:.1f}) | Yaw: {r['yaw_deg']:.1f} | PWM: [{r['pwm_l']},{r['pwm_r']}] | Target: {targ} | Rec: {rec}")

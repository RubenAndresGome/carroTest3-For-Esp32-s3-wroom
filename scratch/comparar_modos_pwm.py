import sqlite3
import json

conn = sqlite3.connect('tmp_db/robot.sqlite3')
conn.row_factory = sqlite3.Row

# Obtener comandos de la sesión 15824
cmds = conn.execute("SELECT * FROM commands WHERE session_id=15824 AND command_type='step' ORDER BY created_at ASC").fetchall()

for c in cmds:
    p = json.loads(c['payload_json'] or '{}')
    rows = conn.execute(
        "SELECT received_at, state, x_mm, y_mm, yaw_deg, pwm_l, pwm_r, pfl, pfr, pbl, pbr, payload_json FROM telemetry WHERE session_id=15824 AND received_at >= ? AND received_at <= ? ORDER BY received_at ASC",
        (c['created_at'], c['completed_at'] or '9999-12-31')
    ).fetchall()
    if not rows: continue
    r0 = rows[0]
    rN = rows[-1]
    p0 = json.loads(r0['payload_json'] or '{}')
    pN = json.loads(rN['payload_json'] or '{}')
    m0 = p0.get('motion', {})
    mN = pN.get('motion', {})
    
    # Buscar si hubo PWM negativo
    pwms = [(r['pwm_l'], r['pwm_r']) for r in rows]
    min_pwm = min(min(p[0], p[1]) for p in pwms)
    max_pwm = max(max(p[0], p[1]) for p in pwms)
    
    print(f"STEP {c['id'][:8]} | Cmd req: {p.get('cm')} cm @ {p.get('heading')}°")
    print(f"   Modo solicitado: {m0.get('requested_mode')} | Efectivo: {m0.get('effective_mode')}")
    print(f"   Rango PWM: min={min_pwm}, max={max_pwm}")
    print(f"   Yaw ini={r0['yaw_deg']:.1f} -> Yaw fin={rN['yaw_deg']:.1f}")
    print(f"   Encoders ini: FL={r0['pfl']}, FR={r0['pfr']}, BL={r0['pbl']}, BR={r0['pbr']}")
    print(f"   Encoders fin: FL={rN['pfl']}, FR={rN['pfr']}, BL={rN['pbl']}, BR={rN['pbr']}")
    print(f"   Delta Encoders: FL={rN['pfl']-r0['pfl']}, FR={rN['pfr']-r0['pfr']}, BL={rN['pbl']-r0['pbl']}, BR={rN['pbr']-r0['pbr']}")
    print(f"   Pos: ({r0['x_mm']/10:.1f}, {r0['y_mm']/10:.1f}) -> ({rN['x_mm']/10:.1f}, {rN['y_mm']/10:.1f}) cm")
    print()

import sqlite3, json

conn = sqlite3.connect('tmp_db/robot.sqlite3')
cur = conn.cursor()

rows = cur.execute('''
    SELECT seq, received_at, state, x_mm, y_mm, yaw_deg, pwm_l, pwm_r, pfl, pfr, pbl, pbr, payload_json 
    FROM telemetry 
    WHERE session_id = 15823 AND received_at >= '2026-09-19 14:37:06' AND received_at <= '2026-09-19 14:37:16'
    ORDER BY received_at, seq
''').fetchall()

print(f"=== GIRO INICIAL DE COMANDO 3 (0 deg -> 90 deg) ===")
print(f"Total muestras: {len(rows)}")
for r in rows:
    seq, rec_at, state, x_mm, y_mm, yaw, pl, pr, pfl, pfr, pbl, pbr, p_str = r
    try: p = json.loads(p_str)
    except: p = {}
    phase = p.get('phase', '')
    dc = p.get('drive_control', {})
    motion = p.get('motion', {})
    target = p.get('target', {})
    gz = p.get('gyro_z', 0)
    print(f"{rec_at}.{seq:03d} | st:{state:10} | ph:{phase:16} | yaw:{yaw:6.1f} | gz:{gz:6.2f} | pwm:[{pl:5},{pr:5}] | ticks:[{pfl:3},{pfr:3},{pbl:3},{pbr:3}] | tgt:{target.get('heading', '-')} | err:{dc.get('heading_error', '-')}")

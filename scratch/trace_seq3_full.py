import sqlite3, json

conn = sqlite3.connect('tmp_db/robot.sqlite3')
cur = conn.cursor()

rows = cur.execute("""
    SELECT received_at, seq, yaw_deg, pwm_l, pwm_r, pfl, pfr, pbl, pbr, payload_json
    FROM telemetry
    WHERE session_id = 15823 AND received_at >= '2026-09-19 14:37:06' AND received_at <= '2026-09-19 14:37:26'
    ORDER BY received_at, seq
""").fetchall()

print(f"Total muestras en Seq 3: {len(rows)}")

# Track turning phases and PWM changes
for r in rows:
    rec_at, seq, yaw, pl, pr, pfl, pfr, pbl, pbr, p_str = r
    try:
        p = json.loads(p_str)
    except:
        p = {}
    ph = p.get('phase', '')
    dc = p.get('drive_control', {})
    dh = dc.get('dynamic_heading_deg', '')
    he = dc.get('heading_error_deg', '')
    lat = p.get('target', {}).get('lateral_error_cm', '')
    # Print when phase changes or significant yaw change
    print(f"{rec_at}.{seq:03d} | ph:{ph:16} | yaw:{yaw:6.1f} | dyn_tgt:{dh} | err:{he} | lat_err:{lat} | pwm:[{pl:5},{pr:5}] | ticks:[{pfl:3},{pfr:3},{pbl:3},{pbr:3}]")

import sqlite3, json, os

conn = sqlite3.connect('tmp_db/robot.sqlite3')
cur = conn.cursor()

# Video duration: 44.53s, creation_time: 2026-09-19T14:37:21Z
# Let's inspect telemetry between 14:36:50 and 14:37:35
rows = cur.execute("""
    SELECT received_at, seq, state, x_mm, y_mm, yaw_deg, pwm_l, pwm_r, pfl, pfr, pbl, pbr, payload_json
    FROM telemetry
    WHERE session_id = 15823 AND received_at >= '2026-09-19 14:37:05' AND received_at <= '2026-09-19 14:37:30'
    ORDER BY received_at, seq
""").fetchall()

print(f"Total rows: {len(rows)}")

# Sample every 1 second or key events
last_sec = ""
for r in rows:
    rec_at, seq, state, x_mm, y_mm, yaw, pl, pr, pfl, pfr, pbl, pbr, p_str = r
    sec = rec_at[:19]
    try:
        p = json.loads(p_str)
    except:
        p = {}
    ph = p.get('phase', '')
    dc = p.get('drive_control', {})
    dyn_h = dc.get('dynamic_heading_deg', '-')
    h_err = dc.get('heading_error_deg', '-')
    lat_err = p.get('target', {}).get('lateral_error_cm', '-')
    
    # Print if phase changed or every 500ms
    print(f"{rec_at}.{seq:03d} | {state:10} | {ph:16} | yaw:{yaw:6.1f} | dyn_h:{dyn_h} | h_err:{h_err} | lat_err:{lat_err} | pwm:[{pl:5},{pr:5}] | ticks:[{pfl:3},{pfr:3},{pbl:3},{pbr:3}] | (X:{x_mm/10:5.1f}, Y:{y_mm/10:5.1f})")

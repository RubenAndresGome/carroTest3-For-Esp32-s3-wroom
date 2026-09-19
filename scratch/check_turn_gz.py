import sqlite3, json

conn = sqlite3.connect('tmp_db/robot.sqlite3')
cur = conn.cursor()

rows = cur.execute("""
    SELECT received_at, seq, yaw_deg, payload_json
    FROM telemetry
    WHERE session_id = 15823 AND received_at >= '2026-09-19 14:37:07' AND received_at <= '2026-09-19 14:37:12'
    ORDER BY received_at, seq
""").fetchall()

print(f"Total muestras en giro 0->90: {len(rows)}")
last_yaw = None
for r in rows:
    rec_at, seq, yaw, p_str = r
    p = json.loads(p_str)
    ph = p.get('phase', '')
    dc = p.get('drive_control', {})
    mot = p.get('motion', {})
    cal_diag = p.get('calibration_diagnostics', {})
    # check if raw gyro is reported
    gz = mot.get('gyro_z', None)
    if gz is None:
        gz = p.get('gyro_z', None)
    d_yaw = (yaw - last_yaw) if last_yaw is not None else 0.0
    last_yaw = yaw
    print(f"{rec_at}.{seq:03d} | ph:{ph:12} | yaw:{yaw:6.1f} | dyaw:{d_yaw:5.1f} | gz:{str(gz):8} | pwm:[{p.get('pwm_l')},{p.get('pwm_r')}]")

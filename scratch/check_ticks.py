import sqlite3

con = sqlite3.connect('tmp_db/robot_20260919_112821.sqlite3')
cur = con.cursor()

rows = cur.execute('''
    SELECT received_at, seq, yaw_deg, pwm_l, pwm_r, pfl, pfr, pbl, pbr 
    FROM telemetry 
    WHERE session_id = 15830 and received_at >= '2026-09-19 17:15:51'
    ORDER BY id ASC
''').fetchall()

print(f"Total rows in Seq 3 (giro_ini): {len(rows)}")
first = rows[0]
print(f"Initial: {first[0]} | yaw={first[2]:.1f} | pwm=({first[3]},{first[4]}) | ticks=(FL:{first[5]}, FR:{first[6]}, BL:{first[7]}, BR:{first[8]})")
for r in rows[::5]:
    dFL = r[5] - first[5]
    dFR = r[6] - first[6]
    dBL = r[7] - first[7]
    dBR = r[8] - first[8]
    print(f"{r[0]} | yaw={r[2]:5.1f} | pwm=({r[3]:5d},{r[4]:5d}) | delta_ticks=(FL:{dFL:4d}, FR:{dFR:4d}, BL:{dBL:4d}, BR:{dBR:4d})")

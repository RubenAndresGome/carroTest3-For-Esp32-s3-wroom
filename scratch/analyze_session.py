import sqlite3
import json

con = sqlite3.connect('tmp_db/robot_20260919_112821.sqlite3')
cur = con.cursor()

cmds = cur.execute('SELECT * FROM commands WHERE session_id = 15830 ORDER BY id ASC').fetchall()
print(f"=== COMANDOS SESION 15830 ({len(cmds)}) ===")
for c in cmds:
    print(c)

print("\n=== TELEMETRIA SIGNIFICATIVA ===")
rows = cur.execute('''
    SELECT received_at, seq, yaw_deg, pwm_l, pwm_r, x_mm, y_mm, payload_json 
    FROM telemetry 
    WHERE session_id = 15830
    ORDER BY id ASC
''').fetchall()

last_phase = ""
for r in rows:
    p = json.loads(r[7]) if r[7] else {}
    phase = p.get('phase', '')
    m = p.get('motion', {})
    dc = p.get('drive_control', {})
    eff = m.get('effective_mode', '')
    req = m.get('requested_mode', '')
    rem = m.get('remaining_cm', 0)
    pwml, pwmr = r[3], r[4]
    
    if phase != last_phase or (pwml < -100 and pwmr < -100) or 'recup' in phase or 'reverse' in eff:
        print(f"{r[0]} | yaw={r[2]:5.1f} | pwm=({pwml:5d},{pwmr:5d}) | pos=({r[5]:5.0f},{r[6]:5.0f}) | phase={phase:15s} | {req}->{eff} rem={rem:5.1f} | dyn_h={dc.get('dynamic_heading_deg', 0):5.1f}")
        last_phase = phase

import sqlite3, json

conn = sqlite3.connect('tmp_db/robot.sqlite3')
cur = conn.cursor()
rows = cur.execute("""
    SELECT received_at, seq, payload_json 
    FROM telemetry 
    WHERE session_id = 15823 AND received_at >= '2026-09-19 14:37:11' AND received_at <= '2026-09-19 14:37:14'
    ORDER BY received_at, seq
""").fetchall()

for r in rows:
    rec_at, seq, p_str = r
    try:
        p = json.loads(p_str)
    except:
        continue
    ph = p.get('phase', '')
    if ph in ('avance', 'pausa_pre_avance', 'recup'):
        dc = p.get('drive_control', {})
        mot = p.get('motion', {})
        tgt = p.get('target', {})
        print(f"--- {rec_at} (seq {seq}) phase: {ph} ---")
        print(f"  yaw: {mot.get('yaw')} | gz: {mot.get('gyro_z')} | pwm_l: {mot.get('pwm_l')} | pwm_r: {mot.get('pwm_r')}")
        print(f"  dc: {dc}")
        print(f"  tgt: {tgt}")

import sqlite3, json

conn = sqlite3.connect('tmp_db/robot.sqlite3')
cur = conn.cursor()
r = cur.execute("SELECT payload_json FROM telemetry WHERE session_id = 15823 AND received_at >= '2026-09-19 14:37:24' AND seq = 527").fetchone()
if r:
    p = json.loads(r[0])
    print('Target:', p.get('target'))
    print('Motion:', p.get('motion'))
    print('Phase:', p.get('phase'))
    print('Drive:', p.get('drive_control'))
else:
    print('Not found')

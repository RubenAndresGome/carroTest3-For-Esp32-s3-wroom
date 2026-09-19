import sqlite3, json

conn = sqlite3.connect('tmp_db/robot.sqlite3')
cur = conn.cursor()
r = cur.execute("SELECT payload_json FROM telemetry WHERE session_id = 15823 ORDER BY id DESC LIMIT 1").fetchone()
p = json.loads(r[0])
print('Health:', p.get('encoder_health'))
print('Fusion:', p.get('encoder_fusion'))
print('Degraded:', p.get('degraded_mode'))
print('Stall:', p.get('stall_accumulated_ms'))
print('Trusted:', p.get('enc_trusted'))

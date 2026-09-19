import sqlite3
import json

conn = sqlite3.connect('tmp_db/robot.sqlite3')
conn.row_factory = sqlite3.Row
rows = conn.execute('SELECT received_at, payload_json FROM telemetry WHERE session_id=15824 ORDER BY id ASC').fetchall()

last_tr = None
for r in rows:
    p = json.loads(r['payload_json'] or '{}')
    tr = p.get('enc_trusted')
    if tr != last_tr:
        print(f"{r['received_at']}: enc_trusted cambio a {tr}, health={p.get('encoder_health')}")
        last_tr = tr

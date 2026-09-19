import sqlite3
import json

conn = sqlite3.connect('tmp_db/robot.sqlite3')
rows = conn.execute("SELECT payload_json FROM commands WHERE command_type='step'").fetchall()
modes = {}
for r in rows:
    p = json.loads(r[0] or '{}')
    m = p.get('drive_mode')
    modes[m] = modes.get(m, 0) + 1
print('drive_mode distribution:', modes)

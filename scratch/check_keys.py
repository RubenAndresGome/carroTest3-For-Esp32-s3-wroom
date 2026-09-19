import sqlite3, json

conn = sqlite3.connect('tmp_db/robot.sqlite3')
cur = conn.cursor()
r = cur.execute("SELECT payload_json FROM telemetry WHERE session_id = 15823 LIMIT 1").fetchone()
p = json.loads(r[0])
print("Keys in root payload:")
print(list(p.keys()))
for k in p:
    if isinstance(p[k], dict):
        print(f"  Keys in {k}: {list(p[k].keys())}")

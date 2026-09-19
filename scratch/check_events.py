import sqlite3
import json

con = sqlite3.connect('tmp_db/robot_20260919_112821.sqlite3')
cur = con.cursor()

events = cur.execute('SELECT * FROM events WHERE session_id = 15830 ORDER BY id ASC').fetchall()
print(f"=== EVENTOS SESION 15830 ({len(events)}) ===")
for e in events:
    print(e)

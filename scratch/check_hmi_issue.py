import sqlite3

con = sqlite3.connect('tmp_db/robot_20260919_122110.sqlite3')
cur = con.cursor()

# Get column names of sessions
cur.execute('PRAGMA table_info(sessions)')
cols = [c[1] for c in cur.fetchall()]
print("Columnas de sessions:", cols)

# Check sessions from id > 15830
sessions = cur.execute('SELECT * FROM sessions WHERE id >= 15825 ORDER BY id DESC LIMIT 20').fetchall()
print(f"=== SESIONES RECIENTES (total >= 15825: {len(sessions)}) ===")
for s in sessions[:10]:
    print(s)

print("\n=== COMANDOS RECIENTES ===")
cmds = cur.execute('SELECT * FROM commands ORDER BY id DESC LIMIT 15').fetchall()
for c in cmds:
    print(c)

print("\n=== EVENTOS RECIENTES ===")
evts = cur.execute('SELECT * FROM events ORDER BY id DESC LIMIT 15').fetchall()
for e in evts:
    print(e)

import sqlite3, json

conn = sqlite3.connect('tmp_db/robot.sqlite3')
cur = conn.cursor()

print("=== SESIONES RECIENTES ===")
sessions = cur.execute("""
    SELECT session_id, MIN(received_at), MAX(received_at), COUNT(*) 
    FROM telemetry 
    GROUP BY session_id 
    ORDER BY session_id DESC 
    LIMIT 10
""").fetchall()
for s in sessions:
    print(f"Session {s[0]}: {s[1]} -> {s[2]} ({s[3]} rows)")

print("\n=== COMANDOS RECIENTES ===")
cmds = cur.execute("""
    SELECT id, session_id, command_type, status, created_at, payload_json 
    FROM commands 
    ORDER BY created_at DESC 
    LIMIT 20
""").fetchall()
for c in cmds:
    print(f"Cmd {c[0][:8]} | sess:{c[1]} | type:{c[2]:12} | st:{c[3]:10} | at:{c[4]} | payload:{c[5]}")

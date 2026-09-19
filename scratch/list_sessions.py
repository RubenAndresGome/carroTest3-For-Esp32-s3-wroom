import sqlite3

conn = sqlite3.connect('tmp_db/robot.sqlite3')
cur = conn.cursor()

sessions = cur.execute("""
    SELECT session_id, MIN(received_at), MAX(received_at), COUNT(*) 
    FROM telemetry 
    WHERE received_at >= '2026-09-19'
    GROUP BY session_id
    ORDER BY session_id
""").fetchall()

print(f"Total sesiones hoy 19-sep-2026: {len(sessions)}")
for s in sessions:
    print(f"Session {s[0]}: {s[1]} -> {s[2]} ({s[3]} rows)")

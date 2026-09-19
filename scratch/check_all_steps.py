import sqlite3, json

conn = sqlite3.connect('tmp_db/robot.sqlite3')
cur = conn.cursor()

# Ver resumen de cada comando de la sesion 15823
cmds = cur.execute("""
    SELECT id, command_type, status, created_at, payload_json 
    FROM commands 
    WHERE session_id = 15823 
    ORDER BY id
""").fetchall()

print("=== COMANDOS SESION 15823 ===")
for c in cmds:
    print(c[0], c[1], c[2], c[3], c[4])

# Ver telemetria clave al final de cada paso
# Seq 1 termina alrededor de 14:37:03
# Seq 2 termina alrededor de 14:37:07
# Seq 3 termina alrededor de 14:37:25
print("\n=== MUESTRAS CLAVE DE TELEMETRIA ===")
rows = cur.execute("""
    SELECT received_at, seq, state, x_mm, y_mm, yaw_deg, pfl, pfr, pbl, pbr, payload_json
    FROM telemetry 
    WHERE session_id = 15823 AND (
        received_at IN ('2026-09-19 14:36:59', '2026-09-19 14:37:03', '2026-09-19 14:37:07', '2026-09-19 14:37:25')
        OR (received_at = '2026-09-19 14:37:08' AND seq = 380)
        OR (received_at = '2026-09-19 14:37:11' AND seq = 410)
    )
    ORDER BY received_at, seq
""").fetchall()

for r in rows:
    p = json.loads(r[10])
    print(f"{r[0]} | seq:{r[1]} | state:{r[2]} | phase:{p.get('phase')} | yaw:{r[5]:.2f} | (X:{r[3]/10:.1f}, Y:{r[4]/10:.1f}) | ticks:{r[6]},{r[7]},{r[8]},{r[9]}")

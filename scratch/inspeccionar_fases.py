import sqlite3
import json

conn = sqlite3.connect('tmp_db/robot.sqlite3')
conn.row_factory = sqlite3.Row

# Obtener comandos de la sesión 15824
cmds = conn.execute("SELECT * FROM commands WHERE session_id=15824 ORDER BY created_at ASC").fetchall()

for c in cmds:
    print(f"[{c['created_at']} -> {c['completed_at']}] Cmd {c['id'][:8]} | Type: {c['command_type']} | Status: {c['status']}")
    print(f"  Payload: {c['payload_json']}")
    # Buscar muestras donde cambie de fase o reporte route
    rows = conn.execute(
        "SELECT received_at, state, x_mm, y_mm, yaw_deg, pwm_l, pwm_r, payload_json FROM telemetry WHERE session_id=15824 AND received_at >= ? AND received_at <= ? ORDER BY received_at ASC",
        (c['created_at'], c['completed_at'] or '9999-12-31')
    ).fetchall()
    fases = set()
    directions = set()
    for r in rows:
        p = json.loads(r['payload_json'] or '{}')
        fase_cmd = p.get('route', {}).get('fase') or p.get('fase')
        dir_tras = p.get('route', {}).get('direction') or p.get('direction')
        if fase_cmd: fases.add(fase_cmd)
        if dir_tras: directions.add(dir_tras)
    print(f"  Fases detectadas: {fases}")
    print(f"  Direcciones: {directions}")

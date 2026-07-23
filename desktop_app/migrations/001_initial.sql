BEGIN IMMEDIATE;
CREATE TABLE IF NOT EXISTS settings (
    key TEXT PRIMARY KEY,
    value_json TEXT NOT NULL,
    updated_at TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS sessions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    started_at TEXT NOT NULL,
    ended_at TEXT,
    firmware_version TEXT,
    robot_id TEXT
);
CREATE TABLE IF NOT EXISTS commands (
    id TEXT PRIMARY KEY,
    session_id INTEGER REFERENCES sessions(id),
    command_type TEXT NOT NULL,
    payload_json TEXT NOT NULL,
    status TEXT NOT NULL CHECK(status IN ('queued','sent','acknowledged','completed','rejected','failed')),
    created_at TEXT NOT NULL,
    sent_at TEXT,
    ack_at TEXT,
    completed_at TEXT,
    error TEXT
);
CREATE TABLE IF NOT EXISTS events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    session_id INTEGER REFERENCES sessions(id),
    kind TEXT NOT NULL,
    severity TEXT NOT NULL CHECK(severity IN ('info','warning','error','critical')),
    payload_json TEXT NOT NULL,
    created_at TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS telemetry (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    session_id INTEGER NOT NULL REFERENCES sessions(id),
    seq INTEGER NOT NULL,
    received_at TEXT NOT NULL,
    robot_uptime_ms INTEGER,
    state TEXT NOT NULL,
    x_mm REAL NOT NULL,
    y_mm REAL NOT NULL,
    yaw_deg REAL NOT NULL,
    payload_json TEXT NOT NULL,
    UNIQUE(session_id, seq)
);
CREATE INDEX IF NOT EXISTS idx_commands_session ON commands(session_id, created_at);
CREATE INDEX IF NOT EXISTS idx_events_session ON events(session_id, created_at);
CREATE INDEX IF NOT EXISTS idx_telemetry_session ON telemetry(session_id, seq);
COMMIT;

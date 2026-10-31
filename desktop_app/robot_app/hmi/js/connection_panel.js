// @ts-check
/** @typedef {import('@frontend/protocol').Telemetry} TelemetryPayload */

/* ==============================================
   CONNECTION PANEL (Enlace HMI -> ESP32 e Historial SQLite)
   ============================================== */
const ConnectionPanel = {
    state: 'stopped',

    init() {
        document.getElementById('btn-connect')?.addEventListener('click', () => this.change('connect'));
        document.getElementById('btn-reconnect')?.addEventListener('click', () => this.change('reconnect'));
        document.getElementById('btn-disconnect')?.addEventListener('click', () => this.change('disconnect'));
        this.loadHistory();
    },

    async change(action) {
        if (!WSControl.flaskMode) return LogSystem.add('WARN', 'La gestión explícita está disponible en la aplicación Flask.');
        try {
            const hostInput = /** @type {HTMLInputElement | null} */ (document.getElementById('robot-host'));
            const host = hostInput ? hostInput.value.trim() : '192.168.4.1';
            if (action !== 'disconnect') {
                await WSControl.api('/api/v1/config/robot', { method: 'PUT', body: JSON.stringify({ robot_host: host }) });
            }
            const snapshot = await WSControl.api(`/api/v1/connection/${action}`, { method: 'POST', body: '{}' });
            this.applySnapshot(snapshot);
            LogSystem.add('INFO', `${action.toUpperCase()} solicitado a Flask.`);
        } catch (error) {
            LogSystem.add('ERROR', humanizeError(error.message));
        }
    },

    applySnapshot(snapshot) {
        const hostInput = /** @type {HTMLInputElement | null} */ (document.getElementById('robot-host'));
        if (hostInput) hostInput.value = snapshot.robot_host || '192.168.4.1';
        if (typeof RouteManager !== 'undefined') RouteManager.applyCapabilities(snapshot.capabilities || {});
        this.applyConnection(snapshot.connection || {});
        this.applySession({ recording: snapshot.recording, session_id: snapshot.session_id });
        const proto = document.getElementById('protocol-version');
        if (proto) proto.textContent = snapshot.connection?.state === 'stopped' ? '—' : (snapshot.connection?.protocol || '—');
        if (snapshot.telemetry) this.applyIdentity(snapshot.telemetry);
    },

    applyConnection(connection) {
        this.state = connection.state || 'stopped';
        state.connected = this.state === 'connected';
        const label = { stopped: 'DESCONECTADO', connecting: 'CONECTANDO', connected: 'CONECTADO', backoff: 'REINTENTANDO' }[this.state] || this.state.toUpperCase();
        document.getElementById('connection-panel-state').textContent = label;
        document.getElementById('connection-panel-detail').textContent = connection.detail || (state.connected ? 'WebSocket y protocolo robot-s3-steps-v3 activos.' : 'Selecciona ROBOT_S3_LOCAL en el Wi‑Fi del dispositivo antes de conectar.');
        document.getElementById('connection-panel-dot').classList.toggle('ok', state.connected);

        if ((this.state === 'stopped' || this.state === 'backoff') && typeof RouteManager !== 'undefined') {
            if (RouteManager.isRunning) RouteManager.failRoute(`Conexión con el robot interrumpida (${label}); ruta cancelada por seguridad.`);
            RouteManager.unlockTabs();
            if (this.state === 'stopped') {
                RouteManager.queue = []; RouteManager.logicalSteps = []; RouteManager.currentStepIndex = 0;
                RouteManager.updateListUI(); RouteManager.drawPlan();
            }
        }
        Calibration.updateAvailability();
        Telemetry._ui();
    },

    applySession(session) {
        document.getElementById('active-session').textContent = session.recording ? `#${session.session_id}` : '—';
        if (!session.recording && this.state === 'stopped' && typeof RouteManager !== 'undefined') {
            if (RouteManager.isRunning) RouteManager.failRoute('Sesión detenida; ruta cancelada.');
            RouteManager.queue = []; RouteManager.logicalSteps = []; RouteManager.currentStepIndex = 0;
            RouteManager.updateListUI(); RouteManager.drawPlan();
        }
    },

    applyIdentity(telemetry) {
        if (telemetry.robot_id) document.getElementById('robot-id').textContent = telemetry.robot_id;
        if (telemetry.firmware_version) document.getElementById('firmware-version').textContent = telemetry.firmware_version;
        if (telemetry.protocol) document.getElementById('protocol-version').textContent = telemetry.protocol;
    },

    esc(value) {
        const node = document.createElement('span');
        node.textContent = value ?? '—';
        return node.innerHTML;
    },

    async loadHistory() {
        if (!WSControl.flaskMode) return;
        try {
            const rows = await WSControl.api('/api/v1/sessions'), body = document.getElementById('session-history');
            if (!rows.length) { body.innerHTML = '<tr><td colspan="8" class="session-empty">Sin sesiones registradas</td></tr>'; return; }
            const localDay = row => {
                if (!row.started_local_day) return '—';
                const offset = Number(row.started_utc_offset_min);
                if (!Number.isFinite(offset)) return row.started_local_day;
                const sign = offset >= 0 ? '+' : '−', abs = Math.abs(offset);
                return `${row.started_local_day} · UTC${sign}${String(Math.floor(abs / 60)).padStart(2, '0')}:${String(abs % 60).padStart(2, '0')}`;
            };
            body.innerHTML = rows.map(r => `<tr><td>#${r.id}</td><td>${this.esc(localDay(r))}</td><td>${this.esc(r.started_at)}</td><td>${this.esc(r.ended_at || 'ACTIVA')}</td><td>${this.esc(r.robot_id || 'pendiente')}</td><td>${r.samples}</td><td>${this.esc(r.disconnect_reason || '—')}</td><td><a href="/api/v1/sessions/${r.id}/telemetry.csv">CSV</a><a href="/api/v1/sessions/${r.id}.json">JSON</a></td></tr>`).join('');
        } catch (error) {
            LogSystem.add('ERROR', `Historial: ${humanizeError(error.message)}`);
        }
    }
};

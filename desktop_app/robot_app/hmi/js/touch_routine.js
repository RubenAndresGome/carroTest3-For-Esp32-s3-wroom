// @ts-check
/** @typedef {import('@frontend/protocol').TurnAction} TurnAction */
/** @typedef {import('@frontend/protocol').MoveAction} MoveAction */
/** @typedef {import('@frontend/protocol').RoutineAction} RoutineAction */
/** @typedef {import('@frontend/protocol').Routine} Routine */

/* ==============================================
   TOUCH ROUTINE BUILDER & JOG CONTROLLER
   ============================================== */
const TouchRoutine = {
    /** @type {RoutineAction[]} */
    actions: [],
    /** @type {string | null} */
    currentRoutineId: null,
    /** @type {boolean} */
    _initialized: false,

    init() {
        if (this._initialized) return;
        this._initialized = true;
        this._bindJogControls();
        this._bindSequenceTools();
        this._bindStorageControls();
        this._bindRunControls();
        this.loadRoutinesList();
        this.render();
    },

    _bindJogControls() {
        document.querySelectorAll('.btn-jog[data-jog-type="turn"]').forEach(btn => {
            btn.addEventListener('click', () => {
                const angle = Number(btn.getAttribute('data-val') || 0);
                if (angle !== 0) this.addAction({ type: 'turn', delta_deg: angle });
            });
        });
        document.getElementById('btn-jog-custom-turn')?.addEventListener('click', () => {
            const input = /** @type {HTMLInputElement | null} */ (document.getElementById('touch-jog-custom-angle'));
            const angle = Number(input?.value || 0);
            if (!angle || isNaN(angle) || angle < -180 || angle > 180) return LogSystem.add('WARN', 'El ángulo debe estar entre -180° y 180°.');
            this.addAction({ type: 'turn', delta_deg: angle });
            if (input) input.value = '';
        });
        document.querySelectorAll('.btn-jog[data-jog-type="move"]').forEach(btn => {
            btn.addEventListener('click', () => {
                const dist = Number(btn.getAttribute('data-val') || 0);
                const dir = btn.getAttribute('data-dir') === 'reverse' ? 'reverse' : 'forward';
                if (dist > 0) this.addAction({ type: 'move', distance_cm: dist, direction: dir });
            });
        });
        const addCustomMove = (/** @type {'forward' | 'reverse'} */ dir) => {
            const input = /** @type {HTMLInputElement | null} */ (document.getElementById('touch-jog-custom-dist'));
            const dist = Number(input?.value || 0);
            if (!dist || isNaN(dist) || dist <= 0 || dist > 200) return LogSystem.add('WARN', 'La distancia debe estar entre 1 y 200 cm.');
            this.addAction({ type: 'move', distance_cm: dist, direction: dir });
            if (input) input.value = '';
        };
        document.getElementById('btn-jog-custom-fwd')?.addEventListener('click', () => addCustomMove('forward'));
        document.getElementById('btn-jog-custom-rev')?.addEventListener('click', () => addCustomMove('reverse'));
    },

    _bindSequenceTools() {
        document.getElementById('btn-routine-compact')?.addEventListener('click', () => this.compact());
        document.getElementById('btn-routine-invert')?.addEventListener('click', () => this.invert());
        document.getElementById('btn-routine-clear')?.addEventListener('click', () => this.clear());

        const listEl = document.getElementById('touch-routine-list');
        listEl?.addEventListener('click', (e) => {
            const btn = /** @type {HTMLElement} */ (e.target).closest('button[data-act]');
            if (!btn) return;
            const act = btn.getAttribute('data-act');
            const idx = Number(btn.getAttribute('data-idx'));
            if (isNaN(idx) || idx < 0 || idx >= this.actions.length) return;

            if (act === 'del') {
                this.actions.splice(idx, 1);
            } else if (act === 'up' && idx > 0) {
                const tmp = this.actions[idx - 1]; this.actions[idx - 1] = this.actions[idx]; this.actions[idx] = tmp;
            } else if (act === 'down' && idx < this.actions.length - 1) {
                const tmp = this.actions[idx + 1]; this.actions[idx + 1] = this.actions[idx]; this.actions[idx] = tmp;
            }
            this.currentRoutineId = null;
            this.render();
            this.refreshPreview();
        });
    },

    _bindStorageControls() {
        document.getElementById('btn-routine-save')?.addEventListener('click', () => this.saveRoutine());
        document.getElementById('btn-routine-load')?.addEventListener('click', () => this.loadRoutine());
        document.getElementById('btn-routine-delete')?.addEventListener('click', () => this.deleteRoutine());
    },

    _bindRunControls() {
        document.getElementById('touch-routine-ack')?.addEventListener('change', () => this._updateRunButtonState());
        document.getElementById('btn-touch-routine-run')?.addEventListener('click', () => this.runRoutine());
        document.getElementById('btn-touch-routine-refresh-preview')?.addEventListener('click', () => this.refreshPreview());
        document.querySelectorAll('input[name="routine-direction"]').forEach(r => r.addEventListener('change', () => this.refreshPreview()));
    },

    _calcTotalDist() {
        return this.actions.reduce((sum, act) => sum + (act.type === 'move' ? Number(act.distance_cm) : 0), 0);
    },

    _updateRunButtonState() {
        const ack = /** @type {HTMLInputElement | null} */ (document.getElementById('touch-routine-ack'))?.checked;
        const btn = /** @type {HTMLButtonElement | null} */ (document.getElementById('btn-touch-routine-run'));
        if (btn) btn.disabled = !ack || this.actions.length === 0;
    },

    /**
     * @param {RoutineAction} action
     */
    async addAction(action) {
        if (this.actions.length >= 128) return LogSystem.add('WARN', 'Límite máximo de 128 acciones alcanzado.');
        if (action.type === 'move' && this._calcTotalDist() + action.distance_cm > 2000) {
            return LogSystem.add('WARN', 'Límite de 2000 cm de desplazamiento superado.');
        }

        const planOnly = /** @type {HTMLInputElement | null} */ (document.getElementById('touch-routine-plan-only'))?.checked;
        if (!planOnly) {
            try {
                await WSControl.api('/api/v1/jog', { method: 'POST', body: JSON.stringify({ action, execute: true }) });
                LogSystem.add('CMD', action.type === 'turn'
                    ? `Jog: giro ${action.delta_deg}° ejecutado`
                    : `Jog: desplazamiento ${action.distance_cm} cm (${action.direction}) ejecutado`);
            } catch (err) {
                LogSystem.add('ERROR', `Error ejecutando jog: ${err.message}`);
                return;
            }
        }

        this.actions.push(action);
        this.currentRoutineId = null;
        this.render();
        this.refreshPreview();
    },

    render() {
        const countEl = document.getElementById('routine-actions-count');
        if (countEl) countEl.textContent = String(this.actions.length);
        const distEl = document.getElementById('routine-total-dist');
        if (distEl) distEl.textContent = this._calcTotalDist().toFixed(1);
        const listEl = document.getElementById('touch-routine-list');
        if (!listEl) return;

        if (this.actions.length === 0) {
            listEl.innerHTML = '<div class="routine-empty-hint" id="touch-routine-empty-hint">Secuencia vacía. Usa la botonera jog para enseñar acciones o carga una rutina guardada.</div>';
        } else {
            listEl.innerHTML = this.actions.map((act, idx) => {
                const isTurn = act.type === 'turn';
                const badgeClass = isTurn ? 'turn' : 'move';
                const badgeLabel = isTurn ? 'GIRO' : (act.direction === 'reverse' ? 'REV' : 'FWD');
                const desc = isTurn ? `${act.delta_deg > 0 ? '+' : ''}${act.delta_deg.toFixed(1)}° ${act.delta_deg > 0 ? '↻' : '↺'}` : `${act.direction === 'reverse' ? '↓' : '↑'} ${act.distance_cm.toFixed(1)} cm`;
                const upDis = idx === 0 ? 'disabled' : '', downDis = idx === this.actions.length - 1 ? 'disabled' : '';
                return `<div class="routine-action-item" data-idx="${idx}" data-kind="${act.type}"><div class="routine-action-info"><span class="routine-action-idx">#${idx + 1}</span><span class="routine-action-badge ${badgeClass}">${badgeLabel}</span><span class="routine-action-desc">${desc}</span></div><div class="routine-action-tools"><button type="button" class="btn-tool" data-act="up" data-idx="${idx}" title="Subir" ${upDis}>↑</button><button type="button" class="btn-tool" data-act="down" data-idx="${idx}" title="Bajar" ${downDis}>↓</button><button type="button" class="btn-tool" data-act="del" data-idx="${idx}" title="Eliminar">✕</button></div></div>`;
            }).join('');
        }
        this._updateRunButtonState();
    },

    compact() {
        if (this.actions.length <= 1) return;
        /** @type {RoutineAction[]} */
        const compacted = [];
        for (const act of this.actions) {
            if (compacted.length === 0) { compacted.push({ ...act }); continue; }
            const prev = compacted[compacted.length - 1];
            if (prev.type === 'turn' && act.type === 'turn') {
                let sum = (prev.delta_deg + act.delta_deg) % 360;
                if (sum > 180) sum -= 360; else if (sum <= -180) sum += 360;
                if (Math.abs(sum) < 0.01) compacted.pop(); else prev.delta_deg = sum;
            } else if (prev.type === 'move' && act.type === 'move' && prev.direction === act.direction) {
                const combined = prev.distance_cm + act.distance_cm;
                if (combined <= 200) prev.distance_cm = combined; else compacted.push({ ...act });
            } else { compacted.push({ ...act }); }
        }
        this.actions = compacted; this.currentRoutineId = null;
        this.render(); this.refreshPreview(); LogSystem.add('INFO', 'Rutina compactada.');
    },

    invert() {
        if (this.actions.length === 0) return;
        /** @type {RoutineAction[]} */
        const inverted = [];
        for (let i = this.actions.length - 1; i >= 0; i--) {
            const act = this.actions[i];
            if (act.type === 'turn') {
                let a = (-act.delta_deg) % 360;
                if (a > 180) a -= 360; else if (a <= -180) a += 360;
                inverted.push({ type: 'turn', delta_deg: a });
            } else {
                inverted.push({ type: 'move', distance_cm: act.distance_cm, direction: act.direction === 'reverse' ? 'forward' : 'reverse' });
            }
        }
        this.actions = inverted; this.currentRoutineId = null;
        this.render(); this.refreshPreview(); LogSystem.add('INFO', 'Rutina invertida exactamente (G⁻¹).');
    },

    clear() {
        if (this.actions.length === 0) return;
        if (!confirm('¿Limpiar todas las acciones de la rutina actual?')) return;
        this.actions = []; this.currentRoutineId = null;
        this.render(); this.clearPreview(); LogSystem.add('INFO', 'Secuencia de rutina reiniciada.');
    },

    async loadRoutinesList() {
        try {
            const list = await WSControl.api('/api/v1/routines');
            const select = /** @type {HTMLSelectElement | null} */ (document.getElementById('touch-routine-select'));
            if (!select) return;
            select.innerHTML = '<option value="">-- Seleccionar rutina guardada --</option>' +
                (list || []).map((/** @type {any} */ r) => `<option value="${r.id}">${r.name} (${(r.actions || []).length} pasos)</option>`).join('');
        } catch (err) { LogSystem.add('ERROR', `Error cargando lista de rutinas: ${err.message}`); }
    },

    async saveRoutine() {
        if (this.actions.length === 0) return LogSystem.add('WARN', 'No hay acciones en la rutina para guardar.');
        const nameInput = /** @type {HTMLInputElement | null} */ (document.getElementById('touch-routine-name'));
        const name = (nameInput?.value || '').trim() || `Rutina ${new Date().toLocaleTimeString()}`;
        try {
            const saved = await WSControl.api('/api/v1/routines', { method: 'POST', body: JSON.stringify({ name, actions: this.actions }) });
            this.currentRoutineId = saved.id;
            LogSystem.add('INFO', `Rutina "${saved.name}" guardada con éxito en SQLite.`);
            await this.loadRoutinesList();
            const select = /** @type {HTMLSelectElement | null} */ (document.getElementById('touch-routine-select'));
            if (select) select.value = saved.id;
        } catch (err) { LogSystem.add('ERROR', `Error guardando rutina: ${err.message}`); }
    },

    async loadRoutine() {
        const select = /** @type {HTMLSelectElement | null} */ (document.getElementById('touch-routine-select'));
        const routineId = select?.value;
        if (!routineId) return LogSystem.add('WARN', 'Selecciona una rutina guardada.');
        try {
            const r = await WSControl.api(`/api/v1/routines/${routineId}`);
            this.actions = r.actions || []; this.currentRoutineId = r.id;
            const nameInput = /** @type {HTMLInputElement | null} */ (document.getElementById('touch-routine-name'));
            if (nameInput) nameInput.value = r.name || '';
            this.render(); this.refreshPreview(); LogSystem.add('INFO', `Rutina "${r.name}" cargada.`);
        } catch (err) { LogSystem.add('ERROR', `Error cargando rutina: ${err.message}`); }
    },

    async deleteRoutine() {
        const select = /** @type {HTMLSelectElement | null} */ (document.getElementById('touch-routine-select'));
        const routineId = select?.value;
        if (!routineId) return LogSystem.add('WARN', 'Selecciona una rutina para eliminar.');
        if (!confirm('¿Eliminar la rutina seleccionada de la base de datos?')) return;
        try {
            await WSControl.api(`/api/v1/routines/${routineId}`, { method: 'DELETE' });
            if (this.currentRoutineId === routineId) this.currentRoutineId = null;
            await this.loadRoutinesList(); LogSystem.add('INFO', 'Rutina eliminada de SQLite.');
        } catch (err) { LogSystem.add('ERROR', `Error eliminando rutina: ${err.message}`); }
    },

    clearPreview() {
        const canvas = /** @type {HTMLCanvasElement | null} */ (document.getElementById('touch-routine-preview'));
        if (!canvas) return;
        const ctx = canvas.getContext('2d');
        if (!ctx) return;
        ctx.clearRect(0, 0, canvas.width, canvas.height);
        ctx.fillStyle = '#09111b'; ctx.fillRect(0, 0, canvas.width, canvas.height);
    },

    async refreshPreview() {
        const canvas = /** @type {HTMLCanvasElement | null} */ (document.getElementById('touch-routine-preview'));
        if (!canvas) return;
        const ctx = canvas.getContext('2d');
        if (!ctx) return;
        const w = canvas.width, h = canvas.height, pad = 30;
        ctx.clearRect(0, 0, w, h); ctx.fillStyle = '#09111b'; ctx.fillRect(0, 0, w, h);

        ctx.strokeStyle = 'rgba(120,145,170,.18)'; ctx.lineWidth = 1;
        for (let x = pad; x < w; x += 60) { ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, h); ctx.stroke(); }
        for (let y = pad; y < h; y += 40) { ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke(); }
        if (this.actions.length === 0) return;

        const dirRadio = /** @type {HTMLInputElement | null} */ (document.querySelector('input[name="routine-direction"]:checked'));
        const dirVal = dirRadio?.value || 'forward';
        const direction = dirVal === 'pivot_forward' ? 'reverse' : dirVal;
        const reverseStrategy = dirVal === 'pivot_forward' ? 'pivot_forward' : 'mirror';

        try {
            const preview = await WSControl.api('/api/v1/routines/preview', {
                method: 'POST', body: JSON.stringify({ actions: this.actions, direction, reverse_strategy: reverseStrategy })
            });
            const fwdPoses = preview.forward?.poses || preview.poses || [], revPoses = preview.reverse?.poses || [];
            const allPoses = [...fwdPoses, ...revPoses];
            if (allPoses.length === 0) return;

            const xs = allPoses.map((/** @type {any} */ p) => Number(p.x_mm)), ys = allPoses.map((/** @type {any} */ p) => Number(p.y_mm));
            let minX = Math.min(...xs), maxX = Math.max(...xs), minY = Math.min(...ys), maxY = Math.max(...ys);
            if (maxX === minX) { maxX += 100; minX -= 100; }
            if (maxY === minY) { maxY += 100; minY -= 100; }

            const scale = Math.min((w - 2 * pad) / (maxX - minX), (h - 2 * pad) / (maxY - minY));
            const map = (/** @type {{x_mm: number, y_mm: number}} */ p) => ({
                x: pad + (Number(p.x_mm) - minX) * scale, y: h - pad - (Number(p.y_mm) - minY) * scale
            });

            if (revPoses.length > 1) {
                ctx.save(); ctx.strokeStyle = '#ffab00'; ctx.lineWidth = 2; ctx.setLineDash([6, 4]); ctx.beginPath();
                revPoses.forEach((/** @type {any} */ p, /** @type {number} */ i) => { const q = map(p); i === 0 ? ctx.moveTo(q.x, q.y) : ctx.lineTo(q.x, q.y); });
                ctx.stroke(); ctx.restore();
            }
            if (fwdPoses.length > 1) {
                ctx.save(); ctx.strokeStyle = '#00b0ff'; ctx.lineWidth = 2.5; ctx.beginPath();
                fwdPoses.forEach((/** @type {any} */ p, /** @type {number} */ i) => { const q = map(p); i === 0 ? ctx.moveTo(q.x, q.y) : ctx.lineTo(q.x, q.y); });
                ctx.stroke(); ctx.fillStyle = '#00e676';
                fwdPoses.forEach((/** @type {any} */ p) => { const q = map(p); ctx.beginPath(); ctx.arc(q.x, q.y, 4, 0, Math.PI * 2); ctx.fill(); });
                ctx.restore();
            }
            const lastP = (dirVal === 'forward' ? fwdPoses : revPoses).slice(-1)[0] || fwdPoses[0];
            if (lastP) {
                const lp = map(lastP), rad = ((Number(lastP.yaw_deg) || 0) - 90) * Math.PI / 180;
                ctx.save(); ctx.strokeStyle = '#00e676'; ctx.lineWidth = 2; ctx.beginPath();
                ctx.moveTo(lp.x, lp.y); ctx.lineTo(lp.x + 16 * Math.cos(rad), lp.y + 16 * Math.sin(rad)); ctx.stroke(); ctx.restore();
            }
            const originPt = map({ x_mm: 0, y_mm: 0 });
            ctx.save(); ctx.fillStyle = '#fff'; ctx.beginPath(); ctx.arc(originPt.x, originPt.y, 3, 0, Math.PI * 2); ctx.fill(); ctx.restore();
        } catch (err) {
            LogSystem.add('WARN', `Previsualización no disponible: ${err.message}`);
        }
    },

    async runRoutine() {
        if (this.actions.length === 0) return LogSystem.add('WARN', 'La rutina no contiene acciones.');
        const ackBox = /** @type {HTMLInputElement | null} */ (document.getElementById('touch-routine-ack'));
        if (!ackBox?.checked) return LogSystem.add('WARN', 'Debes confirmar que el área está despejada.');
        const dirRadio = /** @type {HTMLInputElement | null} */ (document.querySelector('input[name="routine-direction"]:checked'));
        const dirVal = dirRadio?.value || 'forward';
        const direction = dirVal === 'pivot_forward' ? 'reverse' : dirVal;
        const reverseStrategy = dirVal === 'pivot_forward' ? 'pivot_forward' : 'mirror';

        try {
            if (!this.currentRoutineId) {
                const nameInput = /** @type {HTMLInputElement | null} */ (document.getElementById('touch-routine-name'));
                const name = (nameInput?.value || '').trim() || `Rutina ${new Date().toLocaleTimeString()}`;
                const saved = await WSControl.api('/api/v1/routines', { method: 'POST', body: JSON.stringify({ name, actions: this.actions }) });
                this.currentRoutineId = saved.id;
                await this.loadRoutinesList();
                const select = /** @type {HTMLSelectElement | null} */ (document.getElementById('touch-routine-select'));
                if (select) select.value = saved.id;
            }
            const body = { direction, reverse_strategy: reverseStrategy };
            const mission = await WSControl.api(`/api/v1/routines/${this.currentRoutineId}/run`, { method: 'POST', body: JSON.stringify(body) });
            LogSystem.add('CMD', `Rutina iniciada en lazo cerrado (${dirVal}). ID: ${this.currentRoutineId}`);
            if (typeof RouteManager !== 'undefined') RouteManager.onMission(mission);
        } catch (err) {
            LogSystem.add('ERROR', `Error al ejecutar rutina: ${err.message}`);
        }
    }
};

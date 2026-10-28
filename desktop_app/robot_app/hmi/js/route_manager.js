// @ts-check
/** @typedef {import('@frontend/protocol').Telemetry} TelemetryPayload */
/** @typedef {import('@frontend/protocol').CommandName} CommandName */

/* ==============================================
   ROUTE MANAGER (Facade Coordinator & Form Orchestration)
   ============================================== */
const RouteManager = {
    queue: [],
    logicalSteps: [],
    telemetryTrail: [],
    currentOrigin: { x: 0, y: 0 },
    isRunning: false,
    routeStartedAt: 0,
    currentStepIndex: 0,
    waitingForMovement: false,
    moveTimeout: null,
    activeCommandId: null,
    rectangularCeroAutomatico: null,
    lockedMode: null,
    nextLogicalStepId: 1,
    vectorialEnabled: false,
    /** @type {TelemetryPayload | null} */
    lastRobotTelemetry: null,
    /** @type {HTMLElement | null} */
    preview: null,
    forks: new Map(),
    activeBranch: 'main',
    comparingAB: false,
    nextForkId: 1,
    mainBranchBackup: null,
    isOckhamSimulating: false,

    init() {
        this._bindDOMElements();
        RouteCharts.init(this);
        this._bindRouteNameUI();
        this._bindModeAndAxisControls();
        this._bindActionButtons();
        this._bindForkToolbar();
        RouteForksManager.loadFromStorage(this);
        this.updateLabels();
        this.actualizarRestriccionInputs();
    },

    _bindDOMElements() {
        const g = id => document.getElementById(id);
        this.modeSelect = /** @type {HTMLSelectElement} */ (g('route-mode')); this.list = g('route-list');
        this.input1 = /** @type {HTMLInputElement} */ (g('route-input-1')); this.input2 = /** @type {HTMLInputElement} */ (g('route-input-2'));
        this.lbl1 = g('route-label-1'); this.lbl2 = g('route-label-2');
        this.axisDefault = /** @type {HTMLInputElement} */ (g('route-axis-default')); this.axisDefaultGroup = g('route-axis-default-group');
        this.preview = g('route-polar-preview'); this.planCanvas = /** @type {HTMLCanvasElement} */ (g('route-plan-chart'));
        this.angularMeta = g('route-angular-meta'); this.angularSummary = g('route-angular-summary');
        this.angularXLabel = g('route-angular-x-label'); this.angularYLabel = g('route-angular-y-label');
        this.vectorialSafety = g('route-vectorial-safety'); this.vectorialToggle = /** @type {HTMLInputElement} */ (g('route-vectorial-enabled'));
        this.vectorialStatus = g('route-vectorial-status'); this.modeChip = g('route-mode-chip');
    },

    _bindRouteNameUI() {
        this.routeNameInput = /** @type {HTMLInputElement | null} */ (document.getElementById('route-name-input'));
        const btnRename = document.getElementById('btn-rename-route'), btnSaveName = document.getElementById('btn-save-route-name');
        this.mainRouteName = localStorage.getItem('hmi_main_route_name') || 'Ruta Planificada Robot_A';
        if (this.routeNameInput) {
            this.routeNameInput.value = this.mainRouteName;
            this.routeNameInput.addEventListener('change', () => this.saveCurrentRouteName());
            this.routeNameInput.addEventListener('keydown', (e) => {
                if (e.key === 'Enter') { e.preventDefault(); this.saveCurrentRouteName(); this.routeNameInput.blur(); }
            });
        }
        if (btnRename) btnRename.addEventListener('click', () => { this.routeNameInput?.focus(); this.routeNameInput?.select(); });
        if (btnSaveName) btnSaveName.addEventListener('click', () => this.saveCurrentRouteName());
    },

    _bindModeAndAxisControls() {
        this.modeSelect.addEventListener('change', (e) => {
            const val = /** @type {HTMLSelectElement} */ (e.target).value;
            const isRect = val === 'rect' || val === 'RECTANGULAR_XY';
            if (this.axisDefault) { if (isRect) this.axisDefault.checked = true; else this.axisDefault.disabled = false; }
            this.actualizarRestriccionInputs(isRect);
            this.updateLabels();
            this.updatePreview();
        });

        this.vectorialToggle.addEventListener('change', async () => {
            const req = this.vectorialToggle.checked;
            this.vectorialToggle.disabled = true;
            try {
                const config = await WSControl.api('/api/v1/config/robot', {
                    method: 'PUT', body: JSON.stringify({ experimental_vectorial_routes: req })
                });
                this.applyConfig(config);
                LogSystem.add(req ? 'WARN' : 'INFO', req ? 'Ejecución angular vectorial experimental habilitada.' : 'Ejecución angular vectorial deshabilitada.');
            } catch (err) {
                this.vectorialToggle.checked = this.vectorialEnabled;
                LogSystem.add('ERROR', err.message);
            } finally {
                this.vectorialToggle.disabled = this.isRunning;
                this.renderVectorialSafety();
            }
        });

        this.axisDefault.addEventListener('change', () => { this.rectangularCeroAutomatico = null; this.actualizarRestriccionInputs(); });
        const onInput = input => { this.completarEjeRectangular(input); this.actualizarRestriccionInputs(); this.updatePreview(); };
        this.input1.addEventListener('input', () => onInput(this.input1));
        this.input2.addEventListener('input', () => onInput(this.input2));
    },

    _bindActionButtons() {
        const on = (id, evt, fn) => document.getElementById(id)?.addEventListener(evt, fn);
        on('route-form', 'submit', (e) => { e.preventDefault(); this.addStep(); });
        [['btn-route-start', () => this.start()], ['btn-route-preview', () => this.toggleCinema('card-plan-map')],
         ['btn-route-pop', () => this.removeLastStep()], ['btn-route-stop', () => this.stopAndReset()],
         ['btn-route-discard', () => this.stopAndReset()], ['btn-route-ockham', () => this.ockhamReturn()],
         ['btn-confirm-ockham', () => this.confirmOckhamReturn()], ['btn-cancel-ockham', () => this.cancelOckhamReturn()],
         ['btn-route-clear-memory', () => this.clearRobotMemory()]
        ].forEach(([id, fn]) => on(id, 'click', fn));
    },

    _bindForkToolbar() {
        const on = (id, evt, fn) => document.getElementById(id)?.addEventListener(evt, fn);
        on('btn-fork-route', 'click', () => this.forkCurrentRoute());
        on('route-branch-select', 'change', (e) => this.switchBranch(/** @type {HTMLSelectElement} */ (e.target).value));
        on('btn-compare-branches', 'click', () => this.toggleCompareAB());
        on('btn-delete-branch', 'click', () => this.deleteBranch(this.activeBranch));
    },

    activeMode() {
        return ROUTE_PLANNING_MODES[this.lockedMode || this.modeSelect.value] || ROUTE_PLANNING_MODES.rect;
    },

    logicalEndpoint() {
        return this.logicalSteps.length ? this.logicalSteps[this.logicalSteps.length - 1].end : this.currentOrigin;
    },

    updateModeLock() {
        this.lockedMode = this.logicalSteps.length ? this.logicalSteps[0].mode : null;
        this.modeSelect.disabled = Boolean(this.lockedMode);
        this.modeSelect.title = this.lockedMode ? 'Vacía la ruta para cambiar el modo de entrada.' : '';
        if (this.lockedMode) this.modeSelect.value = this.lockedMode;
    },

    drawPlan() { RouteCharts.drawPlan(this); }, updateTelemetryMap(x, y, h) { RouteCharts.updateTelemetryMap(this, x, y, h); },
    autoScale() { RouteCharts.autoScale(this); }, resizeCharts() { RouteCharts.resizeCharts(this); },
    toggleCinema(cardId) { RouteCharts.toggleCinema(this, cardId); },
    _syncCinemaButton(cardId, isCinema) { RouteCharts.syncCinemaButton(cardId, isCinema); },

    clearTelemetryTrail() {
        this.telemetryTrail = [];
        this.renderMetrics();
        if (this.telemChart) {
            this.telemChart.data.datasets[0].data = [];
            this.telemChart.update('none');
        }
    },
    renderMetrics() {
        const dur = this.routeStartedAt ? ((Date.now() - this.routeStartedAt) / 1000) : 0, n = this.telemetryTrail.length;
        const ep = this.logicalEndpoint(), act = this.telemetryTrail[n - 1] || { x: state.telemetria.posX, y: state.telemetria.posY };
        const dev = (this.isRunning || n) ? Math.hypot(act.x - (ep.x - this.currentOrigin.x), act.y - (ep.y - this.currentOrigin.y)) : 0;
        document.getElementById('route-metric-duration').textContent = `${dur.toFixed(1)} s`;
        document.getElementById('route-metric-samples').textContent = String(n);
        document.getElementById('route-metric-segments').textContent = `${this.queue.length} / 256`;
        document.getElementById('route-metric-deviation').textContent = `${dev.toFixed(1)} cm`;
    },
    applyCapabilities(capabilities) {
        const v = capabilities?.angular_vectorial || {};
        if (typeof v.enabled === 'boolean') this.applyConfig({ experimental_vectorial_routes: v.enabled });
    },
    applyConfig(config) {
        if (typeof config?.experimental_vectorial_routes === 'boolean') {
            this.vectorialEnabled = config.experimental_vectorial_routes;
            if (this.vectorialToggle) this.vectorialToggle.checked = this.vectorialEnabled;
            this.renderVectorialSafety();
        }
    },
    /** @param {TelemetryPayload} raw */
    applyVectorialTelemetry(raw) {
        this.lastRobotTelemetry = raw || null;
        this.renderVectorialSafety();
    },

    vectorialReadiness() {
        return ROUTE_PLANNING_MODES.vectorial.checkVectorialReadiness(this.lastRobotTelemetry, this.vectorialEnabled);
    },

    renderVectorialSafety() {
        if (!this.vectorialStatus) return;
        const r = this.vectorialReadiness(), raw = this.lastRobotTelemetry, h = raw?.encoder_health || {};
        const hText = ['fl', 'fr', 'bl', 'br'].map(w => `${w.toUpperCase()}=${h[w] || 'unknown'}`).join(' · ');
        this.vectorialStatus.textContent = `${r.reason} ${hText}`;
        this.vectorialStatus.classList.toggle('ready', r.ready);
        this.vectorialToggle.checked = this.vectorialEnabled;
        this.vectorialToggle.disabled = this.isRunning;
    },

    updateLabels() {
        this.activeMode().configureForm(this);
        this.drawPlan();
    },

    completarEjeRectangular(campo) {
        if (this.modeSelect.value !== 'rect' || !this.axisDefault.checked) return;
        const val = campo.value.trim();
        if (!val || val === '-') {
            if (this.rectangularCeroAutomatico === campo.id) this.rectangularCeroAutomatico = null;
            return;
        }
        const other = campo === this.input1 ? this.input2 : this.input1;
        if (!other.value.trim() || this.rectangularCeroAutomatico === campo.id) {
            other.value = '0';
            this.rectangularCeroAutomatico = other.id;
        }
    },

    actualizarRestriccionInputs(activar = null) {
        if (!this.input1 || !this.input2) return;
        const sign1 = document.getElementById('route-sign-1'), sign2 = document.getElementById('route-sign-2');
        const isRect = this.modeSelect?.value === 'rect' || this.modeSelect?.value === 'RECTANGULAR_XY';
        const isLocked = activar === false ? false : (activar === true || (isRect && Boolean(this.axisDefault && this.axisDefault.checked)));

        if (!isLocked) {
            this._setInputLock(this.input1, sign1, false);
            this._setInputLock(this.input2, sign2, false);
            return;
        }
        const val1 = this.input1.value.trim(), val2 = this.input2.value.trim();
        const has1 = val1 !== '' && val1 !== '-' && Math.abs(Number.parseFloat(val1) || 0) > 0.001;
        const has2 = val2 !== '' && val2 !== '-' && Math.abs(Number.parseFloat(val2) || 0) > 0.001;

        if (has1 && !has2) {
            this.input2.value = '0';
            this._setInputLock(this.input2, sign2, true);
            this._setInputLock(this.input1, sign1, false);
        } else if (has2 && !has1) {
            this.input1.value = '0';
            this._setInputLock(this.input1, sign1, true);
            this._setInputLock(this.input2, sign2, false);
        } else {
            this._setInputLock(this.input1, sign1, false);
            this._setInputLock(this.input2, sign2, false);
        }
    },

    _setInputLock(input, signEl, locked) {
        const op = locked ? '0.5' : '1', pe = locked ? 'none' : '';
        input.style.opacity = op; input.style.pointerEvents = pe;
        if (signEl) { signEl.style.opacity = op; signEl.style.pointerEvents = pe; }
    },
    saveCurrentRouteName() {
        if (!this.routeNameInput) return;
        const newName = this.routeNameInput.value.trim() || 'Ruta Planificada Robot_A';
        this.routeNameInput.value = newName;
        if (this.activeBranch === 'main') {
            this.mainRouteName = newName;
            try { localStorage.setItem('hmi_main_route_name', newName); } catch (_) {}
        } else if (this.forks.has(this.activeBranch)) {
            const fork = this.forks.get(this.activeBranch);
            fork.name = newName;
            RouteForksManager.saveToStorage(this);
        }
        RouteForksManager.updateBranchSelector(this);
        if (typeof LogSystem !== 'undefined') LogSystem.add('INFO', `Nombre de ruta activo: '${newName}'.`);
    },
    updatePreview() { this.activeMode().updatePreview(this); },
    addStep() {
        if (this.isRunning) return LogSystem.add('WARN', 'Ruta en ejecución. Detén primero.');
        const v1 = Number.parseFloat(this.input1.value), v2 = Number.parseFloat(this.input2.value);
        if (this.logicalSteps.length === 0) {
            this.currentOrigin = { x: state.telemetria.posX, y: state.telemetria.posY };
        }
        try {
            const mode = this.activeMode();
            const step = mode.createLogicalStep(this, v1, v2);
            if (this.logicalSteps.length === 0) this.clearTelemetryTrail();
            const qBefore = this.queue.length;
            mode.expandExecutionWaypoints(this, step);
            if (this.queue.length > 256) {
                this.queue.length = qBefore;
                throw new Error('La ruta excedería el máximo de 256 pasos ejecutables.');
            }
            this.logicalSteps.push(step);
            this.updateModeLock();
            LogSystem.add('INFO', step.execution.length
                ? `Paso lógico agregado: ${step.description}`
                : `Orientación visual agregada: ${step.description}; no se enviará movimiento.`);
        } catch (err) {
            LogSystem.add('ERROR', err.message);
            return;
        }
        this.input1.value = ''; this.input2.value = ''; this.rectangularCeroAutomatico = null;
        this.actualizarRestriccionInputs();
        RouteForksManager.syncActiveBranch(this);
        this.updateListUI(); this.updateLabels(); this.updatePreview(); this.autoScale();
    },

    removeLastStep() {
        if (this.isRunning) return LogSystem.add('WARN', 'No se puede modificar una ruta en ejecución.');
        if (this.logicalSteps.length === 0) return LogSystem.add('WARN', 'No hay pasos para deshacer.');
        const removed = this.logicalSteps.pop();
        this.queue = this.queue.filter(wp => wp.logicalStepId !== removed.id);
        this.currentStepIndex = 0;
        if (this.logicalSteps.length === 0) {
            this.currentOrigin = { x: state.telemetria.posX, y: state.telemetria.posY };
        }
        RouteForksManager.syncActiveBranch(this);
        this.updateModeLock(); this.updateListUI(); this.updateLabels(); this.updatePreview(); this.autoScale();
        this.actualizarRestriccionInputs();
        LogSystem.add('INFO', `Paso lógico eliminado: ${removed.description}`);
    },

    appendLimited(x0, y0, x1, y1, label, logicalStepId, component) {
        const dist = Math.hypot(x1 - x0, y1 - y0);
        const count = Math.max(1, Math.ceil(dist / 200));
        for (let i = 1; i <= count; i++) {
            const x = x0 + (x1 - x0) * i / count, y = y0 + (y1 - y0) * i / count;
            this.queue.push({
                x, y, logicalStepId, component,
                desc: `${label}${count > 1 ? ` ${i}/${count}` : ''} a (${(x - this.currentOrigin.x).toFixed(2)}, ${(y - this.currentOrigin.y).toFixed(2)})`
            });
        }
    },

    updateListUI() {
        if (this.queue.length === 0) {
            const msg = this.logicalSteps.length ? 'Sólo hay orientaciones visuales; no existe desplazamiento para enviar.' : 'La ruta está vacía. Agrega puntos arriba.';
            this.list.innerHTML = `<tr><td colspan="5" style="color:var(--text-muted)">${msg}</td></tr>`;
            return;
        }
        this.list.innerHTML = this.queue.map((wp, i) => {
            const isPast = i < this.currentStepIndex, isCur = i === this.currentStepIndex && this.isRunning;
            const rowClass = isPast ? 'route-step-done' : (isCur ? 'route-step-current' : '');
            const status = isPast ? 'Completado' : (isCur ? 'En curso' : 'Pendiente');
            const statusClass = isPast ? 'done' : (isCur ? 'current' : 'pending');
            return `<tr class="${rowClass}"><td>${i + 1}</td><td>${wp.desc}</td><td>${wp.x.toFixed(2)}</td><td>${wp.y.toFixed(2)}</td><td><span class="route-status ${statusClass}">${status}</span></td></tr>`;
        }).join('');
    },

    forkCurrentRoute(name) { RouteForksManager.fork(this, name); }, switchBranch(key) { RouteForksManager.switchBranch(this, key); },
    deleteBranch(key) { RouteForksManager.deleteBranch(this, key); }, toggleCompareAB() { RouteForksManager.toggleCompareAB(this); },
    _syncActiveBranch() { RouteForksManager.syncActiveBranch(this); }, _updateBranchSelector() { RouteForksManager.updateBranchSelector(this); },
    _saveForksToStorage() { RouteForksManager.saveToStorage(this); }, _loadForksFromStorage() { RouteForksManager.loadFromStorage(this); },

    lockTabs() { RouteLifecycle.lockTabs(this); },
    unlockTabs() { RouteLifecycle.unlockTabs(this); },
    start() { return RouteLifecycle.start(this); },
    _persistRouteHistory(name, mode) { return RouteLifecycle._persistRouteHistory(this, name, mode); },

    onRobotEvent(event) {},

    onMission(mission) {
        this.currentStepIndex = mission.current_index || 0;
        this.activeCommandId = mission.active_command_id || null;
        this.waitingForMovement = Boolean(mission.running);

        if (mission.blocked && mission.error) {
            const interrupted = mission.error === 'robot_restarted_mid_mission';
            const safety = document.getElementById('route-mission-safety');
            if (safety) {
                safety.textContent = interrupted
                    ? `MISIÓN INTERRUMPIDA en el paso ${this.currentStepIndex + 1}. El ESP32 se reinició: el paso no será repetido. Borra la misión, recentra pose y decide una ruta nueva.`
                    : `Misión bloqueada: ${mission.error}`;
                safety.style.color = 'var(--danger)';
            }
            return this.failRoute(interrupted ? 'robot_restarted_mid_mission: repetición automática bloqueada' : mission.error);
        }

        RouteOckhamCockpit.onMissionUpdate(this, mission);

        if (this.isRunning && !mission.running) {
            this.isRunning = false;
            this.unlockTabs();
            clearTimeout(this.moveTimeout);
            LogSystem.add('INFO', 'Ruta completada por el carrito.');
            if (mission.kind === 'ockham_return') {
                LogSystem.add('INFO', 'Regreso de Ockham y alineación final a 0° completados.');
                const stageLabel = document.getElementById('ockham-current-stage');
                if (stageLabel) stageLabel.textContent = 'Ockham: Completado';
            }
        }
        this.renderVectorialSafety();
        this.updateListUI();
        this.drawPlan();
    },

    onFsmIdle() {},
    failRoute(reason) { return RouteLifecycle.failRoute(this, reason); },
    stopAndReset() { return RouteLifecycle.stopAndReset(this); },
    clearRobotMemory() { return RouteLifecycle.clearRobotMemory(this); },
    async ockhamReturn() { await RouteOckhamCockpit.ockhamReturn(this); },
    async confirmOckhamReturn() { await RouteOckhamCockpit.confirmOckhamReturn(this); },
    cancelOckhamReturn() { RouteOckhamCockpit.cancelOckhamReturn(this); }
};

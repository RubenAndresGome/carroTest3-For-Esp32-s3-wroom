// @ts-check
/** @typedef {import('@frontend/protocol').Telemetry} TelemetryPayload */
/** @typedef {import('@frontend/protocol').CommandName} CommandName */

/* ==============================================
   SURFACE FRICTION MANAGER (SQLite REST API & Calibration Presets)
   ============================================== */

const SurfaceManager = {
    surfaces: [],
    activeSurfaceId: null,
    pendingAutoSaveName: null,

    init() {
        this._bindToggles();
        this._bindAutoName();
        this._bindActions();
        this.loadSurfaces();
    },

    _bindToggles() {
        const toggleBtn = document.getElementById('btn-toggle-surface-manager');
        const form = document.getElementById('surface-manager-form');
        if (toggleBtn && form) {
            toggleBtn.addEventListener('click', () => {
                const isHidden = form.style.display === 'none' || !form.style.display;
                form.style.display = isHidden ? 'block' : 'none';
                toggleBtn.textContent = isHidden ? '✕ Cerrar Editor' : 'Modo Experto (PWM)';
            });
        }

        const toggleCalBtn = document.getElementById('btn-toggle-cal-diagnostics');
        const calPanel = document.getElementById('cal-physical-panel');
        if (toggleCalBtn && calPanel) {
            toggleCalBtn.addEventListener('click', () => {
                const isHidden = calPanel.style.display === 'none' || !calPanel.style.display;
                calPanel.style.display = isHidden ? 'block' : 'none';
                toggleCalBtn.textContent = isHidden ? '✕ Ocultar Diagnósticos' : 'Diagnóstico MPU/PCNT';
            });
        }
    },

    _bindAutoName() {
        const autoNameInput = /** @type {HTMLInputElement | null} */ (document.getElementById('surface-auto-name'));
        const calibrateLabel = document.getElementById('btn-calibrate-label');
        if (autoNameInput && calibrateLabel) {
            autoNameInput.addEventListener('input', () => {
                const val = autoNameInput.value.trim();
                calibrateLabel.textContent = val
                    ? `Calibrar y Registrar "${val}"`
                    : 'Calibrar físicamente (Giro ±25°)';
            });
        }
    },

    _bindActions() {
        const saveLiveBtn = document.getElementById('btn-save-current-as-surface');
        if (saveLiveBtn) saveLiveBtn.addEventListener('click', () => this.saveCurrentLiveSurface());

        const select = /** @type {HTMLSelectElement | null} */ (document.getElementById('select-surface'));
        if (select) select.addEventListener('change', () => this.onSurfaceSelect(select.value));

        const applyBtn = document.getElementById('btn-apply-surface');
        if (applyBtn) applyBtn.addEventListener('click', () => this.applySelected());

        const saveBtn = document.getElementById('btn-save-surface');
        if (saveBtn) saveBtn.addEventListener('click', () => this.saveCurrent());

        const captureBtn = document.getElementById('btn-capture-surface');
        if (captureBtn) captureBtn.addEventListener('click', () => this.captureFromTelemetry());

        const deleteBtn = document.getElementById('btn-delete-surface');
        if (deleteBtn) deleteBtn.addEventListener('click', () => this.deleteSelected());
    },

    async loadSurfaces(selectId = null) {
        if (!WSControl.flaskMode) return;
        try {
            const data = await WSControl.api('/api/v1/calibration/surfaces');
            this.surfaces = Array.isArray(data) ? data : [];
            const select = /** @type {HTMLSelectElement | null} */ (document.getElementById('select-surface'));
            if (!select) return;

            select.innerHTML = '';
            if (this.surfaces.length === 0) {
                const opt = document.createElement('option');
                opt.value = '';
                opt.textContent = 'Sin superficies guardadas';
                select.appendChild(opt);
                return;
            }

            this.surfaces.forEach(s => {
                const opt = document.createElement('option');
                opt.value = s.id;
                const pwmP = s.pwm_pos ?? s.pwm_positive_8bit ?? 155;
                const pwmN = s.pwm_neg ?? s.pwm_negative_8bit ?? 155;
                opt.textContent = `${s.name} (PWM: ${pwmP}/${pwmN})`;
                select.appendChild(opt);
            });

            const targetId = selectId || this.activeSurfaceId || this.surfaces[0].id;
            select.value = targetId;
            this.onSurfaceSelect(select.value);
        } catch (err) {
            LogSystem.add('ERROR', `Error cargando superficies: ${err.message}`);
        }
    },

    onSurfaceSelect(id) {
        const surface = this.surfaces.find(s => s.id === id);
        if (!surface) return;

        const nameInput = /** @type {HTMLInputElement | null} */ (document.getElementById('surface-name'));
        const descInput = /** @type {HTMLInputElement | null} */ (document.getElementById('surface-desc'));
        const pwmPosInput = /** @type {HTMLInputElement | null} */ (document.getElementById('surface-pwm-pos'));
        const pwmNegInput = /** @type {HTMLInputElement | null} */ (document.getElementById('surface-pwm-neg'));
        const candPosSelect = /** @type {HTMLSelectElement | null} */ (document.getElementById('surface-cand-pos'));
        const candNegSelect = /** @type {HTMLSelectElement | null} */ (document.getElementById('surface-cand-neg'));

        if (nameInput) nameInput.value = surface.name || '';
        if (descInput) descInput.value = surface.description || '';
        const pwmP = surface.pwm_pos ?? surface.pwm_positive_8bit ?? 155;
        const pwmN = surface.pwm_neg ?? surface.pwm_negative_8bit ?? 155;
        const candP = surface.cand_pos ?? surface.positive_polarity ?? 1;
        const candN = surface.cand_neg ?? surface.negative_polarity ?? -1;

        if (pwmPosInput) pwmPosInput.value = String(pwmP);
        if (pwmNegInput) pwmNegInput.value = String(pwmN);
        if (candPosSelect) candPosSelect.value = String(candP);
        if (candNegSelect) candNegSelect.value = String(candN);
    },

    async applySelected() {
        if (['FALLO', 'ESTOP'].includes(Calibration.robotState || Calibration.lastRobotState)) {
            const msg = `El robot está en ${Calibration.robotState || Calibration.lastRobotState}. Presiona "Rearmar" antes de aplicar una superficie.`;
            OperatorFeedback.show('surface-alert', msg, 'warn');
            LogSystem.add('WARN', msg);
            return;
        }
        const select = /** @type {HTMLSelectElement | null} */ (document.getElementById('select-surface'));
        const id = select?.value;
        if (!id) {
            OperatorFeedback.show('surface-alert', 'Selecciona una superficie primero.', 'warn');
            return LogSystem.add('WARN', 'Selecciona una superficie primero.');
        }

        OperatorFeedback.clear('surface-alert');
        const applyBtn = /** @type {HTMLButtonElement | null} */ (document.getElementById('btn-apply-surface'));
        if (applyBtn) applyBtn.disabled = true;

        try {
            LogSystem.add('INFO', `Inyectando superficie ${id} al robot...`);
            const result = await WSControl.api(`/api/v1/calibration/surfaces/${encodeURIComponent(id)}/apply`, {
                method: 'POST', body: '{}'
            });
            this.activeSurfaceId = id;
            this._updateActiveSurfaceBadge(result.surface);

            Calibration.commandId = result.command_id;
            Calibration.applyingSurface = true;
            clearTimeout(Calibration.applySurfaceTimeout);
            Calibration.applySurfaceTimeout = setTimeout(() => {
                Calibration.applyingSurface = false;
                Calibration.updateAvailability();
            }, 3500);

            Calibration.setState('LISTO — SUPERFICIE APLICADA', true);
            LogSystem.add('INFO', `Superficie "${result.surface?.name || id}" aplicada y calibrada en ESP32.`);
        } catch (err) {
            const msg = `Fallo al aplicar superficie: ${humanizeError(err.message)}`;
            OperatorFeedback.show('surface-alert', msg, 'error');
            LogSystem.add('ERROR', msg);
        } finally {
            if (applyBtn) applyBtn.disabled = false;
        }
    },

    _updateActiveSurfaceBadge(surface) {
        const info = document.getElementById('surface-active-info');
        if (info && surface) {
            const pP = surface.pwm_pos ?? surface.pwm_positive_8bit ?? 155;
            const pN = surface.pwm_neg ?? surface.pwm_negative_8bit ?? 155;
            info.textContent = `· Perfil: ${surface.name} (PWM ${pP}/${pN})`;
        }
    },

    async saveCurrentLiveSurface() {
        const autoNameInput = /** @type {HTMLInputElement | null} */ (document.getElementById('surface-auto-name'));
        const name = autoNameInput?.value?.trim();
        if (!name) {
            OperatorFeedback.show('surface-alert', 'Escribe el nombre del piso antes de guardar.', 'warn');
            LogSystem.add('WARN', 'Escribe el nombre del piso antes de guardar.');
            if (autoNameInput) autoNameInput.focus();
            return;
        }
        OperatorFeedback.clear('surface-alert');
        const { pwmPos, pwmNeg } = this._extractPwmFromTelemetry();
        try {
            const saved = await WSControl.api('/api/v1/calibration/surfaces', {
                method: 'POST',
                body: JSON.stringify({
                    name, description: 'Guardado desde calibración activa',
                    pwm_pos: pwmPos, pwm_neg: pwmNeg, cand_pos: 1, cand_neg: -1
                })
            });
            LogSystem.add('INFO', `Superficie "${saved.name}" registrada en SQLite (PWM: ${pwmPos}/${pwmNeg}).`);
            if (autoNameInput) autoNameInput.value = '';
            const calibrateLabel = document.getElementById('btn-calibrate-label');
            if (calibrateLabel) calibrateLabel.textContent = 'Calibrar físicamente (Giro ±25°)';
            await this.loadSurfaces(saved.id);
        } catch (err) {
            const msg = `Error guardando superficie: ${humanizeError(err.message)}`;
            OperatorFeedback.show('surface-alert', msg, 'error');
            LogSystem.add('ERROR', msg);
        }
    },

    async handleCalibrationCompleted() {
        const name = this.pendingAutoSaveName;
        this.pendingAutoSaveName = null;
        if (!name) return;

        const { pwmPos, pwmNeg } = this._extractPwmFromTelemetry();
        try {
            const saved = await WSControl.api('/api/v1/calibration/surfaces', {
                method: 'POST',
                body: JSON.stringify({
                    name, description: 'Calibrado físicamente por el robot',
                    pwm_pos: pwmPos, pwm_neg: pwmNeg, cand_pos: 1, cand_neg: -1
                })
            });
            LogSystem.add('INFO', `Calibración finalizada: Superficie "${saved.name}" registrada en SQLite (PWM: ${pwmPos}/${pwmNeg}).`);
            const autoNameInput = /** @type {HTMLInputElement | null} */ (document.getElementById('surface-auto-name'));
            if (autoNameInput) autoNameInput.value = '';
            const calibrateLabel = document.getElementById('btn-calibrate-label');
            if (calibrateLabel) calibrateLabel.textContent = 'Calibrar físicamente (Giro ±25°)';
            await this.loadSurfaces(saved.id);
        } catch (err) {
            LogSystem.add('ERROR', `Error registrando calibración automática en SQLite: ${humanizeError(err.message)}`);
        }
    },

    _extractPwmFromTelemetry() {
        const raw = Calibration.lastRaw || {};
        const history = raw.torque_history || {};
        const diag = raw.calibration_diagnostics || {};
        const pwmPos = Math.max(140, Math.min(247, Number(history.base_positive_left_8bit ?? history.base_positive_8bit ?? diag.pwm_8bit ?? 155)));
        const pwmNeg = Math.max(140, Math.min(247, Number(history.base_negative_left_8bit ?? history.base_negative_8bit ?? 155)));
        return { pwmPos, pwmNeg };
    },

    async saveCurrent() {
        const name = (/** @type {HTMLInputElement | null} */ (document.getElementById('surface-name')))?.value?.trim();
        const desc = (/** @type {HTMLInputElement | null} */ (document.getElementById('surface-desc')))?.value?.trim() || '';
        const pwmPos = Number((/** @type {HTMLInputElement | null} */ (document.getElementById('surface-pwm-pos')))?.value);
        const pwmNeg = Number((/** @type {HTMLInputElement | null} */ (document.getElementById('surface-pwm-neg')))?.value);
        const candPos = Number((/** @type {HTMLSelectElement | null} */ (document.getElementById('surface-cand-pos')))?.value);
        const candNeg = Number((/** @type {HTMLSelectElement | null} */ (document.getElementById('surface-cand-neg')))?.value);

        if (!name) {
            OperatorFeedback.show('surface-alert', 'El nombre de la superficie es obligatorio.', 'warn');
            return LogSystem.add('WARN', 'El nombre de la superficie es obligatorio.');
        }
        if (!Number.isFinite(pwmPos) || pwmPos < 140 || pwmPos > 247 || !Number.isFinite(pwmNeg) || pwmNeg < 140 || pwmNeg > 247) {
            OperatorFeedback.show('surface-alert', 'PWM debe estar entre 140 y 247.', 'warn');
            return LogSystem.add('WARN', 'PWM debe estar entre 140 y 247.');
        }
        if (candPos === candNeg || ![-1, 1].includes(candPos) || ![-1, 1].includes(candNeg)) {
            OperatorFeedback.show('surface-alert', 'Las polaridades deben ser opuestas (+1 y -1).', 'warn');
            return LogSystem.add('WARN', 'Las polaridades deben ser opuestas (+1 y -1).');
        }

        OperatorFeedback.clear('surface-alert');
        try {
            const saved = await WSControl.api('/api/v1/calibration/surfaces', {
                method: 'POST',
                body: JSON.stringify({ name, description: desc, pwm_pos: pwmPos, pwm_neg: pwmNeg, cand_pos: candPos, cand_neg: candNeg })
            });
            LogSystem.add('INFO', `Superficie guardada en SQLite: ${saved.name} (ID: ${saved.id})`);
            await this.loadSurfaces(saved.id);
        } catch (err) {
            const msg = `Error guardando superficie: ${humanizeError(err.message)}`;
            OperatorFeedback.show('surface-alert', msg, 'error');
            LogSystem.add('ERROR', msg);
        }
    },

    captureFromTelemetry() {
        const { pwmPos, pwmNeg } = this._extractPwmFromTelemetry();
        const pwmPosInput = /** @type {HTMLInputElement | null} */ (document.getElementById('surface-pwm-pos'));
        const pwmNegInput = /** @type {HTMLInputElement | null} */ (document.getElementById('surface-pwm-neg'));
        if (pwmPosInput) pwmPosInput.value = String(pwmPos);
        if (pwmNegInput) pwmNegInput.value = String(pwmNeg);

        const nameInput = /** @type {HTMLInputElement | null} */ (document.getElementById('surface-name'));
        if (nameInput && !nameInput.value) {
            nameInput.value = `Captura ${new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })}`;
        }
        LogSystem.add('INFO', `Valores capturados de telemetría: PWM+ ${pwmPos}, PWM− ${pwmNeg}`);
    },

    async deleteSelected() {
        const select = /** @type {HTMLSelectElement | null} */ (document.getElementById('select-surface'));
        const id = select?.value;
        if (!id) {
            OperatorFeedback.show('surface-alert', 'Selecciona una superficie para borrar.', 'warn');
            return LogSystem.add('WARN', 'Selecciona una superficie para borrar.');
        }

        const surface = this.surfaces.find(s => s.id === id);
        const name = surface ? surface.name : id;
        if (!confirm(`¿Eliminar la superficie "${name}" de SQLite?`)) return;

        OperatorFeedback.clear('surface-alert');
        try {
            await WSControl.api(`/api/v1/calibration/surfaces/${encodeURIComponent(id)}`, { method: 'DELETE' });
            LogSystem.add('INFO', `Superficie "${name}" eliminada de SQLite.`);
            if (this.activeSurfaceId === id) {
                this.activeSurfaceId = null;
                const info = document.getElementById('surface-active-info');
                if (info) info.textContent = '· Perfil: —';
            }
            await this.loadSurfaces();
        } catch (err) {
            const msg = `Error eliminando superficie: ${humanizeError(err.message)}`;
            OperatorFeedback.show('surface-alert', msg, 'error');
            LogSystem.add('ERROR', msg);
        }
    }
};

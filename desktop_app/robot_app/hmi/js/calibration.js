// @ts-check
/** @typedef {import('@frontend/protocol').Telemetry} TelemetryPayload */
/** @typedef {import('@frontend/protocol').CommandName} CommandName */

/* ==============================================
   CALIBRATION (Canonical MPU6050 FSM & Diagnostics)
   ============================================== */
const Calibration = {
    calibrated: false, commandId: null, commandPending: false, rearmCommandId: null, rearmPending: false,
    selfTestCommandId: null, selfTestPassed: true, robotState: '', mpuReady: false, allowedCommands: null,
    applyingSurface: false, applySurfaceTimeout: null, lastRaw: {},
    lastTurnMode: 'PIVOT_CONTINUOUS', lastRequestedTurnMode: 'AUTO', lastPowerKey: '', lastRobotState: '',
    lastTerminalKey: '', lastBrakingPhase: 'OFF', lastAlignmentStage: 'OFF', lastDegradedMode: null,
    lastEncoderHealthKey: '', lastAntiFrictionKey: '',

    _bindCalibrateButton() {
        document.getElementById('btn-calibrate')?.addEventListener('click', async () => {
            if (!state.connected) return LogSystem.add('WARN', 'Conecta primero con el carrito.');
            if (!this.canCalibrate()) return LogSystem.add('WARN', this.calibrationBlockReason());

            const confirmed = OperatorFeedback.confirmMotion(
                '¿Iniciar la calibración física de giro ±25°? El robot rotará en el sitio. Confirma que el área está despejada.'
            );
            if (!confirmed) return;

            const desiredName = (/** @type {HTMLInputElement | null} */ (document.getElementById('surface-auto-name')))?.value?.trim() || null;
            if (typeof SurfaceManager !== 'undefined') SurfaceManager.pendingAutoSaveName = desiredName;
            this.commandPending = true; this.updateAvailability();
            const calPanel = document.getElementById('cal-physical-panel'), toggleCalBtn = document.getElementById('btn-toggle-cal-diagnostics');
            if (calPanel) calPanel.style.display = 'block';
            if (toggleCalBtn) toggleCalBtn.textContent = '✕ Ocultar Diagnósticos';

            if (typeof RouteManager !== 'undefined') {
                if (RouteManager.isRunning) RouteManager.failRoute('Calibración solicitada: ruta activa descartada.');
                RouteManager.queue = []; RouteManager.logicalSteps = []; RouteManager.currentStepIndex = 0;
                RouteManager.updateListUI(); RouteManager.drawPlan();
            }

            try {
                await WSControl.api('/api/v1/missions', { method: 'DELETE' }).catch(() => {});
                LogSystem.add('INFO', desiredName
                    ? `Iniciando calibración física para registrar "${desiredName}"...`
                    : 'Enviando comando de calibración al robot...');
                const result = await WSControl.send({ cmd: 'calibrate' });
                if (!result) {
                    this.commandPending = false;
                    if (typeof SurfaceManager !== 'undefined') SurfaceManager.pendingAutoSaveName = null;
                    this.updateAvailability();
                    return;
                }
                this.commandId = result.id;
                this.setState('CALIBRACIÓN SOLICITADA', false);
            } catch (err) {
                this.commandPending = false;
                if (typeof SurfaceManager !== 'undefined') SurfaceManager.pendingAutoSaveName = null;
                this.updateAvailability();
                LogSystem.add('ERROR', 'Error enviando calibracion: ' + humanizeError(err.message));
            }
        });
    },
    _bindClearFaultButton() {
        document.getElementById('btn-clear-fault')?.addEventListener('click', async () => {
            if (!this.canRearm()) return LogSystem.add('WARN', 'El rearme sólo está disponible conectado y en FALLO o ESTOP.');
            this.rearmPending = true;
            this.updateAvailability();
            LogSystem.add('CMD', `Solicitando rearme desde ${this.robotState}...`);
            const result = await WSControl.send({cmd:'clear_fault'});
            if (!result) { this.rearmPending = false; this.updateAvailability(); return; }
            this.rearmCommandId = result.id;
        });
    },
    init() {
        document.body.classList.add('calibration-locked');
        const calBtn = /** @type {HTMLButtonElement | null} */ (document.getElementById('btn-calibrate'));
        if (calBtn) calBtn.disabled = true;
        const selfBtn = /** @type {HTMLButtonElement | null} */ (document.getElementById('btn-self-test'));
        if (selfBtn) selfBtn.disabled = true;
        const selfState = document.getElementById('self-test-state');
        if (selfState) selfState.textContent = 'INTEGRADA EN FIRMWARE';
        this._bindCalibrateButton();
        this._bindClearFaultButton();
    },
    normalizeRobotState(value) {
        const current = String(value || '').trim().toUpperCase();
        return ['FAULT', 'SAFE_STOP'].includes(current) ? 'FALLO' : current;
    },
    hasAllowed(command) { return !Array.isArray(this.allowedCommands) || this.allowedCommands.includes(command); },
    canCalibrate() {
        return state.connected && ['DESARMADO','LISTO'].includes(this.robotState) && this.mpuReady &&
            !this.commandPending && !this.rearmPending && this.hasAllowed('calibrate');
    },
    canRearm() {
        return state.connected && ['FALLO','ESTOP'].includes(this.robotState) && !this.rearmPending && this.hasAllowed('clear_fault');
    },

    mpuStatus(raw) {
        const mpu = raw && typeof raw.mpu === 'object' && raw.mpu ? raw.mpu : {};
        const present = raw.mpu_present ?? mpu.present;
        const stale = raw.mpu_stale ?? mpu.stale;
        const calibrated = raw.mpu_calibrated ?? mpu.calibrated;
        const i2cOk = raw.i2c_ok ?? mpu.i2c_ok;
        if (present !== true) return { ready: false, reason: present === false ? 'MPU no detectada.' : 'Esperando diagnóstico de la MPU.' };
        if (i2cOk === false) return { ready: false, reason: 'Bus I²C de la MPU con fallo.' };
        if (stale !== false) return { ready: false, reason: stale === true ? 'Telemetría MPU obsoleta.' : 'Esperando vigencia de la MPU.' };
        if (calibrated !== true) return { ready: false, reason: 'MPU presente, pero aún no está calibrada.' };
        return { ready: true, reason: 'MPU apta para calibración física.' };
    },

    calibrationBlockReason() {
        if (!state.connected) return 'Conecta primero con el carrito.';
        if (this.commandPending) return 'Ya existe un comando de calibración pendiente.';
        if (this.rearmPending) return 'Espera a que termine el rearme.';
        if (['FALLO','ESTOP'].includes(this.robotState)) return `El robot está en ${this.robotState}; rearma el sistema antes de calibrar.`;
        if (!['DESARMADO','LISTO'].includes(this.robotState)) return `Calibración no permitida durante ${this.robotState || 'un estado desconocido'}.`;
        const mpu = this.mpuStatus(this.lastRaw || {});
        if (!mpu.ready) return mpu.reason;
        const pcnt = this.lastRaw && typeof this.lastRaw.pcnt_init === 'object' ? this.lastRaw.pcnt_init : {};
        if (pcnt.ready === false) return 'Inicialización PCNT incompleta; revisa el diagnóstico de encoders y reinicia.';
        if (!this.hasAllowed('calibrate')) return 'El firmware no habilita calibración en este instante.';
        return 'Calibración temporalmente no disponible.';
    },

    updateAvailability() {
        const cal = /** @type {HTMLButtonElement | null} */ (document.getElementById('btn-calibrate'));
        const rearm = /** @type {HTMLButtonElement | null} */ (document.getElementById('btn-clear-fault'));
        const applyBtn = /** @type {HTMLButtonElement | null} */ (document.getElementById('btn-apply-surface'));
        const faultState = ['FALLO','ESTOP'].includes(this.robotState);
        if (cal) {
            cal.disabled = !this.canCalibrate();
            cal.title = cal.disabled ? this.calibrationBlockReason() : 'Iniciar calibración física';
        }
        if (rearm) {
            rearm.hidden = !faultState;
            rearm.disabled = !this.canRearm();
        }
        const rearmLbl = document.getElementById('btn-clear-fault-label');
        if (rearmLbl) rearmLbl.textContent = this.rearmPending ? 'Rearmando…' : `Rearmar ${this.robotState || 'sistema'}`;
        if (applyBtn) {
            applyBtn.disabled = faultState || !this.mpuReady;
            applyBtn.title = faultState ? `El robot está en ${this.robotState}. Rearme el sistema antes de aplicar superficie.` : (!this.mpuReady ? 'MPU no listo para inyectar calibración' : 'Aplicar superficie seleccionada al robot');
        }
    },

    showCause(reason, visible) {
        const node = document.getElementById('calibration-cause');
        const pretty = reason ? humanizeError(reason) : 'sin causa reportada';
        if (node) { node.textContent = visible ? `Causa exacta: ${pretty}` : ''; node.classList.toggle('visible', Boolean(visible)); }
    },
    setState(label, ready, tone = '') {
        this.calibrated = ready;
        const node = document.getElementById('calibration-state');
        if (node) { node.textContent = label; node.classList.toggle('ready', ready); node.classList.toggle('fault', tone === 'fault'); node.classList.toggle('estop', tone === 'estop'); }
        document.body.classList.toggle('calibration-locked', !ready);
        this.updateAvailability();
    },
    valueAt(container, key, index, fallback = undefined) {
        if (Array.isArray(container)) return container[index] ?? fallback;
        if (container && typeof container === 'object') return container[key] ?? container[key.toUpperCase()] ?? fallback;
        return fallback;
    },

    renderDiagnostics(raw) {
        const diag = raw && typeof raw.calibration_diagnostics === 'object' && raw.calibration_diagnostics ? raw.calibration_diagnostics : {};
        this._renderPhaseAndRamp(diag, raw);
        this._renderHistory(raw);
        this._renderPcntInit(raw);
        this._renderWheelDeltas(diag);
        this._renderDiagnosticSides(diag);
    },
    _renderPhaseAndRamp(diag, raw) {
        const phase = diag.phase ?? raw.active_command_name ?? '—';
        const ramp = diag.ramp_level ?? diag.ramp_step ?? diag.pasoRampa, rampTotal = diag.ramp_level_count ?? diag.ramp_total ?? diag.pasosRampaTotal;
        const pwm10 = diag.pwm_10bit ?? diag.pwm_target ?? diag.pwmObjetivo;
        const pwm8 = diag.pwm_8bit ?? (Number.isFinite(Number(pwm10)) ? Math.round(Number(pwm10) * 255 / 1023) : undefined);
        const pwmL = diag.pwm_left_8bit, pwmR = diag.pwm_right_8bit;
        const att = diag.attempt ?? diag.retry_attempt, attMax = diag.attempt_max ?? diag.retry_attempt_max;
        document.getElementById('cal-diag-phase').textContent = String(phase || '—').toUpperCase();
        document.getElementById('cal-diag-ramp').textContent = ramp === undefined ? '—' : `${ramp}/${rampTotal ?? '—'}`;
        document.getElementById('cal-diag-pwm').textContent = (pwmL !== undefined || pwmR !== undefined)
            ? `L/R ${pwmL ?? '—'}/${pwmR ?? '—'} de 255` : (pwm10 === undefined ? '—' : `${pwm8 ?? '—'}/255 · ${pwm10}/1023`);
        document.getElementById('cal-diag-attempt').textContent = att === undefined ? '—' : `${att}/${attMax ?? '—'}`;
    },
    _renderHistory(raw) {
        const h = raw && typeof raw.torque_history === 'object' && raw.torque_history ? raw.torque_history : {};
        const count = Number(h.record_count || 0), status = String(h.status || 'sin datos');
        const bPosL = Number(h.base_positive_left_8bit ?? h.base_positive_8bit ?? 140), bPosR = Number(h.base_positive_right_8bit ?? h.base_positive_8bit ?? 140);
        const bNegL = Number(h.base_negative_left_8bit ?? h.base_negative_8bit ?? 140), bNegR = Number(h.base_negative_right_8bit ?? h.base_negative_8bit ?? 140);
        document.getElementById('cal-diag-history').textContent = count
            ? `${count}/10 · base + L/R ${bPosL}/${bPosR} · − L/R ${bNegL}/${bNegR} · ${status}` : `0/10 · base 140 · ${status}`;
    },
    _renderPcntInit(raw) {
        const pcnt = raw && typeof raw.pcnt_init === 'object' && raw.pcnt_init ? raw.pcnt_init : {};
        const failures = ['fl', 'fr', 'bl', 'br'].filter(n => pcnt[n]?.initialized === false)
            .map(n => `${n.toUpperCase()}:${pcnt[n].failed_stage || 'unknown'}(${Number(pcnt[n].error_code || 0)})`);
        document.getElementById('cal-diag-pcnt').textContent = pcnt.ready === true
            ? 'FL/FR/BL/BR listos' : (pcnt.ready === false ? `FALLO · ${failures.join(' · ') || 'sin detalle'}` : 'esperando telemetría');
    },

    _renderWheelDeltas(diag) {
        const deltas = diag.encoder_delta ?? diag.deltas ?? diag.deltaEncoders;
        const responding = diag.encoder_responding ?? diag.encoder_responds ?? diag.encoderResponde;
        const isolated = diag.encoder_isolated ?? diag.isolated ?? diag.encoderAislado, trusted = diag.trusted;
        ['fl', 'fr', 'bl', 'br'].forEach((wheel, index) => {
            const delta = this.valueAt(deltas, wheel, index), responds = this.valueAt(responding, wheel, index);
            const isIsolated = Boolean(this.valueAt(isolated, wheel, index, false)), isTrusted = this.valueAt(trusted, wheel, index);
            let status = 'unknown', label = 'sin datos';
            if (isIsolated || isTrusted === false) { status = 'isolated'; label = 'aislado'; }
            else if (responds === true || isTrusted === true) { status = 'ok'; label = 'responde'; }
            else if (responds === false) { status = 'unknown'; label = 'sin señal en esta fase'; }
            document.getElementById(`cal-diag-delta-${wheel}`).textContent = `Δ ${delta === undefined ? '—' : String(delta)}`;
            document.getElementById(`cal-diag-status-${wheel}`).textContent = label;
            document.getElementById(`cal-diag-${wheel}`).dataset.status = status;
        });
    },
    _renderDiagnosticSides(diag) {
        const silence = Number(diag.encoder_silence_ms || 0), silenceLimit = Number(diag.encoder_silence_limit_ms || 500);
        const silenceLabel = diag.gyro_without_encoders === true ? ` · IMU sin PCNT ${silence}/${silenceLimit} ms` : '';
        const authority = diag.authority === 'imu_primary_bilateral_pcnt' ? 'MPU principal · PCNT bilateral'
            : (diag.authority === 'imu_primary_one_pcnt' ? 'MPU principal · firmware anterior' : 'Autoridad sin identificar');
        const pivot = diag.pivot_pwm && typeof diag.pivot_pwm === 'object' ? diag.pivot_pwm : {};
        const leftTicks = Number(diag.left_ticks || 0), rightTicks = Number(diag.right_ticks || 0);
        const ratio = Number(diag.tick_ratio), ratioLabel = Number.isFinite(ratio) ? ratio.toFixed(2) : '—';
        const yawDelta = Number(diag.yaw_phase_delta || 0), quality = String(diag.pivot_quality || 'unverified');
        const balance = String(diag.balance_adjustment || '—'), translation = Number(diag.estimated_translation_cm || 0);
        const torque = diag.torque_detected === true ? 'torque detectado' : 'buscando torque';
        const pivotState = diag.pivot_validated === true ? 'pivote validado' : 'pivote pendiente';
        const dirLabel = diag.encoder_direction_verified === false ? ' · dirección PCNT no verificable' : '';
        const respCount = Number(diag.pcnt_response_count || 0), wait = String(diag.wait_reason || 'esperando datos');
        document.getElementById('cal-diag-sides').textContent = `${authority} · ${torque} · ${pivotState} · calidad ${quality} · balance ${balance} · ticks L/R ${leftTicks}/${rightTicks} (ratio ${ratioLabel}) · Δyaw ${yawDelta.toFixed(1)}° · traslación estimada ${translation.toFixed(2)} cm · PCNT ${respCount}/4 · espera ${wait} · PWM lógico L/R ${pivot.logical_left ?? '—'}/${pivot.logical_right ?? '—'}${silenceLabel}${dirLabel}`;
    },
    /** @param {TelemetryPayload} raw */
    applyTelemetry(raw) {
        this.lastRaw = raw || {};
        this.selfTestPassed = true;
        this._checkTurnTelemetry(raw);
        this._checkEncoderTelemetry(raw);
        this._checkAntiFrictionTelemetry(raw);

        const fault = raw.fault && typeof raw.fault === 'object' ? raw.fault : {};
        const robotState = this.normalizeRobotState(raw.state || (fault.active ? fault.state : ''));
        this.robotState = robotState;
        this.allowedCommands = Array.isArray(raw.allowed_commands) ? raw.allowed_commands.map(v => String(v).toLowerCase()) : null;
        this.mpuReady = this.mpuStatus(raw).ready;
        this.renderDiagnostics(raw);
        this._checkTerminalState(raw, fault, robotState);
    },
    _checkTurnTelemetry(raw) {
        const reqMode = String(raw.turn_requested_mode || 'AUTO'), turnMode = String(raw.turn_mode || 'PIVOT');
        if (reqMode !== this.lastRequestedTurnMode) {
            LogSystem.add('INFO', `Método de giro solicitado: ${reqMode}.`);
            this.lastRequestedTurnMode = reqMode;
        }
        if (turnMode !== this.lastTurnMode) {
            if (turnMode === 'PIVOT_CONTINUOUS') LogSystem.add('INFO', 'Pivot simple activado; sólo pausa 300 ms cuando sobrepasa el objetivo.');
            this.lastTurnMode = turnMode;
        }
        const pKey = `${raw.turn_left_energized ? 1 : 0}:${Number(raw.turn_left_power_started_ms || 0)}:${raw.turn_right_energized ? 1 : 0}:${Number(raw.turn_right_power_started_ms || 0)}`;
        if (pKey !== this.lastPowerKey && (raw.turn_left_energized || raw.turn_right_energized)) {
            LogSystem.add('INFO', `Energía física de giro: L=${raw.turn_left_energized ? 'ON' : 'OFF'} desde ${Number(raw.turn_left_power_started_ms || 0)} ms; R=${raw.turn_right_energized ? 'ON' : 'OFF'} desde ${Number(raw.turn_right_power_started_ms || 0)} ms.`);
        }
        this.lastPowerKey = pKey;

        const bPhase = String(raw.turn_braking_phase || 'OFF');
        if (bPhase !== this.lastBrakingPhase && bPhase !== 'OFF') {
            LogSystem.add('INFO', `Control angular ${bPhase}: error=${Number(raw.turn_error_deg || 0).toFixed(2)}°, predicho=${Number(raw.turn_predicted_error_deg || 0).toFixed(2)}°, PWM objetivo=${Number(raw.turn_pwm_target_8bit || 0)}/255.`);
        }
        this.lastBrakingPhase = bPhase;

        const aStage = String(raw.alignment_stage || 'OFF');
        if (aStage !== this.lastAlignmentStage && aStage !== 'OFF') {
            LogSystem.add('INFO', `Alineación ${aStage}: fase=${raw.move_phase || 'OFF'}, objetivo=${Number(raw.heading_target_deg || 0).toFixed(1)}°, error=${Number(raw.heading_error_deg || 0).toFixed(1)}°, tolerancia=±${Number(raw.turn_tolerance_deg || 0).toFixed(1)}°.`);
        }
        this.lastAlignmentStage = aStage;
    },
    _checkEncoderTelemetry(raw) {
        const encoderHealth = raw.encoder_health || {};
        const key = ['fl', 'fr', 'bl', 'br'].map(k => String(encoderHealth[k] || 'unknown')).join('/');
        const warning = ['fl', 'fr', 'bl', 'br'].some(k => !['healthy', 'ok'].includes(String(encoderHealth[k] || 'unknown')));
        const degraded = Boolean(raw.degraded_mode);

        if (this.lastDegradedMode !== null && degraded !== this.lastDegradedMode) {
            LogSystem.add(degraded ? 'WARN' : 'INFO', degraded ? `Modo degradado activo: ${key}. La ruta continúa porque queda al menos un encoder confiable por lado.` : `Fusión de encoders restaurada: ${key}.`);
        } else if (degraded && key !== this.lastEncoderHealthKey) {
            LogSystem.add('WARN', `Salud de encoders en modo degradado: ${key}.`);
        } else if (warning && key !== this.lastEncoderHealthKey) {
            LogSystem.add('WARN', `Encoder sin respuesta estable: ${key}. Se mantiene vigilancia antes de excluirlo.`);
        }
        this.lastDegradedMode = degraded;
        this.lastEncoderHealthKey = key;
    },
    _checkAntiFrictionTelemetry(raw) {
        const anti = raw.anti_friction || {};
        const antiKey = `${Boolean(anti.active)}:${Number(anti.pulse_index || 0)}:${Boolean(anti.pulse_on)}`;
        if (anti.active && antiKey !== this.lastAntiFrictionKey) {
            LogSystem.add('WARN', `Antifricción ${Number(anti.pulse_index || 0)}/${Number(anti.pulse_total || 7)} ${anti.pulse_on ? 'ON' : 'OFF'}: PWM ${Number(anti.target_8bit || 0)}/255 (${Number(anti.target_percent || 0).toFixed(1)}%).`);
        } else if (!anti.active && this.lastAntiFrictionKey.startsWith('true') && anti.movement_confirmed) {
            LogSystem.add('INFO', 'Antifricción terminada: movimiento confirmado en ambos lados.');
        }
        this.lastAntiFrictionKey = antiKey;
    },
    _checkTerminalState(raw, fault, robotState) {
        const terminal = raw.last_terminal || {};
        const diag = raw.calibration_diagnostics && typeof raw.calibration_diagnostics === 'object' ? raw.calibration_diagnostics : {};
        const terminalReason = String(fault.detail || terminal.detail || terminal.reason || diag.reason || raw.finish_reason || 'sin causa reportada');
        const terminalKey = `${terminal.id || ''}:${terminal.type || ''}:${terminalReason}`;

        if (['FALLO', 'ESTOP'].includes(robotState) && (robotState !== this.lastRobotState || terminalKey !== this.lastTerminalKey)) {
            LogSystem.add('ERROR', `Robot pasó a ${robotState}: ${humanizeError(terminalReason)}. PWM físico detenido por seguridad.`);
            this.lastTerminalKey = terminalKey;
        }
        this.lastRobotState = robotState;

        if (robotState === 'ESTOP') {
            OperatorFeedback.estopStatus('robot');
        } else {
            const estopEl = document.getElementById('estop-status');
            if (estopEl && estopEl.dataset.stage !== 'failed') {
                OperatorFeedback.estopStatus('idle');
            }
        }

        if (['FALLO', 'ESTOP'].includes(robotState)) {
            this.commandPending = false;
            const isEstop = robotState === 'ESTOP';
            this.setState(isEstop ? 'ESTOP — REARME REQUERIDO' : 'FALLO — REARME REQUERIDO', false, isEstop ? 'estop' : 'fault');
            this.showCause(terminalReason, true);
        } else {
            this.rearmPending = false; this.rearmCommandId = null; this.showCause('', false);
            if (Boolean(raw.calibrated ?? raw.cal)) {
                this.applyingSurface = false;
                this.setState('LISTO — RUTAS HABILITADAS', true);
            } else if (this.applyingSurface) {
                // Esperando confirmación de telemetría
            } else if (robotState === 'CALIBRANDO') {
                const intento = Number(raw.turn_attempt || 0), max = Number(raw.turn_attempt_max || 0);
                const intentoTexto = (intento && max) ? ` — GIRO ${intento}/${max} · ${raw.turn_mode || 'PIVOT'}` : '';
                this.setState(`${raw.state} — ${Math.round((raw.command_progress || 0) * 100)}%${intentoTexto}`, false);
                const calPanel = document.getElementById('cal-physical-panel');
                if (calPanel && calPanel.style.display === 'none') {
                    calPanel.style.display = 'block';
                    const btn = document.getElementById('btn-toggle-cal-diagnostics');
                    if (btn) btn.textContent = '✕ Ocultar Calibración';
                }
            } else if (['DESARMADO', 'LISTO'].includes(robotState) && this.mpuReady) {
                this.setState('LISTO PARA CALIBRAR', false);
            } else {
                this.setState(this.mpuStatus(raw).reason.toUpperCase(), false);
            }
        }
    },

    onRobotEvent(event) {
        if (event.id === this.rearmCommandId) {
            this._handleRearmEvent(event);
            return;
        }
        if (event.id === this.selfTestCommandId) {
            this._handleSelfTestEvent(event);
            return;
        }
        if (event.id !== this.commandId) return;
        if (event.type === 'queued' || event.type === 'accepted' || event.type === 'progress') return;

        this.commandPending = false;
        this.commandId = null;
        if (event.type === 'completed') {
            this.setState('LISTO — CONTROLES HABILITADOS', true);
            LogSystem.add('INFO', 'Calibración física completada; rutas habilitadas.');
            if (typeof SurfaceManager !== 'undefined') SurfaceManager.handleCalibrationCompleted();
        }
        if (event.type === 'fault' || event.type === 'rejected') {
            if (typeof SurfaceManager !== 'undefined') SurfaceManager.pendingAutoSaveName = null;
            const reason = humanizeError(event.detail || event.reason || 'calibración rechazada');
            this.setState(this.calibrated ? 'LISTO — CALIBRACIÓN RECHAZADA' : `CALIBRACIÓN FALLIDA — ${reason}`, this.calibrated, 'fault');
            this.showCause(reason, true);
            LogSystem.add('ERROR', `No se pudo calibrar: ${reason}.`);
        }
        this.updateAvailability();
    },
    _handleRearmEvent(event) {
        if (event.type === 'queued' || event.type === 'accepted' || event.type === 'progress') return;
        this.rearmPending = false;
        this.rearmCommandId = null;
        if (event.type === 'completed') {
            this.rearmPending = true;
            this.setState('REARME ACEPTADO — ESPERANDO TELEMETRÍA', false);
            LogSystem.add('INFO', 'Rearme aceptado; esperando confirmación del nuevo estado del robot.');
        } else {
            const reason = humanizeError(event.detail || event.reason || 'rearme rechazado');
            this.showCause(reason, true);
            LogSystem.add('ERROR', `No se pudo rearmar: ${reason}.`);
            this.updateAvailability();
        }
    },
    _handleSelfTestEvent(event) {
        if (event.type === 'queued' || event.type === 'accepted' || event.type === 'progress') return;
        const ok = event.type === 'completed';
        this.selfTestPassed = ok;
        document.getElementById('self-test-state').textContent = ok ? 'APROBADA' : `FALLO — ${humanizeError(event.detail || event.reason || 'sin detalle')}`;
        const selfBtn = /** @type {HTMLButtonElement | null} */ (document.getElementById('btn-self-test'));
        if (selfBtn) selfBtn.disabled = false;
        this.updateAvailability();
        LogSystem.add(ok ? 'INFO' : 'ERROR', ok ? 'Autoevaluación lógica aprobada.' : `Autoevaluación falló: ${humanizeError(event.detail || event.reason || 'sin detalle')}`);
    }
};

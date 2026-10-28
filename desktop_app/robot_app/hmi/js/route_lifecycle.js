// @ts-check
/* ==============================================
   ROUTE LIFECYCLE (Ciclo de vida, ejecución, persistencia y paradas de rutas)
   ============================================== */
const RouteLifecycle = {
    /** @param {any} manager */
    lockTabs(manager) {
        document.querySelectorAll('.tab-btn').forEach(elem => {
            const b = /** @type {HTMLElement} */ (elem);
            if (b.dataset.tab !== 'rutas') b.classList.add('disabled');
        });
        document.getElementById('route-running-badge')?.classList.add('show');
    },

    /** @param {any} manager */
    unlockTabs(manager) {
        document.querySelectorAll('.tab-btn').forEach(elem => {
            const b = /** @type {HTMLElement} */ (elem);
            b.classList.remove('disabled');
        });
        document.getElementById('route-running-badge')?.classList.remove('show');
    },

    /** @param {any} manager */
    async start(manager) {
        if (manager.logicalSteps.length && manager.queue.length === 0) {
            return LogSystem.add('WARN', 'La ruta sólo contiene orientaciones visuales; agrega una hipotenusa con longitud mayor que cero.');
        }
        if (manager.isRunning || manager.queue.length === 0 || manager.currentStepIndex >= manager.queue.length) return;
        const mode = manager.activeMode();
        if (mode.key === 'vectorial' && !manager.vectorialReadiness().ready) {
            return LogSystem.add('ERROR', `Angular vectorial bloqueado: ${manager.vectorialReadiness().reason}`);
        }
        const curName = manager.routeNameInput?.value.trim() || manager.mainRouteName || 'Ruta Planificada Robot_A';
        manager.saveCurrentRouteName();
        this._persistRouteHistory(manager, curName, mode);

        try {
            const prev = await WSControl.api('/api/v1/missions');
            if (prev.blocked) {
                await WSControl.api('/api/v1/missions', { method: 'DELETE' });
                LogSystem.add('WARN', `Misión bloqueada ${prev.id || ''} descartada antes de iniciar.`);
            }
            const mission = await WSControl.api('/api/v1/missions', {
                method: 'POST', body: JSON.stringify(mode.missionPayload(manager))
            });
            manager.isRunning = true;
            manager.routeStartedAt = Date.now();
            manager.currentStepIndex = mission.current_index || 0;
            manager.activeCommandId = mission.active_command_id;
            manager.waitingForMovement = true;
            manager.clearTelemetryTrail();
            this.lockTabs(manager);
            LogSystem.add('INFO', `Ruta '${curName}' guardada. Misión ${mission.id} iniciada con ${mission.total_segments} segmentos.`);
            manager.updateListUI();
            manager.drawPlan();
        } catch (err) {
            manager.isRunning = false;
            this.unlockTabs(manager);
            LogSystem.add('ERROR', err.message);
        }
    },

    /** @param {any} manager @param {string} name @param {any} mode */
    _persistRouteHistory(manager, name, mode) {
        try {
            const saved = JSON.parse(localStorage.getItem('hmi_saved_routes') || '[]');
            const record = { name, savedAt: new Date().toISOString(), steps: manager.queue.length, mode: mode.key, payload: mode.missionPayload(manager) };
            const idx = saved.findIndex(r => r.name === name);
            if (idx >= 0) saved[idx] = record; else saved.unshift(record);
            localStorage.setItem('hmi_saved_routes', JSON.stringify(saved.slice(0, 20)));
        } catch (_) {}
    },

    /** @param {any} manager @param {string} name @param {any} mode */
    persistRouteHistory(manager, name, mode) {
        return this._persistRouteHistory(manager, name, mode);
    },

    /** @param {any} manager @param {string} reason */
    failRoute(manager, reason) {
        manager.isRunning = false; manager.waitingForMovement = false; manager.activeCommandId = null;
        clearTimeout(manager.moveTimeout);
        this.unlockTabs(manager); manager.updateListUI(); manager.renderVectorialSafety();
        LogSystem.add('ERROR', reason);
    },

    /** @param {any} manager */
    async stopAndReset(manager) {
        if (manager.logicalSteps.length > 0 && !confirm('¿Borrar la ruta actual y reiniciar el origen?')) return;
        try {
            const m = await WSControl.api('/api/v1/missions', { method: 'DELETE' });
            if (m.id !== null || m.blocked) throw new Error('El backend no confirmó el borrado de la misión');
            manager.isRunning = false; manager.isOckhamSimulating = false; this.unlockTabs(manager);
            clearTimeout(manager.moveTimeout);
            manager.waitingForMovement = false; manager.activeCommandId = null;
            manager.queue = []; manager.logicalSteps = []; manager.nextLogicalStepId = 1; manager.currentStepIndex = 0;
            manager.currentOrigin = { x: state.telemetria.posX, y: state.telemetria.posY };
            manager.clearTelemetryTrail(); manager.updateModeLock();
            LogSystem.add('WARN', 'Ruta detenida y reset confirmado por backend.');
            manager.updateListUI(); manager.updateLabels(); manager.updatePreview();
            manager.renderVectorialSafety(); manager.actualizarRestriccionInputs();
        } catch (err) {
            LogSystem.add('ERROR', `No se pudo borrar la ruta: ${err.message}`);
        }
    },

    /** @param {any} manager */
    async clearRobotMemory(manager) {
        const box = document.getElementById('route-ockham-confirm-box');
        if (box) box.style.display = 'none';
        if (!confirm('¿Detener el robot y borrar la misión y su checkpoint de Python?')) return;
        try {
            await WSControl.api('/api/v1/missions/robot-memory', { method: 'DELETE' });
            manager.isRunning = false; manager.waitingForMovement = false; manager.activeCommandId = null;
            manager.isOckhamSimulating = false; this.unlockTabs(manager);
            LogSystem.add('INFO', 'Misión y checkpoint de Python eliminados; se solicitó STOP si había un paso activo.');
            manager.currentStepIndex = 0; manager.updateListUI(); manager.drawPlan();
        } catch (err) {
            LogSystem.add('ERROR', `No se pudo liberar la memoria: ${err.message}`);
        }
    }
};

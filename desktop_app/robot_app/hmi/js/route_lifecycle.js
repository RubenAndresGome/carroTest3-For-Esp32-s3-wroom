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
        if (manager.stoppedAtIndex !== null && manager.stoppedAtIndex !== undefined) {
            OperatorFeedback.show(
                'route-start-alert',
                'La ruta fue detenida a mitad. Descártala o edita los pasos antes de volver a ejecutar desde el origen.',
                'warn'
            );
            return;
        }
        if (manager.logicalSteps.length && manager.queue.length === 0) {
            const msg = 'La ruta sólo contiene orientaciones visuales; agrega una hipotenusa con longitud mayor que cero.';
            OperatorFeedback.show('route-start-alert', msg, 'warn');
            return LogSystem.add('WARN', msg);
        }
        if (manager.queue.length === 0) {
            OperatorFeedback.show('route-start-alert', 'Agrega al menos un paso a la ruta antes de iniciar.', 'warn');
            return;
        }
        if (manager.isRunning || manager.currentStepIndex >= manager.queue.length) return;
        const mode = manager.activeMode();
        if (mode.key === 'vectorial' && !manager.vectorialReadiness().ready) {
            const reason = `Angular vectorial bloqueado: ${manager.vectorialReadiness().reason}`;
            OperatorFeedback.show('route-start-alert', reason, 'error');
            return LogSystem.add('ERROR', reason);
        }

        const totalCm = RouteForksManager.calculateDistance(manager.queue, manager.currentOrigin);
        const ok = OperatorFeedback.confirmMotion(
            `¿Guardar e iniciar la ruta con ${manager.queue.length} segmento(s) (${totalCm.toFixed(1)} cm)? Confirma que el área está despejada.`
        );
        if (!ok) return;

        OperatorFeedback.clear('route-start-alert');
        const curName = manager.routeNameInput?.value.trim() || manager.mainRouteName || 'Ruta Planificada Robot_A';
        manager.saveCurrentRouteName();

        const startBtn = /** @type {HTMLButtonElement | null} */ (document.getElementById('btn-route-start'));
        if (startBtn) {
            startBtn.disabled = true;
            startBtn.setAttribute('aria-busy', 'true');
        }

        try {
            const prev = await WSControl.api('/api/v1/missions');
            if (prev.blocked) {
                await WSControl.api('/api/v1/missions', { method: 'DELETE' });
                LogSystem.add('WARN', `Misión bloqueada ${prev.id || ''} descartada antes de iniciar.`);
            }
            const mission = await WSControl.api('/api/v1/missions', {
                method: 'POST', body: JSON.stringify(mode.missionPayload(manager))
            });
            this._persistRouteHistory(manager, curName, mode);
            manager.stoppedAtIndex = null;
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
            const msg = humanizeError(err.message);
            OperatorFeedback.show('route-start-alert', msg, 'error');
            LogSystem.add('ERROR', msg);
        } finally {
            if (startBtn) {
                startBtn.disabled = false;
                startBtn.removeAttribute('aria-busy');
            }
        }
    },

    /** @param {any} manager @param {string} name @param {any} mode */
    _persistRouteHistory(manager, name, mode) {
        try {
            RouteLibrary.upsert(RouteLibrary.buildRecord(manager, name, mode));
        } catch (_) {}
    },

    /** @param {any} manager @param {string} name @param {any} mode */
    persistRouteHistory(manager, name, mode) {
        return this._persistRouteHistory(manager, name, mode);
    },

    /** @param {any} manager @param {string} name @param {any} mode */
    saveLastRoute(manager, name, mode) {
        return this._persistRouteHistory(manager, name, mode);
    },

    /** @param {any} manager @param {string} reason */
    failRoute(manager, reason) {
        manager.isRunning = false; manager.waitingForMovement = false; manager.activeCommandId = null;
        clearTimeout(manager.moveTimeout);
        this.unlockTabs(manager); manager.updateListUI(); manager.renderVectorialSafety();
        const msg = humanizeError(reason);
        OperatorFeedback.show('route-start-alert', msg, 'error');
        LogSystem.add('ERROR', msg);
    },

    /** @param {any} manager */
    async stopRoute(manager) {
        try {
            await WSControl.api('/api/v1/missions', { method: 'DELETE' });
        } catch (err) {
            LogSystem.add('ERROR', `Error deteniendo misión: ${humanizeError(err.message)}`);
        }
        clearTimeout(manager.moveTimeout);
        manager.stoppedAtIndex = manager.isRunning ? manager.currentStepIndex : manager.stoppedAtIndex;
        manager.isRunning = false;
        manager.waitingForMovement = false;
        manager.activeCommandId = null;
        this.unlockTabs(manager);
        LogSystem.add('WARN', 'Ejecución detenida. La ruta permanece cargada; usa Descartar para borrarla o edítala antes de reiniciar.');
        OperatorFeedback.show('route-start-alert', 'Ejecución detenida. Edita o descarta la ruta antes de reiniciar desde el origen.', 'warn');
        manager.updateListUI();
        manager.renderVectorialSafety();
    },

    /** @param {any} manager */
    async discardRoute(manager) {
        if (manager.logicalSteps.length > 0 && !confirm('¿Descartar la ruta actual y reiniciar el origen?')) return;
        try {
            const m = await WSControl.api('/api/v1/missions', { method: 'DELETE' });
            if (m.id !== null || m.blocked) throw new Error('El backend no confirmó el borrado de la misión');
            manager.isRunning = false; manager.isOckhamSimulating = false; this.unlockTabs(manager);
            clearTimeout(manager.moveTimeout);
            manager.waitingForMovement = false; manager.activeCommandId = null;
            manager.stoppedAtIndex = null;
            manager.queue = []; manager.logicalSteps = []; manager.nextLogicalStepId = 1; manager.currentStepIndex = 0;
            manager.currentOrigin = { x: state.telemetria.posX, y: state.telemetria.posY };
            OperatorFeedback.clear('route-start-alert');
            OperatorFeedback.clear('route-form-alert');
            manager.clearTelemetryTrail(); manager.updateModeLock();
            LogSystem.add('WARN', 'Ruta descartada y origen reiniciado.');
            manager.updateListUI(); manager.updateLabels(); manager.updatePreview();
            manager.renderVectorialSafety(); manager.actualizarRestriccionInputs();
        } catch (err) {
            const msg = humanizeError(err.message);
            OperatorFeedback.show('route-start-alert', `No se pudo borrar la ruta: ${msg}`, 'error');
            LogSystem.add('ERROR', `No se pudo borrar la ruta: ${msg}`);
        }
    },

    /** @param {any} manager */
    async stopAndReset(manager) {
        return this.discardRoute(manager);
    },

    /** @param {any} manager */
    async clearRobotMemory(manager) {
        const box = document.getElementById('route-ockham-confirm-box');
        if (box) box.style.display = 'none';
        if (!confirm('¿Detener el robot y borrar la misión y su checkpoint de Python?')) return;
        try {
            await WSControl.api('/api/v1/missions/robot-memory', { method: 'DELETE' });
            manager.isRunning = false; manager.waitingForMovement = false; manager.activeCommandId = null;
            manager.isOckhamSimulating = false; manager.stoppedAtIndex = null; this.unlockTabs(manager);
            OperatorFeedback.clear('route-start-alert');
            LogSystem.add('INFO', 'Misión y checkpoint de Python eliminados; se solicitó STOP si había un paso activo.');
            manager.currentStepIndex = 0; manager.updateListUI(); manager.drawPlan();
        } catch (err) {
            const msg = humanizeError(err.message);
            OperatorFeedback.show('route-start-alert', `No se pudo liberar la memoria: ${msg}`, 'error');
            LogSystem.add('ERROR', `No se pudo liberar la memoria: ${msg}`);
        }
    }
};

/* ==============================================
   ROUTE OCKHAM COCKPIT (Safe Return Home Protocol)
   ============================================== */

const RouteOckhamCockpit = {
    /**
     * Inicia la simulación previa del retorno seguro a origen de Ockham.
     */
    _buildOckhamSteps(manager, points, vectorialReturn) {
        let logicalStart = manager.currentOrigin;
        return (points || []).map((point, index) => {
            const end = { x: Number(point.x_mm) / 10, y: Number(point.y_mm) / 10 };
            const id = manager.nextLogicalStepId++;
            const dx = end.x - logicalStart.x, dy = end.y - logicalStart.y;
            const absoluteAngleDeg = normalizeSignedDegrees(Math.atan2(dy, dx) * 180 / Math.PI);
            const prev = manager.logicalSteps.length ? manager.logicalSteps[manager.logicalSteps.length - 1].absoluteAngleDeg : 0;
            const theta = normalizeSignedDegrees(absoluteAngleDeg - prev);

            const step = vectorialReturn
                ? { id, number: index + 1, mode: 'vectorial', start: logicalStart, end, dx, dy, length: Math.hypot(dx, dy), theta, referenceAngleDeg: prev, absoluteAngleDeg, execution: [], description: 'Vector inverso Ockham' }
                : { id, number: index + 1, mode: 'rect', start: logicalStart, end, dx, dy, execution: [], description: 'Paso inverso Ockham' };

            manager.logicalSteps.push(step);
            logicalStart = end;
            return {
                x: end.x, y: end.y, logicalStepId: id,
                component: vectorialReturn ? 'vector' : 'ockham',
                desc: vectorialReturn ? 'Vector inverso Ockham' : 'Paso inverso Ockham'
            };
        });
    },

    /**
     * Inicia la simulación previa del retorno seguro a origen de Ockham.
     */
    async ockhamReturn(manager) {
        try {
            manager.isOckhamSimulating = true;
            const preview = await WSControl.api('/api/v1/missions/return-home', {
                method: 'POST',
                body: JSON.stringify({ preview: true })
            });

            manager.logicalSteps = [];
            manager.nextLogicalStepId = 1;
            const vectorialReturn = preview.mode === 'angular_vectorial';
            manager.modeSelect.value = vectorialReturn ? 'vectorial' : 'rect';

            manager.queue = this._buildOckhamSteps(manager, preview.planned_points, vectorialReturn);
            this._updateSimulationUI(manager, preview);
        } catch (error) {
            manager.isOckhamSimulating = false;
            LogSystem.add('ERROR', `No se pudo simular Ockham: ${error.message}`);
        }
    },

    _updateSimulationUI(manager, preview) {
        manager.updateModeLock();
        manager.currentStepIndex = 0;
        manager.updateListUI();
        manager.drawPlan();
        manager.autoScale();

        const box = document.getElementById('route-ockham-confirm-box');
        const info = document.getElementById('route-ockham-sim-info');
        if (box && info) {
            box.style.display = 'block';
            info.textContent = `Simulación calculada: ${preview.total_steps} pasos enlazados hacia (0, 0) con retorno optimizado (reversa o avance automático) y supervisión de exactitud. Confirma para arrancar.`;
        }
        const stageLabel = document.getElementById('ockham-current-stage');
        if (stageLabel) stageLabel.textContent = `Simulación Ockham: ${preview.total_steps} tramos listos`;
        LogSystem.add('INFO', `Simulación de regreso Ockham lista: ${preview.total_steps} pasos enlazados calculados hacia el origen.`);
    },

    /**
     * Confirma e inicia la ejecución física de la misión Ockham.
     */
    async confirmOckhamReturn(manager) {
        const box = document.getElementById('route-ockham-confirm-box');
        if (box) box.style.display = 'none';
        manager.isOckhamSimulating = false;

        try {
            const mission = await WSControl.api('/api/v1/missions/return-home', {
                method: 'POST',
                body: JSON.stringify({ preview: false })
            });
            manager.currentStepIndex = mission.current_index || 0;
            manager.activeCommandId = mission.active_command_id || null;
            manager.isRunning = true;
            manager.waitingForMovement = true;
            manager.clearTelemetryTrail();
            manager.lockTabs();
            LogSystem.add('CMD', 'Ruta Ockham inversa confirmada e iniciada con retorno automático; supervisión de exactitud activa.');
            manager.updateListUI();
            manager.drawPlan();
        } catch (error) {
            LogSystem.add('ERROR', `No se pudo iniciar regreso Ockham: ${error.message}`);
        }
    },

    /**
     * Cancela la simulación en curso del retorno Ockham.
     */
    cancelOckhamReturn(manager) {
        const box = document.getElementById('route-ockham-confirm-box');
        if (box) box.style.display = 'none';
        manager.isOckhamSimulating = false;

        const stageLabel = document.getElementById('ockham-current-stage');
        if (stageLabel) stageLabel.textContent = 'Ockham: Inactivo';
        manager.queue = [];
        manager.logicalSteps = [];
        manager.currentStepIndex = 0;
        manager.updateListUI();
        manager.drawPlan();
        LogSystem.add('INFO', 'Simulación de regreso Ockham cancelada por el operador.');
    },

    /**
     * Procesa la telemetría y estado de una misión activa Ockham o estándar.
     */
    onMissionUpdate(manager, mission) {
        this._updateBadgesAndStage(manager, mission);
        this._updateAccuracyMetrics(mission.accuracy);
        this._updateSafetyText(manager, mission);
    },

    _updateBadgesAndStage(manager, mission) {
        const settlingBadge = document.getElementById('ockham-badge-settling');
        const realigningBadge = document.getElementById('ockham-badge-realigning');
        const stageLabel = document.getElementById('ockham-current-stage');

        if (mission.kind === 'ockham_return' || manager.isOckhamSimulating) {
            if (stageLabel) {
                stageLabel.textContent = mission.running
                    ? `OCKHAM: Tramo ${manager.currentStepIndex + 1}/${mission.total_segments || 0} (${String(mission.stage || 'idle').toUpperCase()})`
                    : (manager.isOckhamSimulating ? 'Simulación lista para confirmar' : 'Ockham: Inactivo');
            }
            if (settlingBadge) settlingBadge.style.display = (mission.stage === 'settling') ? 'inline-block' : 'none';
            if (realigningBadge) realigningBadge.style.display = (mission.stage === 'realigning') ? 'inline-block' : 'none';
        } else {
            if (stageLabel) stageLabel.textContent = 'Ockham: Inactivo';
            if (settlingBadge) settlingBadge.style.display = 'none';
            if (realigningBadge) realigningBadge.style.display = 'none';
        }
    },

    _updateAccuracyMetrics(acc) {
        if (!acc) return;
        const elPos = document.getElementById('route-live-acc-pos');
        const elAng = document.getElementById('route-live-acc-ang');
        const chipLat = document.getElementById('ockham-val-lat');
        const chipAng = document.getElementById('ockham-val-ang');
        const chipLong = document.getElementById('ockham-val-long');

        if (elPos) elPos.textContent = `${Number(acc.error_long_cm || 0).toFixed(1)} / ${Number(acc.error_lat_cm || 0).toFixed(1)} cm`;
        if (elAng) elAng.textContent = `${Number(acc.error_angular_deg || 0).toFixed(1)}° / ${Number(acc.accumulated_lateral_cm || 0).toFixed(1)} cm`;
        if (chipLat) chipLat.textContent = `${Number(acc.error_lat_cm || 0).toFixed(1)} cm`;
        if (chipAng) chipAng.textContent = `${Number(acc.error_angular_deg || 0).toFixed(1)}°`;
        if (chipLong) chipLong.textContent = `${Number(acc.error_long_cm || 0).toFixed(1)} cm`;
    },

    _updateSafetyText(manager, mission) {
        const safety = document.getElementById('route-mission-safety');
        if (!safety) return;
        if (mission.stage === 'settling') {
            safety.textContent = `Punto de exactitud: evaluando asentamiento y errores (${manager.currentStepIndex}/${mission.total_segments || 0})...`;
            safety.style.color = 'var(--accent, #f59e0b)';
        } else if (mission.stage === 'realigning') {
            safety.textContent = `Reposicionamiento ortogonal: desvío angular > 1.5° o lateral > 2.5 cm. Re-alineando a eje cardinal...`;
            safety.style.color = 'var(--accent, #f59e0b)';
        } else {
            safety.textContent = `Protección de pasos: ${String(mission.stage || 'idle').toUpperCase()} · checkpoint ${manager.currentStepIndex}/${mission.total_segments || 0}.`;
            safety.style.color = '';
        }
    }
};

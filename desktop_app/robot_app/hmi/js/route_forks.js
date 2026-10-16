// @ts-check
/** @typedef {import('@frontend/protocol').Telemetry} TelemetryPayload */
/** @typedef {import('@frontend/protocol').CommandName} CommandName */

/* ==============================================
   ROUTE FORKS MANAGER (Bifurcations, A/B Compare & Persistence)
   ============================================== */

const RouteForksManager = {
    STORAGE_KEY: 'hmi_route_forks',

    /**
     * Calcula la distancia métrica total euclídea de una cola de waypoints.
     */
    calculateDistance(queue, origin = { x: 0, y: 0 }) {
        let dist = 0;
        let prev = origin;
        for (let i = 0; i < queue.length; i++) {
            dist += Math.hypot(queue[i].x - prev.x, queue[i].y - prev.y);
            prev = queue[i];
        }
        return dist;
    },

    /**
     * Clona el estado completo de una ruta de forma segura y profunda.
     */
    snapshotRoute(manager, name) {
        return {
            name,
            mode: manager.activeMode().key,
            currentOrigin: { ...manager.currentOrigin },
            logicalSteps: JSON.parse(JSON.stringify(manager.logicalSteps)),
            queue: JSON.parse(JSON.stringify(manager.queue)),
            nextLogicalStepId: manager.nextLogicalStepId
        };
    },

    /**
     * Crea una bifurcación a partir de la ruta activa.
     */
    fork(manager, name) {
        if (manager.isRunning) {
            return LogSystem.add('WARN', 'No se puede bifurcar mientras la ruta está en ejecución.');
        }
        const forkKey = `fork-${manager.nextForkId++}`;
        const forkName = name || `Ruta Fork ${manager.nextForkId - 1} [B]`;
        const branchData = this.snapshotRoute(manager, forkName);

        manager.forks.set(forkKey, branchData);
        this.saveToStorage(manager);
        this.updateBranchSelector(manager);
        this.switchBranch(manager, forkKey);
        LogSystem.add('INFO', `Ruta bifurcada exitosamente: '${forkName}'.`);
    },

    /**
     * Cambia la rama activa de planificación ('main' o 'fork-N').
     */
    switchBranch(manager, branchKey) {
        if (manager.isRunning) {
            return LogSystem.add('WARN', 'Ruta en ejecución. Detén antes de cambiar de rama.');
        }
        this._backupCurrentBranch(manager);
        manager.activeBranch = branchKey;

        const targetData = this._resolveTargetBranch(manager, branchKey);
        if (!targetData) return;

        this._applyBranchData(manager, targetData, branchKey);
        this._updateBranchUI(manager, branchKey);
    },

    _backupCurrentBranch(manager) {
        if (manager.activeBranch === 'main') {
            manager.mainBranchBackup = this.snapshotRoute(manager, manager.mainRouteName);
        } else if (manager.forks.has(manager.activeBranch)) {
            const currentName = manager.forks.get(manager.activeBranch).name;
            manager.forks.set(manager.activeBranch, this.snapshotRoute(manager, currentName));
            this.saveToStorage(manager);
        }
    },

    _resolveTargetBranch(manager, branchKey) {
        if (branchKey === 'main') {
            return manager.mainBranchBackup || {
                mode: 'rect', currentOrigin: { x: 0, y: 0 },
                logicalSteps: [], queue: [], nextLogicalStepId: 1
            };
        }
        return manager.forks.get(branchKey);
    },

    _applyBranchData(manager, targetData, branchKey) {
        manager.logicalSteps = JSON.parse(JSON.stringify(targetData.logicalSteps || []));
        manager.queue = JSON.parse(JSON.stringify(targetData.queue || []));
        manager.currentOrigin = { ...(targetData.currentOrigin || { x: 0, y: 0 }) };
        manager.nextLogicalStepId = targetData.nextLogicalStepId || (manager.logicalSteps.length + 1);
        manager.currentStepIndex = 0;

        const modeSelect = /** @type {HTMLSelectElement | null} */ (document.getElementById('route-mode'));
        if (modeSelect && targetData.mode) {
            modeSelect.value = targetData.mode;
        }
        if (manager.routeNameInput) {
            manager.routeNameInput.value = branchKey === 'main'
                ? (manager.mainRouteName || 'Ruta Planificada Robot_A')
                : (manager.forks.get(branchKey)?.name || `Ruta Fork [${branchKey}]`);
        }
    },

    _updateBranchUI(manager, branchKey) {
        const delBtn = document.getElementById('btn-delete-branch');
        if (delBtn) delBtn.style.display = branchKey === 'main' ? 'none' : 'inline-flex';

        const sel = /** @type {HTMLSelectElement | null} */ (document.getElementById('route-branch-select'));
        if (sel) sel.value = branchKey;

        manager.updateModeLock();
        manager.updateListUI();
        manager.updateLabels();
        manager.updatePreview();
        manager.autoScale();
        manager.drawPlan();
    },

    /**
     * Elimina una rama bifurcada secundaria.
     */
    deleteBranch(manager, branchKey) {
        if (branchKey === 'main') {
            return LogSystem.add('WARN', 'La ruta principal (A) no puede ser eliminada.');
        }
        const name = manager.forks.get(branchKey)?.name || branchKey;
        manager.forks.delete(branchKey);
        this.saveToStorage(manager);
        this.updateBranchSelector(manager);

        if (manager.activeBranch === branchKey) {
            this.switchBranch(manager, 'main');
        } else {
            manager.drawPlan();
        }
        LogSystem.add('INFO', `Rama bifurcada '${name}' eliminada.`);
    },

    /**
     * Sincroniza en memoria y almacenamiento la rama activa modificada.
     */
    syncActiveBranch(manager) {
        if (manager.activeBranch !== 'main' && manager.forks.has(manager.activeBranch)) {
            const currentName = manager.forks.get(manager.activeBranch).name;
            manager.forks.set(manager.activeBranch, this.snapshotRoute(manager, currentName));
            this.saveToStorage(manager);
        } else if (manager.activeBranch === 'main') {
            manager.mainBranchBackup = this.snapshotRoute(manager, manager.mainRouteName);
        }
    },

    /**
     * Alterna la visualización de comparación A/B en el gráfico.
     */
    toggleCompareAB(manager) {
        manager.comparingAB = !manager.comparingAB;
        const btn = document.getElementById('btn-compare-branches');
        const metricsWrap = document.getElementById('route-fork-metrics');
        if (btn) {
            btn.classList.toggle('active', manager.comparingAB);
            btn.classList.toggle('btn-accent', manager.comparingAB);
        }
        if (metricsWrap) {
            metricsWrap.style.display = manager.comparingAB ? 'inline-flex' : 'none';
        }
        manager.drawPlan();
    },

    /**
     * Actualiza el selector HTML de ramas disponibles.
     */
    updateBranchSelector(manager) {
        const sel = /** @type {HTMLSelectElement | null} */ (document.getElementById('route-branch-select'));
        if (!sel) return;
        const baseLabel = manager.mainRouteName ? `${manager.mainRouteName} (Base A)` : 'Ruta Base (A)';
        sel.innerHTML = `<option value="main">${baseLabel}</option>`;
        manager.forks.forEach((data, key) => {
            const opt = document.createElement('option');
            opt.value = key;
            opt.textContent = data.name;
            sel.appendChild(opt);
        });
        sel.value = manager.activeBranch;
    },

    /**
     * Persiste las ramas en localStorage.
     */
    saveToStorage(manager) {
        try {
            const serializable = {};
            manager.forks.forEach((val, key) => { serializable[key] = val; });
            localStorage.setItem(this.STORAGE_KEY, JSON.stringify(serializable));
        } catch (_) {}
    },

    /**
     * Carga las ramas desde localStorage.
     */
    loadFromStorage(manager) {
        try {
            const raw = localStorage.getItem(this.STORAGE_KEY);
            if (!raw) return;
            const parsed = JSON.parse(raw);
            Object.entries(parsed).forEach(([k, v]) => {
                manager.forks.set(k, v);
                const match = k.match(/fork-(\d+)/);
                if (match) manager.nextForkId = Math.max(manager.nextForkId, parseInt(match[1], 10) + 1);
            });
            this.updateBranchSelector(manager);
        } catch (_) {}
    }
};

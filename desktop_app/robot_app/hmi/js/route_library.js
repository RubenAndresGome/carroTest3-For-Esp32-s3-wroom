// @ts-check

/* ==============================================
   BIBLIOTECA DE RUTAS GUARDADAS
   Guarda la ruta sin ejecutarla y permite recargarla reproduciendo
   las entradas originales del formulario (mismo cálculo que al capturar).
   ============================================== */
const RouteLibrary = {
    STORAGE_KEY: 'hmi_saved_routes',
    MAX_ROUTES: 20,
    /** @type {HTMLSelectElement | null} */
    select: null,
    /** @type {HTMLButtonElement | null} */
    loadBtn: null,

    /** @param {any} manager */
    init(manager) {
        this.select = /** @type {HTMLSelectElement | null} */ (document.getElementById('route-library-select'));
        this.loadBtn = /** @type {HTMLButtonElement | null} */ (document.getElementById('btn-route-library-load'));
        document.getElementById('btn-route-save')?.addEventListener('click', () => this.save(manager));
        this.select?.addEventListener('change', () => { if (this.loadBtn) this.loadBtn.disabled = !this.select?.value; });
        this.loadBtn?.addEventListener('click', () => this.load(manager, this.select?.value || ''));
        this.refresh();
    },

    /** @returns {any[]} */
    readAll() {
        try {
            const data = JSON.parse(localStorage.getItem(this.STORAGE_KEY) || '[]');
            return Array.isArray(data) ? data : [];
        } catch (_) { return []; }
    },

    /** @param {any[]} routes */
    writeAll(routes) {
        try { localStorage.setItem(this.STORAGE_KEY, JSON.stringify(routes.slice(0, this.MAX_ROUTES))); } catch (_) {}
    },

    /**
     * Registro reproducible de la ruta: modo + entradas del formulario por paso.
     * @param {any} manager @param {string} name @param {any} mode
     */
    buildRecord(manager, name, mode) {
        return {
            name,
            savedAt: new Date().toISOString(),
            steps: manager.queue.length,
            mode: mode.key,
            inputs: manager.logicalSteps.map((/** @type {any} */ s) => Array.isArray(s.inputValues) ? s.inputValues : null),
            payload: mode.missionPayload(manager)
        };
    },

    /** @param {any} record */
    upsert(record) {
        const routes = this.readAll();
        const idx = routes.findIndex(r => r.name === record.name);
        if (idx >= 0) routes[idx] = record; else routes.unshift(record);
        this.writeAll(routes);
        this.refresh();
        return idx >= 0;
    },

    /** @param {any} manager */
    save(manager) {
        if (manager.logicalSteps.length === 0) {
            return OperatorFeedback.show('route-start-alert', 'Agrega al menos un paso antes de guardar la ruta.', 'warn');
        }
        const name = manager.routeNameInput?.value.trim();
        if (!name) {
            manager.routeNameInput?.focus();
            return OperatorFeedback.show('route-start-alert', 'Escribe un nombre en el paso 1 para guardar la ruta.', 'warn');
        }
        manager.saveCurrentRouteName();
        const replaced = this.upsert(this.buildRecord(manager, name, manager.activeMode()));
        const msg = replaced ? `Ruta «${name}» actualizada.` : `Ruta «${name}» guardada con ${manager.logicalSteps.length} paso(s).`;
        OperatorFeedback.show('route-start-alert', msg, 'info');
        LogSystem.add('INFO', msg);
    },

    refresh() {
        if (!this.select) return;
        const routes = this.readAll();
        const current = this.select.value;
        this.select.innerHTML = '';
        const placeholder = document.createElement('option');
        placeholder.value = '';
        placeholder.textContent = routes.length ? 'Elegir ruta guardada…' : 'Aún no hay rutas guardadas';
        this.select.appendChild(placeholder);
        routes.forEach(r => {
            const opt = document.createElement('option');
            opt.value = r.name;
            const replayable = Array.isArray(r.inputs) && r.inputs.length && r.inputs.every((/** @type {any} */ v) => Array.isArray(v));
            opt.textContent = replayable ? `${r.name} · ${r.inputs.length} paso(s)` : `${r.name} · solo historial`;
            opt.disabled = !replayable;
            this.select?.appendChild(opt);
        });
        this.select.value = routes.some(r => r.name === current) ? current : '';
        if (this.loadBtn) this.loadBtn.disabled = !this.select.value;
    },

    /** @param {any} manager @param {string} name */
    load(manager, name) {
        const record = this.readAll().find(r => r.name === name);
        if (!record || !Array.isArray(record.inputs)) return;
        if (manager.isRunning) {
            return OperatorFeedback.show('route-start-alert', 'No se puede cargar una ruta mientras otra se ejecuta.', 'warn');
        }
        if (manager.logicalSteps.length && !confirm(`¿Reemplazar la ruta actual por «${name}»?`)) return;

        manager.queue = []; manager.logicalSteps = []; manager.nextLogicalStepId = 1;
        manager.currentStepIndex = 0; manager.stoppedAtIndex = null;
        manager.currentOrigin = { x: state.telemetria.posX, y: state.telemetria.posY };
        manager.updateModeLock();
        manager.modeSelect.value = record.mode;
        manager.modeSelect.dispatchEvent(new Event('change'));

        for (const [v1, v2] of record.inputs) {
            manager.input1.value = String(v1);
            manager.input2.value = String(v2);
            manager.addStep();
        }
        if (manager.routeNameInput) manager.routeNameInput.value = name;
        manager.saveCurrentRouteName();
        const msg = `Ruta «${name}» cargada con ${manager.logicalSteps.length} paso(s). Revisa la cola antes de ejecutar.`;
        OperatorFeedback.show('route-start-alert', msg, 'info');
        LogSystem.add('INFO', msg);
    }
};

if (typeof window !== 'undefined') {
    // @ts-ignore
    window.RouteLibrary = RouteLibrary;
}

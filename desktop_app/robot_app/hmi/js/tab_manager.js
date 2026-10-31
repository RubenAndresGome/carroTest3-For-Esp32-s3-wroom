// @ts-check
/** @typedef {import('@frontend/protocol').Telemetry} TelemetryPayload */
/** @typedef {import('@frontend/protocol').CommandName} CommandName */

/* ==============================================
   TAB MANAGER
   ============================================== */
const TabManager = {
    _handleWindowTaskbarClick(tabId, winId) {
        if (typeof WindowManager === 'undefined') return false;
        const win = WindowManager.windows.get(winId);
        if (!win) return false;

        if (WindowManager.mode === 'single') {
            if (!Calibration.calibrated && tabId !== 'conexion') {
                LogSystem.add('WARN', 'Completa primero la calibración física de torque y giro.');
                return true;
            }
            if (typeof RouteManager !== 'undefined' && RouteManager.isRunning && tabId !== 'rutas') return true;
            WindowManager.focusSingle(winId);
            return true;
        }

        if (WindowManager.mode !== 'tabs') {
            if (win.isMinimized) {
                WindowManager.restoreWindow(winId);
                WindowManager.bringToFront(winId);
                if (WindowManager.mode === 'tiling') {
                    win.el.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
                }
            } else if (win.el.classList.contains('is-focused') && WindowManager.mode === 'floating') {
                WindowManager.minimizeWindow(winId);
            } else {
                WindowManager.bringToFront(winId);
                if (WindowManager.mode === 'tiling') {
                    win.el.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
                }
            }
            return true;
        }

        if (win.isMinimized) {
            WindowManager.restoreWindow(winId);
            this.switch(tabId);
            return true;
        }
        return false;
    },

    /**
     * @param {KeyboardEvent} event
     * @param {number} index
     * @param {HTMLElement[]} tabs
     */
    _handleKeyNavigation(event, index, tabs) {
        let targetIndex = null;
        if (event.key==='ArrowRight') targetIndex = (index + 1) % tabs.length;
        if (event.key==='ArrowLeft') targetIndex = (index - 1 + tabs.length) % tabs.length;
        if (event.key==='Home') targetIndex = 0;
        if (event.key==='End') targetIndex = tabs.length - 1;
        if (targetIndex === null) return;
        event.preventDefault();
        tabs[targetIndex].focus();
        tabs[targetIndex].scrollIntoView({ block: 'nearest', inline: 'nearest' });
    },

    init() {
        const tabs = /** @type {HTMLElement[]} */ ([...document.querySelectorAll('.tab-btn')]);
        tabs.forEach((button, index) => {
            button.addEventListener('click', () => {
                if (button.getAttribute('aria-disabled') === 'true') return;
                const tabId = button.dataset.tab;
                const winId = button.getAttribute('data-win') || (tabId === 'boost' ? 'tab-boost' : `tab-${tabId}`);
                if (this._handleWindowTaskbarClick(tabId, winId)) return;
                this.switch(tabId);
            });
            button.addEventListener('keydown', event => this._handleKeyNavigation(event, index, tabs));
        });
    },

    _syncWindowManagerTab(id) {
        if (typeof WindowManager === 'undefined') return;
        const winId = id.startsWith('tab-') ? id : `tab-${id}`;
        const win = WindowManager.windows.get(winId) || WindowManager.windows.get(id);
        if (!win) return;

        if (WindowManager.mode === 'single') {
            WindowManager.focusSingle(win.id);
            return;
        }
        win.isMinimized = false;
        win.el.classList.remove('is-minimized');
        win.el.style.display = (WindowManager.mode === 'tiling' || WindowManager.mode === 'floating') ? 'flex' : 'block';
        if (WindowManager.mode === 'floating') {
            WindowLayout.applyWindowRect(win);
        } else if (WindowManager.mode === 'tiling') {
            win.el.style.width = '100%';
            win.el.style.height = win.tilingHeight ? `${win.tilingHeight}px` : '520px';
            win.el.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
        }
        WindowManager.bringToFront(win.id);
    },

    switch(id) {
        const requestedTab = document.querySelector(`.tab-btn[data-tab="${id}"]`);
        if (!requestedTab || requestedTab.getAttribute('aria-disabled') === 'true') return;
        if (!Calibration.calibrated && id !== 'conexion') return LogSystem.add('WARN', 'Completa primero la calibración física de torque y giro.');
        if (typeof RouteManager !== 'undefined' && RouteManager.isRunning && id !== 'rutas') return;
        if (state.activeTab === 'touch' && id !== 'touch') (/** @type {RobotJoystick} */ (/** @type {unknown} */ (Touch))).end();
        state.activeTab = id;

        document.querySelectorAll('.tab-btn').forEach(elem => {
            const b = /** @type {HTMLElement} */ (elem);
            const a = b.dataset.tab === id;
            b.classList.toggle('active', a);
            if (a) b.classList.remove('is-minimized');
            b.setAttribute('aria-selected', a ? 'true' : 'false');
            b.tabIndex = a ? 0 : -1;
        });

        document.querySelectorAll('.tab-content').forEach(elem => {
            const c = /** @type {HTMLElement} */ (elem);
            const isActive = c.id === `tab-${id}` || c.id === id;
            c.classList.toggle('active', isActive);
            if (isActive) {
                const isFlexMode = typeof WindowManager !== 'undefined' && ['single', 'floating', 'tiling'].includes(WindowManager.mode);
                c.style.display = isFlexMode ? 'flex' : 'block';
            } else if (typeof WindowManager === 'undefined' || WindowManager.mode === 'tabs' || WindowManager.mode === 'single') {
                c.style.display = 'none';
            }
        });

        this._syncWindowManagerTab(id);
        if (id === 'rutas' && typeof RouteManager !== 'undefined') RouteManager.resizeCharts();
    }
};

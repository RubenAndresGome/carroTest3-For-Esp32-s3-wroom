// @ts-check
/** @typedef {import('@frontend/protocol').Telemetry} TelemetryPayload */
/** @typedef {import('@frontend/protocol').CommandName} CommandName */

/* ==============================================
   WINDOW MANAGER (Multi-Panel Workspace & Window Lifecycle)
   ============================================== */

const WindowManager = {
    mode: 'tabs',
    preferredMode: 'tabs',
    activeSingleId: 'tab-conexion',
    windows: new Map(),
    maxZIndex: 10,
    workspaceEl: null,

    init() {
        this._initWorkspaceEl();
        this._bindModeButtons();
        this._bindForkMenu();
        this._registerPanels();
        const savedMode = (typeof SettingsManager !== 'undefined' && SettingsManager.settings?.default_mode)
            || localStorage.getItem('hmi_workspace_mode') || 'tabs';
        this.preferredMode = ['tabs', 'single', 'tiling', 'floating'].includes(savedMode) ? savedMode : 'tabs';
        this.setMode(this.preferredMode, true, window.innerWidth <= 900);
        window.addEventListener('resize', () => {
            if (window.innerWidth <= 900 && (this.mode === 'floating' || this.mode === 'tiling')) this.setMode('tabs', true, true);
            else if (window.innerWidth > 900 && this.mode !== this.preferredMode) this.setMode(this.preferredMode, true, true);
        });
    },
    _initWorkspaceEl() {
        this.workspaceEl = document.getElementById('hmi-workspace');
        if (!this.workspaceEl) {
            const firstTab = document.querySelector('.tab-content');
            if (firstTab?.parentNode) {
                const parent = firstTab.parentNode, ws = document.createElement('div');
                ws.id = 'hmi-workspace'; ws.className = 'workspace-tabs';
                parent.insertBefore(ws, firstTab);
                parent.querySelectorAll('.tab-content').forEach(tab => ws.appendChild(tab));
                this.workspaceEl = ws;
            }
        }
    },
    _bindModeButtons() {
        ['tabs', 'single', 'tiling', 'floating'].forEach(m => {
            const btn = document.getElementById(`ws-mode-${m}`);
            if (btn) btn.addEventListener('click', () => this.setMode(m));
        });
    },
    _bindForkMenu() {
        const btnAdd = document.getElementById('ws-btn-add-fork'), menu = document.getElementById('glass-fork-menu'), closeBtn = document.getElementById('btn-close-fork-menu');
        if (btnAdd && menu) {
            btnAdd.addEventListener('click', (e) => {
                e.stopPropagation();
                const isHidden = menu.style.display === 'none';
                if (isHidden) { this.renderForkMenu(); menu.style.display = 'block'; btnAdd.setAttribute('aria-expanded', 'true'); }
                else { menu.style.display = 'none'; btnAdd.setAttribute('aria-expanded', 'false'); }
            });
        }
        if (closeBtn && menu) closeBtn.addEventListener('click', () => { menu.style.display = 'none'; btnAdd?.setAttribute('aria-expanded', 'false'); });
        document.addEventListener('click', (e) => {
            if (menu && menu.style.display !== 'none' && !menu.contains(/** @type {Node} */ (e.target)) && e.target !== btnAdd) {
                menu.style.display = 'none'; btnAdd?.setAttribute('aria-expanded', 'false');
            }
        });
    },
    _registerPanels() {
        const panels = [
            { id: 'tab-conexion', title: 'Conexión y Sesiones', icon: '<svg class="ui-icon" aria-hidden="true" viewBox="0 0 24 24"><path d="M8 12a4 4 0 0 1 4-4h3"/><path d="M16 8l-2-2"/><path d="M16 8l-2 2"/><path d="M16 12a4 4 0 0 1-4 4H9"/><path d="M8 16l2-2"/><path d="M8 16l2 2"/></svg>', singleton: true },
            { id: 'tab-mapa', title: 'Mapa de Navegación', icon: '<svg class="ui-icon" aria-hidden="true" viewBox="0 0 24 24"><path d="M3 6l5-2 8 3 5-2v13l-5 2-8-3-5 2z"/><path d="M8 4v13M16 7v13"/></svg>', singleton: true, forkable: true },
            { id: 'tab-testing', title: 'Testing S3', icon: '<svg class="ui-icon" aria-hidden="true" viewBox="0 0 24 24"><path d="M9 3h6"/><path d="M10 3v5l-5 9a2 2 0 0 0 2 3h10a2 2 0 0 0 2-3l-5-9V3"/><path d="M8 14h8"/></svg>', singleton: true },
            { id: 'tab-touch', title: 'Control Touch', icon: '<svg class="ui-icon" aria-hidden="true" viewBox="0 0 24 24"><rect x="7" y="2" width="10" height="20" rx="2"/><path d="M10 18h4"/></svg>', singleton: true },
            { id: 'tab-rutas', title: 'Programador de Rutas', icon: '<svg class="ui-icon" aria-hidden="true" viewBox="0 0 24 24"><circle cx="6" cy="18" r="2"/><circle cx="18" cy="6" r="2"/><path d="M8 18h3a3 3 0 0 0 3-3v-6a3 3 0 0 1 3-3"/></svg>', singleton: true, forkable: true },
            { id: 'tab-boost', title: 'HUD Cinético & Boost', icon: '<svg class="ui-icon" aria-hidden="true" viewBox="0 0 24 24"><polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2"/></svg>', singleton: true, isCustomWidget: true, initiallyMinimized: true }
        ];
        panels.forEach((p, idx) => {
            const el = document.getElementById(p.id);
            if (!el) return;
            this.registerWindow(p.id, el, p.title, p.icon, {
                isSingleton: p.singleton, isForkable: Boolean(p.forkable),
                isCustomWidget: Boolean(p.isCustomWidget), initiallyMinimized: Boolean(p.initiallyMinimized),
                initialRect: { x: 20 + (idx * 28) % 240, y: 20 + (idx * 28) % 180, width: 620, height: 480 }
            });
        });
    },
    _getTabButton(id) {
        if (!id) return null;
        const cleanId = id.replace(/^tab-/, '');
        return document.getElementById(`tab-control-${cleanId}`) || document.getElementById(`tab-control-${id}`) ||
               document.querySelector(`.tab-btn[data-tab="${cleanId}"]`) || document.querySelector(`.tab-btn[data-win="${id}"]`);
    },
    _onTaskbarButtonClick(e, id, cleanId) {
        if (e.target?.classList.contains('tab-btn-close')) { e.stopPropagation(); this.closeWindow(id); return; }
        const win = this.windows.get(id);
        if (!win) return;
        if (win.isMinimized) { this.restoreWindow(id); return; }
        if (this.mode === 'tabs') {
            if (typeof TabManager !== 'undefined') TabManager.switch(cleanId); else this.restoreWindow(id);
            return;
        }
        if (this.mode === 'single') { this.focusSingle(id); return; }
        const isCurrentlyActive = win.el.classList.contains('is-focused') && !win.isMinimized;
        if (isCurrentlyActive && this.mode === 'floating') this.minimizeWindow(id);
        else { this.bringToFront(id); if (this.mode === 'tiling') win.el.scrollIntoView({ behavior: 'smooth', block: 'nearest' }); }
    },
    ensureTaskbarButton(id, title, icon, isSingleton = false) {
        let btn = this._getTabButton(id);
        if (!btn) {
            const tabBar = document.querySelector('.tab-bar');
            if (!tabBar) return null;
            const cleanId = id.replace(/^tab-/, ''), isPermanent = ['tab-conexion', 'tab-mapa', 'tab-testing', 'tab-touch', 'tab-rutas'].includes(id);
            btn = document.createElement('button');
            btn.id = `tab-control-${cleanId}`; btn.className = 'tab-btn tab-btn-dynamic';
            btn.dataset.tab = cleanId; btn.dataset.win = id;
            btn.setAttribute('role', 'tab'); btn.setAttribute('aria-controls', id); btn.setAttribute('aria-selected', 'false');
            btn.tabIndex = -1;
            btn.innerHTML = `${icon}<span class="tab-btn-title">${title}</span>${!isPermanent ? `<span class="tab-btn-close" title="Cerrar ventana" aria-label="Cerrar">✕</span>` : ''}`;
            btn.addEventListener('click', (e) => this._onTaskbarButtonClick(e, id, cleanId));
            tabBar.appendChild(btn);
        }
        return btn;
    },
    openWindow(id) {
        const fullId = id.startsWith('tab-') ? id : `tab-${id}`;
        let win = this.windows.get(fullId) || this.windows.get(id);
        if (!win && fullId === 'tab-boost') {
            const el = document.getElementById('tab-boost');
            if (el) {
                this.registerWindow('tab-boost', el, 'HUD Cinético & Boost', '<svg class="ui-icon" aria-hidden="true" viewBox="0 0 24 24"><polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2"/></svg>', { isSingleton: true, isForkable: false });
                win = this.windows.get('tab-boost');
            }
        }
        if (!win) return null;
        this.ensureTaskbarButton(win.id, win.title, win.icon, false);
        this.focusOrRestoreWindow(win.id);
        return win.id;
    },

    focusOrRestoreWindow(id) {
        const win = this.windows.get(id);
        if (!win) return;
        if (this.mode === 'single') { this.focusSingle(id); return; }
        this.restoreWindow(id);
        this.bringToFront(id);
    },

    focusSingle(id) {
        const fullId = this.windows.has(id) ? id : (id.startsWith('tab-') ? id : `tab-${id}`);
        const win = this.windows.get(fullId);
        if (!win) return;
        this.activeSingleId = fullId;
        const cleanId = fullId.replace(/^tab-/, '');
        if (typeof state !== 'undefined' && ['conexion', 'mapa', 'testing', 'touch', 'rutas'].includes(cleanId)) state.activeTab = cleanId;
        if (this.mode === 'single') {
            WindowLayout.applySingle(this.windows, this.activeSingleId, wid => this._getTabButton(wid), wid => this.maximizeWindow(wid));
            if (typeof RouteManager !== 'undefined' && RouteManager.resizeCharts) setTimeout(() => RouteManager.resizeCharts(), 50);
        }
    },

    registerWindow(id, element, title, icon, options = {}) {
        if (!element) return;
        element.classList.add('window-frame');
        let header = element.querySelector('.window-header');
        if (!header) {
            header = this._createWindowHeader(id, title, icon, options);
            element.insertBefore(header, element.firstChild);
        }
        let resizeHandle = element.querySelector('.window-resize-handle');
        if (!resizeHandle) {
            resizeHandle = document.createElement('div');
            resizeHandle.className = 'window-resize-handle';
            resizeHandle.setAttribute('title', 'Arrastrar para redimensionar (táctil / ratón)');
            resizeHandle.setAttribute('aria-label', 'Redimensionar ventana');
            element.appendChild(resizeHandle);
        }
        const isInitiallyMin = Boolean(options.initiallyMinimized);
        const winData = {
            id, el: element, headerEl: header, title, icon,
            isMinimized: isInitiallyMin, isMaximized: false, isTileFocused: false,
            isSingleton: Boolean(options.isSingleton),
            isFork: Boolean(options.isFork) || id.startsWith('fork-'),
            isCustomWidget: Boolean(options.isCustomWidget) || id.startsWith('custom-win-'),
            rect: options.initialRect || { x: 30, y: 30, width: 600, height: 460 },
            zIndex: ++this.maxZIndex
        };
        if (isInitiallyMin) { element.classList.add('is-minimized'); element.style.display = 'none'; }
        this.windows.set(id, winData);
        this._bindHeaderControls(header, id);
        element.addEventListener('pointerdown', () => this.bringToFront(id));
        header.addEventListener('pointerdown', (e) => this._startDrag(e, id));
        header.addEventListener('dblclick', (e) => {
            if (/** @type {HTMLElement} */ (e.target)?.closest?.('.window-controls, .win-btn, button')) return;
            if (this.mode === 'tiling') this.toggleTileFocus(id);
            else if (this.mode === 'floating') this.maximizeWindow(id);
        });
        resizeHandle.addEventListener('pointerdown', (e) => this._startResize(e, id));
        WindowInteract.observeTilingDimensions(winData, () => this.mode);
        if (this.mode === 'floating') WindowLayout.applyWindowRect(winData);
    },

    _createWindowHeader(id, title, icon, options) {
        const header = document.createElement('div');
        header.className = 'window-header';
        const isPermanent = ['tab-conexion', 'tab-mapa', 'tab-testing', 'tab-touch', 'tab-rutas'].includes(id);
        header.innerHTML = `<div class="window-title"><span>${icon}</span><span>${title}</span></div><div class="window-controls">${options.isForkable ? `<button type="button" class="win-btn win-btn-fork" title="Bifurcar vista" aria-label="Bifurcar"><svg class="ui-icon" aria-hidden="true" viewBox="0 0 24 24"><circle cx="18" cy="18" r="3"/><circle cx="6" cy="6" r="3"/><circle cx="18" cy="6" r="3"/><path d="M6 9v3a3 3 0 0 0 3 3h6M18 9v9"/></svg></button>` : ''}<button type="button" class="win-btn win-btn-focus" title="Enfocar mosaico" aria-label="Enfocar mosaico">⊡</button><button type="button" class="win-btn win-btn-min" title="Minimizar a la barra" aria-label="Minimizar">—</button><button type="button" class="win-btn win-btn-max" title="Maximizar" aria-label="Maximizar">□</button>${!isPermanent ? `<button type="button" class="win-btn win-btn-close" title="Cerrar" aria-label="Cerrar">✕</button>` : ''}</div>`;
        return header;
    },

    _bindHeaderControls(header, id) {
        const forkBtn = header.querySelector('.win-btn-fork');
        const focusBtn = header.querySelector('.win-btn-focus');
        const minBtn = header.querySelector('.win-btn-min');
        const maxBtn = header.querySelector('.win-btn-max');
        const closeBtn = header.querySelector('.win-btn-close');
        if (forkBtn) forkBtn.addEventListener('click', (e) => {
            e.stopPropagation();
            if (id === 'tab-rutas' && typeof RouteManager !== 'undefined') RouteManager.forkCurrentRoute();
            else this.createForkWindow(id);
        });
        if (focusBtn) focusBtn.addEventListener('click', (e) => { e.stopPropagation(); this.toggleTileFocus(id); });
        if (minBtn) minBtn.addEventListener('click', (e) => { e.stopPropagation(); this.minimizeWindow(id); });
        if (maxBtn) maxBtn.addEventListener('click', (e) => { e.stopPropagation(); this.maximizeWindow(id); });
        if (closeBtn) closeBtn.addEventListener('click', (e) => { e.stopPropagation(); this.closeWindow(id); });
    },

    setMode(mode, silent = false, isResponsiveOverride = false) {
        if (!['tabs', 'single', 'tiling', 'floating'].includes(mode)) return;
        if (!isResponsiveOverride) this.preferredMode = mode;
        if (this.mode === 'tiling') WindowLayout.preserveTilingDimensions(this.windows);
        let effectiveMode = mode;
        if (window.innerWidth <= 900 && (mode === 'floating' || mode === 'tiling')) {
            if (!silent && typeof LogSystem !== 'undefined') LogSystem.add('INFO', 'Espacio multi-panel adaptado a modo pestañas en pantallas compactas.');
            effectiveMode = 'tabs';
        }
        this.mode = effectiveMode;
        if (!isResponsiveOverride) { try { localStorage.setItem('hmi_workspace_mode', this.preferredMode); } catch (_) {} }
        this._syncModeSelectors(effectiveMode, silent, isResponsiveOverride);
        if (this.workspaceEl) {
            this.workspaceEl.classList.remove('workspace-tabs', 'workspace-single', 'workspace-tiling', 'workspace-floating');
            this.workspaceEl.classList.add(`workspace-${effectiveMode}`);
        }
        const activeTab = (typeof state !== 'undefined' && state.activeTab) ? state.activeTab : 'conexion';
        const activeWinId = activeTab.startsWith('tab-') ? activeTab : `tab-${activeTab}`;
        if (effectiveMode === 'tabs') WindowLayout.applyTabs(this.windows, id => this._getTabButton(id), id => this.maximizeWindow(id));
        else if (effectiveMode === 'single') {
            this.activeSingleId = this.windows.has(this.activeSingleId) ? this.activeSingleId : activeWinId;
            WindowLayout.applySingle(this.windows, this.activeSingleId, id => this._getTabButton(id), id => this.maximizeWindow(id));
        } else if (effectiveMode === 'tiling') WindowLayout.applyTiling(this.windows, id => this._getTabButton(id), id => this.maximizeWindow(id));
        else if (effectiveMode === 'floating') WindowLayout.applyFloating(this.windows, activeWinId, id => this._getTabButton(id), id => this.bringToFront(id), silent);

        if (typeof RouteManager !== 'undefined' && RouteManager.resizeCharts) setTimeout(() => RouteManager.resizeCharts(), 60);
    },

    _syncModeSelectors(effectiveMode, silent, isResponsiveOverride = false) {
        const selMode = /** @type {HTMLSelectElement | null} */ (document.getElementById('setting-default-mode'));
        if (selMode && !isResponsiveOverride && selMode.value !== this.preferredMode) selMode.value = this.preferredMode;
        if (!silent && !isResponsiveOverride && typeof SettingsManager !== 'undefined' && SettingsManager.settings && SettingsManager.settings.default_mode !== this.preferredMode) {
            SettingsManager.settings.default_mode = this.preferredMode;
            try { localStorage.setItem(SettingsManager.STORAGE_KEY, JSON.stringify(SettingsManager.settings)); } catch (_) {}
            SettingsManager.syncBackend();
        }
        ['tabs', 'single', 'tiling', 'floating'].forEach(m => {
            const btn = document.getElementById(`ws-mode-${m}`);
            if (btn) {
                const isActive = m === effectiveMode;
                btn.classList.toggle('active', isActive);
                btn.setAttribute('aria-pressed', isActive ? 'true' : 'false');
            }
        });
    },

    bringToFront(id) {
        const win = this.windows.get(id);
        if (!win) return;
        if (this.maxZIndex >= 80) {
            const sorted = Array.from(this.windows.values()).sort((a, b) => (a.zIndex || 10) - (b.zIndex || 10));
            sorted.forEach((w, idx) => {
                w.zIndex = 11 + idx;
                if (this.mode === 'floating') w.el.style.zIndex = String(w.zIndex);
            });
            this.maxZIndex = 10 + sorted.length;
        }
        win.zIndex = ++this.maxZIndex;
        if (this.mode === 'floating') win.el.style.zIndex = String(win.zIndex);
        this.windows.forEach(w => w.el.classList.remove('is-focused'));
        win.el.classList.add('is-focused');
        this.windows.forEach(w => {
            const btn = this._getTabButton(w.id);
            if (!btn) return;
            if (w.id === id) {
                w.isMinimized = false; w.el.classList.remove('is-minimized');
                btn.classList.add('active'); btn.classList.remove('is-minimized');
                btn.setAttribute('aria-selected', 'true'); btn.tabIndex = 0;
            } else {
                btn.classList.remove('active'); btn.setAttribute('aria-selected', 'false');
                btn.tabIndex = -1; btn.classList.toggle('is-minimized', w.isMinimized);
            }
        });
    },

    minimizeWindow(id) {
        const win = this.windows.get(id);
        if (!win) return;
        if (this.mode === 'single') {
            const fallback = Array.from(this.windows.keys()).find(k => k !== id && !this.windows.get(k)?.isMinimized) || 'tab-conexion';
            this.focusSingle(fallback);
            return;
        }
        win.isMinimized = true;
        win.el.style.display = 'none';
        win.el.classList.add('is-minimized');
        win.el.classList.remove('is-focused', 'is-tile-focused');
        if (this.mode === 'tabs') win.el.classList.remove('active');
        const btn = this._getTabButton(id);
        if (btn) {
            btn.classList.remove('active'); btn.classList.add('is-minimized');
            btn.setAttribute('aria-selected', 'false'); btn.tabIndex = -1;
        }
    },

    restoreWindow(id) {
        const win = this.windows.get(id);
        if (!win) return;
        if (this.mode === 'single') { this.focusSingle(id); return; }
        win.isMinimized = false;
        win.el.classList.remove('is-minimized');
        const cleanId = id.replace(/^tab-/, '');
        if (this.mode === 'floating') {
            win.el.style.display = 'flex';
            WindowLayout.applyWindowRect(win);
        } else if (this.mode === 'tiling') {
            win.el.style.display = 'flex';
            win.el.style.width = '100%';
            win.el.style.height = win.tilingHeight ? `${win.tilingHeight}px` : '520px';
        } else {
            win.el.style.display = 'block';
            win.el.classList.add('active');
            if (typeof state !== 'undefined') state.activeTab = cleanId;
            document.querySelectorAll('.tab-content').forEach(elem => {
                const c = /** @type {HTMLElement} */ (elem);
                if (c.id !== id && c.id !== win.id) { c.classList.remove('active'); c.style.display = 'none'; }
            });
        }
        const btn = this._getTabButton(id);
        if (btn) {
            btn.classList.remove('is-minimized'); btn.classList.add('active');
            btn.setAttribute('aria-selected', 'true'); btn.tabIndex = 0;
        }
        this.bringToFront(id);
        if (this.mode === 'tiling') win.el.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
        if (cleanId === 'rutas' && typeof RouteManager !== 'undefined' && RouteManager.resizeCharts) RouteManager.resizeCharts();
    },

    toggleTileFocus(id) {
        const win = this.windows.get(id);
        if (!win) return;
        const next = !win.el.classList.contains('is-tile-focused');
        this.windows.forEach(w => { w.isTileFocused = false; w.el.classList.remove('is-tile-focused'); });
        win.isTileFocused = next;
        win.el.classList.toggle('is-tile-focused', next);
        if (next) win.el.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
        if (typeof RouteManager !== 'undefined' && RouteManager.resizeCharts) setTimeout(() => RouteManager.resizeCharts(), 50);
    },

    maximizeWindow(id) {
        const win = this.windows.get(id);
        if (!win) return;
        win.isMaximized = !win.isMaximized;
        win.el.classList.toggle('is-maximized', win.isMaximized);
        const maxBtn = win.headerEl.querySelector('.win-btn-max');
        if (maxBtn) { maxBtn.textContent = win.isMaximized ? '❐' : '□'; maxBtn.title = win.isMaximized ? 'Restaurar tamaño' : 'Maximizar'; }
        this.bringToFront(id);
        if (typeof RouteManager !== 'undefined' && RouteManager.resizeCharts) setTimeout(() => RouteManager.resizeCharts(), 50);
    },

    closeWindow(id) {
        if (['tab-conexion', 'tab-mapa', 'tab-testing', 'tab-touch', 'tab-rutas'].includes(id)) return;
        const win = this.windows.get(id);
        if (!win) return;
        const btn = this._getTabButton(id);
        if (btn) btn.remove();
        if (id === 'tab-boost') {
            win.isMinimized = true; win.el.style.display = 'none';
            win.el.classList.remove('active', 'is-focused', 'is-tile-focused');
            if (this.mode === 'single' && this.activeSingleId === id) this.focusSingle('tab-conexion');
            if (this.mode === 'tabs' && typeof state !== 'undefined' && state.activeTab === 'boost' && typeof TabManager !== 'undefined') TabManager.switch('conexion');
            return;
        }
        if (win.el?.parentNode) win.el.parentNode.removeChild(win.el);
        this.windows.delete(id);
        if (this.mode === 'single' && this.activeSingleId === id) this.focusSingle('tab-conexion');
        if (this.mode === 'tabs' && typeof state !== 'undefined' && state.activeTab === id && typeof TabManager !== 'undefined') TabManager.switch('conexion');
    },

    _startDrag(e, id) {
        const win = this.windows.get(id);
        WindowInteract.startDrag(e, win, winId => this.bringToFront(winId), this.mode === 'floating');
    },

    _startResize(e, id) {
        const win = this.windows.get(id);
        WindowInteract.startResize(e, win, this.mode, winId => this.bringToFront(winId), () => {
            if (typeof RouteManager !== 'undefined' && RouteManager.resizeCharts) RouteManager.resizeCharts();
            if (typeof robotChart !== 'undefined' && robotChart?.resize) robotChart.resize();
        });
    },

    createForkWindow(sourcePanelId) {
        const sourceWin = this.windows.get(sourcePanelId);
        if (!sourceWin) return;
        const forkId = `fork-${sourcePanelId}-${Date.now().toString(36)}`;
        const forkEl = WindowLayout.createForkDOM(sourceWin, forkId, this.workspaceEl);
        const forkTitle = `Fork · ${sourceWin.title}`;
        const forkIcon = '<svg class="ui-icon" aria-hidden="true" viewBox="0 0 24 24"><circle cx="18" cy="18" r="3"/><circle cx="6" cy="6" r="3"/><circle cx="18" cy="6" r="3"/><path d="M6 9v3a3 3 0 0 0 3 3h6M18 9v9"/></svg>';
        this.registerWindow(forkId, forkEl, forkTitle, forkIcon, {
            isSingleton: false, isForkable: false,
            initialRect: { x: (sourceWin.rect?.x || 30) + 30, y: (sourceWin.rect?.y || 30) + 30, width: sourceWin.rect?.width || 560, height: sourceWin.rect?.height || 420 }
        });
        this.ensureTaskbarButton(forkId, forkTitle, forkIcon, false);
        if (this.mode === 'tabs') this.setMode('single');
        if (this.mode === 'single') this.focusSingle(forkId);
        else { this.restoreWindow(forkId); this.bringToFront(forkId); }
        if (typeof LogSystem !== 'undefined') LogSystem.add('INFO', `Vista bifurcada creada: ${sourceWin.title}`);
    },

    createCustomWindow(entry, options = {}) {
        const winId = `custom-win-${entry.id}-${Date.now().toString(36)}`;
        const winEl = WindowLayout.createCustomDOM(entry, winId, this.workspaceEl, options);
        this.registerWindow(winId, winEl, entry.name, entry.icon, {
            isSingleton: entry.isSingleton, isForkable: false,
            initialRect: options.initialRect || { x: 50, y: 50, width: 500, height: 380 }
        });
        this.ensureTaskbarButton(winId, entry.name, entry.icon, entry.isSingleton);
        if (this.mode === 'tabs') this.setMode('single');
        if (this.mode === 'single') this.focusSingle(winId);
        else { this.restoreWindow(winId); this.bringToFront(winId); }
        return winId;
    },

    renderForkMenu() {
        WindowLayout.renderForkMenu();
    }
};


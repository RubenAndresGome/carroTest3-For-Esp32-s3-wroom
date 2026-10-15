/* ==============================================
   WINDOW MANAGER (Multi-Panel Workspace & Window Lifecycle)
   ============================================== */
const WindowManager = {
    mode: 'tabs',
    windows: new Map(),
    maxZIndex: 10,
    workspaceEl: null,
    _activeDrag: null,
    _activeResize: null,

    init() {
        this.workspaceEl = document.getElementById('hmi-workspace');
        if (!this.workspaceEl) {
            // Si el contenedor no existe aún en el DOM, envuelve los paneles de pestañas
            const firstTab = document.querySelector('.tab-content');
            if (firstTab && firstTab.parentNode) {
                const parent = firstTab.parentNode;
                const ws = document.createElement('div');
                ws.id = 'hmi-workspace';
                ws.className = 'workspace-tabs';
                parent.insertBefore(ws, firstTab);
                const tabs = parent.querySelectorAll('.tab-content');
                tabs.forEach(tab => ws.appendChild(tab));
                this.workspaceEl = ws;
            }
        }

        // Configuración de botones de modo
        const btnTabs = document.getElementById('ws-mode-tabs');
        const btnTiling = document.getElementById('ws-mode-tiling');
        const btnFloating = document.getElementById('ws-mode-floating');
        const btnAddFork = document.getElementById('ws-btn-add-fork');
        const forkMenu = document.getElementById('glass-fork-menu');
        const btnCloseForkMenu = document.getElementById('btn-close-fork-menu');

        if (btnTabs) btnTabs.addEventListener('click', () => this.setMode('tabs'));
        if (btnTiling) btnTiling.addEventListener('click', () => this.setMode('tiling'));
        if (btnFloating) btnFloating.addEventListener('click', () => this.setMode('floating'));

        if (btnAddFork && forkMenu) {
            btnAddFork.addEventListener('click', (e) => {
                e.stopPropagation();
                const isHidden = forkMenu.style.display === 'none';
                if (isHidden) {
                    this.renderForkMenu();
                    forkMenu.style.display = 'block';
                    btnAddFork.setAttribute('aria-expanded', 'true');
                } else {
                    forkMenu.style.display = 'none';
                    btnAddFork.setAttribute('aria-expanded', 'false');
                }
            });
        }
        if (btnCloseForkMenu && forkMenu) {
            btnCloseForkMenu.addEventListener('click', () => {
                forkMenu.style.display = 'none';
                if (btnAddFork) btnAddFork.setAttribute('aria-expanded', 'false');
            });
        }
        document.addEventListener('click', (e) => {
            if (forkMenu && forkMenu.style.display !== 'none' && !forkMenu.contains(e.target) && e.target !== btnAddFork) {
                forkMenu.style.display = 'none';
                if (btnAddFork) btnAddFork.setAttribute('aria-expanded', 'false');
            }
        });

        // Registrar paneles estándar
        this._registerPanels();

        // Enfoque responsive: forzar tabs si la pantalla es < 768px
        const savedMode = localStorage.getItem('hmi_workspace_mode') || 'tabs';
        const initialMode = window.innerWidth < 768 ? 'tabs' : savedMode;
        this.setMode(initialMode, true);

        window.addEventListener('resize', () => {
            if (window.innerWidth < 768 && this.mode !== 'tabs') {
                this.setMode('tabs');
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
                isSingleton: p.singleton,
                isForkable: Boolean(p.forkable),
                isCustomWidget: Boolean(p.isCustomWidget),
                initiallyMinimized: Boolean(p.initiallyMinimized),
                initialRect: {
                    x: 20 + (idx * 28) % 240,
                    y: 20 + (idx * 28) % 180,
                    width: 620,
                    height: 480
                }
            });
        });
    },

    _getTabButton(id) {
        if (!id) return null;
        const cleanId = id.replace(/^tab-/, '');
        return document.getElementById(`tab-control-${cleanId}`) ||
               document.getElementById(`tab-control-${id}`) ||
               document.querySelector(`.tab-btn[data-tab="${cleanId}"]`) ||
               document.querySelector(`.tab-btn[data-win="${id}"]`);
    },

    ensureTaskbarButton(id, title, icon, isSingleton = false) {
        let btn = this._getTabButton(id);
        if (!btn) {
            const tabBar = document.querySelector('.tab-bar');
            if (!tabBar) return null;
            const cleanId = id.replace(/^tab-/, '');
            btn = document.createElement('button');
            btn.id = `tab-control-${cleanId}`;
            btn.className = 'tab-btn tab-btn-dynamic';
            btn.dataset.tab = cleanId;
            btn.dataset.win = id;
            btn.setAttribute('role', 'tab');
            btn.setAttribute('aria-controls', id);
            btn.setAttribute('aria-selected', 'false');
            btn.tabIndex = -1;
            const isPermanent = ['tab-conexion', 'tab-mapa', 'tab-testing', 'tab-touch', 'tab-rutas'].includes(id);
            btn.innerHTML = `
                ${icon}
                <span class="tab-btn-title">${title}</span>
                ${!isPermanent ? `<span class="tab-btn-close" title="Cerrar ventana" aria-label="Cerrar">✕</span>` : ''}
            `;

            btn.addEventListener('click', (e) => {
                if (e.target && e.target.classList.contains('tab-btn-close')) {
                    e.stopPropagation();
                    this.closeWindow(id);
                    return;
                }
                const win = this.windows.get(id);
                if (!win) return;
                if (win.isMinimized) {
                    this.restoreWindow(id);
                    return;
                }
                const isCurrentlyActive = (this.mode === 'tabs' && state.activeTab === cleanId && !win.isMinimized) ||
                    (this.mode !== 'tabs' && win.el.classList.contains('is-focused') && !win.isMinimized);
                if (isCurrentlyActive) {
                    this.minimizeWindow(id);
                } else {
                    if (this.mode === 'tabs') {
                        if (typeof TabManager !== 'undefined') {
                            TabManager.switch(cleanId);
                        } else {
                            this.restoreWindow(id);
                        }
                    } else {
                        this.bringToFront(id);
                    }
                }
            });

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
                this.registerWindow('tab-boost', el, 'HUD Cinético & Boost', '<svg class="ui-icon" aria-hidden="true" viewBox="0 0 24 24"><polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2"/></svg>', {
                    isSingleton: true,
                    isForkable: false
                });
                win = this.windows.get('tab-boost');
            }
        }
        if (!win) return null;

        this.ensureTaskbarButton(win.id, win.title, win.icon, false);
        this.restoreWindow(win.id);
        this.bringToFront(win.id);
        return win.id;
    },

    registerWindow(id, element, title, icon, options = {}) {
        if (!element) return;
        element.classList.add('window-frame');

        // Si ya tiene un encabezado de ventana inyectado, no duplicar
        let header = element.querySelector('.window-header');
        if (!header) {
            header = document.createElement('div');
            header.className = 'window-header';
            const isPermanent = ['tab-conexion', 'tab-mapa', 'tab-testing', 'tab-touch', 'tab-rutas'].includes(id);
            header.innerHTML = `
                <div class="window-title">
                    <span>${icon}</span>
                    <span>${title}</span>
                </div>
                <div class="window-controls">
                    ${options.isForkable ? `<button type="button" class="win-btn win-btn-fork" title="Bifurcar vista" aria-label="Bifurcar"><svg class="ui-icon" aria-hidden="true" viewBox="0 0 24 24"><circle cx="18" cy="18" r="3"/><circle cx="6" cy="6" r="3"/><circle cx="18" cy="6" r="3"/><path d="M6 9v3a3 3 0 0 0 3 3h6M18 9v9"/></svg></button>` : ''}
                    <button type="button" class="win-btn win-btn-min" title="Minimizar a la barra" aria-label="Minimizar">—</button>
                    <button type="button" class="win-btn win-btn-max" title="Maximizar" aria-label="Maximizar">□</button>
                    ${!isPermanent ? `<button type="button" class="win-btn win-btn-close" title="Cerrar" aria-label="Cerrar">✕</button>` : ''}
                </div>
            `;
            element.insertBefore(header, element.firstChild);
        }

        // Handle de redimensionamiento perimetral
        let resizeHandle = element.querySelector('.window-resize-handle');
        if (!resizeHandle) {
            resizeHandle = document.createElement('div');
            resizeHandle.className = 'window-resize-handle';
            element.appendChild(resizeHandle);
        }

        const isFork = Boolean(options.isFork) || id.startsWith('fork-');
        const isCustomWidget = Boolean(options.isCustomWidget) || id.startsWith('custom-win-');
        const isInitiallyMin = Boolean(options.initiallyMinimized);
        const winData = {
            id,
            el: element,
            headerEl: header,
            title,
            icon,
            isMinimized: isInitiallyMin,
            isMaximized: false,
            isSingleton: Boolean(options.isSingleton),
            isFork,
            isCustomWidget,
            rect: options.initialRect || { x: 30, y: 30, width: 600, height: 460 },
            zIndex: ++this.maxZIndex
        };

        if (isInitiallyMin) {
            element.classList.add('is-minimized');
            element.style.display = 'none';
        }

        this.windows.set(id, winData);

        // Eventos del encabezado
        const forkBtn = header.querySelector('.win-btn-fork');
        const minBtn = header.querySelector('.win-btn-min');
        const maxBtn = header.querySelector('.win-btn-max');
        const closeBtn = header.querySelector('.win-btn-close');

        if (forkBtn) {
            forkBtn.addEventListener('click', (e) => {
                e.stopPropagation();
                if (id === 'tab-rutas' && typeof RouteManager !== 'undefined') {
                    RouteManager.forkCurrentRoute();
                } else {
                    this.createForkWindow(id);
                }
            });
        }
        if (minBtn) {
            minBtn.addEventListener('click', (e) => {
                e.stopPropagation();
                this.minimizeWindow(id);
            });
        }
        if (maxBtn) {
            maxBtn.addEventListener('click', (e) => {
                e.stopPropagation();
                this.maximizeWindow(id);
            });
        }
        if (closeBtn) {
            closeBtn.addEventListener('click', (e) => {
                e.stopPropagation();
                this.closeWindow(id);
            });
        }

        // Foco de ventana al hacer clic
        element.addEventListener('pointerdown', () => this.bringToFront(id));

        // Arrastre por PointerEvents
        header.addEventListener('pointerdown', (e) => this._startDrag(e, id));
        resizeHandle.addEventListener('pointerdown', (e) => this._startResize(e, id));

        // Observar redimensionamiento nativo (CSS resize: both) para preservar dimensiones en mosaico
        if (typeof ResizeObserver !== 'undefined') {
            const ro = new ResizeObserver(() => {
                if (this.mode === 'tiling' && element && element.style.width) {
                    const w = parseFloat(element.style.width);
                    const h = parseFloat(element.style.height);
                    if (Number.isFinite(w) && w > 0) winData.tilingWidth = w;
                    if (Number.isFinite(h) && h > 0) winData.tilingHeight = h;
                }
            });
            ro.observe(element);
        }

        // Aplicar posición inicial si está en floating
        if (this.mode === 'floating') {
            this._applyWindowRect(winData);
        }
    },

    setMode(mode, silent = false) {
        if (window.innerWidth < 768 && mode !== 'tabs') {
            if (!silent && typeof LogSystem !== 'undefined') {
                LogSystem.add('INFO', 'Espacio multi-panel adaptado a modo pestañas en pantallas compactas.');
            }
            mode = 'tabs';
        }

        // Preservar dimensiones de mosaico si se sale de modo tiling
        if (this.mode === 'tiling') {
            this.windows.forEach(win => {
                if (win.el) {
                    const w = parseFloat(win.el.style.width);
                    const h = parseFloat(win.el.style.height);
                    if (Number.isFinite(w) && w > 0) win.tilingWidth = w;
                    if (Number.isFinite(h) && h > 0) win.tilingHeight = h;
                }
            });
        }

        this.mode = mode;
        try {
            localStorage.setItem('hmi_workspace_mode', mode);
        } catch (_) {}

        // Actualizar botones de selector
        ['tabs', 'tiling', 'floating'].forEach(m => {
            const btn = document.getElementById(`ws-mode-${m}`);
            if (btn) {
                const isActive = m === mode;
                btn.classList.toggle('active', isActive);
                btn.setAttribute('aria-pressed', isActive ? 'true' : 'false');
            }
        });

        if (this.workspaceEl) {
            this.workspaceEl.classList.remove('workspace-tabs', 'workspace-tiling', 'workspace-floating');
            this.workspaceEl.classList.add(`workspace-${mode}`);
        }

        if (mode === 'tabs') {
            // Modo clásico: sincronizar con TabManager y restablecer estados minimizados en ventanas base
            this.windows.forEach(win => {
                if (win.isMaximized) this.maximizeWindow(win.id);
                win.el.style.left = '';
                win.el.style.top = '';
                win.el.style.width = '';
                win.el.style.height = '';
                win.el.style.zIndex = '';
                if (!win.isCustomWidget && !win.isFork) {
                    win.isMinimized = false;
                    win.el.classList.remove('is-minimized', 'is-focused');
                    const btn = this._getTabButton(win.id);
                    if (btn) btn.classList.remove('is-minimized');
                }
            });
            if (typeof TabManager !== 'undefined' && typeof state !== 'undefined') {
                TabManager.switch(state.activeTab || 'conexion');
            }
        } else if (mode === 'tiling') {
            // Modo Mosaico: mostrar todas las ventanas base no minimizadas en rejilla
            this.windows.forEach(win => {
                if (win.isMaximized) this.maximizeWindow(win.id);
                win.el.style.left = '';
                win.el.style.top = '';
                win.el.style.width = win.tilingWidth ? `${win.tilingWidth}px` : '';
                win.el.style.height = win.tilingHeight ? `${win.tilingHeight}px` : '';
                win.el.style.zIndex = '';
                if (!win.isCustomWidget && !win.isFork) {
                    win.isMinimized = false;
                    win.el.classList.remove('is-minimized');
                    const btn = this._getTabButton(win.id);
                    if (btn) btn.classList.remove('is-minimized');
                }
                if (!win.isMinimized) {
                    win.el.style.display = 'flex';
                }
            });
        } else if (mode === 'floating') {
            // Modo Flotante: mostrar únicamente la pestaña activa como ventana principal
            // Mantener ventanas secundarias minimizadas y accesibles desde la barra superior o menú (···)
            const activeTab = (typeof state !== 'undefined' && state.activeTab) ? state.activeTab : 'rutas';
            const activeWinId = activeTab.startsWith('tab-') ? activeTab : `tab-${activeTab}`;

            this.windows.forEach(win => {
                if (win.id === activeWinId) {
                    win.isMinimized = false;
                    win.el.style.display = 'block';
                    win.el.classList.remove('is-minimized');
                    this._applyWindowRect(win);
                } else if (!win.isCustomWidget && !win.isFork) {
                    win.isMinimized = true;
                    win.el.style.display = 'none';
                    win.el.classList.add('is-minimized');
                    win.el.classList.remove('is-focused');
                    const btn = this._getTabButton(win.id);
                    if (btn) {
                        btn.classList.remove('active');
                        btn.classList.add('is-minimized');
                        btn.setAttribute('aria-selected', 'false');
                        btn.tabIndex = -1;
                    }
                } else if (!win.isMinimized) {
                    win.el.style.display = 'block';
                    this._applyWindowRect(win);
                }
            });

            this.bringToFront(activeWinId);
            if (!silent && typeof LogSystem !== 'undefined') {
                LogSystem.add('INFO', 'Modo Ventanas activado: Ventanas secundarias minimizadas y accesibles desde (···) o la barra superior.');
            }
        }

        if (typeof RouteManager !== 'undefined' && RouteManager.resizeCharts) {
            setTimeout(() => RouteManager.resizeCharts(), 60);
        }
    },

    _applyWindowRect(win) {
        if (!win || !win.el) return;
        win.el.style.left = `${win.rect.x}px`;
        win.el.style.top = `${win.rect.y}px`;
        win.el.style.width = `${win.rect.width}px`;
        win.el.style.height = `${win.rect.height}px`;
        win.el.style.zIndex = win.zIndex;
    },

    bringToFront(id) {
        const win = this.windows.get(id);
        if (!win) return;
        win.zIndex = ++this.maxZIndex;
        win.el.style.zIndex = win.zIndex;

        this.windows.forEach(w => w.el.classList.remove('is-focused'));
        win.el.classList.add('is-focused');

        // Sincronizar estado visual de barra superior estilo Windows
        this.windows.forEach(w => {
            const btn = this._getTabButton(w.id);
            if (!btn) return;
            if (w.id === id) {
                w.isMinimized = false;
                btn.classList.add('active');
                btn.classList.remove('is-minimized');
                btn.setAttribute('aria-selected', 'true');
                btn.tabIndex = 0;
            } else {
                btn.classList.remove('active');
                btn.setAttribute('aria-selected', 'false');
                btn.tabIndex = -1;
                if (w.isMinimized) {
                    btn.classList.add('is-minimized');
                } else {
                    btn.classList.remove('is-minimized');
                }
            }
        });
    },

    minimizeWindow(id) {
        const win = this.windows.get(id);
        if (!win) return;
        win.isMinimized = true;
        win.el.style.display = 'none';
        win.el.classList.remove('is-focused');

        if (this.mode === 'tabs') {
            win.el.classList.remove('active');
        }

        const btn = this._getTabButton(id);
        if (btn) {
            btn.classList.remove('active');
            btn.classList.add('is-minimized');
            btn.setAttribute('aria-selected', 'false');
            btn.tabIndex = -1;
        }
    },

    restoreWindow(id) {
        const win = this.windows.get(id);
        if (!win) return;
        win.isMinimized = false;

        const cleanId = id.replace(/^tab-/, '');

        if (this.mode === 'floating') {
            win.el.style.display = 'block';
            this._applyWindowRect(win);
        } else if (this.mode === 'tiling') {
            win.el.style.display = 'flex';
        } else { // tabs
            win.el.style.display = 'block';
            win.el.classList.add('active');
            if (typeof state !== 'undefined') state.activeTab = cleanId;
            document.querySelectorAll('.tab-content').forEach(c => {
                if (c.id !== id && c.id !== win.id) {
                    c.classList.remove('active');
                    c.style.display = 'none';
                }
            });
        }

        const btn = this._getTabButton(id);
        if (btn) {
            btn.classList.remove('is-minimized');
            btn.classList.add('active');
            btn.setAttribute('aria-selected', 'true');
            btn.tabIndex = 0;
        }

        this.bringToFront(id);

        if (cleanId === 'rutas' && typeof RouteManager !== 'undefined' && RouteManager.resizeCharts) {
            RouteManager.resizeCharts();
        }
    },

    maximizeWindow(id) {
        const win = this.windows.get(id);
        if (!win) return;
        win.isMaximized = !win.isMaximized;
        win.el.classList.toggle('is-maximized', win.isMaximized);

        const maxBtn = win.headerEl.querySelector('.win-btn-max');
        if (maxBtn) {
            maxBtn.textContent = win.isMaximized ? '❐' : '□';
            maxBtn.title = win.isMaximized ? 'Restaurar tamaño' : 'Maximizar';
        }

        this.bringToFront(id);
        if (typeof RouteManager !== 'undefined' && RouteManager.resizeCharts) {
            setTimeout(() => RouteManager.resizeCharts(), 50);
        }
    },

    closeWindow(id) {
        const isPermanent = ['tab-conexion', 'tab-mapa', 'tab-testing', 'tab-touch', 'tab-rutas'].includes(id);
        if (isPermanent) return;

        const win = this.windows.get(id);
        if (!win) return;

        const btn = this._getTabButton(id);
        if (btn) btn.remove();

        if (id === 'tab-boost') {
            win.isMinimized = false;
            win.el.style.display = 'none';
            win.el.classList.remove('active', 'is-focused');
            if (this.mode === 'tabs' && typeof state !== 'undefined' && state.activeTab === 'boost') {
                if (typeof TabManager !== 'undefined') TabManager.switch('conexion');
            }
            return;
        }

        if (win.el && win.el.parentNode) {
            win.el.parentNode.removeChild(win.el);
        }
        this.windows.delete(id);

        if (this.mode === 'tabs' && typeof state !== 'undefined' && state.activeTab === id) {
            if (typeof TabManager !== 'undefined') TabManager.switch('conexion');
        }
    },

    _startDrag(e, id) {
        if (this.mode !== 'floating') return;
        const win = this.windows.get(id);
        if (!win || win.isMaximized) return;

        // Pointer capture para arrastre fiable fuera de bordes
        win.headerEl.setPointerCapture(e.pointerId);
        this.bringToFront(id);

        // Salvaguarda de rendimiento: desactivar desenfoque durante arrastre
        win.el.classList.add('is-interacting');
        document.body.classList.add('is-dragging-window');

        this._activeDrag = {
            id,
            pointerId: e.pointerId,
            startX: e.clientX,
            startY: e.clientY,
            winX: win.rect.x,
            winY: win.rect.y
        };

        const onMove = (ev) => {
            if (!this._activeDrag || ev.pointerId !== this._activeDrag.pointerId) return;
            const dx = ev.clientX - this._activeDrag.startX;
            const dy = ev.clientY - this._activeDrag.startY;
            win.rect.x = Math.max(0, this._activeDrag.winX + dx);
            win.rect.y = Math.max(0, this._activeDrag.winY + dy);
            win.el.style.left = `${win.rect.x}px`;
            win.el.style.top = `${win.rect.y}px`;
        };

        const onUp = (ev) => {
            if (!this._activeDrag || ev.pointerId !== this._activeDrag.pointerId) return;
            try { win.headerEl.releasePointerCapture(ev.pointerId); } catch (_) {}
            win.el.classList.remove('is-interacting');
            document.body.classList.remove('is-dragging-window');
            win.headerEl.removeEventListener('pointermove', onMove);
            win.headerEl.removeEventListener('pointerup', onUp);
            win.headerEl.removeEventListener('pointercancel', onUp);
            this._activeDrag = null;
        };

        win.headerEl.addEventListener('pointermove', onMove);
        win.headerEl.addEventListener('pointerup', onUp);
        win.headerEl.addEventListener('pointercancel', onUp);
    },

    _startResize(e, id) {
        if (this.mode !== 'floating' && this.mode !== 'tiling') return;
        const win = this.windows.get(id);
        if (!win || win.isMaximized) return;

        const handle = win.el.querySelector('.window-resize-handle');
        if (!handle) return;

        handle.setPointerCapture(e.pointerId);
        win.el.classList.add('is-interacting');
        document.body.classList.add('is-dragging-window');
        this.bringToFront(id);

        this._activeResize = {
            id,
            pointerId: e.pointerId,
            startX: e.clientX,
            startY: e.clientY,
            startW: win.el.offsetWidth || win.rect.width,
            startH: win.el.offsetHeight || win.rect.height
        };

        const onMove = (ev) => {
            if (!this._activeResize || ev.pointerId !== this._activeResize.pointerId) return;
            const dw = ev.clientX - this._activeResize.startX;
            const dh = ev.clientY - this._activeResize.startY;
            win.rect.width = Math.max(300, this._activeResize.startW + dw);
            win.rect.height = Math.max(240, this._activeResize.startH + dh);
            win.el.style.width = `${win.rect.width}px`;
            win.el.style.height = `${win.rect.height}px`;
            if (this.mode === 'tiling') {
                win.tilingWidth = win.rect.width;
                win.tilingHeight = win.rect.height;
            }
        };

        const onUp = (ev) => {
            if (!this._activeResize || ev.pointerId !== this._activeResize.pointerId) return;
            try { handle.releasePointerCapture(ev.pointerId); } catch (_) {}
            win.el.classList.remove('is-interacting');
            document.body.classList.remove('is-dragging-window');
            handle.removeEventListener('pointermove', onMove);
            handle.removeEventListener('pointerup', onUp);
            handle.removeEventListener('pointercancel', onUp);
            if (this.mode === 'tiling') {
                win.tilingWidth = win.rect.width;
                win.tilingHeight = win.rect.height;
            }
            this._activeResize = null;
            if (typeof RouteManager !== 'undefined' && RouteManager.resizeCharts) {
                RouteManager.resizeCharts();
            }
        };

        handle.addEventListener('pointermove', onMove);
        handle.addEventListener('pointerup', onUp);
        handle.addEventListener('pointercancel', onUp);
    },

    createForkWindow(sourcePanelId) {
        const sourceWin = this.windows.get(sourcePanelId);
        if (!sourceWin) return;

        const forkId = `fork-${sourcePanelId}-${Date.now().toString(36)}`;
        const forkEl = document.createElement('div');
        forkEl.id = forkId;
        forkEl.className = 'tab-content window-fork';

        // Clonar contenido seguro (sin controles singleton ni duplicación de IDs)
        if (sourcePanelId === 'tab-mapa') {
            const chartCard = sourceWin.el.querySelector('.nav-chart-card');
            if (chartCard) {
                const clone = chartCard.cloneNode(true);
                clone.querySelectorAll('[id]').forEach(el => {
                    el.id = `${forkId}-${el.id}`;
                });
                forkEl.appendChild(clone);
            }
        } else {
            const innerContent = sourceWin.el.querySelector('.card');
            if (innerContent) {
                const clone = innerContent.cloneNode(true);
                clone.querySelectorAll('#btn-apply-pwm, .btn-estop, #btn-chart-move').forEach(b => b.remove());
                clone.querySelectorAll('[id]').forEach(el => {
                    el.id = `${forkId}-${el.id}`;
                });
                forkEl.appendChild(clone);
            }
        }

        if (this.workspaceEl) {
            this.workspaceEl.appendChild(forkEl);
        } else {
            document.body.appendChild(forkEl);
        }

        const forkTitle = `Fork · ${sourceWin.title}`;
        const forkIcon = '<svg class="ui-icon" aria-hidden="true" viewBox="0 0 24 24"><circle cx="18" cy="18" r="3"/><circle cx="6" cy="6" r="3"/><circle cx="18" cy="6" r="3"/><path d="M6 9v3a3 3 0 0 0 3 3h6M18 9v9"/></svg>';
        this.registerWindow(forkId, forkEl, forkTitle, forkIcon, {
            isSingleton: false,
            isForkable: false,
            initialRect: {
                x: (sourceWin.rect?.x || 30) + 30,
                y: (sourceWin.rect?.y || 30) + 30,
                width: sourceWin.rect?.width || 560,
                height: sourceWin.rect?.height || 420
            }
        });
        this.ensureTaskbarButton(forkId, forkTitle, forkIcon, false);

        if (this.mode !== 'floating') {
            this.setMode('floating');
        }
        this.bringToFront(forkId);
        if (typeof LogSystem !== 'undefined') {
            LogSystem.add('INFO', `Vista bifurcada creada en ventana flotante: ${sourceWin.title}`);
        }
    },

    createCustomWindow(entry, options = {}) {
        const winId = `custom-win-${entry.id}-${Date.now().toString(36)}`;
        const winEl = document.createElement('div');
        winEl.id = winId;
        winEl.className = 'tab-content window-widget';

        const card = document.createElement('div');
        card.className = 'card';
        card.innerHTML = `<div class="card-title">${entry.icon} ${entry.name}</div><div class="custom-win-body"></div>`;
        winEl.appendChild(card);

        if (typeof entry.factory === 'function') {
            entry.factory(card.querySelector('.custom-win-body'), options);
        }

        if (this.workspaceEl) {
            this.workspaceEl.appendChild(winEl);
        } else {
            document.body.appendChild(winEl);
        }

        this.registerWindow(winId, winEl, entry.name, entry.icon, {
            isSingleton: entry.isSingleton,
            isForkable: false,
            initialRect: options.initialRect || { x: 50, y: 50, width: 500, height: 380 }
        });
        this.ensureTaskbarButton(winId, entry.name, entry.icon, entry.isSingleton);

        if (this.mode !== 'floating') {
            this.setMode('floating');
        }
        this.bringToFront(winId);
        return winId;
    },

    renderForkMenu() {
        const listEl = document.getElementById('glass-fork-menu-list');
        if (!listEl || typeof HMI_FORK_REGISTRY === 'undefined') return;
        const items = HMI_FORK_REGISTRY.list();
        listEl.innerHTML = items.map(item => `
            <div class="glass-fork-menu-item" data-module-id="${item.id}">
                <div>
                    <div style="font-weight:700; font-size:11px; color:var(--text-primary);">${item.icon} ${item.name}</div>
                    <div style="font-size:9px; color:var(--text-muted);">${item.metadata?.description || ''}</div>
                </div>
                <button type="button" class="btn btn-sm btn-accent" style="padding:2px 7px; font-size:10px;">Abrir</button>
            </div>
        `).join('');

        listEl.querySelectorAll('.glass-fork-menu-item').forEach(el => {
            el.addEventListener('click', () => {
                const modId = el.dataset.moduleId;
                const menu = document.getElementById('glass-fork-menu');
                if (menu) menu.style.display = 'none';
                const btnAdd = document.getElementById('ws-btn-add-fork');
                if (btnAdd) btnAdd.setAttribute('aria-expanded', 'false');
                HMI_FORK_REGISTRY.createInstance(modId);
            });
        });
    }
};

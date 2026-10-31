// @ts-check
/** @typedef {import('@frontend/protocol').Telemetry} TelemetryPayload */
/** @typedef {import('@frontend/protocol').CommandName} CommandName */

/* ==============================================
   WINDOW LAYOUT (Layout State Machine: Tabs, Single, Tiling & Floating)
   ============================================== */

const WindowLayout = {
    /**
     * Aplica la posición y dimensiones exactas a una ventana flotante.
     */
    applyWindowRect(win) {
        if (!win || !win.el) return;
        win.el.style.left = `${win.rect.x}px`;
        win.el.style.top = `${win.rect.y}px`;
        win.el.style.width = `${win.rect.width}px`;
        win.el.style.height = `${win.rect.height}px`;
        win.el.style.zIndex = win.zIndex;
    },

    /**
     * Preserva las dimensiones actuales de mosaico al cambiar de modo.
     */
    preserveTilingDimensions(windows) {
        windows.forEach(win => {
            if (!win.el) return;
            const h = parseFloat(win.el.style.height) || win.el.offsetHeight;
            if (Number.isFinite(h) && h >= 380) {
                win.tilingHeight = h;
                win.rect.height = h;
            }
            const w = parseFloat(win.el.style.width);
            if (Number.isFinite(w) && w > 0) win.tilingWidth = w;
        });
    },

    /**
     * Aplica el modo de pestañas clásico.
     */
    applyTabs(windows, getTabButtonFn, restoreMaximizedFn) {
        windows.forEach(win => {
            if (win.isMaximized && typeof restoreMaximizedFn === 'function') {
                restoreMaximizedFn(win.id);
            }
            win.el.classList.remove('is-tile-focused');
            win.el.style.left = '';
            win.el.style.top = '';
            win.el.style.width = '';
            win.el.style.height = '';
            win.el.style.zIndex = '';
            if (!win.isCustomWidget && !win.isFork) {
                win.isMinimized = false;
                win.el.classList.remove('is-minimized', 'is-focused');
                const btn = getTabButtonFn(win.id);
                if (btn) btn.classList.remove('is-minimized');
            }
        });
        if (typeof TabManager !== 'undefined' && typeof state !== 'undefined') {
            TabManager.switch(state.activeTab || 'conexion');
        }
    },

    /**
     * Aplica el modo de Ventana Única (Fase B1): una sola ventana activa al 100% con barra de título.
     */
    applySingle(windows, activeWinId, getTabButtonFn, restoreMaximizedFn) {
        const targetId = windows.has(activeWinId) ? activeWinId : 'tab-conexion';
        windows.forEach(win => {
            if (win.isMaximized && typeof restoreMaximizedFn === 'function') {
                restoreMaximizedFn(win.id);
            }
            win.el.classList.remove('is-tile-focused');
            win.el.style.left = '';
            win.el.style.top = '';
            win.el.style.width = '100%';
            win.el.style.height = '';
            win.el.style.zIndex = '';
            const isTarget = win.id === targetId;
            win.isMinimized = !isTarget && (win.isCustomWidget || win.isFork ? win.isMinimized : false);
            win.el.classList.toggle('active', isTarget);
            win.el.classList.toggle('is-focused', isTarget);
            win.el.classList.remove('is-minimized');
            win.el.style.display = isTarget ? 'flex' : 'none';

            const btn = getTabButtonFn(win.id);
            if (btn) {
                btn.classList.toggle('active', isTarget);
                btn.classList.remove('is-minimized');
                btn.setAttribute('aria-selected', isTarget ? 'true' : 'false');
                btn.tabIndex = isTarget ? 0 : -1;
            }
        });
    },

    /**
     * Aplica el modo de mosaico vertical con 100% de ancho y doble scroll.
     */
    applyTiling(windows, getTabButtonFn, restoreMaximizedFn) {
        windows.forEach(win => {
            if (win.isMaximized && typeof restoreMaximizedFn === 'function') {
                restoreMaximizedFn(win.id);
            }
            win.el.style.left = '';
            win.el.style.top = '';
            win.el.style.width = '100%';
            win.el.style.height = win.tilingHeight ? `${win.tilingHeight}px` : '520px';
            win.el.style.zIndex = '';
            if (!win.isCustomWidget && !win.isFork) {
                win.isMinimized = false;
                win.el.classList.remove('is-minimized');
                const btn = getTabButtonFn(win.id);
                if (btn) btn.classList.remove('is-minimized');
            }
            if (!win.isMinimized) {
                win.el.style.display = 'flex';
            }
        });
    },

    /**
     * Aplica el modo flotante individual con ventanas secundarias minimizadas.
     */
    applyFloating(windows, activeWinId, getTabButtonFn, bringToFrontFn, silent) {
        windows.forEach(win => {
            win.el.classList.remove('is-tile-focused');
            if (win.id === activeWinId) {
                win.isMinimized = false;
                win.el.style.display = 'flex';
                win.el.classList.remove('is-minimized');
                this.applyWindowRect(win);
            } else if (!win.isCustomWidget && !win.isFork) {
                win.isMinimized = true;
                win.el.style.display = 'none';
                win.el.classList.add('is-minimized');
                win.el.classList.remove('is-focused');
                const btn = getTabButtonFn(win.id);
                if (btn) {
                    btn.classList.remove('active');
                    btn.classList.add('is-minimized');
                    btn.setAttribute('aria-selected', 'false');
                    btn.tabIndex = -1;
                }
            } else if (!win.isMinimized) {
                win.el.style.display = 'flex';
                this.applyWindowRect(win);
            }
        });

        bringToFrontFn(activeWinId);
        if (!silent && typeof LogSystem !== 'undefined') {
            LogSystem.add('INFO', 'Modo Ventanas activado: Ventanas secundarias minimizadas y accesibles desde (···) o la barra superior.');
        }
    },

    /**
     * Clona el contenido DOM de un panel fuente para instanciar una vista bifurcada.
     */
    createForkDOM(sourceWin, forkId, workspaceEl) {
        const forkEl = document.createElement('div');
        forkEl.id = forkId;
        forkEl.className = 'tab-content window-fork';

        if (sourceWin.id === 'tab-mapa') {
            const chartCard = sourceWin.el.querySelector('.nav-chart-card');
            if (chartCard) {
                const clone = chartCard.cloneNode(true);
                clone.querySelectorAll('[id]').forEach(el => { el.id = `${forkId}-${el.id}`; });
                forkEl.appendChild(clone);
            }
        } else {
            const innerContent = sourceWin.el.querySelector('.card');
            if (innerContent) {
                const clone = innerContent.cloneNode(true);
                clone.querySelectorAll('#btn-apply-pwm, .btn-estop, #btn-chart-move').forEach(b => b.remove());
                clone.querySelectorAll('[id]').forEach(el => { el.id = `${forkId}-${el.id}`; });
                forkEl.appendChild(clone);
            }
        }

        if (workspaceEl) workspaceEl.appendChild(forkEl);
        else document.body.appendChild(forkEl);
        return forkEl;
    },

    /**
     * Construye el contenedor DOM para widgets o ventanas personalizadas de registro.
     */
    createCustomDOM(entry, winId, workspaceEl, options = {}) {
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

        if (workspaceEl) workspaceEl.appendChild(winEl);
        else document.body.appendChild(winEl);
        return winEl;
    },

    /**
     * Renderiza el menú ··· con sección de Ventanas abiertas + catálogo de módulos (Fase B3).
     */
    renderForkMenu() {
        const listEl = document.getElementById('glass-fork-menu-list');
        if (!listEl || typeof HMI_FORK_REGISTRY === 'undefined') return;
        const wm = typeof WindowManager !== 'undefined' ? WindowManager : null;
        const closeMenu = () => {
            const menu = document.getElementById('glass-fork-menu');
            if (menu) menu.style.display = 'none';
            document.getElementById('ws-btn-add-fork')?.setAttribute('aria-expanded', 'false');
        };

        const openWinsHtml = wm ? Array.from(wm.windows.values()).filter(w => w.isOpen !== false).map(w => {
            const isPermanent = ['tab-conexion', 'tab-mapa', 'tab-testing', 'tab-touch', 'tab-rutas'].includes(w.id);
            const isDebugWin = w.id === 'tab-boost' || w.id === 'tab-testing';
            const stateBadge = w.isMaximized ? 'Max' : (w.isMinimized ? 'Min' : (w.el.classList.contains('is-focused') ? 'Activa' : 'Abierta'));
            return `
                <div class="glass-fork-menu-win fork-menu-open-item" data-win-id="${w.id}"${isDebugWin ? ' data-debug-only' : ''}>
                    <button type="button" class="glass-fork-win-focus fork-menu-open-title" data-action="focus" data-win-id="${w.id}">
                        <span class="glass-fork-win-title">${w.icon} <span>${w.title}</span></span>
                        <span class="glass-fork-win-badge fork-menu-item-badge">${stateBadge}</span>
                    </button>
                    <div class="glass-fork-win-actions fork-menu-open-actions">
                        <button type="button" class="win-btn" data-action="min" data-win-id="${w.id}" title="Minimizar">—</button>
                        <button type="button" class="win-btn" data-action="max" data-win-id="${w.id}" title="Maximizar / Restaurar">${w.isMaximized ? '❐' : '□'}</button>
                        ${!isPermanent ? `<button type="button" class="win-btn win-btn-close" data-action="close" data-win-id="${w.id}" title="Cerrar">✕</button>` : ''}
                    </div>
                </div>
            `;
        }).join('') : '';

        const items = HMI_FORK_REGISTRY.list();
        const catalogHtml = items.map(item => {
            const isOpenSingleton = Boolean(item.isSingleton && wm && (wm.windows.get(`tab-${item.id}`) || wm.windows.get(item.id) || (item.id === 'hud_boost' && wm.windows.get('tab-boost') && !wm.windows.get('tab-boost').isMinimized)));
            const debugAttr = item.metadata?.debugOnly ? ' data-debug-only' : '';
            return `
                <div class="glass-fork-menu-item" data-module-id="${item.id}"${debugAttr}>
                    <div class="fork-menu-item-main">
                        <span class="fork-menu-item-icon">${item.icon}</span>
                        <div>
                            <div class="glass-fork-item-title fork-menu-item-title"><span>${item.name}</span></div>
                            <div class="glass-fork-item-desc fork-menu-item-desc">${item.metadata?.description || ''}</div>
                        </div>
                    </div>
                    <button type="button" class="btn btn-sm btn-accent glass-fork-open-btn fork-menu-item-badge">${isOpenSingleton ? 'Ir a' : 'Abrir'}</button>
                </div>
            `;
        }).join('');

        listEl.innerHTML = `
            <div class="glass-fork-section-title fork-menu-section-title">Ventanas abiertas</div>
            <div class="glass-fork-win-list fork-menu-open-list">${openWinsHtml}</div>
            <div class="glass-fork-section-title fork-menu-section-title">Abrir módulo</div>
            <div class="glass-fork-catalog-list">${catalogHtml}</div>
        `;

        listEl.querySelectorAll('[data-action]').forEach(elem => {
            const btn = /** @type {HTMLElement} */ (elem);
            btn.addEventListener('click', (e) => {
                e.stopPropagation();
                if (!wm) return;
                const winId = btn.dataset.winId || '';
                const act = btn.dataset.action;
                if (act === 'focus') {
                    closeMenu();
                    wm.focusOrRestoreWindow(winId);
                } else if (act === 'min') {
                    wm.minimizeWindow(winId);
                    this.renderForkMenu();
                } else if (act === 'max') {
                    closeMenu();
                    wm.maximizeWindow(winId);
                } else if (act === 'close') {
                    wm.closeWindow(winId);
                    this.renderForkMenu();
                }
            });
        });

        listEl.querySelectorAll('.glass-fork-menu-item').forEach(elem => {
            const el = /** @type {HTMLElement} */ (elem);
            el.addEventListener('click', () => {
                const modId = el.dataset.moduleId;
                closeMenu();
                HMI_FORK_REGISTRY.createInstance(modId);
            });
        });
    }
};

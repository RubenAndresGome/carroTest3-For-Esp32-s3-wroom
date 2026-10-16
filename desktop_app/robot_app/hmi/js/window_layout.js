// @ts-check
/** @typedef {import('@frontend/protocol').Telemetry} TelemetryPayload */
/** @typedef {import('@frontend/protocol').CommandName} CommandName */

/* ==============================================
   WINDOW LAYOUT (Layout State Machine: Tabs, Tiling & Floating)
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

        listEl.querySelectorAll('.glass-fork-menu-item').forEach(elem => {
            const el = /** @type {HTMLElement} */ (elem);
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

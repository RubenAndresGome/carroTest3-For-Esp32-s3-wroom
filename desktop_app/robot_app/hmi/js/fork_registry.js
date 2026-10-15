/* ==============================================
   HMI FORK REGISTRY (Extensible User Modules & Widgets)
   ============================================== */
(function() {
    'use strict';

    const HMI_FORK_REGISTRY = {
        _registry: new Map(),

        register(id, definition) {
            if (!id || typeof id !== 'string') {
                throw new Error('HMI_FORK_REGISTRY: El identificador debe ser un string válido.');
            }
            if (!definition || typeof definition !== 'object') {
                throw new Error(`HMI_FORK_REGISTRY: Definición inválida para ${id}`);
            }
            this._registry.set(id, {
                id,
                name: definition.name || id,
                icon: definition.icon || 'Fork',
                isSingleton: Boolean(definition.isSingleton),
                factory: definition.factory || null,
                onDestroy: definition.onDestroy || null,
                metadata: definition.metadata || {}
            });
        },

        get(id) {
            return this._registry.get(id) || null;
        },

        has(id) {
            return this._registry.has(id);
        },

        list() {
            return Array.from(this._registry.values()).map(item => ({
                id: item.id,
                name: item.name,
                icon: item.icon,
                isSingleton: item.isSingleton,
                metadata: { ...item.metadata }
            }));
        },

        createInstance(id, options = {}) {
            const entry = this.get(id);
            if (!entry) {
                throw new Error(`HMI_FORK_REGISTRY: Módulo '${id}' no encontrado.`);
            }

            if (id === 'hud_boost' || id === 'tab-boost') {
                if (typeof WindowManager !== 'undefined' && WindowManager.openWindow) {
                    return WindowManager.openWindow('tab-boost');
                }
            }

            if (id === 'route_fork') {
                if (typeof WindowManager !== 'undefined' && WindowManager.createForkWindow) {
                    return WindowManager.createForkWindow('tab-rutas');
                }
            }

            if (entry.isSingleton && typeof WindowManager !== 'undefined') {
                const existingWin = WindowManager.windows.get(`tab-${id}`) || WindowManager.windows.get(id);
                if (existingWin) {
                    if (existingWin.isMinimized) WindowManager.restoreWindow(existingWin.id);
                    WindowManager.bringToFront(existingWin.id);
                    if (WindowManager.mode === 'tabs') {
                        existingWin.el.style.display = 'block';
                    }
                    return existingWin.id;
                }
            }

            if (typeof WindowManager !== 'undefined' && WindowManager.createCustomWindow) {
                return WindowManager.createCustomWindow(entry, options);
            }

            if (typeof entry.factory === 'function') {
                return entry.factory(null, options);
            }

            return null;
        }
    };

    // Pre-registro de módulos canónicos
    HMI_FORK_REGISTRY.register('hud_boost', {
        name: 'HUD Cinético & Boost (DRV8833)',
        icon: 'Boost',
        isSingleton: true,
        metadata: {
            category: 'telemetry',
            description: 'Monitoreo de ráfaga térmica y reparto diferencial de par DRV8833'
        }
    });

    HMI_FORK_REGISTRY.register('route_fork', {
        name: 'Bifurcación de Ruta (A/B)',
        icon: 'Fork',
        isSingleton: false,
        metadata: {
            category: 'routes',
            description: 'Comparación diferencial de trayectorias planificadas'
        },
        factory(container) {
            if (!container) return;
            container.innerHTML = `
                <p style="font-size:11px; color:var(--text-secondary); margin-bottom:8px;">
                    Crea y administra ramas alternativas para comparar trayectorias sin alterar la ruta base.
                </p>
                <div style="display:flex; gap:8px; margin-bottom:10px;">
                    <button type="button" class="btn btn-sm btn-accent" id="btn-modal-fork-new">Nueva Bifurcación</button>
                    <button type="button" class="btn btn-sm" id="btn-modal-compare-ab">Comparar A/B</button>
                </div>
                <div id="fork-modal-branch-list" style="font-size:11px; font-family:var(--font-mono); color:var(--text-primary); max-height:160px; overflow-y:auto;">
                </div>
            `;
            const btnNew = container.querySelector('#btn-modal-fork-new');
            const btnComp = container.querySelector('#btn-modal-compare-ab');
            const listEl = container.querySelector('#fork-modal-branch-list');

            const refreshList = () => {
                if (!listEl || typeof RouteManager === 'undefined') return;
                const branches = [['main', { name: 'Ruta Base (A)', queue: RouteManager.mainBranchBackup?.queue || RouteManager.queue }]];
                RouteManager.forks.forEach((val, key) => branches.push([key, val]));
                listEl.innerHTML = branches.map(([key, val]) => {
                    const isAct = key === RouteManager.activeBranch;
                    const steps = (val.queue || []).length;
                    return `<div style="display:flex; justify-content:space-between; align-items:center; padding:4px 6px; margin-bottom:3px; background:rgba(255,255,255,0.04); border-radius:4px; ${isAct ? 'border:1px solid var(--accent);' : ''}">
                        <span>${isAct ? '• ' : ''}${val.name} (${steps} pasos)</span>
                        ${!isAct ? `<button type="button" class="btn btn-sm btn-switch-branch" data-branch="${key}" style="padding:2px 6px; font-size:10px;">Activar</button>` : '<span style="color:var(--accent); font-size:10px;">ACTIVA</span>'}
                    </div>`;
                }).join('');
                listEl.querySelectorAll('.btn-switch-branch').forEach(b => {
                    b.addEventListener('click', () => {
                        RouteManager.switchBranch(b.dataset.branch);
                        refreshList();
                    });
                });
            };

            if (btnNew) btnNew.addEventListener('click', () => {
                if (typeof RouteManager !== 'undefined') {
                    RouteManager.forkCurrentRoute();
                    refreshList();
                }
            });
            if (btnComp) btnComp.addEventListener('click', () => {
                if (typeof RouteManager !== 'undefined') {
                    RouteManager.toggleCompareAB();
                }
            });
            refreshList();
        }
    });

    HMI_FORK_REGISTRY.register('telem_inspect', {
        name: 'Consola de Telemetría Extendida',
        icon: 'Telemetría',
        isSingleton: false,
        metadata: {
            category: 'diagnostics',
            description: 'Visualizador de señales de bajo nivel y PCNT'
        },
        factory(container) {
            if (!container) return;
            container.innerHTML = `
                <div style="font-family:var(--font-mono); font-size:11px; color:var(--text-primary);">
                    <div style="display:grid; grid-template-columns: 1fr 1fr; gap:6px; margin-bottom:8px;">
                        <div class="metric-chip-glass"><span style="color:var(--text-muted); font-size:9px;">POSE GLOBAL</span><div id="telem-inspect-pose">X: 0.00 cm · Y: 0.00 cm</div></div>
                        <div class="metric-chip-glass"><span style="color:var(--text-muted); font-size:9px;">RUMBO YAW</span><div id="telem-inspect-yaw">θ: 0.00° (MPU)</div></div>
                    </div>
                    <div style="display:grid; grid-template-columns: repeat(4, 1fr); gap:4px; margin-bottom:8px;">
                        <div class="metric-chip-glass" style="text-align:center;"><span style="color:var(--text-muted); font-size:8px;">FL</span><div id="telem-inspect-fl">0</div></div>
                        <div class="metric-chip-glass" style="text-align:center;"><span style="color:var(--text-muted); font-size:8px;">FR</span><div id="telem-inspect-fr">0</div></div>
                        <div class="metric-chip-glass" style="text-align:center;"><span style="color:var(--text-muted); font-size:8px;">BL</span><div id="telem-inspect-bl">0</div></div>
                        <div class="metric-chip-glass" style="text-align:center;"><span style="color:var(--text-muted); font-size:8px;">BR</span><div id="telem-inspect-br">0</div></div>
                    </div>
                    <div style="display:grid; grid-template-columns: 1fr 1fr; gap:6px;">
                        <div class="metric-chip-glass"><span style="color:var(--text-muted); font-size:9px;">PWM FÍSICO</span><div id="telem-inspect-pwm">L: 0 · R: 0</div></div>
                        <div class="metric-chip-glass"><span style="color:var(--text-muted); font-size:9px;">ESTADO FSM</span><div id="telem-inspect-state">INIT</div></div>
                    </div>
                </div>
            `;
            const updateFields = () => {
                if (!document.body.contains(container)) {
                    clearInterval(interval);
                    return;
                }
                const t = (typeof state !== 'undefined' && state.telemetria) || {};
                const poseEl = container.querySelector('#telem-inspect-pose');
                const yawEl = container.querySelector('#telem-inspect-yaw');
                const flEl = container.querySelector('#telem-inspect-fl');
                const frEl = container.querySelector('#telem-inspect-fr');
                const blEl = container.querySelector('#telem-inspect-bl');
                const brEl = container.querySelector('#telem-inspect-br');
                const pwmEl = container.querySelector('#telem-inspect-pwm');
                const stateEl = container.querySelector('#telem-inspect-state');

                if (poseEl) poseEl.textContent = `X: ${(t.posX || 0).toFixed(2)} cm · Y: ${(t.posY || 0).toFixed(2)} cm`;
                if (yawEl) yawEl.textContent = `θ: ${(t.angulo || 0).toFixed(1)}° (${t.calibrado ? 'CAL' : 'NO_CAL'})`;
                if (flEl) flEl.textContent = t.pfl || 0;
                if (frEl) frEl.textContent = t.pfr || 0;
                if (blEl) blEl.textContent = t.pbl || 0;
                if (brEl) brEl.textContent = t.pbr || 0;
                if (pwmEl) pwmEl.textContent = `L: ${t.pwmL || 0} · R: ${t.pwmR || 0}`;
                if (stateEl && typeof state !== 'undefined') stateEl.textContent = state.estado || 'INIT';
            };
            const interval = setInterval(updateFields, 100);
            updateFields();
        }
    });

    window.HMI_FORK_REGISTRY = HMI_FORK_REGISTRY;
})();

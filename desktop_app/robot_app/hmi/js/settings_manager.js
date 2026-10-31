// @ts-check
/** @typedef {import('@frontend/protocol').Telemetry} TelemetryPayload */
/** @typedef {import('@frontend/protocol').CommandName} CommandName */

/* ==============================================
   SETTINGS MANAGER (User Settings, Profiles, Themes, Guide & Modes)
   ============================================== */

const SettingsManager = {
    STORAGE_KEY: 'hmi_user_settings',
    settings: {
        profile: 'operator',
        telemetry_recording: 'always',
        theme: 'cyber-dark',
        language: 'es',
        default_mode: 'tabs'
    },
    init() {
        this.load();
        this.apply();
        this.bindUI();
        this.loadFromBackend();
    },
    translations: {
        es: {
            tab_conexion: 'Conexión',
            tab_mapa: 'Mapa de Navegación',
            tab_testing: 'Testing S3',
            tab_touch: 'Control Touch (no disponible)',
            tab_rutas: 'Rutas (Ockham)',
            ws_tabs: 'Pestañas',
            ws_single: 'Única',
            ws_tiling: 'Mosaico',
            ws_floating: 'Flotante',
            btn_rearm: 'Rearmar sistema',
            btn_reboot: 'Reiniciar Robot',
            btn_close: 'Cerrar aplicación',
            btn_fork: 'Crear nueva ruta planificada',
            btn_start: 'Ejecutar ruta',
            btn_save_route: 'Guardar ruta',
            btn_preview: 'Previsualizar',
            btn_stop: 'Detener',
            btn_discard: 'Descartar',
            btn_clear_mem: 'Borrar misión Python',
            settings_title: 'Configuración del HMI'
        },
        en: {
            tab_conexion: 'Connection',
            tab_mapa: 'Navigation Map',
            tab_testing: 'Testing S3',
            tab_touch: 'Touch Control (unavailable)',
            tab_rutas: 'Routes (Ockham)',
            ws_tabs: 'Tabs',
            ws_single: 'Single',
            ws_tiling: 'Tiling',
            ws_floating: 'Floating',
            btn_rearm: 'Rearm system',
            btn_reboot: 'Reboot Robot',
            btn_close: 'Close application',
            btn_fork: 'Create new planned route',
            btn_start: 'Run route',
            btn_save_route: 'Save route',
            btn_preview: 'Preview',
            btn_stop: 'Stop',
            btn_discard: 'Discard',
            btn_clear_mem: 'Clear Python mission',
            settings_title: 'HMI Settings'
        }
    },
    load() {
        try {
            const raw = localStorage.getItem(this.STORAGE_KEY);
            if (raw) {
                const parsed = JSON.parse(raw);
                this.settings = Object.assign({}, this.settings, parsed);
            }
        } catch (_) {}
    },
    async loadFromBackend() {
        if (typeof WSControl !== 'undefined' && WSControl.api) {
            try {
                const data = await WSControl.api('/api/v1/config/user-settings');
                if (data && typeof data === 'object') {
                    const previousMode = this.settings.default_mode;
                    this.settings = Object.assign({}, this.settings, data);
                    try {
                        localStorage.setItem(this.STORAGE_KEY, JSON.stringify(this.settings));
                    } catch (_) {}
                    this.apply();
                    if (typeof WindowManager !== 'undefined' && this.settings.default_mode && (this.settings.default_mode !== previousMode || WindowManager.mode !== this.settings.default_mode)) {
                        WindowManager.setMode(this.settings.default_mode, true);
                    }
                }
            } catch (_) {}
        }
    },
    save(newSettings) {
        const previousMode = this.settings.default_mode;
        this.settings = Object.assign({}, this.settings, newSettings);
        try {
            localStorage.setItem(this.STORAGE_KEY, JSON.stringify(this.settings));
        } catch (_) {}
        this.apply();
        if (typeof WindowManager !== 'undefined' && this.settings.default_mode && this.settings.default_mode !== previousMode) {
            WindowManager.setMode(this.settings.default_mode);
        }
        this.syncBackend();
    },
    apply() {
        const profile = this.settings.profile === 'technician' ? 'technician' : 'operator';
        document.body.dataset.profile = profile;

        document.documentElement.setAttribute('data-theme', this.settings.theme);
        document.body.classList.remove('theme-cyber-dark', 'theme-high-contrast', 'theme-light-glass');
        document.body.classList.add(`theme-${this.settings.theme}`);

        document.documentElement.lang = this.settings.language;
        this.applyTranslations(this.settings.language);

        const selProfile = /** @type {HTMLSelectElement | null} */ (document.getElementById('setting-profile'));
        const selTelem = /** @type {HTMLSelectElement | null} */ (document.getElementById('setting-telemetry-mode'));
        const selTheme = /** @type {HTMLSelectElement | null} */ (document.getElementById('setting-theme'));
        const selLang = /** @type {HTMLSelectElement | null} */ (document.getElementById('setting-language'));
        const selMode = /** @type {HTMLSelectElement | null} */ (document.getElementById('setting-default-mode'));
        if (selProfile) selProfile.value = profile;
        if (selTelem) selTelem.value = this.settings.telemetry_recording;
        if (selTheme) selTheme.value = this.settings.theme;
        if (selLang) selLang.value = this.settings.language;
        if (selMode) selMode.value = this.settings.default_mode;

        ['tabs', 'single', 'tiling', 'floating'].forEach(m => {
            const btn = document.getElementById(`ws-mode-${m}`);
            if (btn) {
                const isActive = m === (typeof WindowManager !== 'undefined' ? WindowManager.mode : this.settings.default_mode);
                btn.classList.toggle('active', isActive);
                btn.setAttribute('aria-pressed', isActive ? 'true' : 'false');
            }
        });
    },
    applyTranslations(lang) {
        const t = this.translations[lang] || this.translations.es;
        const setLabel = (selector, text) => {
            const el = document.querySelector(selector);
            if (el) {
                const icon = el.querySelector('svg');
                if (icon) {
                    el.innerHTML = `${icon.outerHTML}${text}`;
                } else {
                    el.textContent = text;
                }
            }
        };

        setLabel('#tab-control-conexion', t.tab_conexion);
        setLabel('#tab-control-mapa', t.tab_mapa);
        setLabel('#tab-control-testing', t.tab_testing);
        setLabel('#tab-control-touch', t.tab_touch);
        setLabel('#tab-control-rutas', t.tab_rutas);

        const wsTabsLabel = document.querySelector('#ws-mode-tabs .ws-label');
        if (wsTabsLabel) wsTabsLabel.textContent = t.ws_tabs;
        const wsSingleLabel = document.querySelector('#ws-mode-single .ws-label');
        if (wsSingleLabel) wsSingleLabel.textContent = t.ws_single;
        const wsTilingLabel = document.querySelector('#ws-mode-tiling .ws-label');
        if (wsTilingLabel) wsTilingLabel.textContent = t.ws_tiling;
        const wsFloatingLabel = document.querySelector('#ws-mode-floating .ws-label');
        if (wsFloatingLabel) wsFloatingLabel.textContent = t.ws_floating;

        const rearmLbl = document.getElementById('btn-clear-fault-label');
        if (rearmLbl && (!rearmLbl.textContent || rearmLbl.textContent.includes('Rearmar') || rearmLbl.textContent.includes('Rearm'))) {
            rearmLbl.textContent = t.btn_rearm;
        }
        const rebootLbl = document.querySelector('#btn-reboot-robot .btn-text');
        if (rebootLbl) rebootLbl.textContent = t.btn_reboot;
        const closeLbl = document.querySelector('#btn-close-app .btn-text');
        if (closeLbl) closeLbl.textContent = t.btn_close;

        setLabel('#btn-fork-route', t.btn_fork);
        setLabel('#btn-route-start', t.btn_start);
        setLabel('#btn-route-save', t.btn_save_route);
        setLabel('#btn-route-preview', t.btn_preview);
        setLabel('#btn-route-stop', t.btn_stop);
        setLabel('#btn-route-discard', t.btn_discard);
        setLabel('#btn-route-clear-memory', t.btn_clear_mem);

        const modalTitle = document.getElementById('settings-modal-title');
        if (modalTitle) modalTitle.textContent = t.settings_title;
    },
    applyInitialMode() {
        if (this.settings.default_mode && typeof WindowManager !== 'undefined') {
            WindowManager.setMode(this.settings.default_mode, true);
        }
    },
    async syncBackend() {
        if (typeof WSControl !== 'undefined' && WSControl.api) {
            try {
                await WSControl.api('/api/v1/config/user-settings', {
                    method: 'PUT',
                    body: JSON.stringify(this.settings)
                });
            } catch (_) {}
            try {
                await WSControl.api('/api/v1/config/robot', {
                    method: 'PUT',
                    body: JSON.stringify({
                        telemetry_recording_mode: this.settings.telemetry_recording
                    })
                });
            } catch (_) {}
        }
    },
    switchModalTab(tabName) {
        const tabs = ['ajustes', 'guia', 'acerca'];
        const target = tabs.includes(tabName) ? tabName : 'ajustes';
        tabs.forEach(name => {
            const btn = document.getElementById(`modal-tab-btn-${name}`);
            const panel = document.getElementById(`modal-tab-${name}`);
            const isActive = name === target;
            if (btn) {
                btn.classList.toggle('active', isActive);
                btn.setAttribute('aria-selected', isActive ? 'true' : 'false');
            }
            if (panel) {
                panel.hidden = !isActive;
            }
        });
    },
    openModal(tabName = 'ajustes') {
        const modal = document.getElementById('modal-settings');
        const btnOpen = document.getElementById('btn-open-settings');
        if (!modal) return;
        this.apply();
        this.switchModalTab(tabName);
        modal.hidden = false;
        modal.style.display = 'flex';
        if (btnOpen) btnOpen.setAttribute('aria-expanded', 'true');
    },
    closeModal() {
        const modal = document.getElementById('modal-settings');
        const btnOpen = document.getElementById('btn-open-settings');
        if (!modal) return;
        modal.hidden = true;
        modal.style.display = 'none';
        if (btnOpen) btnOpen.setAttribute('aria-expanded', 'false');
    },
    openGuideSection(sectionId) {
        const aliasMap = { calibrar: 'calibracion', conectar: 'conexion' };
        const targetId = aliasMap[sectionId] || sectionId;
        this.openModal('guia');
        const detailsList = /** @type {NodeListOf<HTMLDetailsElement>} */ (document.querySelectorAll('#modal-tab-guia details.guide-section'));
        detailsList.forEach(d => {
            const isMatch = d.dataset.guideId === targetId;
            d.open = isMatch;
            if (isMatch && typeof d.scrollIntoView === 'function') {
                d.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
            }
        });
    },
    _bindModeSelection(selMode) {
        if (!selMode) return;
        selMode.addEventListener('change', (e) => {
            const newMode = e.target.value;
            if (['tabs', 'single', 'tiling', 'floating'].includes(newMode)) {
                ['tabs', 'single', 'tiling', 'floating'].forEach(m => {
                    const btn = document.getElementById(`ws-mode-${m}`);
                    if (btn) {
                        const isActive = m === newMode;
                        btn.classList.toggle('active', isActive);
                        btn.setAttribute('aria-pressed', isActive ? 'true' : 'false');
                    }
                });
                if (typeof WindowManager !== 'undefined') {
                    WindowManager.setMode(newMode);
                }
            }
        });
    },
    _bindModalVisibility(btnOpen, btnClose, modal) {
        if (btnOpen && modal) {
            btnOpen.addEventListener('click', () => this.openModal('ajustes'));
        }
        if (btnClose && modal) {
            btnClose.addEventListener('click', () => this.closeModal());
        }
        if (modal) {
            modal.addEventListener('click', (e) => {
                if (e.target === modal) this.closeModal();
            });
        }
        document.querySelectorAll('.modal-tab-btn').forEach(btn => {
            btn.addEventListener('click', () => {
                const tab = btn.getAttribute('data-modal-tab') || 'ajustes';
                this.switchModalTab(tab);
            });
        });
    },
    _bindSaveSettings(btnSave) {
        if (!btnSave) return;
        const selProfile = /** @type {HTMLSelectElement | null} */ (document.getElementById('setting-profile'));
        if (selProfile) {
            selProfile.addEventListener('change', () => {
                this.settings.profile = selProfile.value === 'technician' ? 'technician' : 'operator';
                document.body.dataset.profile = this.settings.profile;
            });
        }
        btnSave.addEventListener('click', () => {
            const selTelem = /** @type {HTMLSelectElement | null} */ (document.getElementById('setting-telemetry-mode'));
            const selTheme = /** @type {HTMLSelectElement | null} */ (document.getElementById('setting-theme'));
            const selLang = /** @type {HTMLSelectElement | null} */ (document.getElementById('setting-language'));
            const selModeEl = /** @type {HTMLSelectElement | null} */ (document.getElementById('setting-default-mode'));

            this.save({
                profile: selProfile ? selProfile.value : this.settings.profile,
                telemetry_recording: selTelem ? selTelem.value : this.settings.telemetry_recording,
                theme: selTheme ? selTheme.value : this.settings.theme,
                language: selLang ? selLang.value : this.settings.language,
                default_mode: selModeEl ? selModeEl.value : this.settings.default_mode
            });

            this.closeModal();
            if (typeof LogSystem !== 'undefined') {
                LogSystem.add('INFO', 'Configuración guardada y aplicada correctamente.');
            }
        });
    },
    bindUI() {
        const btnOpen = document.getElementById('btn-open-settings');
        const btnClose = document.getElementById('btn-close-settings');
        const btnSave = document.getElementById('btn-save-settings');
        const modal = document.getElementById('modal-settings');
        const selMode = document.getElementById('setting-default-mode');

        this._bindModeSelection(selMode);
        this._bindModalVisibility(btnOpen, btnClose, modal);
        this._bindSaveSettings(btnSave);
    }
};


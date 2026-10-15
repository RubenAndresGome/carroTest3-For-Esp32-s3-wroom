    /* ==============================================
       GESTOR DE CONFIGURACIÓN DE USUARIO (SettingsManager)
       ============================================== */
    const SettingsManager = {
        STORAGE_KEY: 'hmi_user_settings',
        settings: {
            telemetry_recording: 'always',
            theme: 'cyber-dark',
            language: 'es',
            default_mode: 'tabs'
        },
        init() {
            this.load();
            this.apply();
            this.bindUI();
        },
        translations: {
            es: {
                tab_conexion: 'Conexión',
                tab_mapa: 'Mapa de Navegación',
                tab_testing: 'Testing S3',
                tab_touch: 'Control Touch (no disponible)',
                tab_rutas: 'Rutas (Ockham)',
                ws_tabs: 'Pestañas',
                ws_tiling: 'Mosaico',
                ws_floating: 'Flotante',
                btn_rearm: 'Rearmar sistema',
                btn_reboot: 'Reiniciar Robot',
                btn_close: 'Cerrar aplicación',
                btn_fork: 'Crear nueva ruta planificada',
                btn_start: 'Guardar la Ruta en "Rutas" / Programar recorrido',
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
                ws_tiling: 'Tiling',
                ws_floating: 'Floating',
                btn_rearm: 'Rearm system',
                btn_reboot: 'Reboot Robot',
                btn_close: 'Close application',
                btn_fork: 'Create new planned route',
                btn_start: 'Save Route in "Routes" / Schedule run',
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
            document.documentElement.setAttribute('data-theme', this.settings.theme);
            document.body.classList.remove('theme-cyber-dark', 'theme-high-contrast', 'theme-light-glass');
            document.body.classList.add(`theme-${this.settings.theme}`);

            document.documentElement.lang = this.settings.language;
            this.applyTranslations(this.settings.language);

            const selTelem = document.getElementById('setting-telemetry-mode');
            const selTheme = document.getElementById('setting-theme');
            const selLang = document.getElementById('setting-language');
            const selMode = document.getElementById('setting-default-mode');
            if (selTelem) selTelem.value = this.settings.telemetry_recording;
            if (selTheme) selTheme.value = this.settings.theme;
            if (selLang) selLang.value = this.settings.language;
            if (selMode) selMode.value = this.settings.default_mode;
        },
        applyTranslations(lang) {
            const t = this.translations[lang] || this.translations.es;
            const setLabel = (selector, text) => {
                const el = document.querySelector(selector);
                if (el) {
                    const icon = el.querySelector('svg');
                    if (icon) {
                        const iconHtml = icon.outerHTML;
                        el.innerHTML = `${iconHtml}${text}`;
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
            setLabel('#btn-route-preview', t.btn_preview);
            setLabel('#btn-route-stop', t.btn_stop);
            setLabel('#btn-route-discard', t.btn_discard);
            setLabel('#btn-route-clear-memory', t.btn_clear_mem);

            const modalTitle = document.getElementById('settings-modal-title');
            if (modalTitle) modalTitle.textContent = t.settings_title;
        },
        applyInitialMode() {
            if (this.settings.default_mode && this.settings.default_mode !== 'tabs') {
                if (typeof WindowManager !== 'undefined') {
                    WindowManager.setMode(this.settings.default_mode);
                }
            }
        },
        async syncBackend() {
            if (typeof WSControl !== 'undefined' && WSControl.api) {
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
        bindUI() {
            const btnOpen = document.getElementById('btn-open-settings');
            const btnClose = document.getElementById('btn-close-settings');
            const btnSave = document.getElementById('btn-save-settings');
            const modal = document.getElementById('modal-settings');

            if (btnOpen && modal) {
                btnOpen.addEventListener('click', () => {
                    this.apply();
                    modal.style.display = 'flex';
                    btnOpen.setAttribute('aria-expanded', 'true');
                });
            }
            if (btnClose && modal) {
                btnClose.addEventListener('click', () => {
                    modal.style.display = 'none';
                    if (btnOpen) btnOpen.setAttribute('aria-expanded', 'false');
                });
            }
            if (modal) {
                modal.addEventListener('click', (e) => {
                    if (e.target === modal) {
                        modal.style.display = 'none';
                        if (btnOpen) btnOpen.setAttribute('aria-expanded', 'false');
                    }
                });
            }
            if (btnSave && modal) {
                btnSave.addEventListener('click', () => {
                    const selTelem = document.getElementById('setting-telemetry-mode');
                    const selTheme = document.getElementById('setting-theme');
                    const selLang = document.getElementById('setting-language');
                    const selMode = document.getElementById('setting-default-mode');

                    this.save({
                        telemetry_recording: selTelem ? selTelem.value : this.settings.telemetry_recording,
                        theme: selTheme ? selTheme.value : this.settings.theme,
                        language: selLang ? selLang.value : this.settings.language,
                        default_mode: selMode ? selMode.value : this.settings.default_mode
                    });

                    modal.style.display = 'none';
                    if (btnOpen) btnOpen.setAttribute('aria-expanded', 'false');
                    if (typeof LogSystem !== 'undefined') {
                        LogSystem.add('INFO', 'Configuración guardada y aplicada correctamente.');
                    }
                });
            }
        }
    };

    function init() {
        SettingsManager.init();
        LogSystem.init(); MotorViz.init(); EncoderSpeed.init(); FSMDisplay.update('INIT');
        Calibration.init(); SurfaceManager.init(); TabManager.init(); ConnectionPanel.init(); initChart(); Touch.init();
        if (typeof WindowManager !== 'undefined') {
            WindowManager.init();
            SettingsManager.applyInitialMode();
        }
        if (typeof RouteManager !== 'undefined') RouteManager.init();
        document.querySelectorAll('.route-cinema, .route-cinema-floating-close').forEach(button=>button.addEventListener('click',()=>RouteManager.toggleCinema(button.dataset.card)));
        document.addEventListener('keydown', (e) => {
            if (e.key === 'Escape' || e.key === 'Esc') {
                const modal = document.getElementById('modal-settings');
                if (modal && modal.style.display !== 'none') {
                    modal.style.display = 'none';
                    const btnOpen = document.getElementById('btn-open-settings');
                    if (btnOpen) btnOpen.setAttribute('aria-expanded', 'false');
                }
                const openForkMenu = document.getElementById('glass-fork-menu');
                if (openForkMenu && openForkMenu.style.display !== 'none') {
                    openForkMenu.style.display = 'none';
                }
                const activeCinema = document.querySelector('.route-map-card.cinema-mode');
                if (activeCinema && typeof RouteManager !== 'undefined') {
                    RouteManager.toggleCinema(activeCinema.id);
                }
            }
        });
        document.addEventListener('visibilitychange',()=>{if(document.hidden)Touch.end();});
        window.addEventListener('blur',()=>Touch.end());
        window.addEventListener('pagehide',()=>Touch.end());

        // Control de alternancia de signo (+/-) para entradas numéricas
        document.querySelectorAll('[data-toggle-sign]').forEach(button=>button.addEventListener('click',()=>{
            const input = document.getElementById(button.dataset.toggleSign);
            if (!input) return;
            const val = input.value.trim();
            if (val === '' || val === '0' || val === '0.0') {
                input.value = '-';
            } else if (val === '-') {
                input.value = '';
            } else if (val === '-0' || val === '-0.0') {
                input.value = '0';
            } else if (val.startsWith('-')) {
                input.value = val.slice(1);
            } else {
                input.value = '-' + val;
            }
            input.dispatchEvent(new Event('input', {bubbles:true}));
            input.focus();
        }));

        // E‑STOP
        document.getElementById('btn-estop').addEventListener('click', function() {
            this.classList.add('btn-flash');
            setTimeout(() => this.classList.remove('btn-flash'), 300);
            if (typeof RouteManager !== 'undefined') {
                if (RouteManager.isRunning) {
                    RouteManager.failRoute('Parada de emergencia (E-STOP) activada por el operador: ruta cancelada.');
                }
                RouteManager.unlockTabs();
            }
            WSControl.send({cmd:"estop"});
            LogSystem.add('ERROR', 'E-STOP enviado al robot');
        });
        document.getElementById('btn-close-app').addEventListener('click',()=>WSControl.closeApplication(false));

        // Reset FSM / Rearmar HMI
        const btnResetHmi = document.getElementById('btn-reset-hmi');
        if (btnResetHmi) {
            btnResetHmi.addEventListener('click', async function() {
                this.classList.add('btn-flash');
                setTimeout(() => this.classList.remove('btn-flash'), 250);
                this.disabled = true;
                try {
                    LogSystem.add('CMD', 'Restableciendo FSM y purgando memoria de misión de Python...');
                    await WSControl.api('/api/v1/missions/robot-memory', { method: 'DELETE' }).catch(err => {
                        LogSystem.add('WARN', `Aviso al purgar memoria: ${err.message}`);
                    });
                    if (typeof RouteManager !== 'undefined') {
                        RouteManager.isRunning = false;
                        RouteManager.waitingForMovement = false;
                        RouteManager.activeCommandId = null;
                        clearTimeout(RouteManager.moveTimeout);
                        RouteManager.unlockTabs();
                        RouteManager.updateListUI();
                        RouteManager.drawPlan();
                    }
                    if (state.connected) {
                        await WSControl.send({ cmd: 'clear_fault' });
                    }
                    if (typeof FSMDisplay !== 'undefined') {
                        FSMDisplay.update('IDLE');
                    }
                    LogSystem.add('INFO', 'FSM restablecida y pestañas desbloqueadas correctamente.');
                } catch (e) {
                    LogSystem.add('ERROR', `Fallo al restablecer FSM: ${e.message}`);
                } finally {
                    this.disabled = false;
                    if (typeof Calibration !== 'undefined') {
                        Calibration.updateAvailability();
                    }
                }
            });
        }

        // Reiniciar Robot (CPU ESP32) con confirmación modal e interlock seguro (PWM=0)
        const btnReboot = document.getElementById('btn-reboot-robot');
        if (btnReboot) {
            btnReboot.addEventListener('click', async function() {
                if (!confirm('ADVERTENCIA DE SEGURIDAD:\n¿Confirmas el reinicio de la CPU del Robot ESP32?\n\nLos motores se detendrán inmediatamente (PWM=0) y se reiniciará el microcontrolador.')) return;
                this.classList.add('btn-flash');
                this.disabled = true;
                try {
                    LogSystem.add('WARN', 'Reinicio de Robot: Forzando parada inmediata (PWM=0)...');
                    if (state.connected) {
                        await WSControl.send({ cmd: 'stop' });
                    }
                    await WSControl.api('/api/v1/missions/robot-memory', { method: 'DELETE' }).catch(() => {});
                    if (typeof RouteManager !== 'undefined') {
                        RouteManager.isRunning = false;
                        RouteManager.unlockTabs();
                    }
                    await new Promise(r => setTimeout(r, 150));
                    LogSystem.add('CMD', 'Solicitando reconexión y reinicio de enlace ESP32...');
                    await WSControl.api('/api/v1/connection/reconnect', { method: 'POST' }).catch(() => {});
                    LogSystem.add('INFO', 'Comando de reinicio ejecutado. Reconectando con ESP32...');
                } catch (err) {
                    LogSystem.add('ERROR', `Error durante reinicio de robot: ${err.message}`);
                } finally {
                    setTimeout(() => { if (btnReboot) btnReboot.disabled = false; }, 1500);
                }
            });
        }

        // Testing: Aplicar PWM (Bloqueado por protocolo canónico de pasos)
        document.getElementById('btn-apply-pwm').disabled = true;
        document.getElementById('btn-apply-pwm').title = `No disponible en ${CANONICAL_CONFIG.PROTOCOL_NAME}. El hardware DRV8833 exige control cerrado por MPU.`;
        document.getElementById('btn-apply-pwm').addEventListener('click', function() {
            LogSystem.add('WARN', `Control PWM directo bloqueado: el protocolo ${CANONICAL_CONFIG.PROTOCOL_NAME} protege los DRV8833 exigiendo control en lazo cerrado.`);
            this.classList.add('btn-flash'); setTimeout(()=>this.classList.remove('btn-flash'),250);
        });

        document.querySelectorAll('.dir-btn').forEach(b=>b.addEventListener('click',function(){
            const m = this.dataset.motor;
            document.querySelectorAll(`.dir-btn[data-motor="${m}"]`).forEach(el => el.classList.remove('active-fwd','active-rev','active-stop'));
            this.classList.add(this.dataset.dir==='fwd'?'active-fwd':this.dataset.dir==='rev'?'active-rev':'active-stop');
            state.motorDir[m] = this.dataset.dir;
        }));

        document.querySelectorAll('[data-cmd]').forEach(b=>b.addEventListener('click',function(){
            const c=this.dataset.cmd;
            const dist=parseFloat(document.getElementById('cmd-dist').value)||50;
            const angle=parseFloat(document.getElementById('cmd-angle').value)||90;
            this.classList.add('btn-flash'); setTimeout(()=>this.classList.remove('btn-flash'),250);

            if(c==='avance'){
                WSControl.send({cmd:'step',heading:0,cm:dist,drive_mode:'forward'});
            } else if(c==='retroceso'){
                WSControl.send({cmd:'step',heading:180,cm:dist,drive_mode:'reverse'});
            } else if(c==='giro-izq'){
                WSControl.send({cmd:'turn_to',heading:(Number(state.telemetria.angulo||0)-angle+360)%360});
            } else if(c==='giro-der'){
                WSControl.send({cmd:'turn_to',heading:(Number(state.telemetria.angulo||0)+angle)%360});
            } else if(c==='parada'){
                WSControl.send({cmd:"stop"});
                LogSystem.add('CMD', 'Parada inmediata enviada');
            } else if(c==='reset'){
                WSControl.send({cmd:"reset_pose"});
                LogSystem.add('CMD', 'Reset pose enviado');
            }
        }));

        document.querySelectorAll('.log-filter-btn').forEach(b=>b.addEventListener('click',function(){
            this.classList.toggle('active');
            this.setAttribute('aria-pressed', this.classList.contains('active') ? 'true' : 'false');
            LogSystem.setFilter(this.dataset.filter, this.classList.contains('active'));
        }));
        
        document.getElementById('btn-download-log').addEventListener('click',()=>LogSystem.downloadTXT());
        document.getElementById('btn-clear-log').addEventListener('click',()=>{LogSystem.clear();LogSystem.add('INFO','Log limpiado');});
        document.getElementById('btn-purge-db-logs').addEventListener('click', async () => {
            const select = document.getElementById('select-purge-days');
            const days = parseInt(select.value, 10);
            const label = select.options[select.selectedIndex].text;
            if (!confirm(`¿Confirmas eliminar los registros de la Base de Datos con antigüedad: ${label}?`)) return;
            try {
                const token = document.querySelector('meta[name="app-token"]').content;
                const res = await fetch('/api/v1/sessions/cleanup', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json', 'X-App-Token': token },
                    body: JSON.stringify({ days })
                });
                const data = await res.json();
                if (res.ok) {
                    LogSystem.add('INFO', `Limpieza de Base de Datos exitosa: ${data.message}`);
                    if (typeof ConnectionPanel !== 'undefined' && ConnectionPanel.loadHistory) ConnectionPanel.loadHistory();
                } else {
                    LogSystem.add('ERROR', `Error al limpiar BD: ${data.error || 'error desconocido'}`);
                }
            } catch (err) {
                LogSystem.add('ERROR', `Error al conectar con backend para limpieza: ${err.message}`);
            }
        });

        // Soporte de navegación por URL y hash (/plan, /boost)
        const path = window.location.pathname.toLowerCase();
        const hash = window.location.hash.toLowerCase();
        if (path.endsWith('/plan') || hash === '#/plan' || hash === '#plan') {
            if (typeof TabManager !== 'undefined') TabManager.switch('rutas');
            if (typeof WindowManager !== 'undefined') WindowManager.bringToFront('tab-rutas');
        } else if (path.endsWith('/boost') || hash === '#/boost' || hash === '#boost') {
            if (typeof WindowManager !== 'undefined' && WindowManager.openWindow) {
                WindowManager.openWindow('tab-boost');
            }
        }

        WSControl.init();
    }

    if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', init); 
    else init();

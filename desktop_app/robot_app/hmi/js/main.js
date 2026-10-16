// @ts-check
/** @typedef {import('@frontend/protocol').Telemetry} TelemetryPayload */
/** @typedef {import('@frontend/protocol').CommandName} CommandName */

/* ==============================================
   HMI APPLICATION BOOTSTRAP (init & Global Event Listeners)
   ============================================== */

/**
 * Vincula atajos de teclado globales (Escape para modales y modo cine).
 */
function bindKeyboardShortcuts() {
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

    document.addEventListener('visibilitychange', () => {
        if (document.hidden) (/** @type {RobotJoystick} */ (/** @type {unknown} */ (Touch))).end();
    });
    window.addEventListener('blur', () => (/** @type {RobotJoystick} */ (/** @type {unknown} */ (Touch))).end());
    window.addEventListener('pagehide', () => (/** @type {RobotJoystick} */ (/** @type {unknown} */ (Touch))).end());
}

/**
 * Control de alternancia de signo (+/-) para entradas numéricas.
 */
function bindSignTogglers() {
    document.querySelectorAll('[data-toggle-sign]').forEach(elem => {
        const button = /** @type {HTMLElement} */ (elem);
        button.addEventListener('click', () => {
            const input = /** @type {HTMLInputElement | null} */ (document.getElementById(button.dataset.toggleSign || ''));
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
            input.dispatchEvent(new Event('input', { bubbles: true }));
            input.focus();
        });
    });
}

/**
 * Botón E-STOP de parada de emergencia inmediata.
 */
function _bindEstopButton() {
    const btnEstop = document.getElementById('btn-estop');
    if (!btnEstop) return;
    btnEstop.addEventListener('click', function() {
        this.classList.add('btn-flash');
        setTimeout(() => this.classList.remove('btn-flash'), 300);
        if (typeof RouteManager !== 'undefined') {
            if (RouteManager.isRunning) {
                RouteManager.failRoute('Parada de emergencia (E-STOP) activada por el operador: ruta cancelada.');
            }
            RouteManager.unlockTabs();
        }
        WSControl.send({ cmd: 'estop' });
        LogSystem.add('ERROR', 'E-STOP enviado al robot');
    });

    const btnCloseApp = document.getElementById('btn-close-app');
    if (btnCloseApp) {
        btnCloseApp.addEventListener('click', () => WSControl.closeApplication(false));
    }
}

/**
 * Botón de restablecimiento de FSM y purga de memoria de Python.
 */
function _bindResetHmiButton() {
    const btnResetHmi = /** @type {HTMLButtonElement | null} */ (document.getElementById('btn-reset-hmi'));
    if (!btnResetHmi) return;

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

/**
 * Botón de reinicio de CPU ESP32 con confirmación e interlock seguro.
 */
function _bindRebootRobotButton() {
    const btnReboot = /** @type {HTMLButtonElement | null} */ (document.getElementById('btn-reboot-robot'));
    if (!btnReboot) return;

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

/**
 * Controles de emergencia y rearmado del HMI.
 */
function bindEmergencyControls() {
    _bindEstopButton();
    _bindResetHmiButton();
    _bindRebootRobotButton();
}

/**
 * Controles de prueba rápida y motores en pestaña Testing.
 */
function bindTestingControls() {
    const btnApplyPwm = /** @type {HTMLButtonElement | null} */ (document.getElementById('btn-apply-pwm'));
    if (btnApplyPwm) {
        btnApplyPwm.disabled = true;
        btnApplyPwm.title = `No disponible en ${CANONICAL_CONFIG.PROTOCOL_NAME}. El hardware DRV8833 exige control cerrado por MPU.`;
        btnApplyPwm.addEventListener('click', function() {
            LogSystem.add('WARN', `Control PWM directo bloqueado: el protocolo ${CANONICAL_CONFIG.PROTOCOL_NAME} protege los DRV8833 exigiendo control en lazo cerrado.`);
            this.classList.add('btn-flash');
            setTimeout(() => this.classList.remove('btn-flash'), 250);
        });
    }

    document.querySelectorAll('.dir-btn').forEach(elem => {
        const b = /** @type {HTMLElement} */ (elem);
        b.addEventListener('click', function() {
            const m = this.dataset.motor;
            if (!m) return;
            document.querySelectorAll(`.dir-btn[data-motor="${m}"]`).forEach(el => el.classList.remove('active-fwd', 'active-rev', 'active-stop'));
            this.classList.add(this.dataset.dir === 'fwd' ? 'active-fwd' : this.dataset.dir === 'rev' ? 'active-rev' : 'active-stop');
            state.motorDir[m] = this.dataset.dir;
        });
    });

    document.querySelectorAll('[data-cmd]').forEach(elem => {
        const b = /** @type {HTMLElement} */ (elem);
        b.addEventListener('click', function() {
            _executeQuickCommand(this);
        });
    });
}

/**
 * Ejecuta comando rápido de prueba (avance, retroceso, giro, parada, reset).
 * @param {HTMLElement} btn
 */
function _executeQuickCommand(btn) {
    const c = btn.dataset.cmd;
    const distInput = /** @type {HTMLInputElement | null} */ (document.getElementById('cmd-dist'));
    const angleInput = /** @type {HTMLInputElement | null} */ (document.getElementById('cmd-angle'));
    const dist = parseFloat(distInput?.value || '') || 50;
    const angle = parseFloat(angleInput?.value || '') || 90;
    btn.classList.add('btn-flash');
    setTimeout(() => btn.classList.remove('btn-flash'), 250);

    if (c === 'avance') {
        WSControl.send({ cmd: 'step', heading: 0, cm: dist, drive_mode: 'forward' });
    } else if (c === 'retroceso') {
        WSControl.send({ cmd: 'step', heading: 180, cm: dist, drive_mode: 'reverse' });
    } else if (c === 'giro-izq') {
        WSControl.send({ cmd: 'turn_to', heading: (Number(state.telemetria.angulo || 0) - angle + 360) % 360 });
    } else if (c === 'giro-der') {
        WSControl.send({ cmd: 'turn_to', heading: (Number(state.telemetria.angulo || 0) + angle) % 360 });
    } else if (c === 'parada') {
        WSControl.send({ cmd: 'stop' });
        LogSystem.add('CMD', 'Parada inmediata enviada');
    } else if (c === 'reset') {
        WSControl.send({ cmd: 'reset_pose' });
        LogSystem.add('CMD', 'Reset pose enviado');
    }
}

/**
 * Mantenimiento de logs y purga de base de datos SQLite.
 */
function bindLogAndDatabaseMaintenance() {
    document.querySelectorAll('.log-filter-btn').forEach(elem => {
        const b = /** @type {HTMLElement} */ (elem);
        b.addEventListener('click', function() {
            this.classList.toggle('active');
            this.setAttribute('aria-pressed', this.classList.contains('active') ? 'true' : 'false');
            LogSystem.setFilter(this.dataset.filter, this.classList.contains('active'));
        });
    });

    document.getElementById('btn-download-log')?.addEventListener('click', () => LogSystem.downloadTXT());
    document.getElementById('btn-clear-log')?.addEventListener('click', () => {
        LogSystem.clear();
        LogSystem.add('INFO', 'Log limpiado');
    });

    const btnPurge = document.getElementById('btn-purge-db-logs');
    if (btnPurge) {
        btnPurge.addEventListener('click', _handleDatabasePurge);
    }
}

/**
 * Ejecuta solicitud de purga de historial en base de datos.
 */
async function _handleDatabasePurge() {
    const select = /** @type {HTMLSelectElement | null} */ (document.getElementById('select-purge-days'));
    if (!select) return;
    const days = parseInt(select.value, 10);
    const label = select.options[select.selectedIndex]?.text || '';
    if (!confirm(`¿Confirmas eliminar los registros de la Base de Datos con antigüedad: ${label}?`)) return;

    try {
        const meta = /** @type {HTMLMetaElement | null} */ (document.querySelector('meta[name="app-token"]'));
        const token = meta?.content || '';
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
}

/**
 * Manejo de rutas URL y fragmentos hash (/plan, /boost).
 */
function handleUrlRouting() {
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
}

/**
 * Punto de entrada principal de la aplicación HMI.
 */
function init() {
    SettingsManager.init();
    LogSystem.init();
    MotorViz.init();
    EncoderSpeed.init();
    FSMDisplay.update('INIT');
    Calibration.init();
    SurfaceManager.init();
    TabManager.init();
    ConnectionPanel.init();
    initChart();
    (/** @type {RobotJoystick} */ (/** @type {unknown} */ (Touch))).init();

    if (typeof WindowManager !== 'undefined') {
        WindowManager.init();
        SettingsManager.applyInitialMode();
    }
    if (typeof RouteManager !== 'undefined') {
        RouteManager.init();
    }

    document.querySelectorAll('.route-cinema, .route-cinema-floating-close').forEach(elem => {
        const button = /** @type {HTMLElement} */ (elem);
        button.addEventListener('click', () => {
            if (button.dataset.card && typeof RouteManager !== 'undefined') {
                RouteManager.toggleCinema(button.dataset.card);
            }
        });
    });

    bindKeyboardShortcuts();
    bindSignTogglers();
    bindEmergencyControls();
    bindTestingControls();
    bindLogAndDatabaseMaintenance();
    handleUrlRouting();

    WSControl.init();
}

if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
} else {
    init();
}

// @ts-check
/** @typedef {import('@frontend/protocol').Telemetry} TelemetryPayload */
/** @typedef {import('@frontend/protocol').CommandName} CommandName */

/* ==============================================
   CONFIGURACIÓN CANÓNICA Y PROTOCOLO
   ============================================== */
const CANONICAL_CONFIG = {
    PROTOCOL_NAME: 'robot-s3-steps-v3',
    EXPECTED_FIRMWARE: 'robot-s3-v3.7',
    SUPPORTED_COMMANDS: new Set(['calibrate', 'step', 'turn_to', 'stop', 'estop', 'clear_fault', 'set_comp', 'reset_pose']),
};

/* ==============================================
   CHART.JS LOGIC & VARIABLES GLOBALES
   ============================================== */
let robotChart;
let totalDistance = 0;
let totalPoints = 0;
let currentPoint = 0;
let lastPoint = { x: 0, y: 0 };

/**
 * Crea la instancia de Chart.js tipo scatter para la trayectoria global.
 * @param {CanvasRenderingContext2D} ctx
 * @param {CSSStyleDeclaration} chartTheme
 */
function _createScatterChart(ctx, chartTheme) {
    const chartInfo = chartTheme.getPropertyValue('--info').trim();
    const chartAccent = chartTheme.getPropertyValue('--accent').trim();
    const chartGrid = chartTheme.getPropertyValue('--border').trim();
    const chartText = chartTheme.getPropertyValue('--text-secondary').trim();

    return new Chart(ctx, {
        type: 'scatter',
        data: {
            datasets: [
                {
                    label: 'Trayectoria del Robot', data: [{ x: 0, y: 0 }], borderColor: chartInfo, backgroundColor: chartInfo,
                    showLine: true, tension: 0.3, borderWidth: 2, pointRadius: 3, pointBackgroundColor: chartInfo
                },
                {
                    label: 'Posición Actual', data: [{ x: 0, y: 0 }], borderColor: chartAccent, backgroundColor: chartAccent,
                    showLine: false, pointRadius: 7, pointBackgroundColor: chartAccent
                }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                datalabels: {
                    align: 'top', color: chartText, font: { weight: 'bold' },
                    formatter: function(value, context) {
                        const dataset = context.chart.data.datasets[0].data;
                        if (value === dataset[dataset.length - 1]) {
                            return 'Distancia total: ' + totalDistance.toFixed(2);
                        }
                        return '';
                    }
                },
                zoom: {
                    pan: { enabled: true, mode: 'xy' },
                    zoom: { wheel: { enabled: true }, pinch: { enabled: true }, mode: 'xy' }
                }
            },
            scales: {
                x: { title: { display: true, text: 'X', color: chartText }, ticks: { color: chartText }, grid: { color: chartGrid }, min: -1000, max: 1000 },
                y: { title: { display: true, text: 'Y', color: chartText }, ticks: { color: chartText }, grid: { color: chartGrid }, min: -1000, max: 1000 }
            }
        },
        plugins: [ChartDataLabels]
    });
}

/**
 * Vincula el formulario para añadir coordenadas manuales al gráfico.
 */
function _bindChartFormEvents() {
    const form = document.getElementById('chart-form');
    if (!form) return;

    form.addEventListener('submit', function(event) {
        event.preventDefault();
        const xEl = /** @type {HTMLInputElement | null} */ (document.getElementById('chart-x'));
        const yEl = /** @type {HTMLInputElement | null} */ (document.getElementById('chart-y'));
        const xInput = xEl ? xEl.value : '';
        const yInput = yEl ? yEl.value : '';
        if (xInput === '' || yInput === '') return;
        const x = parseFloat(xInput);
        const y = parseFloat(yInput);
        if (!Number.isFinite(x) || !Number.isFinite(y)) return;

        const newPoint = { x: lastPoint.x + x, y: lastPoint.y + y };
        robotChart.data.datasets[0].data.push(newPoint);

        const dx = newPoint.x - lastPoint.x;
        const dy = newPoint.y - lastPoint.y;
        totalDistance += Math.sqrt(dx * dx + dy * dy);

        lastPoint = newPoint;
        totalPoints = 1;
        currentPoint = 0;

        robotChart.data.datasets[1].data[0] = newPoint;
        robotChart.update();

        WSControl.sendMove(newPoint.x, newPoint.y);
        actualizarBarra();
        if (xEl) xEl.value = '';
        if (yEl) yEl.value = '';
    });
}

/**
 * Vincula los botones de reset y zoom del gráfico.
 */
function _bindChartControlButtons() {
    const btnReset = document.getElementById('btn-chart-reset');
    if (btnReset) {
        btnReset.addEventListener('click', function() {
            robotChart.data.datasets[0].data = [{ x: 0, y: 0 }];
            robotChart.data.datasets[1].data = [{ x: 0, y: 0 }];
            totalDistance = 0;
            totalPoints = 0;
            currentPoint = 0;
            lastPoint = { x: 0, y: 0 };
            const bar = document.getElementById('chart-progressBar');
            if (bar) bar.style.width = '0%';
            const lbl = document.getElementById('chart-distanceLabel');
            if (lbl) lbl.innerHTML = 'Distancia recorrida: <strong>0.00 cm</strong>';
            robotChart.update();
        });
    }

    const btnZoom = document.getElementById('btn-chart-zoom');
    if (btnZoom) {
        btnZoom.addEventListener('click', function() {
            if (robotChart) robotChart.resetZoom();
        });
    }
}

/**
 * Inicializa el gráfico central de trayectoria y sus escuchadores.
 */
function initChart() {
    const canvas = /** @type {HTMLCanvasElement | null} */ (document.getElementById('robotChart'));
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;
    const chartTheme = getComputedStyle(document.documentElement);

    robotChart = _createScatterChart(ctx, chartTheme);
    _bindChartFormEvents();
    _bindChartControlButtons();
}

function actualizarBarra() {
    const progressBar = document.getElementById('chart-progressBar');
    let porcentaje = totalPoints === 0 ? 0 : (currentPoint / totalPoints) * 100;
    if (progressBar) progressBar.style.width = porcentaje + '%';
    const lbl = document.getElementById('chart-distanceLabel');
    if (lbl) lbl.innerHTML = `Distancia recorrida: <strong>${totalDistance.toFixed(2)} cm</strong>`;
    if (robotChart) robotChart.update();
}

/* ==============================================
   CONFIG GENERAL Y ESTADO UI
   ============================================== */
const CONFIG = {
    MAX_RPM_VISUAL: 120,
    MIN_RPM_VISUAL: 5,
    TRAIL_MAX: 200,
    LOG_MAX: 500,
    JOYSTICK_MAX_RADIUS: 108,
    JOYSTICK_DEAD_ZONE: 18
};

/** @type {HmiState} */
const state = {
    connected: false,
    demoMode: false,
    estado: 'INIT',
    estadoAnterior: '',
    telemetria: { distancia: 0, angulo: 0, posX: 0, posY: 0, orientacion: 0, pfl: 0, pfr: 0, pbl: 0, pbr: 0, pwmL: 0, pwmR: 0 },
    clientes: 0,
    trail: [],
    logFilters: { info: true, warn: true, error: true, cmd: true },
    motorDir: { left: 'stop', right: 'stop' },
    activeTab: 'conexion',
    joy: { active: false, nx: 0, ny: 0, magnitude: 0, angle: 0, pwmL: 0, pwmR: 0, dirL: 'stop', dirR: 'stop' },
    manualDriveSupported: false
};

/* ==============================================
   TRADUCCIÓN DE CÓDIGOS DE ERROR
   ============================================== */
const ERROR_MESSAGES = Object.freeze(/** @type {const} */ ({
    robot_restarted_mid_mission: 'El ESP32 se reinició durante la misión; el paso no se repetirá automáticamente',
    robot_restarted_mid_step: 'El ESP32 se reinició durante el paso; ejecución cancelada',
    operator_stop: 'Detenida por el operador',
    operator_clear: 'Limpieza de misión por el operador',
    operator_app_close: 'Cierre de la aplicación por el operador',
    forced_app_close: 'Cierre forzado de la aplicación',
    application_closed: 'Aplicación cerrada',
    session_closed: 'Sesión cerrada',
    app_restarted_with_pending_mission: 'La aplicación se reinició con una misión pendiente',
    mission_segment_timeout: 'Tiempo límite del segmento de misión excedido',
    mission_active_command_mismatch: 'Incoherencia entre comando activo y reporte del robot',
    mission_reconciliation_missing_command: 'Comando de misión no encontrado durante la reconciliación',
    '5v_fuente_desconectada': 'Alimentación de 5V o USB desconectada durante la misión',
    heading_recovery_exhausted: 'Intentos de recuperación de rumbo agotados sin converger',
    final_endpoint_out_of_tolerance: 'Punto final fuera de la tolerancia permitida',
    Time_out_eliminada_por_tiempo_maximo_para_expiracion: 'Misión cancelada por superar el tiempo máximo de expiración',
    stopped_by_estop: 'Misión detenida por parada de emergencia (E-STOP)',
    calibration_started: 'Misión cancelada por inicio de calibración',
    calibration_completed: 'Misión cancelada por calibración completada',
    cal_connection_lost: 'Conexión con el backend perdida durante la calibración',
    mpu_unavailable_cal: 'MPU no disponible o no calibrada al iniciar calibración',
    mpu_lost_cal: 'Señales de MPU perdidas durante la calibración',
    mpu_lost: 'Señales de IMU MPU perdidas durante la operación',
    motor_output_error: 'Error en salida de potencia a motores',
    motor_output_unavailable: 'Salida a motores no disponible o deshabilitada',
    motor_setup_error: 'Error de inicialización en configuración de motores',
    cal_yaw_sign_mismatch: 'Discrepancia en el signo angular de calibración MPU',
    cal_dir_failed: 'Fallo al determinar la dirección de giro en calibración',
    cal_stall_left: 'Bloqueo detectado en ruedas izquierdas durante calibración',
    cal_stall_right: 'Bloqueo detectado en ruedas derechas durante calibración',
    cal_return_timeout: 'Tiempo límite de retorno a origen superado en calibración',
    cal_yaw_divergence: 'Divergencia angular en calibración MPU',
    cal_unavailable: 'Calibración no disponible en el estado actual',
    cal_injection_failed: 'Fallo de inyección de comando de calibración',
    cal_icr_invalid: 'Parámetros ICR de calibración inválidos',
    cal_gyro_scale_invalid: 'Escala de giróscopo de calibración inválida',
    cal_pivot_one_side_only: 'Calibración inválida: sólo un lado registró movimiento',
    cal_pivot_asymmetric: 'Calibración inválida: pivote asimétrico fuera de rango',
    turn_stall_left: 'Bloqueo del lado izquierdo durante maniobra de giro',
    turn_stall_right: 'Bloqueo del lado derecho durante maniobra de giro',
    turn_timeout_total: 'Tiempo límite total de giro superado',
    turn_timeout_attempt: 'Tiempo límite de intento de giro superado',
    turn_angular_divergence: 'Divergencia angular acumulada durante el giro',
    turn_not_calibrated: 'Giro bloqueado: robot no calibrado',
    turn_no_progress: 'Giro sin progreso angular detectado por MPU',
    turn_unavailable: 'Maniobra de giro no disponible',
    turn_payload_invalid: 'Parámetros de giro inválidos',
    drive_stall_left: 'Bloqueo detectado en ruedas izquierdas durante avance',
    drive_stall_right: 'Bloqueo detectado en ruedas derechas durante avance',
    drive_timeout: 'Tiempo límite de avance excedido sin alcanzar objetivo',
    chassis_reverse_slip_detected: 'Deslizamiento inverso anómalo detectado en el chasis',
    enc_no_side: 'Pérdida de pulsos de encoder en uno de los lados',
    enc_no_estimation: 'Imposible estimar trayectoria por encoders',
    enc_no_estimation_settle: 'Fallo en asentamiento por lectura de encoders',
    endpoint_not_reached: 'Punto final del recorrido no alcanzado',
    pcnt_init_failed: 'Fallo de inicialización en contadores de pulsos PCNT',
    cal_timeout: 'Tiempo máximo de calibración superado',
    cal_aborted: 'Calibración abortada'
}));

/** @param {unknown} code @returns {string} */
function humanizeError(code) {
    const key = String(code ?? '').trim();
    if (!key) return 'error desconocido';
    /** @type {string | undefined} */
    const msg = ERROR_MESSAGES[/** @type {keyof typeof ERROR_MESSAGES} */ (key)];
    if (msg) return `${msg} (código: ${key})`;
    for (const [prefix, template] of Object.entries(ERROR_MESSAGES)) {
        if (key.startsWith(prefix)) {
            return `${template} (código: ${key})`;
        }
    }
    return key;
}

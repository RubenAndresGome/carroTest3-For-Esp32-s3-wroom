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

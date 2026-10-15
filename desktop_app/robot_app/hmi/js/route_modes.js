/* ==============================================
   ROUTE MODES & PLANNING STRATEGIES (Strategy Pattern)
   ============================================== */

/**
 * Normaliza un ángulo a [-180, 180) grados con signo canónico.
 * @param {number|string} value - Ángulo en grados.
 * @returns {number} Ángulo normalizado en [-180, 180).
 */
const normalizeSignedDegrees = value => {
    const normalized = ((Number(value) + 180) % 360 + 360) % 360 - 180;
    return Object.is(normalized, -0) ? 0 : normalized;
};

/**
 * Limpia componentes infinitesimales por redondeo de coma flotante.
 * @param {number} value - Componente cartesiano.
 * @returns {number} Valor limpio o cero si es menor al umbral.
 */
const cleanRouteComponent = value => Math.abs(value) < 1e-9 ? 0 : value;

/**
 * Clase base abstracta para modos de planeación de ruta.
 */
class RoutePlanningMode {
    constructor(key) {
        this.key = key;
    }

    configureForm() {
        throw new Error('Modo de planeación sin formulario concreto');
    }

    createLogicalStep() {
        throw new Error('Modo de planeación sin cálculo concreto');
    }

    buildVisualization() {
        throw new Error('Modo de planeación sin visualización concreta');
    }

    updatePreview() {}

    missionPayload(manager) {
        return {
            points: manager.queue.map(wp => ({
                x_mm: wp.x * 10,
                y_mm: wp.y * 10
            }))
        };
    }

    expandExecutionWaypoints(manager, step) {
        step.execution.forEach(segment => manager.appendLimited(
            segment.x0, segment.y0, segment.x1, segment.y1,
            segment.label, step.id, segment.component
        ));
    }
}

/**
 * Modo de planeación ortogonal rectangular (ΔX, ΔY).
 */
class RectangularRouteMode extends RoutePlanningMode {
    constructor() {
        super('rect');
    }

    configureForm(manager) {
        manager.lbl1.textContent = 'Coordenada Relativa X (+X Derecha):';
        manager.lbl2.textContent = 'Coordenada Relativa Y (+Y Frente):';
        manager.input1.removeAttribute('min');
        manager.input1.removeAttribute('max');
        manager.input2.removeAttribute('min');
        manager.input2.removeAttribute('max');
        const sign1 = document.getElementById('route-sign-1');
        if (sign1) sign1.style.display = '';
        manager.preview.style.display = 'none';
        manager.axisDefaultGroup.hidden = false;
        manager.axisDefault.disabled = false;
        manager.axisDefault.checked = true;
        manager.actualizarRestriccionInputs(true);
        manager.angularMeta.hidden = true;
        manager.vectorialSafety.hidden = true;
        manager.modeChip.textContent = 'RECTANGULAR · FIN 0°';
        manager.planCanvas.setAttribute('aria-label', 'Gráfica rectangular del plan teórico');
    }

    createLogicalStep(manager, dx, dy) {
        if (!Number.isFinite(dx) || !Number.isFinite(dy)) {
            throw new Error('Ingresa coordenadas X e Y válidas.');
        }
        if (Math.abs(dx) <= 0.01 && Math.abs(dy) <= 0.01) {
            throw new Error('El paso rectangular no contiene desplazamiento.');
        }
        const start = manager.logicalEndpoint();
        const end = { x: start.x + dx, y: start.y + dy };
        const id = manager.nextLogicalStepId++;
        const number = manager.logicalSteps.length + 1;
        const execution = [];
        if (Math.abs(dx) > 0.01) {
            execution.push({
                x0: start.x, y0: start.y, x1: end.x, y1: start.y,
                label: `P${number} · Componente X`, component: 'x'
            });
        }
        if (Math.abs(dy) > 0.01) {
            execution.push({
                x0: end.x, y0: start.y, x1: end.x, y1: end.y,
                label: `P${number} · Componente Y`, component: 'y'
            });
        }
        return {
            id, number, mode: this.key, start, end, dx, dy, execution,
            description: `Rectangular ΔX=${dx.toFixed(2)}, ΔY=${dy.toFixed(2)}`
        };
    }

    buildVisualization(manager) {
        return {
            label: 'Ruta planificada',
            points: [
                { x: 0, y: 0 },
                ...manager.queue.map(wp => ({
                    x: wp.x - manager.currentOrigin.x,
                    y: wp.y - manager.currentOrigin.y
                }))
            ],
            geometry: null
        };
    }
}

/**
 * Modo de planeación angular base (Longitud L, Ángulo θ relativo).
 */
class AngularRouteMode extends RoutePlanningMode {
    constructor(key, apiMode, direct) {
        super(key);
        this.apiMode = apiMode;
        this.direct = direct;
    }

    configureForm(manager) {
        manager.lbl1.textContent = 'Longitud L (cm):';
        manager.lbl2.textContent = 'Ángulo θ relativo (°):';
        manager.input1.min = '0';
        manager.input1.removeAttribute('max');
        manager.input2.min = '-180';
        manager.input2.max = '180';
        const sign1 = document.getElementById('route-sign-1');
        if (sign1) sign1.style.display = 'none';
        manager.preview.style.display = 'block';
        manager.axisDefaultGroup.hidden = true;
        manager.axisDefault.disabled = true;
        manager.rectangularCeroAutomatico = null;
        manager.actualizarRestriccionInputs(false);
        manager.angularMeta.hidden = false;
        manager.vectorialSafety.hidden = !this.direct;
        manager.angularXLabel.textContent = this.direct ? 'Proyección matemática X' : 'Componente X ejecutable';
        manager.angularYLabel.textContent = this.direct ? 'Proyección matemática Y' : 'Componente Y ejecutable';
        manager.modeChip.textContent = this.direct ? 'ANGULAR VECTORIAL · EXPERIMENTAL' : 'ANGULAR X→Y · FIN 0°';
        manager.planCanvas.setAttribute('aria-label', this.direct
            ? 'Gráfica angular vectorial con hipotenusas ejecutables y proyecciones matemáticas'
            : 'Gráfica angular con hipotenusas y componentes ortogonales ejecutables');
        manager.renderVectorialSafety();
        this.updatePreview(manager);
    }

    createLogicalStep(manager, length, theta) {
        if (!Number.isFinite(length) || length < 0) {
            throw new Error('La longitud L debe ser un número mayor o igual que cero.');
        }
        if (!Number.isFinite(theta) || theta < -180 || theta > 180) {
            throw new Error('El ángulo θ debe estar entre -180° y 180°.');
        }
        const start = manager.logicalEndpoint();
        const prev = manager.logicalSteps.length ? manager.logicalSteps[manager.logicalSteps.length - 1].absoluteAngleDeg : 0;
        const absoluteAngleDeg = normalizeSignedDegrees(prev + theta);
        const rad = absoluteAngleDeg * Math.PI / 180;
        const dx = cleanRouteComponent(length * Math.cos(rad));
        const dy = cleanRouteComponent(length * Math.sin(rad));
        const end = { x: start.x + dx, y: start.y + dy };
        const id = manager.nextLogicalStepId++;
        const number = manager.logicalSteps.length + 1;
        const execution = [];

        if (length > 0.01 && this.direct) {
            execution.push({
                x0: start.x, y0: start.y, x1: end.x, y1: end.y,
                label: `V${number} · Vector directo`, component: 'vector'
            });
        } else {
            if (Math.abs(dx) > 0.01) {
                execution.push({
                    x0: start.x, y0: start.y, x1: end.x, y1: start.y,
                    label: `V${number} · Componente X`, component: 'x'
                });
            }
            if (Math.abs(dy) > 0.01) {
                execution.push({
                    x0: end.x, y0: start.y, x1: end.x, y1: end.y,
                    label: `V${number} · Componente Y`, component: 'y'
                });
            }
        }

        return {
            id, number, mode: this.key, start, end, dx, dy, length, theta,
            referenceAngleDeg: prev, absoluteAngleDeg, execution,
            description: `V${number}: L=${length.toFixed(2)} cm, θ=${theta.toFixed(2)}°, φ=${absoluteAngleDeg.toFixed(2)}°`
        };
    }

    buildVisualization(manager) {
        const relative = pt => ({ x: pt.x - manager.currentOrigin.x, y: pt.y - manager.currentOrigin.y });
        return {
            label: this.direct ? 'Ruta angular vectorial' : 'Hipotenusas angulares',
            points: [{ x: 0, y: 0 }, ...manager.logicalSteps.map(step => relative(step.end))],
            geometry: manager.logicalSteps.map(step => ({
                number: step.number,
                start: relative(step.start),
                end: relative(step.end),
                corner: relative({ x: step.end.x, y: step.start.y }),
                dx: step.dx,
                dy: step.dy,
                length: step.length,
                theta: step.theta,
                referenceAngleDeg: step.referenceAngleDeg,
                absoluteAngleDeg: step.absoluteAngleDeg,
                direct: this.direct
            }))
        };
    }

    missionPayload(manager) {
        return {
            mode: this.apiMode,
            vectors: manager.logicalSteps.map(step => ({
                length_cm: step.length,
                relative_angle_deg: step.theta
            }))
        };
    }

    updatePreview(manager) {
        const length = Number.parseFloat(manager.input1.value) || 0;
        const theta = Number.parseFloat(manager.input2.value) || 0;
        const prev = manager.logicalSteps.length ? manager.logicalSteps[manager.logicalSteps.length - 1].absoluteAngleDeg : 0;
        const absAngle = normalizeSignedDegrees(prev + theta);
        const rad = absAngle * Math.PI / 180;
        const dx = cleanRouteComponent(length * Math.cos(rad));
        const dy = cleanRouteComponent(length * Math.sin(rad));
        manager.preview.textContent = `V${manager.logicalSteps.length + 1} · L=${length.toFixed(2)} cm · θ=${theta.toFixed(2)}° · φ=${absAngle.toFixed(2)}° · ΔX=${dx.toFixed(2)} cm · ΔY=${dy.toFixed(2)} cm`;
    }
}

/**
 * Modo angular por descomposición ortogonal X -> Y.
 */
class AngularDecompositionRouteMode extends AngularRouteMode {
    constructor() {
        super('polar', 'angular_decomposition', false);
    }
}

/**
 * Modo angular vectorial directo (experimental).
 */
class AngularVectorialRouteMode extends AngularRouteMode {
    constructor() {
        super('vectorial', 'angular_vectorial', true);
    }

    checkVectorialReadiness(telemetry, enabled) {
        if (!enabled) return { ready: false, reason: 'La bandera experimental está deshabilitada.' };
        if (!telemetry) return { ready: false, reason: 'Esperando telemetría del robot.' };
        const h = telemetry.encoder_health || {};
        const bad = ['fl', 'fr', 'bl', 'br'].filter(w => String(h[w] || 'unknown').toLowerCase() !== 'healthy');
        if (bad.length) return { ready: false, reason: `Encoders no saludables: ${bad.map(w => w.toUpperCase()).join(', ')}.` };
        if (telemetry.degraded_mode) return { ready: false, reason: 'El robot reporta modo degradado.' };
        const mpu = telemetry.mpu || {};
        if (mpu.present !== true || mpu.calibrated !== true || mpu.stale !== false) {
            return { ready: false, reason: 'La MPU debe estar presente, calibrada y vigente.' };
        }
        if (String(telemetry.state || '').toLowerCase() !== 'listo') {
            return { ready: false, reason: `El robot debe estar LISTO; estado: ${telemetry.state || 'desconocido'}.` };
        }
        return { ready: true, reason: 'Apto: cuatro encoders healthy, MPU vigente y robot LISTO.' };
    }
}

/**
 * Registro de modos de planeación disponibles.
 */
const ROUTE_PLANNING_MODES = {
    rect: new RectangularRouteMode(),
    polar: new AngularDecompositionRouteMode(),
    vectorial: new AngularVectorialRouteMode()
};

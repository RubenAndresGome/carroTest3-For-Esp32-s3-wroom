// @ts-check
/** @typedef {import('@frontend/protocol').Telemetry} TelemetryPayload */
/** @typedef {import('@frontend/protocol').CommandName} CommandName */

/* ==============================================
   ROUTE VECTOR STACK & GEOMETRY (Vectorial Planning Layer)
   ============================================== */

/**
 * Normaliza un ángulo a [-180, 180) grados.
 * @param {number|string} value - Ángulo en grados.
 * @returns {number} Ángulo normalizado en [-180, 180).
 */
const normalizeSignedDeg = value => {
    const val = Number(value);
    if (!Number.isFinite(val)) return 0;
    const n = ((val + 180) % 360 + 360) % 360 - 180;
    return Object.is(n, -0) ? 0 : n;
};

/**
 * Normaliza un ángulo a [0, 360) grados.
 * @param {number|string} value - Ángulo en grados.
 * @returns {number} Ángulo normalizado en [0, 360).
 */
const normalize360Deg = value => {
    const val = Number(value);
    if (!Number.isFinite(val)) return 0;
    return ((val % 360) + 360) % 360;
};

/**
 * Limpia residuos numéricos de coma flotante.
 * @param {number} value
 * @returns {number}
 */
const cleanFloat = value => Math.abs(value) < 1e-9 ? 0 : value;

/**
 * Nodo individual de la pila vectorial (P1, P2, ... Pn).
 * Almacena tanto la entrada del usuario (θReal relativa al vector anterior)
 * como todos los ángulos complementarios calculados internamente (θChart, θΔX, θΔY, Rumbo).
 */
class VectorNode {
    /**
     * @param {Object} params
     * @param {number} params.index - Índice secuencial 1..N.
     * @param {number} params.id - Identificador lógico único de paso.
     * @param {number} params.length - Longitud L en cm.
     * @param {number} params.thetaRealDeg - Ángulo θReal relativo a la prolongación del vector anterior (o +X para P1).
     * @param {number} params.prevChartAngleDeg - Ángulo θChart del vector previo respecto a la paralela +X.
     * @param {{ x: number, y: number }} [params.start] - Vértice inicial en cm.
     */
    constructor({ index, id, length, thetaRealDeg, prevChartAngleDeg, start }) {
        this.index = index;
        this.number = index;
        this.id = id;
        this.length = length;
        this.thetaRealDeg = normalizeSignedDeg(thetaRealDeg);
        this.prevChartAngleDeg = normalizeSignedDeg(prevChartAngleDeg);

        // θChart = normalize(prevChart + θReal) medido desde la paralela a X+ en sentido antihorario
        this.thetaChartDeg = normalizeSignedDeg(this.prevChartAngleDeg + this.thetaRealDeg);
        this.thetaDeltaXDeg = this.thetaChartDeg;
        this.thetaDeltaYDeg = normalizeSignedDeg(90 - this.thetaChartDeg);
        this.headingDeg = normalize360Deg(90 - this.thetaChartDeg);

        const rad = this.thetaChartDeg * Math.PI / 180;
        this.dx = cleanFloat(length * Math.cos(rad));
        this.dy = cleanFloat(length * Math.sin(rad));

        const originStart = start || { x: 0, y: 0 };
        this.start = { x: originStart.x, y: originStart.y };
        this.end = { x: cleanFloat(originStart.x + this.dx), y: cleanFloat(originStart.y + this.dy) };
        this.mode = 'vectorial';

        this.description = `P${this.index}: L=${this.length.toFixed(1)} cm, θReal=${this.thetaRealDeg.toFixed(1)}°, θChart=${this.thetaChartDeg.toFixed(1)}°, Rumbo=${this.headingDeg.toFixed(1)}°`;

        this.execution = length > 0.01 ? [{
            x0: this.start.x, y0: this.start.y, x1: this.end.x, y1: this.end.y,
            label: `P${this.index} · Vector directo`, component: 'vector'
        }] : [];
    }
}

/**
 * Gestor y orquestador de la Pila Vectorial anidada (Vector Route Stack).
 * Permite encadenar múltiples pasos vectoriales (P1 → P2 → ... → Pn)
 * manteniendo la continuidad de vértices y la trigonometría angular.
 */
class VectorRouteStack {
    /**
     * @param {any[]} [nodes]
     */
    constructor(nodes = []) {
        /** @type {VectorNode[]} */
        this.nodes = [...nodes];
    }

    clear() {
        this.nodes = [];
    }

    /**
     * @param {any[]} [logicalSteps]
     * @returns {VectorNode[]}
     */
    getNodes(logicalSteps) {
        if (Array.isArray(logicalSteps)) {
            return /** @type {VectorNode[]} */ (logicalSteps.filter(s => s && s.mode === 'vectorial'));
        }
        return this.nodes;
    }

    /**
     * @param {any[]} [logicalSteps]
     * @returns {VectorNode | null}
     */
    lastNode(logicalSteps) {
        // Si se pasa la lista lógica, es la única fuente de verdad (aunque esté vacía).
        // Caer en this.nodes tras deshacer el último paso conservaba un rumbo obsoleto (p. ej. 45° en vez de +X).
        const list = Array.isArray(logicalSteps) ? logicalSteps : this.nodes;
        if (!list || list.length === 0) return null;
        for (let i = list.length - 1; i >= 0; i--) {
            const node = list[i];
            if (node && node.mode === 'vectorial') return node;
        }
        return null;
    }

    /**
     * Crea un nuevo nodo vectorial enlazado al final del vector anterior.
     * @param {Object} params
     * @param {number} params.index
     * @param {number} params.id
     * @param {number} params.length
     * @param {number} params.thetaRealDeg
     * @param {number} params.prevChartAngleDeg
     * @param {{ x: number, y: number }} params.start
     * @returns {VectorNode}
     */
    createNode(params) {
        const node = new VectorNode(params);
        this.nodes.push(node);
        return node;
    }

    /**
     * Configura el formulario para el modo vectorial interactivo.
     * @param {any} manager
     */
    configureForm(manager) {
        manager.lbl1.textContent = 'Longitud L (cm):';
        const count = manager.logicalSteps ? manager.logicalSteps.length : 0;
        if (count === 0) {
            manager.lbl2.textContent = 'Ángulo θ1 respecto a +X (°):';
        } else {
            manager.lbl2.textContent = `Ángulo θReal relativo a P${count} (°):`;
        }
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
        manager.vectorialSafety.hidden = false;
        manager.angularXLabel.textContent = 'Proyección matemática ΔX';
        manager.angularYLabel.textContent = 'Proyección matemática ΔY';
        manager.modeChip.textContent = 'PILA VECTORIAL · EXPERIMENTAL';
        manager.planCanvas.setAttribute(
            'aria-label',
            'Gráfica de pila vectorial con paralelogramos virtuales, paralelas X+ y rumbos de cuerpo'
        );
        manager.renderVectorialSafety();
        const previewInfo = this.updatePreview(manager.input1.value, manager.input2.value, manager.logicalSteps);
        manager.preview.textContent = previewInfo.text;
    }

    /**
     * Calcula la previsualización en vivo para la entrada de texto del operador.
     * @param {string|number} lengthVal
     * @param {string|number} thetaVal
     * @param {any[]} logicalSteps
     * @returns {{ text: string, length: number, thetaReal: number, thetaChart: number, thetaDeltaY: number, heading: number, dx: number, dy: number }}
     */
    updatePreview(lengthVal, thetaVal, logicalSteps) {
        const length = Number.parseFloat(String(lengthVal)) || 0;
        const thetaReal = Number.parseFloat(String(thetaVal)) || 0;
        const prevNode = this.lastNode(logicalSteps);
        const prevChart = prevNode ? prevNode.thetaChartDeg : 0;
        const thetaChart = normalizeSignedDeg(prevChart + thetaReal);
        const rad = thetaChart * Math.PI / 180;
        const dx = cleanFloat(length * Math.cos(rad));
        const dy = cleanFloat(length * Math.sin(rad));
        const thetaDeltaY = normalizeSignedDeg(90 - thetaChart);
        const heading = normalize360Deg(90 - thetaChart);
        const stepNum = (logicalSteps ? logicalSteps.length : 0) + 1;

        const text = `P${stepNum} · L=${length.toFixed(2)} cm · θReal=${thetaReal.toFixed(2)}° · θChart=${thetaChart.toFixed(2)}° · θΔY=${thetaDeltaY.toFixed(2)}° · Rumbo=${heading.toFixed(2)}° · ΔX=${dx.toFixed(2)} cm · ΔY=${dy.toFixed(2)} cm`;
        return { text, length, thetaReal, thetaChart, thetaDeltaY, heading, dx, dy };
    }

    /**
     * Construye la estructura geométrica para visualización en RouteCharts.
     * @param {any[]} logicalSteps
     * @param {{ x: number, y: number }} origin
     */
    toVisualization(logicalSteps, origin) {
        const rel = pt => ({ x: pt.x - origin.x, y: pt.y - origin.y });
        return {
            label: 'Ruta angular vectorial',
            points: [{ x: 0, y: 0 }, ...logicalSteps.map(step => rel(step.end))],
            geometry: logicalSteps.map(step => ({
                number: step.number || step.index,
                start: rel(step.start),
                end: rel(step.end),
                corner: rel({ x: step.end.x, y: step.start.y }),
                dx: step.dx,
                dy: step.dy,
                length: step.length,
                thetaRealDeg: step.thetaRealDeg !== undefined ? step.thetaRealDeg : step.theta,
                thetaChartDeg: step.thetaChartDeg !== undefined ? step.thetaChartDeg : step.absoluteAngleDeg,
                thetaDeltaXDeg: step.thetaDeltaXDeg !== undefined ? step.thetaDeltaXDeg : step.absoluteAngleDeg,
                thetaDeltaYDeg: step.thetaDeltaYDeg !== undefined ? step.thetaDeltaYDeg : normalizeSignedDeg(90 - step.absoluteAngleDeg),
                headingDeg: step.headingDeg !== undefined ? step.headingDeg : normalize360Deg(90 - step.absoluteAngleDeg),
                referenceAngleDeg: step.prevChartAngleDeg !== undefined ? step.prevChartAngleDeg : (step.referenceAngleDeg || 0),
                absoluteAngleDeg: step.thetaChartDeg !== undefined ? step.thetaChartDeg : (step.absoluteAngleDeg || 0),
                direct: true
            }))
        };
    }

    /**
     * Genera el payload atómico para la API de misiones en Python.
     * @param {any[]} logicalSteps
     */
    missionPayload(logicalSteps) {
        return {
            mode: 'angular_vectorial',
            vectors: logicalSteps.map(step => ({
                length_cm: step.length,
                relative_angle_deg: step.thetaRealDeg !== undefined ? step.thetaRealDeg : step.theta
            }))
        };
    }
}

// Registro global para compatibilidad de scripts y testing
if (typeof window !== 'undefined') {
    // @ts-ignore
    window.VectorNode = VectorNode;
    // @ts-ignore
    window.VectorRouteStack = VectorRouteStack;
    // @ts-ignore
    window.RouteVectorStack = { VectorNode, VectorRouteStack, normalizeSignedDeg, normalize360Deg, cleanFloat };
}

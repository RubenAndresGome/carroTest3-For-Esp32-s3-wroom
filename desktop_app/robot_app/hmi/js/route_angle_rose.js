// @ts-check

/* ==============================================
   ROSA DE ÁNGULOS (modo vectorial)
   Muestra θReal relativo a la prolongación del último paso y su valor
   absoluto resultante (θChart). El botón 0° siempre queda sobre la referencia.
   ============================================== */
const RouteAngleRose = {
    CENTER: 130,
    RADIUS: 96,
    BUTTON_HALF: 28,
    /** @type {HTMLElement | null} */
    root: null,

    /** @param {any} manager */
    init(manager) {
        this.root = document.getElementById('route-angle-rose');
        if (!this.root) return;
        this.root.querySelectorAll('.rose-btn').forEach(elem => {
            const btn = /** @type {HTMLButtonElement} */ (elem);
            btn.addEventListener('click', () => {
                if (manager.isRunning) return;
                manager.input2.value = btn.dataset.rel || '0';
                manager.input2.dispatchEvent(new Event('input'));
                manager.input1.focus();
            });
        });
        this.render(manager);
    },

    /**
     * Ángulo absoluto de referencia: θChart del último paso vectorial vigente, o +X (0°) sin pasos.
     * @param {any} manager
     * @returns {{ refDeg: number, refIndex: number }}
     */
    reference(manager) {
        const steps = Array.isArray(manager.logicalSteps) ? manager.logicalSteps : [];
        for (let i = steps.length - 1; i >= 0; i--) {
            if (steps[i] && steps[i].mode === 'vectorial') return { refDeg: steps[i].thetaChartDeg, refIndex: i + 1 };
        }
        return { refDeg: 0, refIndex: 0 };
    },

    /** Normaliza a [-180, 180), igual que la pila vectorial. @param {number} deg */
    norm(deg) {
        const n = ((deg + 180) % 360 + 360) % 360 - 180;
        if (n === -180) return 180; // se muestra 180° en vez de −180° para la reversa
        return Object.is(n, -0) ? 0 : n;
    },

    /** @param {number} deg */
    fmt(deg) {
        const r = Math.round(deg * 10) / 10;
        return `${r > 0 ? '+' : ''}${r}°`;
    },

    /** @param {any} manager */
    render(manager) {
        if (!this.root) return;
        const isVectorial = manager.activeMode && manager.activeMode().key === 'vectorial';
        this.root.hidden = !isVectorial;
        if (!isVectorial) return;

        const { refDeg, refIndex } = this.reference(manager);
        const rawRel = Number.parseFloat(manager.input2?.value);
        const rel = Number.isFinite(rawRel) ? rawRel : 0;
        const newDeg = this.norm(refDeg + rel);

        document.getElementById('route-rose-ref')?.setAttribute('transform', `rotate(${-refDeg} ${this.CENTER} ${this.CENTER})`);
        document.getElementById('route-rose-new')?.setAttribute('transform', `rotate(${-newDeg} ${this.CENTER} ${this.CENTER})`);

        this.root.querySelectorAll('.rose-btn').forEach(elem => {
            const btn = /** @type {HTMLButtonElement} */ (elem);
            const slot = Number(btn.dataset.rel || 0);
            const absDeg = this.norm(refDeg + slot);
            const rad = absDeg * Math.PI / 180;
            btn.style.left = `${this.CENTER + this.RADIUS * Math.cos(rad) - this.BUTTON_HALF}px`;
            btn.style.top = `${this.CENTER - this.RADIUS * Math.sin(rad) - this.BUTTON_HALF}px`;
            const absEl = btn.querySelector('.rose-abs');
            if (absEl) absEl.textContent = `→ ${this.fmt(absDeg)}`;
            const active = Number.isFinite(rawRel) && Math.abs(rawRel - slot) < 0.05;
            btn.classList.toggle('active', active);
            btn.setAttribute('aria-pressed', String(active));
            btn.setAttribute('aria-label', `θReal ${this.fmt(slot)}, queda en ${this.fmt(absDeg)} absoluto`);
            btn.disabled = Boolean(manager.isRunning);
        });

        const refText = document.getElementById('route-rose-ref-text');
        if (refText) refText.textContent = refIndex ? `prolongación de P${refIndex} (${this.fmt(refDeg)} abs.)` : '+X (0°), sin pasos';
        const newText = document.getElementById('route-rose-new-text');
        if (newText) newText.textContent = `${this.fmt(rel)} → ${this.fmt(newDeg)} abs.`;
    }
};

if (typeof window !== 'undefined') {
    // @ts-ignore
    window.RouteAngleRose = RouteAngleRose;
}

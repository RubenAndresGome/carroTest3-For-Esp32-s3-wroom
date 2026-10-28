// @ts-check
/* ==============================================
   OPERATOR FEEDBACK (Confirmaciones, alertas en línea y estado E-STOP)
   ============================================== */
const OperatorFeedback = {
    /** @param {string} slotId @param {string} message @param {'error'|'warn'|'info'|'ok'} [level] */
    show(slotId, message, level = 'error') {
        const slot = document.getElementById(slotId)
            || (slotId === 'test-feedback-banner' ? document.getElementById('testing-cmd-alert') : null);
        if (!slot) return;
        slot.textContent = message;          // nunca innerHTML (CSP / XSS)
        slot.dataset.level = level;
        slot.hidden = false;
    },
    /** @param {string} slotId @param {string} message @param {'error'|'warn'|'info'|'ok'} [level] */
    alert(slotId, message, level = 'error') {
        this.show(slotId, message, level);
    },
    /** @param {string} slotId */
    clear(slotId) {
        const slot = document.getElementById(slotId)
            || (slotId === 'test-feedback-banner' ? document.getElementById('testing-cmd-alert') : null);
        if (slot) { slot.textContent = ''; slot.hidden = true; }
    },
    /** Único punto de confirmación de movimiento; hoy usa confirm() (onJsConfirm en Android). */
    /** @param {string} message @returns {boolean} */
    confirmMotion(message) { return window.confirm(message); },
    /** @param {HTMLInputElement | null} input @returns {number | null} */
    readBoundedNumber(input) {
        if (!input) return null;
        const raw = input.value.trim(), value = Number(raw);
        const min = (input.min === '' || input.min == null) ? -Infinity : Number(input.min);
        const max = (input.max === '' || input.max == null) ? Infinity : Number(input.max);
        const ok = raw !== '' && Number.isFinite(value) && value >= min && value <= max;
        input.setAttribute('aria-invalid', ok ? 'false' : 'true');
        return ok ? value : null;
    },
    /** @param {'idle'|'sent'|'confirmed'|'robot'|'failed'} stage @param {string} [detail] */
    estopStatus(stage, detail = '') {
        const node = document.getElementById('estop-status');
        if (!node) return;
        /** @type {Record<string, string>} */
        const text = {
            idle: '',
            sent: 'E-STOP enviado…',
            confirmed: 'E-STOP aceptado por el backend · esperando estado ESTOP del robot',
            robot: 'Robot en ESTOP · motores detenidos · rearme requerido',
            failed: `E-STOP NO CONFIRMADO (${detail || 'sin respuesta'}). Pulsa de nuevo o corta la alimentación.`
        };
        node.textContent = text[stage] || '';
        node.dataset.stage = stage;
        node.hidden = stage === 'idle';
    }
};

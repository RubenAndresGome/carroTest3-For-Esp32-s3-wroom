// @ts-check
/** @typedef {import('@frontend/protocol').Telemetry} TelemetryPayload */
/** @typedef {import('@frontend/protocol').CommandName} CommandName */

/* ==============================================
   TOUCH / JOYSTICK & SUBMODE CONTROLLER
   ============================================== */
const Touch = {
    wrap: null, svg: null, thumb: null, dirLine: null, arc: null,
    cx: 150, cy: 150, maxR: 108, deadR: 18,
    _sendTimer: 0, _lastPwmL: 0, _lastPwmR: 0, _leaseTimer: null, _begun: false,
    _beginPromise: null,

    _bindInputEvents() {
        const onUp = (/** @type {PointerEvent} */ e) => {
            if (state.joy.active) { try { this.wrap?.releasePointerCapture(e.pointerId); } catch (_) {} this.end(); }
        };
        if (window.PointerEvent) {
            this.wrap?.addEventListener('pointerdown', (/** @type {PointerEvent} */ e) => {
                this.start(e); try { this.wrap?.setPointerCapture(e.pointerId); } catch (_) {}
            });
            window.addEventListener('pointermove', e => { if (state.joy.active) this.move(e); });
            window.addEventListener('pointerup', onUp); window.addEventListener('pointercancel', onUp);
        } else {
            this.wrap?.addEventListener('touchstart', e => this.start(e), { passive: false });
            window.addEventListener('touchmove', e => { if (state.joy.active) this.move(e); }, { passive: false });
            ['touchend', 'touchcancel', 'mouseup'].forEach(evt => window.addEventListener(evt, () => { if (state.joy.active) this.end(); }));
            this.wrap?.addEventListener('mousedown', e => this.start(e));
            window.addEventListener('mousemove', e => { if (state.joy.active) this.move(e); });
        }
    },

    init() {
        this.wrap = document.getElementById('joystick-wrapper');
        this.svg = document.getElementById('joystick-svg');
        this.thumb = document.getElementById('joy-thumb');
        this.dirLine = document.getElementById('joy-dir-line');
        this.arc = document.getElementById('joy-speed-arc');
        this._bindInputEvents();
        document.getElementById('touch-submode-free')?.addEventListener('click', () => this.switchSubmode('free'));
        document.getElementById('touch-submode-routine')?.addEventListener('click', () => this.switchSubmode('routine'));
        this.setCapability(false);
    },

    /** @param {'free' | 'routine'} submode */
    switchSubmode(submode) {
        const isFree = submode === 'free';
        if (!isFree && state.joy.active) this.end();
        const setBtn = (id, act) => { const b = document.getElementById(id); b?.classList.toggle('active', act); b?.setAttribute('aria-checked', String(act)); };
        setBtn('touch-submode-free', isFree); setBtn('touch-submode-routine', !isFree);
        const setPanel = (id, act) => { const p = document.getElementById(id); if (p) { p.classList.toggle('active', act); act ? p.removeAttribute('hidden') : p.setAttribute('hidden', ''); } };
        setPanel('touch-free-panel', isFree); setPanel('touch-routine-panel', !isFree);
        if (!isFree && typeof TouchRoutine !== 'undefined') TouchRoutine.init();
    },

    setCapability(enabled) {
        const tab = document.getElementById('tab-control-touch'), help = document.getElementById('touch-unavailable-help'), status = document.getElementById('touch-capability');
        if (!tab) return;
        tab.setAttribute('aria-disabled', String(!enabled)); tab.classList.toggle('unavailable', !enabled);
        tab.innerHTML = `<svg class="ui-icon" aria-hidden="true" viewBox="0 0 24 24"><rect x="7" y="2" width="10" height="20" rx="2"/><path d="M10 18h4"/>${enabled ? '' : '<path d="M4 4l16 16"/>'}</svg>${enabled ? 'Control Touch' : 'Control Touch (no disponible)'}`;
        tab.title = enabled ? 'Control manual habilitado por manual_drive_v1.' : 'No disponible: la telemetría no anuncia manual_drive_v1.';
        if (help) help.textContent = enabled ? 'Control Touch habilitado por manual_drive_v1.' : 'Control Touch deshabilitado: requiere manual_drive_v1.';
        if (status) { status.textContent = enabled ? 'manual_drive_v1 anunciado: control Touch disponible.' : 'Esperando telemetría: Touch requiere manual_drive_v1.'; status.classList.toggle('ready', enabled); status.classList.toggle('blocked', !enabled); }
        if (!enabled && state.joy.active) this.end();
    },

    _pos(e) {
        const t = (e.touches && e.touches[0]) || (e.changedTouches && e.changedTouches[0]);
        const cx = t ? t.clientX : e.clientX, cy = t ? t.clientY : e.clientY;
        const r = this.svg?.getBoundingClientRect() || { left: 0, top: 0, width: 300 };
        const s = r.width > 0 ? 300 / r.width : 1;
        return { x: (cx - r.left) * s, y: (cy - r.top) * s };
    },

    _sendManual(pwmL, pwmR, dirL, dirR, force = false) {
        const l = dirL === 'rev' ? -pwmL : dirL === 'stop' ? 0 : pwmL;
        const r = dirR === 'rev' ? -pwmR : dirR === 'stop' ? 0 : pwmR;
        this._lastPwmL = l; this._lastPwmR = r;
        const now = Date.now();
        if (!force && now - this._sendTimer < 80) return;
        this._sendTimer = now;
        if (!this._begun) return;
        WSControl.manual('drive', { left: l, right: r }).catch(err => {
            if (err.status === 409 && state.joy.active) this.end();
            LogSystem.add('ERROR', `Control Touch: ${err.message}`);
        });
    },

    _sendCurrent(force = false) {
        const dL = this._lastPwmL < 0 ? 'rev' : this._lastPwmL > 0 ? 'fwd' : 'stop';
        const dR = this._lastPwmR < 0 ? 'rev' : this._lastPwmR > 0 ? 'fwd' : 'stop';
        this._sendManual(Math.abs(this._lastPwmL), Math.abs(this._lastPwmR), dL, dR, force);
    },

    start(e) {
        if (e.cancelable) e.preventDefault();
        if (state.joy.active) this.end();
        if (!state.manualDriveSupported || !Calibration.calibrated || !state.connected || state.activeTab !== 'touch') {
            return LogSystem.add('WARN', 'Control Touch requiere la capacidad manual_drive_v1 anunciada por telemetría.');
        }
        if (!['IDLE', 'MANUAL'].includes(state.estado)) return LogSystem.add('WARN', `Joystick bloqueado en ${state.estado}.`);
        if (e.touches && e.touches.length > 1) return;
        state.joy.active = true; this.wrap?.classList.add('active'); document.body.classList.add('is-touch-interacting');
        this.move(e);
        this._beginPromise = WSControl.manual('start', { mode: 'free' }).then(res => {
            if (state.joy.active) {
                this._begun = true; this._sendCurrent(true); clearInterval(this._leaseTimer);
                this._leaseTimer = setInterval(() => { if (state.joy.active) this._sendCurrent(true); }, 100);
            }
            return res;
        }).catch(err => {
            state.joy.active = false; this._begun = false; this._resetVis();
            document.body.classList.remove('is-touch-interacting');
            clearInterval(this._leaseTimer); this._leaseTimer = null;
            LogSystem.add('ERROR', `No se pudo iniciar Control Touch: ${err.message}`);
            return null;
        });
    },

    move(e) {
        if (!state.joy.active) return;
        if (e.cancelable) e.preventDefault();
        if (e.touches && e.touches.length > 1) { this.end(); return; }
        const p = this._pos(e); let dx = p.x - this.cx, dy = p.y - this.cy, dist = Math.sqrt(dx * dx + dy * dy);
        if (dist > this.maxR) { dx = (dx / dist) * this.maxR; dy = (dy / dist) * this.maxR; }
        const tx = String(this.cx + dx), ty = String(this.cy + dy);
        this.thumb?.setAttribute('cx', tx); this.thumb?.setAttribute('cy', ty);
        this.wrap?.querySelector('.joy-thumb-outer')?.setAttribute('cx', tx);
        this.wrap?.querySelector('.joy-thumb-outer')?.setAttribute('cy', ty);
        this.dirLine?.setAttribute('x2', tx); this.dirLine?.setAttribute('y2', ty);
        const cd = Math.min(dist, this.maxR), nx = cd > 0 ? dx / this.maxR : 0, ny = cd > 0 ? dy / this.maxR : 0;
        let mag = cd < this.deadR ? 0 : cd / this.maxR;
        const ang = Math.atan2(dx, -dy) * 180 / Math.PI, fwd = -ny, turn = nx;
        let lM = fwd + turn, rM = fwd - turn, ma = Math.max(Math.abs(lM), Math.abs(rM), 0.001);
        if (ma > 1) { lM /= ma; rM /= ma; }
        const rL = Math.round(lM * mag * 230), rR = Math.round(rM * mag * 230);
        const dL = rL > 2 ? 'fwd' : rL < -2 ? 'rev' : 'stop', dR = rR > 2 ? 'fwd' : rR < -2 ? 'rev' : 'stop';
        const pL = Math.min(Math.abs(rL), 230), pR = Math.min(Math.abs(rR), 230);
        Object.assign(state.joy, { nx, ny, magnitude: mag, angle: ang, pwmL: pL, pwmR: pR, dirL: dL, dirR: dR });
        const circ = 2 * Math.PI * 118;
        this.arc?.setAttribute('stroke-dasharray', `${mag * circ * 0.75} ${circ}`);
        this.arc?.setAttribute('transform', `rotate(${ang - 90} ${this.cx} ${this.cy})`);
        this._bars(pL, pR, dL, dR);
        const spdEl = document.getElementById('joy-speed-val'); if (spdEl) spdEl.textContent = `${Math.round(mag * 100)}%`;
        const dirEl = document.getElementById('joy-dir-val'); if (dirEl) dirEl.textContent = this._dirName(ang, mag);
        this._sendManual(pL, pR, dL, dR);
    },

    end() {
        const debeDetener = state.joy.active || this._leaseTimer !== null;
        state.joy.active = false; this.wrap?.classList.remove('active'); this._resetVis();
        document.body.classList.remove('is-touch-interacting');
        clearInterval(this._leaseTimer); this._leaseTimer = null;
        this._bars(0, 0, 'stop', 'stop');
        const spdEl = document.getElementById('joy-speed-val'); if (spdEl) spdEl.textContent = '0%';
        const dirEl = document.getElementById('joy-dir-val'); if (dirEl) dirEl.textContent = 'EN REPOSO';
        this._lastPwmL = 0; this._lastPwmR = 0; this._begun = false;
        const begin = this._beginPromise; this._beginPromise = null;
        if (debeDetener) {
            WSControl.manual('drive', { left: 0, right: 0 }).catch(() => {});
            Promise.resolve(begin).catch(() => null)
                .then(() => WSControl.manual('stop'))
                .catch(err => LogSystem.add('ERROR', `No se pudo cerrar Control Touch: ${err.message}`));
        }
    },

    _resetVis() {
        const c = String(this.cx);
        ['cx', 'cy'].forEach(a => { this.thumb?.setAttribute(a, c); this.wrap?.querySelector('.joy-thumb-outer')?.setAttribute(a, c); });
        this.dirLine?.setAttribute('x2', c); this.dirLine?.setAttribute('y2', c);
        this.arc?.setAttribute('stroke-dasharray', `0 ${2 * Math.PI * 118}`);
    },

    _bars(pL, pR, dL, dR) {
        const sc = 100 / 230;
        [['left', pL, dL], ['right', pR, dR]].forEach(([side, p, d]) => {
            const fill = document.getElementById(`bar-${side}-fill`);
            if (fill) {
                const h = Number(p) * sc;
                fill.setAttribute('y', String(d === 'fwd' ? 120 - h : 120));
                fill.setAttribute('height', String(d === 'stop' ? 0 : h));
                fill.setAttribute('class', d === 'stop' ? '' : `bar-fill-${d}`);
            }
            const vEl = document.getElementById(`bar-${side}-val`), dEl = document.getElementById(`bar-${side}-dir`);
            if (vEl) vEl.textContent = String(p); if (dEl) dEl.textContent = String(d);
        });
    },

    _dirName(a, m) {
        if (m < 0.05) return 'EN REPOSO';
        const dirs = [
            [-22.5, 22.5, 'AVANCE'], [22.5, 67.5, 'AVANCE + DER'], [67.5, 112.5, 'DERECHA'],
            [112.5, 157.5, 'REVERSA + DER'], [-157.5, -112.5, 'REVERSA + IZQ'], [-112.5, -67.5, 'IZQUIERDA'],
            [-67.5, -22.5, 'AVANCE + IZQ']
        ];
        const match = dirs.find(([min, max]) => a > min && a <= max);
        return String(match ? match[2] : 'REVERSA');
    }
};

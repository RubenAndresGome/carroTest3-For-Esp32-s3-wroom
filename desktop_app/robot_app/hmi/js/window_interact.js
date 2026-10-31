// @ts-check
/** @typedef {import('@frontend/protocol').Telemetry} TelemetryPayload */
/** @typedef {import('@frontend/protocol').CommandName} CommandName */

/* ==============================================
   WINDOW INTERACTION (Pointer Drag, Tactile Resize & Observer)
   ============================================== */

const WindowInteract = {
    _activeDrag: null,
    _activeResize: null,

    /**
     * Inicia el arrastre de una ventana flotante por su encabezado mediante Pointer Events (A1).
     */
    startDrag(e, win, bringToFrontFn, isFloatingMode) {
        if (!isFloatingMode || !win || win.isMaximized) return;
        const target = /** @type {HTMLElement | null} */ (e.target);
        if (target?.closest?.('.window-controls, .win-btn, button')) return;
        if (e.button !== undefined && e.button !== 0) return;

        try { win.headerEl.setPointerCapture(e.pointerId); } catch (_) {}
        bringToFrontFn(win.id);

        win.el.classList.add('is-interacting');
        document.body.classList.add('is-dragging-window');

        this._activeDrag = {
            win,
            pointerId: e.pointerId,
            startX: e.clientX,
            startY: e.clientY,
            winX: win.rect.x,
            winY: win.rect.y
        };

        const onMove = (ev) => this._onDragMove(ev);
        const onUp = (ev) => this._onDragUp(ev, onMove, onUp);

        window.addEventListener('pointermove', onMove);
        window.addEventListener('pointerup', onUp);
        window.addEventListener('pointercancel', onUp);
        win.headerEl.addEventListener('lostpointercapture', onUp);
    },

    _onDragMove(ev) {
        if (!this._activeDrag || ev.pointerId !== this._activeDrag.pointerId) return;
        const dx = ev.clientX - this._activeDrag.startX;
        const dy = ev.clientY - this._activeDrag.startY;
        const win = this._activeDrag.win;
        const w = win.el?.offsetWidth || win.rect.width || 320;
        const maxX = Math.max(0, window.innerWidth - Math.min(w, window.innerWidth));
        const maxY = Math.max(0, window.innerHeight - 48);
        win.rect.x = Math.min(maxX, Math.max(0, this._activeDrag.winX + dx));
        win.rect.y = Math.min(maxY, Math.max(0, this._activeDrag.winY + dy));
        win.el.style.left = `${win.rect.x}px`;
        win.el.style.top = `${win.rect.y}px`;
    },

    _onDragUp(ev, onMove, onUp) {
        if (!this._activeDrag || (ev.pointerId !== undefined && ev.pointerId !== this._activeDrag.pointerId)) return;
        const win = this._activeDrag.win;
        try { win.headerEl.releasePointerCapture(this._activeDrag.pointerId); } catch (_) {}
        win.el.classList.remove('is-interacting');
        document.body.classList.remove('is-dragging-window');

        window.removeEventListener('pointermove', onMove);
        window.removeEventListener('pointerup', onUp);
        window.removeEventListener('pointercancel', onUp);
        win.headerEl.removeEventListener('lostpointercapture', onUp);
        this._activeDrag = null;
    },

    /**
     * Inicia la redimensión con tirador táctil de 28px con Pointer Events.
     */
    startResize(e, win, mode, bringToFrontFn, onResizeEnd) {
        if (mode !== 'floating' && mode !== 'tiling') return;
        if (!win || win.isMaximized) return;

        const handle = win.el.querySelector('.window-resize-handle');
        if (!handle) return;

        e.stopPropagation();
        if (e.cancelable) e.preventDefault();

        try { handle.setPointerCapture(e.pointerId); } catch (_) {}
        win.el.classList.add('is-interacting');
        document.body.classList.add('is-dragging-window');
        bringToFrontFn(win.id);

        this._activeResize = {
            win,
            mode,
            handle,
            pointerId: e.pointerId,
            startX: e.clientX,
            startY: e.clientY,
            startW: win.el.offsetWidth || win.rect.width,
            startH: win.el.offsetHeight || win.rect.height
        };

        const onMove = (ev) => this._onResizeMove(ev);
        const onUp = (ev) => this._onResizeUp(ev, onMove, onUp, onResizeEnd);

        window.addEventListener('pointermove', onMove);
        window.addEventListener('pointerup', onUp);
        window.addEventListener('pointercancel', onUp);
        handle.addEventListener('lostpointercapture', onUp);
    },

    _onResizeMove(ev) {
        if (!this._activeResize || ev.pointerId !== this._activeResize.pointerId) return;
        const dw = ev.clientX - this._activeResize.startX;
        const dh = ev.clientY - this._activeResize.startY;
        const { win, mode, startW, startH } = this._activeResize;

        if (mode === 'tiling') {
            win.rect.height = Math.max(380, startH + dh);
            win.el.style.width = '100%';
            win.el.style.height = `${win.rect.height}px`;
            win.tilingHeight = win.rect.height;
        } else {
            win.rect.width = Math.max(320, startW + dw);
            win.rect.height = Math.max(240, startH + dh);
            win.el.style.width = `${win.rect.width}px`;
            win.el.style.height = `${win.rect.height}px`;
        }
    },

    _onResizeUp(ev, onMove, onUp, onResizeEnd) {
        if (!this._activeResize || (ev.pointerId !== undefined && ev.pointerId !== this._activeResize.pointerId)) return;
        const { win, handle, mode } = this._activeResize;
        try { handle.releasePointerCapture(this._activeResize.pointerId); } catch (_) {}
        win.el.classList.remove('is-interacting');
        document.body.classList.remove('is-dragging-window');

        window.removeEventListener('pointermove', onMove);
        window.removeEventListener('pointerup', onUp);
        window.removeEventListener('pointercancel', onUp);
        handle.removeEventListener('lostpointercapture', onUp);

        if (mode === 'tiling') {
            win.tilingHeight = win.rect.height;
            win.el.style.width = '100%';
        }
        this._activeResize = null;
        if (typeof onResizeEnd === 'function') onResizeEnd();
    },

    /**
     * Instala un ResizeObserver en el elemento para preservar dimensiones nativas de mosaico.
     */
    observeTilingDimensions(win, getModeFn) {
        if (typeof ResizeObserver === 'undefined' || !win?.el) return;
        const ro = new ResizeObserver(() => {
            if (getModeFn() === 'tiling' && win.el) {
                const h = parseFloat(win.el.style.height) || win.el.offsetHeight;
                if (Number.isFinite(h) && h >= 380) {
                    win.tilingHeight = h;
                    win.rect.height = h;
                }
                const w = parseFloat(win.el.style.width);
                if (Number.isFinite(w) && w > 0) win.tilingWidth = w;
            }
        });
        ro.observe(win.el);
        win.resizeObserver = ro;
    }
};

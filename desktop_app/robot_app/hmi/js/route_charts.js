// @ts-check
/** @typedef {import('@frontend/protocol').Telemetry} TelemetryPayload */
/** @typedef {import('@frontend/protocol').CommandName} CommandName */

/* ==============================================
   ROUTE CHARTS & VISUALIZATION (Chart.js & Plugins)
   ============================================== */

const RouteCharts = {
    colors: {},
    /** @type {ResizeObserver | null} */
    _resizeObserver: null,

    /** Opciones base compartidas para lienzos cartesianos Chart.js. */
    chartOptions() {
        const theme = getComputedStyle(document.documentElement);
        const grid = (theme.getPropertyValue('--chart-grid') || theme.getPropertyValue('--border')).trim();
        const text = theme.getPropertyValue('--text-secondary').trim();
        return {
            responsive: true, maintainAspectRatio: false, animation: false, normalized: true,
            plugins: {
                legend: { position: 'top', labels: { color: text, boxWidth: 28, boxHeight: 8, font: { size: 10 } } },
                datalabels: { display: false },
                zoom: { pan: { enabled: true, mode: 'xy' }, zoom: { wheel: { enabled: true }, pinch: { enabled: true }, mode: 'xy' } }
            },
            scales: {
                x: { type: 'linear', min: -50, max: 50, title: { display: true, text: 'X (cm) [← Izq / Der →]', color: text }, ticks: { color: text }, grid: { color: grid } },
                y: { type: 'linear', min: -50, max: 50, title: { display: true, text: 'Y (cm) [↓ Rev / Fte ↑]', color: text }, ticks: { color: text }, grid: { color: grid } }
            }
        };
    },

    _drawBadge(ctx, txt, x, y, col) {
        ctx.save();
        ctx.font = '700 10px monospace';
        const w = ctx.measureText(txt).width + 8;
        ctx.fillStyle = 'rgba(5,9,14,.86)';
        ctx.fillRect(x - w / 2, y - 9, w, 15);
        ctx.fillStyle = col; ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
        ctx.fillText(txt, x, y - 1);
        ctx.restore();
    },

    _drawArrow(ctx, px, from, to, col, txt, side = 1) {
        const a = px(from), b = px(to), ang = Math.atan2(b.y - a.y, b.x - a.x);
        ctx.save();
        ctx.strokeStyle = col; ctx.fillStyle = col; ctx.lineWidth = 1.7; ctx.setLineDash([6, 5]);
        ctx.beginPath(); ctx.moveTo(a.x, a.y); ctx.lineTo(b.x, b.y); ctx.stroke(); ctx.setLineDash([]);
        ctx.beginPath(); ctx.moveTo(b.x, b.y);
        ctx.lineTo(b.x - 8 * Math.cos(ang - 0.45), b.y - 8 * Math.sin(ang - 0.45));
        ctx.lineTo(b.x - 8 * Math.cos(ang + 0.45), b.y - 8 * Math.sin(ang + 0.45));
        ctx.closePath(); ctx.fill(); ctx.restore();
        this._drawBadge(ctx, txt, (a.x + b.x) / 2 - Math.sin(ang) * 11 * side, (a.y + b.y) / 2 + Math.cos(ang) * 11 * side, col);
    },

    _drawStepGeometry(ctx, px, step, stepIndex, geometry, colors) {
        const start = px(step.start), end = px(step.end), isVec = Boolean(step.direct);
        ctx.save();
        ctx.strokeStyle = 'rgba(148, 163, 184, 0.55)'; ctx.lineWidth = 1.2; ctx.setLineDash([3, 3]);
        ctx.beginPath(); ctx.moveTo(start.x, start.y); ctx.lineTo(start.x + 32, start.y); ctx.stroke();
        ctx.restore();
        this._drawBadge(ctx, 'X+', start.x + 40, start.y, 'rgba(148, 163, 184, 0.85)');

        if (Math.abs(step.dx) > 0.01) this._drawArrow(ctx, px, step.start, step.corner, colors.info, `ΔX ${step.dx.toFixed(1)} cm`, -1);
        if (Math.abs(step.dy) > 0.01) this._drawArrow(ctx, px, step.corner, step.end, colors.warn, `ΔY ${step.dy.toFixed(1)} cm`, 1);
        if (isVec && Math.abs(step.dx) > 0.01 && Math.abs(step.dy) > 0.01) {
            const oppCorner = px({ x: step.start.x, y: step.end.y });
            ctx.save();
            ctx.strokeStyle = 'rgba(148, 163, 184, 0.25)'; ctx.lineWidth = 1; ctx.setLineDash([2, 4]);
            ctx.beginPath(); ctx.moveTo(start.x, start.y); ctx.lineTo(oppCorner.x, oppCorner.y); ctx.lineTo(end.x, end.y); ctx.stroke();
            ctx.restore();
        }

        const refDeg = step.referenceAngleDeg || 0;
        if (step.number > 1 || Math.abs(refDeg) > 0.01) {
            const refRad = refDeg * Math.PI / 180;
            ctx.save();
            ctx.strokeStyle = 'rgba(203, 213, 225, 0.45)'; ctx.lineWidth = 1.1; ctx.setLineDash([3, 3]);
            ctx.beginPath(); ctx.moveTo(start.x, start.y);
            ctx.lineTo(start.x + 34 * Math.cos(refRad), start.y - 34 * Math.sin(refRad));
            ctx.stroke(); ctx.restore();
        }

        const coincident = geometry.slice(0, stepIndex).filter(p => Math.hypot(p.start.x - step.start.x, p.start.y - step.start.y) < 0.01).length;
        if (isVec && step.thetaRealDeg !== undefined) {
            const arcRadius = 26 + coincident * 16;
            this._drawArcSegment(ctx, start, arcRadius, refDeg, step.thetaRealDeg, colors.accent);
            const realLabel = step.number === 1 ? `θ1: ${step.thetaRealDeg.toFixed(1)}°` : `θReal: ${step.thetaRealDeg.toFixed(1)}°`;
            this._drawBadge(ctx, realLabel, start.x - 30, start.y - 38 - coincident * 18, colors.accent);
            if (Math.abs(step.thetaChartDeg) > 0.01) {
                this._drawArcSegment(ctx, start, 16, 0, step.thetaChartDeg, colors.info);
                if (step.number > 1 || Math.abs(step.thetaRealDeg - step.thetaChartDeg) > 0.01) {
                    this._drawBadge(ctx, `θChart: ${step.thetaChartDeg.toFixed(1)}°`, start.x + 30, start.y - 18, colors.info);
                }
            }
        } else {
            const arcRadius = 28 + coincident * 30;
            const theta = step.theta !== undefined ? step.theta : (step.thetaRealDeg || 0);
            this._drawArcSegment(ctx, start, arcRadius, refDeg, theta, colors.accent);
            this._drawBadge(ctx, `θ${step.number} ${theta.toFixed(1)}°`, start.x - 34, start.y - 44 - coincident * 18, colors.accent);
        }

        if (step.length > 0.01) {
            const lbl = isVec ? `P${step.number} · ${step.length.toFixed(1)} cm` : `V${step.number} · ${step.length.toFixed(2)} cm`;
            this._drawBadge(ctx, lbl, (start.x + end.x) / 2, (start.y + end.y) / 2 + 12, colors.vector);
        }
        this._drawBadge(ctx, `P${step.number}`, end.x + 8, end.y - 8, colors.vector);
    },

    _drawArcSegment(ctx, center, radius, fromDeg, sweepDeg, col) {
        if (Math.abs(sweepDeg) < 0.1) return;
        const samples = Math.max(2, Math.ceil(Math.abs(sweepDeg) / 8));
        ctx.save();
        ctx.strokeStyle = col; ctx.lineWidth = 1.8; ctx.beginPath();
        for (let idx = 0; idx <= samples; idx++) {
            const a = (fromDeg + sweepDeg * idx / samples) * Math.PI / 180;
            const x = center.x + radius * Math.cos(a), y = center.y - radius * Math.sin(a);
            if (idx === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
        }
        ctx.stroke(); ctx.restore();
    },

    createAngularOverlayPlugin(colors) {
        return {
            id: 'routeAngularOverlay',
            afterDatasetsDraw: chart => {
                const geometry = chart.$angularGeometry;
                if (!geometry?.length) return;
                const ctx = chart.ctx, xScale = chart.scales.x, yScale = chart.scales.y;
                const px = pt => ({ x: xScale.getPixelForValue(pt.x), y: yScale.getPixelForValue(pt.y) });
                ctx.save();
                const area = chart.chartArea;
                ctx.beginPath(); ctx.rect(area.left, area.top, area.width, area.height); ctx.clip();
                geometry.forEach((step, stepIndex) => this._drawStepGeometry(ctx, px, step, stepIndex, geometry, colors));
                ctx.restore();
            }
        };
    },

    createTolerancePlugin() {
        return {
            id: 'routeEndpointTolerance',
            afterDatasetsDraw(chart) {
                const target = chart.$endpointTolerance;
                if (!target) return;
                const x = chart.scales.x.getPixelForValue(target.x), y = chart.scales.y.getPixelForValue(target.y);
                const rx = Math.abs(chart.scales.x.getPixelForValue(target.x + 5) - x);
                const ry = Math.abs(chart.scales.y.getPixelForValue(target.y + 5) - y);
                const ctx = chart.ctx;
                ctx.save();
                ctx.strokeStyle = 'rgba(245,158,11,.9)'; ctx.lineWidth = 1.5; ctx.setLineDash([5, 3]);
                ctx.beginPath(); ctx.ellipse(x, y, rx, ry, 0, 0, Math.PI * 2); ctx.stroke();
                ctx.restore();
            }
        };
    },

    _initPlanChart(manager, angularPlugin) {
        return new Chart(manager.planCanvas.getContext('2d'), {
            type: 'scatter',
            data: {
                datasets: [
                    {
                        label: 'Ruta planificada', data: [{ x: 0, y: 0 }], showLine: true,
                        borderColor: this.colors.info, backgroundColor: this.colors.info, borderWidth: 2, tension: 0,
                        pointRadius: ctx => ctx.dataIndex === 0 ? 0 : 4,
                        pointBackgroundColor: ctx => {
                            const i = ctx.dataIndex - 1;
                            return i < manager.currentStepIndex ? this.colors.accent
                                : (i === manager.currentStepIndex && manager.isRunning ? this.colors.accent : this.colors.info);
                        }
                    },
                    { label: 'Origen', data: [{ x: 0, y: 0 }], pointRadius: 5, pointBackgroundColor: this.colors.reverse, pointBorderColor: this.colors.reverse },
                    { label: 'Ruta Fork [B]', data: [], showLine: true, borderColor: '#ff007f', backgroundColor: '#ff007f', borderDash: [6, 4], borderWidth: 2, pointRadius: 4, pointBackgroundColor: '#ff007f' },
                    {
                        label: 'Simulación Ockham', data: [], showLine: true,
                        borderColor: '#f59e0b', backgroundColor: '#f59e0b', borderDash: [5, 4], borderWidth: 2,
                        pointRadius: ctx => (ctx.dataIndex > 0 && manager.isOckhamRunning && ctx.dataIndex - 1 === manager.currentStepIndex) ? 6 : (ctx.dataIndex === 0 ? 3 : 4),
                        pointBackgroundColor: ctx => ctx.dataIndex === 0 ? '#f59e0b' : (manager.isOckhamRunning ? (ctx.dataIndex - 1 < manager.currentStepIndex ? '#10b981' : (ctx.dataIndex - 1 === manager.currentStepIndex ? '#00e5ff' : '#f59e0b')) : '#f59e0b'),
                        pointBorderColor: ctx => ctx.dataIndex === 0 ? '#f59e0b' : (manager.isOckhamRunning ? (ctx.dataIndex - 1 < manager.currentStepIndex ? '#10b981' : (ctx.dataIndex - 1 === manager.currentStepIndex ? '#00e5ff' : '#f59e0b')) : '#f59e0b')
                    }
                ]
            },
            options: this.chartOptions(),
            plugins: [angularPlugin]
        });
    },

    _initTelemChart(manager, tolerancePlugin) {
        const telemCanvas = /** @type {HTMLCanvasElement} */ (document.getElementById('route-telem-chart'));
        return new Chart(telemCanvas.getContext('2d'), {
            type: 'scatter',
            data: {
                datasets: [
                    { label: 'Traza medida', data: [], showLine: true, borderColor: '#00e5ff', backgroundColor: '#00e5ff', borderWidth: 2, tension: 0.2, pointRadius: 0 },
                    { label: 'Posición actual', data: [{ x: 0, y: 0 }], pointRadius: 6, pointBackgroundColor: this.colors.accent, pointBorderColor: this.colors.accent },
                    { label: 'Orientación', data: [{ x: 0, y: 0 }, { x: 0, y: 8 }], showLine: true, borderColor: this.colors.accent, borderWidth: 2, pointRadius: 0 },
                    { label: 'Origen', data: [{ x: 0, y: 0 }], pointRadius: 4, pointBackgroundColor: this.colors.reverse, pointBorderColor: this.colors.reverse },
                    { label: 'Ruta planificada', data: [], showLine: true, borderColor: this.colors.muted, borderDash: [6, 4], borderWidth: 1.5, pointRadius: 0 },
                    { label: 'Tolerancia final (±5 cm)', data: [], pointRadius: 4, pointBackgroundColor: '#f59e0b', pointBorderColor: '#f59e0b' }
                ]
            },
            options: this.chartOptions(),
            plugins: [tolerancePlugin]
        });
    },

    /** Inicializa los lienzos Chart.js del plan teórico, telemetría y su ResizeObserver. */
    init(manager) {
        const theme = getComputedStyle(document.documentElement);
        this.colors = {
            info: theme.getPropertyValue('--info').trim(),
            accent: theme.getPropertyValue('--accent').trim(),
            reverse: theme.getPropertyValue('--reverse').trim(),
            muted: theme.getPropertyValue('--text-muted').trim(),
            warn: theme.getPropertyValue('--warn').trim(),
            vector: '#c56cff'
        };
        const angularPlugin = this.createAngularOverlayPlugin(this.colors);
        const tolerancePlugin = this.createTolerancePlugin();
        manager.planChart = this._initPlanChart(manager, angularPlugin);
        manager.telemChart = this._initTelemChart(manager, tolerancePlugin);

        if (typeof ResizeObserver !== 'undefined' && !this._resizeObserver) {
            this._resizeObserver = new ResizeObserver(() => this.resizeCharts(manager));
            document.querySelectorAll('.route-map-container').forEach(el => this._resizeObserver?.observe(el));
        }
    },

    drawPlan(manager) {
        if (!manager.planChart) return;
        const mode = manager.activeMode();
        const vis = mode.buildVisualization(manager);
        const dataset = manager.planChart.data.datasets[0];
        dataset.label = vis.label;
        dataset.data = vis.points;

        const isAngular = mode instanceof AngularRouteMode;
        dataset.borderColor = this.colors.vector;
        dataset.backgroundColor = this.colors.vector;
        dataset.borderWidth = isAngular ? 3 : 2;
        dataset.pointRadius = ctx => ctx.dataIndex === 0 ? 0 : 4;
        dataset.pointBackgroundColor = isAngular ? this.colors.vector : (ctx => {
            const i = ctx.dataIndex - 1;
            return i < manager.currentStepIndex ? this.colors.accent
                : (i === manager.currentStepIndex && manager.isRunning ? this.colors.accent : this.colors.info);
        });
        manager.planChart.$angularGeometry = vis.geometry;

        this._renderOverlayAB(manager);
        this._renderOverlayTelem(manager);
        this._renderOverlayOckham(manager);
        this._renderAngularSummary(manager, isAngular);
        manager.planChart.update('none');
    },

    _renderOverlayAB(manager) {
        const forkDataset = manager.planChart.data.datasets[2];
        if (!forkDataset) return;
        if (!manager.comparingAB || manager.forks.size === 0) {
            forkDataset.data = [];
            return;
        }
        const isCustomBranch = manager.activeBranch !== 'main' && manager.mainBranchBackup;
        const compareData = isCustomBranch ? manager.mainBranchBackup : manager.forks.values().next().value;
        forkDataset.label = isCustomBranch ? 'Ruta Base (A)' : (compareData ? compareData.name : 'Ruta Fork [B]');
        const color = isCustomBranch ? '#00e5ff' : '#ff007f';
        forkDataset.borderColor = color;
        forkDataset.pointBackgroundColor = color;
        if (compareData) {
            forkDataset.data = [{ x: 0, y: 0 }, ...(compareData.queue || []).map(wp => ({
                x: wp.x - (compareData.currentOrigin?.x || 0),
                y: wp.y - (compareData.currentOrigin?.y || 0)
            }))];
            const curDist = RouteForksManager.calculateDistance(manager.queue, manager.currentOrigin);
            const forkDist = RouteForksManager.calculateDistance(compareData.queue || [], compareData.currentOrigin || { x: 0, y: 0 });
            const deltaDist = forkDist - curDist;
            const deltaSteps = (compareData.queue?.length || 0) - manager.queue.length;
            const elDist = document.getElementById('route-fork-delta-dist');
            const elSteps = document.getElementById('route-fork-delta-steps');
            if (elDist) elDist.textContent = `Δ Dist: ${deltaDist >= 0 ? '+' : ''}${deltaDist.toFixed(1)} cm`;
            if (elSteps) elSteps.textContent = `Δ Pasos: ${deltaSteps >= 0 ? '+' : ''}${deltaSteps}`;
        }
    },

    _renderOverlayTelem(manager) {
        if (!manager.telemChart) return;
        const execPlan = [{ x: 0, y: 0 }, ...manager.queue.map(wp => ({
            x: wp.x - manager.currentOrigin.x, y: wp.y - manager.currentOrigin.y
        }))];
        const index = Math.min(manager.currentStepIndex, Math.max(0, manager.queue.length - 1));
        const wp = manager.queue[index];
        manager.telemChart.data.datasets[4].data = execPlan;
        manager.telemChart.data.datasets[5].data = wp ? [{ x: wp.x - manager.currentOrigin.x, y: wp.y - manager.currentOrigin.y }] : [];
        manager.telemChart.$endpointTolerance = wp ? { x: wp.x - manager.currentOrigin.x, y: wp.y - manager.currentOrigin.y } : null;
        manager.telemChart.update('none');
    },

    _renderOverlayOckham(manager) {
        const ockhamDataset = manager.planChart.data.datasets[3];
        if (!ockhamDataset) return;
        const active = (manager.isOckhamSimulating || manager.isOckhamRunning) && manager.ockhamQueue && manager.ockhamQueue.length > 0;
        if (active) {
            const startPt = manager.ockhamStart || manager.currentOrigin;
            ockhamDataset.data = [
                { x: startPt.x - manager.currentOrigin.x, y: startPt.y - manager.currentOrigin.y },
                ...manager.ockhamQueue.map(wp => ({ x: wp.x - manager.currentOrigin.x, y: wp.y - manager.currentOrigin.y }))
            ];
            ockhamDataset.label = manager.isOckhamRunning ? 'Retorno Ockham (en curso)' : 'Simulación Ockham';
        } else {
            ockhamDataset.data = [];
            ockhamDataset.label = 'Simulación Ockham';
        }
    },

    _renderAngularSummary(manager, isAngular) {
        if (!manager.angularSummary) return;
        if (isAngular && manager.logicalSteps.length) {
            const last = manager.logicalSteps[manager.logicalSteps.length - 1];
            const chain = manager.logicalSteps.map(step => {
                const thR = step.thetaRealDeg !== undefined ? step.thetaRealDeg : step.theta;
                const thC = step.thetaChartDeg !== undefined ? step.thetaChartDeg : step.absoluteAngleDeg;
                return `P${step.number || step.index}: L=${step.length.toFixed(1)} cm, θReal=${thR.toFixed(1)}°, θChart=${thC.toFixed(1)}°, ΔX=${step.dx.toFixed(1)} cm, ΔY=${step.dy.toFixed(1)} cm`;
            }).join(' · ');
            manager.angularSummary.textContent = `${chain} · fin (${(last.end.x - manager.currentOrigin.x).toFixed(1)}, ${(last.end.y - manager.currentOrigin.y).toFixed(1)}) cm`;
        } else {
            manager.angularSummary.textContent = 'Sin vectores angulares.';
        }
    },

    updateTelemetryMap(manager, posX, posY, orientacion) {
        if (!manager.telemChart) return;
        const rx = posX - manager.currentOrigin.x, ry = posY - manager.currentOrigin.y;
        if (manager.isRunning) {
            manager.telemetryTrail.push({ x: rx, y: ry });
            if (manager.telemetryTrail.length > CONFIG.TRAIL_MAX) manager.telemetryTrail.shift();
        }
        const headingLength = Math.max(8, (manager.telemChart.options.scales.x.max - manager.telemChart.options.scales.x.min) * 0.04);
        manager.telemChart.data.datasets[0].data = manager.telemetryTrail;
        manager.telemChart.data.datasets[1].data = [{ x: rx, y: ry }];
        manager.telemChart.data.datasets[2].data = [
            { x: rx, y: ry },
            { x: rx + headingLength * Math.sin(orientacion), y: ry + headingLength * Math.cos(orientacion) }
        ];
        manager.telemChart.update('none');
        manager.renderMetrics();
    },

    autoScale(manager) {
        const isAngular = manager.activeMode() instanceof AngularRouteMode;
        let maxDist = isAngular ? 8 : 50;
        const checkPoint = (x, y) => {
            const rx = Math.abs(x - manager.currentOrigin.x), ry = Math.abs(y - manager.currentOrigin.y);
            if (rx > maxDist) maxDist = rx;
            if (ry > maxDist) maxDist = ry;
        };
        manager.queue.forEach(wp => checkPoint(wp.x, wp.y));
        if ((manager.isOckhamSimulating || manager.isOckhamRunning) && manager.ockhamQueue) {
            manager.ockhamQueue.forEach(wp => checkPoint(wp.x, wp.y));
        }
        const s = Math.max(isAngular ? 12 : 50, maxDist * (isAngular ? 1.4 : 1.2));
        for (const chart of [manager.planChart, manager.telemChart]) {
            if (!chart) continue;
            chart.options.scales.x.min = -s; chart.options.scales.x.max = s;
            chart.options.scales.y.min = -s; chart.options.scales.y.max = s;
            chart.update('none');
        }
    },

    resizeCharts(manager) {
        if (!manager.planChart || !manager.telemChart) return;
        requestAnimationFrame(() => {
            manager.planChart.resize();
            manager.telemChart.resize();
        });
    },

    toggleCinema(manager, cardId) {
        const card = document.getElementById(cardId);
        if (!card) return;
        const isCinema = card.classList.toggle('cinema-mode');
        const otherId = cardId === 'card-plan-map' ? 'card-telem-map' : 'card-plan-map';
        const otherCard = document.getElementById(otherId);
        if (isCinema && otherCard && otherCard.classList.contains('cinema-mode')) {
            otherCard.classList.remove('cinema-mode');
            this.syncCinemaButton(otherId, false);
        }
        document.body.classList.toggle('cinema-active', Boolean(document.querySelector('.route-map-card.cinema-mode')));
        this.syncCinemaButton(cardId, isCinema);
        setTimeout(() => this.resizeCharts(manager), 50);
    },

    syncCinemaButton(cardId, isCinema) {
        const btn = /** @type {HTMLElement | null} */ (document.querySelector(`.route-cinema[data-card="${cardId}"]`));
        if (btn) {
            btn.setAttribute('aria-pressed', isCinema);
            const isPlan = cardId === 'card-plan-map';
            const labelText = isPlan ? 'Cine (Plan)' : 'Cine (Telemetría)';
            const labelTitle = isPlan ? 'Pantalla completa: Plan Teórico' : 'Pantalla completa: Telemetría Real';
            const iconExpand = '<svg class="ui-icon" aria-hidden="true" viewBox="0 0 24 24"><path d="M8 3H5a2 2 0 0 0-2 2v3m18 0V5a2 2 0 0 0-2-2h-3m0 18h3a2 2 0 0 0 2-2v-3M3 16v3a2 2 0 0 0 2 2h3"/></svg>';
            const iconReduce = '<svg class="ui-icon" aria-hidden="true" viewBox="0 0 24 24"><path d="M4 14h6m0 0v6m0-6L3 21m17-11h-6m0 0V4m0 6l7-7"/></svg>';
            btn.classList.toggle('btn-stop', isCinema);
            btn.innerHTML = isCinema ? `${iconReduce} <span class="cinema-btn-text">Reducir</span>` : `${iconExpand} <span class="cinema-btn-text">${labelText}</span>`;
            btn.title = isCinema ? 'Salir de modo cine (Esc)' : labelTitle;
        }
        if (cardId === 'card-plan-map') {
            const previewBtn = document.getElementById('btn-route-preview');
            if (previewBtn) {
                previewBtn.setAttribute('aria-pressed', isCinema);
                const iconPreview = '<svg class="ui-icon" aria-hidden="true" viewBox="0 0 24 24"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/><circle cx="12" cy="12" r="3"/></svg>';
                const iconClose = '<svg class="ui-icon" aria-hidden="true" viewBox="0 0 24 24"><line x1="18" y1="6" x2="6" y2="18"/><line x1="6" y1="6" x2="18" y2="18"/></svg>';
                previewBtn.innerHTML = isCinema ? `${iconClose} Cerrar previsualización` : `${iconPreview} Previsualizar`;
                previewBtn.classList.toggle('btn-stop', isCinema);
            }
        }
    }
};

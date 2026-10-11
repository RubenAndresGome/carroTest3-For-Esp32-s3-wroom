/* ==============================================
       ROUTE MANAGER (Path Programming)
       ============================================== */
    const normalizeSignedDegrees = value => {
        const normalized = ((Number(value) + 180) % 360 + 360) % 360 - 180;
        return Object.is(normalized, -0) ? 0 : normalized;
    };
    const cleanRouteComponent = value => Math.abs(value) < 1e-9 ? 0 : value;

    class RoutePlanningMode {
        constructor(key) { this.key = key; }
        configureForm() { throw new Error('Modo de planeación sin formulario concreto'); }
        createLogicalStep() { throw new Error('Modo de planeación sin cálculo concreto'); }
        buildVisualization() { throw new Error('Modo de planeación sin visualización concreta'); }
        updatePreview() {}
        missionPayload(manager) {
            return {points:manager.queue.map(wp=>({x_mm:wp.x*10,y_mm:wp.y*10}))};
        }
        expandExecutionWaypoints(manager, step) {
            step.execution.forEach(segment => manager.appendLimited(
                segment.x0, segment.y0, segment.x1, segment.y1,
                segment.label, step.id, segment.component
            ));
        }
    }

    class RectangularRouteMode extends RoutePlanningMode {
        constructor() { super('rect'); }
        configureForm(manager) {
            manager.lbl1.textContent = 'Coordenada Relativa X (+X Derecha):';
            manager.lbl2.textContent = 'Coordenada Relativa Y (+Y Frente):';
            manager.input1.removeAttribute('min'); manager.input1.removeAttribute('max');
            manager.input2.removeAttribute('min'); manager.input2.removeAttribute('max');
            document.getElementById('route-sign-1').style.display = '';
            manager.preview.style.display = 'none';
            manager.axisDefaultGroup.hidden = false;
            manager.axisDefault.disabled = false;
            manager.angularMeta.hidden = true;
            manager.vectorialSafety.hidden = true;
            manager.modeChip.textContent='RECTANGULAR · FIN 0°';
            manager.planCanvas.setAttribute('aria-label', 'Gráfica rectangular del plan teórico');
        }
        createLogicalStep(manager, dx, dy) {
            if (!Number.isFinite(dx) || !Number.isFinite(dy)) throw new Error('Ingresa coordenadas X e Y válidas.');
            if (Math.abs(dx) <= .01 && Math.abs(dy) <= .01) throw new Error('El paso rectangular no contiene desplazamiento.');
            const start = manager.logicalEndpoint();
            const end = {x:start.x + dx, y:start.y + dy};
            const id = manager.nextLogicalStepId++;
            const number = manager.logicalSteps.length + 1;
            const execution = [];
            if (Math.abs(dx) > .01) execution.push({x0:start.x,y0:start.y,x1:end.x,y1:start.y,label:`P${number} · Componente X`,component:'x'});
            if (Math.abs(dy) > .01) execution.push({x0:end.x,y0:start.y,x1:end.x,y1:end.y,label:`P${number} · Componente Y`,component:'y'});
            return {id,number,mode:this.key,start,end,dx,dy,execution,description:`Rectangular ΔX=${dx.toFixed(2)}, ΔY=${dy.toFixed(2)}`};
        }
        buildVisualization(manager) {
            return {
                label:'Ruta planificada',
                points:[{x:0,y:0}, ...manager.queue.map(wp=>({x:wp.x-manager.currentOrigin.x,y:wp.y-manager.currentOrigin.y}))],
                geometry:null
            };
        }
    }

    class AngularRouteMode extends RoutePlanningMode {
        constructor(key, apiMode, direct) {
            super(key); this.apiMode=apiMode; this.direct=direct;
        }
        configureForm(manager) {
            manager.lbl1.textContent = 'Longitud L (cm):';
            manager.lbl2.textContent = 'Ángulo θ relativo (°):';
            manager.input1.min = '0'; manager.input1.removeAttribute('max');
            manager.input2.min = '-180'; manager.input2.max = '180';
            document.getElementById('route-sign-1').style.display = 'none';
            manager.preview.style.display = 'block';
            manager.axisDefaultGroup.hidden = true;
            manager.axisDefault.disabled = true;
            manager.rectangularCeroAutomatico = null;
            manager.angularMeta.hidden = false;
            manager.vectorialSafety.hidden = !this.direct;
            manager.angularXLabel.textContent=this.direct?'Proyección matemática X':'Componente X ejecutable';
            manager.angularYLabel.textContent=this.direct?'Proyección matemática Y':'Componente Y ejecutable';
            manager.modeChip.textContent=this.direct?'ANGULAR VECTORIAL · EXPERIMENTAL':'ANGULAR X→Y · FIN 0°';
            manager.planCanvas.setAttribute('aria-label', this.direct
                ? 'Gráfica angular vectorial con hipotenusas ejecutables y proyecciones matemáticas'
                : 'Gráfica angular con hipotenusas y componentes ortogonales ejecutables');
            manager.renderVectorialSafety();
            this.updatePreview(manager);
        }
        createLogicalStep(manager, length, theta) {
            if (!Number.isFinite(length) || length < 0) throw new Error('La longitud L debe ser un número mayor o igual que cero.');
            if (!Number.isFinite(theta) || theta < -180 || theta > 180) throw new Error('El ángulo θ debe estar entre -180° y 180°.');
            const start = manager.logicalEndpoint();
            const previous = manager.logicalSteps.length ? manager.logicalSteps[manager.logicalSteps.length-1].absoluteAngleDeg : 0;
            const absoluteAngleDeg = normalizeSignedDegrees(previous + theta);
            const radians = absoluteAngleDeg * Math.PI / 180;
            const dx = cleanRouteComponent(length * Math.cos(radians));
            const dy = cleanRouteComponent(length * Math.sin(radians));
            const end = {x:start.x + dx, y:start.y + dy};
            const id = manager.nextLogicalStepId++;
            const number = manager.logicalSteps.length + 1;
            const execution = [];
            if(length>.01&&this.direct){
                execution.push({x0:start.x,y0:start.y,x1:end.x,y1:end.y,label:`V${number} · Vector directo`,component:'vector'});
            }else{
                if (Math.abs(dx) > .01) execution.push({x0:start.x,y0:start.y,x1:end.x,y1:start.y,label:`V${number} · Componente X`,component:'x'});
                if (Math.abs(dy) > .01) execution.push({x0:end.x,y0:start.y,x1:end.x,y1:end.y,label:`V${number} · Componente Y`,component:'y'});
            }
            return {
                id,number,mode:this.key,start,end,dx,dy,length,theta,
                referenceAngleDeg:previous,absoluteAngleDeg,execution,
                description:`V${number}: L=${length.toFixed(2)} cm, θ=${theta.toFixed(2)}°, φ=${absoluteAngleDeg.toFixed(2)}°`
            };
        }
        buildVisualization(manager) {
            const relative = point => ({x:point.x-manager.currentOrigin.x,y:point.y-manager.currentOrigin.y});
            return {
                label:this.direct?'Ruta angular vectorial':'Hipotenusas angulares',
                points:[{x:0,y:0}, ...manager.logicalSteps.map(step=>relative(step.end))],
                geometry:manager.logicalSteps.map(step=>({
                    number:step.number, start:relative(step.start), end:relative(step.end),
                    corner:relative({x:step.end.x,y:step.start.y}),
                    dx:step.dx,dy:step.dy,length:step.length,theta:step.theta,
                    referenceAngleDeg:step.referenceAngleDeg,absoluteAngleDeg:step.absoluteAngleDeg,
                    direct:this.direct
                }))
            };
        }
        missionPayload(manager) {
            return {
                mode:this.apiMode,
                vectors:manager.logicalSteps.map(step=>({
                    length_cm:step.length, relative_angle_deg:step.theta
                }))
            };
        }
        updatePreview(manager) {
            const length = Number.parseFloat(manager.input1.value) || 0;
            const theta = Number.parseFloat(manager.input2.value) || 0;
            const previous = manager.logicalSteps.length ? manager.logicalSteps[manager.logicalSteps.length-1].absoluteAngleDeg : 0;
            const absolute = normalizeSignedDegrees(previous + theta);
            const radians = absolute * Math.PI / 180;
            const dx = cleanRouteComponent(length * Math.cos(radians));
            const dy = cleanRouteComponent(length * Math.sin(radians));
            manager.preview.textContent = `V${manager.logicalSteps.length+1} · L=${length.toFixed(2)} cm · θ=${theta.toFixed(2)}° · φ=${absolute.toFixed(2)}° · ΔX=${dx.toFixed(2)} cm · ΔY=${dy.toFixed(2)} cm`;
        }
    }

    class AngularDecompositionRouteMode extends AngularRouteMode {
        constructor() { super('polar','angular_decomposition',false); }
    }

    class AngularVectorialRouteMode extends AngularRouteMode {
        constructor() { super('vectorial','angular_vectorial',true); }
    }

    const ROUTE_PLANNING_MODES = {
        rect:new RectangularRouteMode(),
        polar:new AngularDecompositionRouteMode(),
        vectorial:new AngularVectorialRouteMode()
    };

    const RouteManager = {
        queue: [], logicalSteps: [], telemetryTrail: [], currentOrigin: {x:0, y:0}, isRunning: false, routeStartedAt:0, currentStepIndex: 0, waitingForMovement: false, moveTimeout: null, activeCommandId:null, rectangularCeroAutomatico:null, lockedMode:null, nextLogicalStepId:1, vectorialEnabled:false, lastRobotTelemetry:null,
        init() {
            this.modeSelect = document.getElementById('route-mode');
            this.input1 = document.getElementById('route-input-1');
            this.input2 = document.getElementById('route-input-2');
            this.lbl1 = document.getElementById('route-label-1');
            this.lbl2 = document.getElementById('route-label-2');
            this.axisDefault = document.getElementById('route-axis-default');
            this.axisDefaultGroup = document.getElementById('route-axis-default-group');
            this.list = document.getElementById('route-list');
            this.preview = document.getElementById('route-polar-preview');
            this.angularMeta = document.getElementById('route-angular-meta');
            this.angularSummary = document.getElementById('route-angular-summary');
            this.angularXLabel = document.getElementById('route-angular-x-label');
            this.angularYLabel = document.getElementById('route-angular-y-label');
            this.vectorialSafety = document.getElementById('route-vectorial-safety');
            this.vectorialToggle = document.getElementById('route-vectorial-enabled');
            this.vectorialStatus = document.getElementById('route-vectorial-status');
            this.modeChip = document.getElementById('route-mode-chip');
            this.planCanvas = document.getElementById('route-plan-chart');
            this._initCharts();
            
            this.modeSelect.addEventListener('change', () => { this.updateLabels(); this.updatePreview(); });
            this.vectorialToggle.addEventListener('change', async()=>{
                const requested=this.vectorialToggle.checked;
                this.vectorialToggle.disabled=true;
                try{
                    const config=await WSControl.api('/api/v1/config/robot',{method:'PUT',body:JSON.stringify({experimental_vectorial_routes:requested})});
                    this.applyConfig(config);
                    LogSystem.add(requested?'WARN':'INFO',requested?'Ejecución angular vectorial experimental habilitada.':'Ejecución angular vectorial deshabilitada.');
                }catch(error){
                    this.vectorialToggle.checked=this.vectorialEnabled;
                    LogSystem.add('ERROR',error.message);
                }finally{
                    this.vectorialToggle.disabled=this.isRunning;
                    this.renderVectorialSafety();
                }
            });
            this.axisDefault.addEventListener('change', () => {
                this.rectangularCeroAutomatico = null;
            });
            this.input1.addEventListener('input', () => { this.completarEjeRectangular(this.input1); this.updatePreview(); });
            this.input2.addEventListener('input', () => { this.completarEjeRectangular(this.input2); this.updatePreview(); });
            document.getElementById('route-form').addEventListener('submit', (e) => {
                e.preventDefault(); this.addStep();
            });
            document.getElementById('btn-route-start').addEventListener('click', () => this.start());
            document.getElementById('btn-route-preview').addEventListener('click', () => this.preview());
            document.getElementById('btn-route-pop').addEventListener('click', () => this.removeLastStep());
            document.getElementById('btn-route-stop').addEventListener('click', () => this.stopAndReset());
            document.getElementById('btn-route-discard').addEventListener('click', () => this.stopAndReset());
            document.getElementById('btn-route-ockham').addEventListener('click', () => this.ockhamReturn());
            document.getElementById('btn-confirm-ockham').addEventListener('click', () => this.confirmOckhamReturn());
            document.getElementById('btn-cancel-ockham').addEventListener('click', () => this.cancelOckhamReturn());
            document.getElementById('btn-route-clear-memory').addEventListener('click', () => this.clearRobotMemory());
            this.updateLabels();
        },
        activeMode() {
            return ROUTE_PLANNING_MODES[this.lockedMode || this.modeSelect.value] || ROUTE_PLANNING_MODES.rect;
        },
        logicalEndpoint() {
            return this.logicalSteps.length ? this.logicalSteps[this.logicalSteps.length-1].end : this.currentOrigin;
        },
        updateModeLock() {
            this.lockedMode = this.logicalSteps.length ? this.logicalSteps[0].mode : null;
            this.modeSelect.disabled = Boolean(this.lockedMode);
            this.modeSelect.title = this.lockedMode ? 'Vacía la ruta para cambiar el modo de entrada.' : '';
            if (this.lockedMode) this.modeSelect.value = this.lockedMode;
        },
        _chartOptions() {
            const theme = getComputedStyle(document.documentElement);
            const grid = theme.getPropertyValue('--border').trim();
            const text = theme.getPropertyValue('--text-secondary').trim();
            return {
                responsive:true, maintainAspectRatio:false, animation:false, normalized:true,
                plugins:{
                    legend:{ position:'top', labels:{ color:text, boxWidth:28, boxHeight:8, font:{size:10} } },
                    datalabels:{ display:false },
                    zoom:{ pan:{enabled:true,mode:'xy'}, zoom:{wheel:{enabled:true},pinch:{enabled:true},mode:'xy'} }
                },
                scales:{
                    x:{ type:'linear', min:-50, max:50, title:{display:true,text:'X (cm) [← Izq / Der →]',color:text}, ticks:{color:text}, grid:{color:grid} },
                    y:{ type:'linear', min:-50, max:50, title:{display:true,text:'Y (cm) [↓ Rev / Fte ↑]',color:text}, ticks:{color:text}, grid:{color:grid} }
                }
            };
        },
        _initCharts() {
            const theme = getComputedStyle(document.documentElement);
            this.colors = {
                info:theme.getPropertyValue('--info').trim(), accent:theme.getPropertyValue('--accent').trim(),
                reverse:theme.getPropertyValue('--reverse').trim(), muted:theme.getPropertyValue('--text-muted').trim(),
                warn:theme.getPropertyValue('--warn').trim(), vector:'#c56cff'
            };
            const angularOverlayPlugin = {
                id:'routeAngularOverlay',
                afterDatasetsDraw:chart => {
                    const geometry=chart.$angularGeometry;
                    if(!geometry?.length)return;
                    const ctx=chart.ctx, xScale=chart.scales.x, yScale=chart.scales.y;
                    const px=point=>({x:xScale.getPixelForValue(point.x),y:yScale.getPixelForValue(point.y)});
                    const label=(text,x,y,color) => {
                        ctx.save(); ctx.font='700 10px monospace';
                        const width=ctx.measureText(text).width+8;
                        ctx.fillStyle='rgba(5,9,14,.86)'; ctx.fillRect(x-width/2,y-9,width,15);
                        ctx.fillStyle=color; ctx.textAlign='center'; ctx.textBaseline='middle'; ctx.fillText(text,x,y-1); ctx.restore();
                    };
                    const arrow=(from,to,color,text,side=1) => {
                        const a=px(from), b=px(to), angle=Math.atan2(b.y-a.y,b.x-a.x);
                        ctx.save(); ctx.strokeStyle=color; ctx.fillStyle=color; ctx.lineWidth=1.7; ctx.setLineDash([6,5]);
                        ctx.beginPath(); ctx.moveTo(a.x,a.y); ctx.lineTo(b.x,b.y); ctx.stroke(); ctx.setLineDash([]);
                        ctx.beginPath(); ctx.moveTo(b.x,b.y); ctx.lineTo(b.x-8*Math.cos(angle-.45),b.y-8*Math.sin(angle-.45));
                        ctx.lineTo(b.x-8*Math.cos(angle+.45),b.y-8*Math.sin(angle+.45)); ctx.closePath(); ctx.fill(); ctx.restore();
                        label(text,(a.x+b.x)/2-Math.sin(angle)*11*side,(a.y+b.y)/2+Math.cos(angle)*11*side,color);
                    };
                    ctx.save();
                    const area=chart.chartArea; ctx.beginPath(); ctx.rect(area.left,area.top,area.width,area.height); ctx.clip();
                    geometry.forEach((step,stepIndex)=>{
                        const start=px(step.start), end=px(step.end);
                        if(Math.abs(step.dx)>.01)arrow(step.start,step.corner,this.colors.info,`X ${step.dx.toFixed(2)} cm`,-1);
                        if(Math.abs(step.dy)>.01)arrow(step.corner,step.end,this.colors.warn,`Y ${step.dy.toFixed(2)} cm`,1);
                        const coincident=geometry.slice(0,stepIndex).filter(previous=>Math.hypot(previous.start.x-step.start.x,previous.start.y-step.start.y)<.01).length;
                        const arcRadius=28+coincident*30, angleLabelRadius=arcRadius+16;
                        const reference=step.referenceAngleDeg*Math.PI/180;
                        ctx.save(); ctx.strokeStyle=this.colors.accent; ctx.lineWidth=1.2; ctx.setLineDash([4,4]); ctx.globalAlpha=.8;
                        ctx.beginPath(); ctx.moveTo(start.x,start.y); ctx.lineTo(start.x+(angleLabelRadius+4)*Math.cos(reference),start.y-(angleLabelRadius+4)*Math.sin(reference)); ctx.stroke(); ctx.restore();
                        const samples=Math.max(2,Math.ceil(Math.abs(step.theta)/8));
                        ctx.save(); ctx.strokeStyle=this.colors.accent; ctx.lineWidth=2; ctx.shadowColor=this.colors.accent; ctx.shadowBlur=5; ctx.beginPath();
                        for(let index=0;index<=samples;index++){
                            const angle=(step.referenceAngleDeg+step.theta*index/samples)*Math.PI/180;
                            const x=start.x+arcRadius*Math.cos(angle), y=start.y-arcRadius*Math.sin(angle);
                            if(index===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);
                        }
                        ctx.stroke(); ctx.restore();
                        label(`θ${step.number} ${step.theta.toFixed(1)}°`,start.x-34,start.y-44-coincident*18,this.colors.accent);
                        if(step.length>.01)label(`V${step.number} · ${step.length.toFixed(2)} cm`,(start.x+end.x)/2,(start.y+end.y)/2+12,this.colors.vector);
                    });
                    ctx.restore();
                }
            };
            this.planChart = new Chart(this.planCanvas.getContext('2d'), {
                type:'scatter',
                data:{ datasets:[
                    { label:'Ruta planificada', data:[{x:0,y:0}], showLine:true, borderColor:this.colors.info, backgroundColor:this.colors.info, borderWidth:2, tension:0, pointRadius:(ctx)=>ctx.dataIndex===0?0:4, pointBackgroundColor:(ctx)=>{ const i=ctx.dataIndex-1; return i<this.currentStepIndex?this.colors.accent:i===this.currentStepIndex&&this.isRunning?this.colors.accent:this.colors.info; } },
                    { label:'Origen', data:[{x:0,y:0}], pointRadius:5, pointBackgroundColor:this.colors.reverse, pointBorderColor:this.colors.reverse }
                ]},
                options:this._chartOptions(), plugins:[angularOverlayPlugin]
            });
            const tolerancePlugin = {
                id:'routeEndpointTolerance',
                afterDatasetsDraw(chart) {
                    const target=chart.$endpointTolerance;
                    if(!target) return;
                    const x=chart.scales.x.getPixelForValue(target.x), y=chart.scales.y.getPixelForValue(target.y);
                    const radiusX=Math.abs(chart.scales.x.getPixelForValue(target.x+5)-x);
                    const radiusY=Math.abs(chart.scales.y.getPixelForValue(target.y+5)-y);
                    const ctx=chart.ctx;
                    ctx.save(); ctx.strokeStyle='rgba(245,158,11,.9)'; ctx.lineWidth=1.5;
                    ctx.setLineDash([5,3]); ctx.beginPath(); ctx.ellipse(x,y,radiusX,radiusY,0,0,Math.PI*2); ctx.stroke(); ctx.restore();
                }
            };
            this.telemChart = new Chart(document.getElementById('route-telem-chart').getContext('2d'), {
                type:'scatter',
                data:{ datasets:[
                    { label:'Traza medida', data:[], showLine:true, borderColor:'#00e5ff', backgroundColor:'#00e5ff', borderWidth:2, tension:.2, pointRadius:0 },
                    { label:'Posición actual', data:[{x:0,y:0}], pointRadius:6, pointBackgroundColor:this.colors.accent, pointBorderColor:this.colors.accent },
                    { label:'Orientación', data:[{x:0,y:0},{x:0,y:8}], showLine:true, borderColor:this.colors.accent, borderWidth:2, pointRadius:0 },
                    { label:'Origen', data:[{x:0,y:0}], pointRadius:4, pointBackgroundColor:this.colors.reverse, pointBorderColor:this.colors.reverse },
                    { label:'Ruta planificada', data:[], showLine:true, borderColor:this.colors.muted, borderDash:[6,4], borderWidth:1.5, pointRadius:0 },
                    { label:'Tolerancia final (±5 cm)', data:[], pointRadius:4, pointBackgroundColor:'#f59e0b', pointBorderColor:'#f59e0b' }
                ]},
                options:this._chartOptions(), plugins:[tolerancePlugin]
            });
        },
        resizeCharts() {
            if (!this.planChart || !this.telemChart) return;
            requestAnimationFrame(()=>{ this.planChart.resize(); this.telemChart.resize(); });
        },
        clearTelemetryTrail() {
            this.telemetryTrail = [];
            this.renderMetrics();
            if (this.telemChart) { this.telemChart.data.datasets[0].data = []; this.telemChart.update('none'); }
        },
        applyCapabilities(capabilities) {
            const vectorial=capabilities&&capabilities.angular_vectorial||{};
            if(typeof vectorial.enabled==='boolean')this.applyConfig({experimental_vectorial_routes:vectorial.enabled});
        },
        applyConfig(config) {
            if(typeof config?.experimental_vectorial_routes==='boolean'){
                this.vectorialEnabled=config.experimental_vectorial_routes;
                if(this.vectorialToggle)this.vectorialToggle.checked=this.vectorialEnabled;
                this.renderVectorialSafety();
            }
        },
        applyVectorialTelemetry(raw) {
            this.lastRobotTelemetry=raw||null;
            this.renderVectorialSafety();
        },
        vectorialReadiness() {
            if(!this.vectorialEnabled)return {ready:false,reason:'La bandera experimental está deshabilitada.'};
            const raw=this.lastRobotTelemetry;
            if(!raw)return {ready:false,reason:'Esperando telemetría del robot.'};
            const health=raw.encoder_health||{};
            const invalid=['fl','fr','bl','br'].filter(wheel=>String(health[wheel]||'unknown').toLowerCase()!=='healthy');
            if(invalid.length)return {ready:false,reason:`Encoders no saludables: ${invalid.map(w=>w.toUpperCase()).join(', ')}.`};
            if(raw.degraded_mode)return {ready:false,reason:'El robot reporta modo degradado.'};
            const mpu=raw.mpu||{};
            if(mpu.present!==true||mpu.calibrated!==true||mpu.stale!==false)return {ready:false,reason:'La MPU debe estar presente, calibrada y vigente.'};
            if(String(raw.state||'').toLowerCase()!=='listo')return {ready:false,reason:`El robot debe estar LISTO; estado: ${raw.state||'desconocido'}.`};
            return {ready:true,reason:'Apto: cuatro encoders healthy, MPU vigente y robot LISTO.'};
        },
        renderVectorialSafety() {
            if(!this.vectorialStatus)return;
            const readiness=this.vectorialReadiness();
            const raw=this.lastRobotTelemetry||{}, health=raw.encoder_health||{};
            const healthText=['fl','fr','bl','br'].map(w=>`${w.toUpperCase()}=${health[w]||'unknown'}`).join(' · ');
            this.vectorialStatus.textContent=`${readiness.reason} ${healthText}`;
            this.vectorialStatus.classList.toggle('ready',readiness.ready);
            this.vectorialToggle.checked=this.vectorialEnabled;
            this.vectorialToggle.disabled=this.isRunning;
        },
        updateLabels() {
            this.activeMode().configureForm(this);
            this.drawPlan();
        },
        completarEjeRectangular(campoEditado) {
            if (this.modeSelect.value !== 'rect' || !this.axisDefault.checked) return;
            const valor = campoEditado.value.trim();
            if (!valor) {
                if (this.rectangularCeroAutomatico === campoEditado.id) this.rectangularCeroAutomatico = null;
                return;
            }
            const otroCampo = campoEditado === this.input1 ? this.input2 : this.input1;
            const reemplazaCeroAutomatico = this.rectangularCeroAutomatico === campoEditado.id;
            if (!otroCampo.value.trim() || reemplazaCeroAutomatico) {
                otroCampo.value = '0';
                this.rectangularCeroAutomatico = otroCampo.id;
            }
        },
        updatePreview() {
            this.activeMode().updatePreview(this);
        },
        addStep() {
            if (this.isRunning) return LogSystem.add('WARN', 'Ruta en ejecución. Detén primero.');
            const v1 = Number.parseFloat(this.input1.value), v2 = Number.parseFloat(this.input2.value);
            if (this.logicalSteps.length === 0) {
                this.currentOrigin = {x: state.telemetria.posX, y: state.telemetria.posY};
            }
            try {
                const mode=this.activeMode();
                const step=mode.createLogicalStep(this,v1,v2);
                if(this.logicalSteps.length===0)this.clearTelemetryTrail();
                const queueLengthBefore=this.queue.length;
                mode.expandExecutionWaypoints(this,step);
                if(this.queue.length>256){
                    this.queue.length=queueLengthBefore;
                    throw new Error('La ruta excedería el máximo de 256 pasos ejecutables.');
                }
                this.logicalSteps.push(step);
                this.updateModeLock();
                LogSystem.add('INFO',step.execution.length?`Paso lógico agregado: ${step.description}`:`Orientación visual agregada: ${step.description}; no se enviará movimiento.`);
            } catch(error) {
                LogSystem.add('ERROR',error.message);
                return;
            }
            this.input1.value = ''; this.input2.value = ''; this.rectangularCeroAutomatico = null;
            this.updateListUI(); this.updateLabels(); this.updatePreview(); this.autoScale();
        },
        removeLastStep() {
            if (this.isRunning) return LogSystem.add('WARN','No se puede modificar una ruta en ejecución.');
            if (this.logicalSteps.length === 0) return LogSystem.add('WARN','No hay pasos para deshacer.');
            const removed=this.logicalSteps.pop();
            this.queue=this.queue.filter(waypoint=>waypoint.logicalStepId!==removed.id);
            this.currentStepIndex=0;
            if (this.logicalSteps.length === 0) {
                this.currentOrigin={x:state.telemetria.posX,y:state.telemetria.posY};
            }
            this.updateModeLock(); this.updateListUI(); this.updateLabels(); this.updatePreview(); this.autoScale();
            LogSystem.add('INFO',`Paso lógico eliminado: ${removed.description}`);
        },
        appendLimited(x0,y0,x1,y1,label,logicalStepId,component) {
            const distance=Math.hypot(x1-x0,y1-y0), count=Math.max(1,Math.ceil(distance/200));
            for(let i=1;i<=count;i++) {
                const x=x0+(x1-x0)*i/count, y=y0+(y1-y0)*i/count;
                this.queue.push({x,y,logicalStepId,component,desc:`${label}${count>1?` ${i}/${count}`:''} a (${(x-this.currentOrigin.x).toFixed(2)}, ${(y-this.currentOrigin.y).toFixed(2)})`});
            }
        },
        updateListUI() {
            if (this.queue.length === 0) {
                const message=this.logicalSteps.length?'Sólo hay orientaciones visuales; no existe desplazamiento para enviar.':'La ruta está vacía. Agrega puntos arriba.';
                this.list.innerHTML = `<tr><td colspan="5" style="color:var(--text-muted)">${message}</td></tr>`;
                return;
            }
            this.list.innerHTML = this.queue.map((wp, i) => {
                const isPast = i < this.currentStepIndex;
                const isCurrent = i === this.currentStepIndex && this.isRunning;
                const rowClass = isPast ? 'route-step-done' : isCurrent ? 'route-step-current' : '';
                const status = isPast ? 'Completado' : isCurrent ? 'En curso' : 'Pendiente';
                const statusClass = isPast ? 'done' : isCurrent ? 'current' : 'pending';
                return `<tr class="${rowClass}"><td>${i+1}</td><td>${wp.desc}</td><td>${wp.x.toFixed(2)}</td><td>${wp.y.toFixed(2)}</td><td><span class="route-status ${statusClass}">${status}</span></td></tr>`;
            }).join('');
        },
        drawPlan() {
            if (!this.planChart) return;
            const mode=this.activeMode();
            const visualization=mode.buildVisualization(this);
            const dataset=this.planChart.data.datasets[0];
            dataset.label=visualization.label; dataset.data=visualization.points;
            const angular=mode instanceof AngularRouteMode;
            dataset.borderColor=this.colors.vector;
            dataset.backgroundColor=this.colors.vector;
            dataset.borderWidth=angular?3:2;
            dataset.pointRadius=angular?(ctx=>ctx.dataIndex===0?0:4):(ctx=>ctx.dataIndex===0?0:4);
            dataset.pointBackgroundColor=angular?this.colors.vector:(ctx=>{const i=ctx.dataIndex-1;return i<this.currentStepIndex?this.colors.accent:i===this.currentStepIndex&&this.isRunning?this.colors.accent:this.colors.info;});
            this.planChart.$angularGeometry=visualization.geometry;
            const executionPlan=[{x:0,y:0}, ...this.queue.map(wp=>({x:wp.x-this.currentOrigin.x,y:wp.y-this.currentOrigin.y}))];
            if (this.telemChart) {
                const index=Math.min(this.currentStepIndex, Math.max(0,this.queue.length-1));
                const waypoint=this.queue[index];
                this.telemChart.data.datasets[4].data=executionPlan;
                this.telemChart.data.datasets[5].data=waypoint ? [{x:waypoint.x-this.currentOrigin.x,y:waypoint.y-this.currentOrigin.y}] : [];
                this.telemChart.$endpointTolerance=waypoint ? {x:waypoint.x-this.currentOrigin.x,y:waypoint.y-this.currentOrigin.y} : null;
                this.telemChart.update('none');
            }
            if(angular&&this.logicalSteps.length){
                const last=this.logicalSteps[this.logicalSteps.length-1];
                const chain=this.logicalSteps.map(step=>`V${step.number}: L=${step.length.toFixed(2)} cm, θ=${step.theta.toFixed(2)}°, φ=${step.absoluteAngleDeg.toFixed(2)}°, ΔX=${step.dx.toFixed(2)} cm, ΔY=${step.dy.toFixed(2)} cm${step.length===0?' (sólo orientación)':''}`).join(' · ');
                this.angularSummary.textContent=`${chain} · punto final (${(last.end.x-this.currentOrigin.x).toFixed(2)}, ${(last.end.y-this.currentOrigin.y).toFixed(2)}) cm`;
            }else this.angularSummary.textContent='Sin vectores angulares.';
            this.planChart.update('none');
        },
        autoScale() {
            const angular=this.activeMode() instanceof AngularRouteMode;
            let maxDist = angular ? 8 : 50;
            this.queue.forEach(wp => {
                let rx = Math.abs(wp.x - this.currentOrigin.x);
                let ry = Math.abs(wp.y - this.currentOrigin.y);
                if (rx > maxDist) maxDist = rx;
                if (ry > maxDist) maxDist = ry;
            });
            const s = Math.max(angular?12:50,maxDist * (angular?1.4:1.2));
            for (const chart of [this.planChart,this.telemChart]) {
                chart.options.scales.x.min=-s; chart.options.scales.x.max=s;
                chart.options.scales.y.min=-s; chart.options.scales.y.max=s;
                chart.update('none');
            }
        },
        updateTelemetryMap(posX, posY, orientacion) {
            if (!this.telemChart) return;
            const rx = posX - this.currentOrigin.x, ry = posY - this.currentOrigin.y;
            if (this.isRunning) {
                this.telemetryTrail.push({x:rx,y:ry});
                if (this.telemetryTrail.length > CONFIG.TRAIL_MAX) this.telemetryTrail.shift();
            }
            const headingLength = Math.max(8,(this.telemChart.options.scales.x.max-this.telemChart.options.scales.x.min)*.04);
            this.telemChart.data.datasets[0].data=this.telemetryTrail;
            this.telemChart.data.datasets[1].data=[{x:rx,y:ry}];
            this.telemChart.data.datasets[2].data=[{x:rx,y:ry},{x:rx+headingLength*Math.sin(orientacion),y:ry+headingLength*Math.cos(orientacion)}];
            this.telemChart.update('none');
            this.renderMetrics();
        },
        renderMetrics() {
            const duration=this.routeStartedAt?((Date.now()-this.routeStartedAt)/1000):0;
            const samples=this.telemetryTrail.length;
            const endpoint=this.logicalEndpoint(), actual=this.telemetryTrail[samples-1]||{x:state.telemetria.posX,y:state.telemetria.posY};
            const deviation=this.isRunning||samples ? Math.hypot(actual.x-(endpoint.x-this.currentOrigin.x),actual.y-(endpoint.y-this.currentOrigin.y)) : 0;
            document.getElementById('route-metric-duration').textContent=`${duration.toFixed(1)} s`;
            document.getElementById('route-metric-samples').textContent=String(samples);
            document.getElementById('route-metric-segments').textContent=`${this.queue.length} / 256`;
            document.getElementById('route-metric-deviation').textContent=`${deviation.toFixed(1)} cm`;
        },
        preview() {
            this.toggleCinema('card-plan-map');
        },
        toggleCinema(cardId) {
            const card = document.getElementById(cardId);
            if (!card) return;
            const isCinema = card.classList.toggle('cinema-mode');

            // Si se activa modo cine en una tarjeta, desactivar la otra
            const otherCardId = (cardId === 'card-plan-map') ? 'card-telem-map' : 'card-plan-map';
            const otherCard = document.getElementById(otherCardId);
            if (isCinema && otherCard && otherCard.classList.contains('cinema-mode')) {
                otherCard.classList.remove('cinema-mode');
                this._syncCinemaButton(otherCardId, false);
            }

            this._syncCinemaButton(cardId, isCinema);
            setTimeout(() => this.resizeCharts(), 50);
        },
        _syncCinemaButton(cardId, isCinema) {
            const btn = document.querySelector(`.route-cinema[data-card="${cardId}"]`);
            if (btn) {
                btn.setAttribute('aria-pressed', isCinema);
                if (isCinema) {
                    btn.classList.add('btn-stop');
                    btn.innerHTML = '🗗 Reducir';
                    btn.title = 'Salir de modo cine (Esc)';
                } else {
                    btn.classList.remove('btn-stop');
                    btn.innerHTML = '⛶ Cine';
                    btn.title = 'Pantalla completa / Modo cine';
                }
            }
            if (cardId === 'card-plan-map') {
                const previewBtn = document.getElementById('btn-route-preview');
                if (previewBtn) {
                    previewBtn.setAttribute('aria-pressed', isCinema);
                    previewBtn.textContent = isCinema ? '✕ Cerrar previsualización' : '👁 Previsualizar';
                    if (isCinema) previewBtn.classList.add('btn-stop');
                    else previewBtn.classList.remove('btn-stop');
                }
            }
        },
        lockTabs() {
            document.querySelectorAll('.tab-btn').forEach(b => {
                if (b.dataset.tab !== 'rutas') b.classList.add('disabled');
            });
            document.getElementById('route-running-badge').classList.add('show');
        },
        unlockTabs() {
            document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('disabled'));
            document.getElementById('route-running-badge').classList.remove('show');
        },
        async start() {
            if(this.logicalSteps.length&&this.queue.length===0)return LogSystem.add('WARN','La ruta sólo contiene orientaciones visuales; agrega una hipotenusa con longitud mayor que cero.');
            if (this.isRunning || this.queue.length === 0 || this.currentStepIndex >= this.queue.length) return;
            const mode=this.activeMode();
            if(mode.key==='vectorial'){
                const readiness=this.vectorialReadiness();
                if(!readiness.ready)return LogSystem.add('ERROR',`Angular vectorial bloqueado: ${readiness.reason}`);
            }
            try {
                const previous=await WSControl.api('/api/v1/missions');
                if(previous.blocked){
                    await WSControl.api('/api/v1/missions',{method:'DELETE'});
                    LogSystem.add('WARN',`Misión bloqueada ${previous.id||''} descartada antes de iniciar.`);
                }
                const mission=await WSControl.api('/api/v1/missions',{method:'POST',body:JSON.stringify(mode.missionPayload(this))});
                this.isRunning=true; this.routeStartedAt=Date.now(); this.currentStepIndex=mission.current_index||0;
                this.activeCommandId=mission.active_command_id; this.waitingForMovement=true;
                this.clearTelemetryTrail();
                this.lockTabs(); LogSystem.add('INFO',`Misión ${mission.id} iniciada con ${mission.total_segments} segmentos.`);
                this.updateListUI(); this.drawPlan();
            } catch(error) {
                this.isRunning = false;
                this.unlockTabs();
                LogSystem.add('ERROR', error.message);
            }
        },
        onRobotEvent(event) {},
        onMission(mission) {
            this.currentStepIndex=mission.current_index||0;
            this.activeCommandId=mission.active_command_id||null;
            this.waitingForMovement=Boolean(mission.running);
            const safety=document.getElementById('route-mission-safety');
            if(mission.blocked&&mission.error){
                const interrupted=mission.error==='robot_restarted_mid_mission';
                safety.textContent=interrupted
                    ? `MISIÓN INTERRUMPIDA en el paso ${this.currentStepIndex+1}. El ESP32 se reinició: el paso no será repetido. Borra la misión, recentra pose y decide una ruta nueva.`
                    : `Misión bloqueada: ${mission.error}`;
                safety.style.color='var(--danger)';
                return this.failRoute(interrupted?'robot_restarted_mid_mission: repetición automática bloqueada':mission.error);
            }
            if(mission.accuracy){
                const acc=mission.accuracy;
                const elPos=document.getElementById('route-live-acc-pos');
                const elAng=document.getElementById('route-live-acc-ang');
                if(elPos) elPos.textContent=`${Number(acc.error_long_cm||0).toFixed(1)} / ${Number(acc.error_lat_cm||0).toFixed(1)} cm`;
                if(elAng) elAng.textContent=`${Number(acc.error_angular_deg||0).toFixed(1)}° / ${Number(acc.accumulated_lateral_cm||0).toFixed(1)} cm`;
            }
            if(mission.stage==='settling'){
                safety.textContent=`Punto de exactitud: evaluando asentamiento y errores (${this.currentStepIndex}/${mission.total_segments||0})...`;
                safety.style.color='var(--accent, #f59e0b)';
            } else if(mission.stage==='realigning'){
                safety.textContent=`Reposicionamiento ortogonal: desvío angular > 1.5° o lateral > 2.5 cm. Re-alineando a eje cardinal...`;
                safety.style.color='var(--accent, #f59e0b)';
            } else {
                safety.textContent=`Protección de pasos: ${String(mission.stage||'idle').toUpperCase()} · checkpoint ${this.currentStepIndex}/${mission.total_segments||0}.`;
                safety.style.color='';
            }
            if(this.isRunning&&!mission.running) {
                this.isRunning=false; this.unlockTabs(); clearTimeout(this.moveTimeout);
                LogSystem.add('INFO','Ruta completada por el carrito.');
                if(mission.kind==='ockham_return') LogSystem.add('INFO','Regreso de Ockham y alineación final a 0° completados.');
            }
            this.renderVectorialSafety();
            this.updateListUI(); this.drawPlan();
        },
        onFsmIdle() {},
        failRoute(reason) {
            this.isRunning=false; this.waitingForMovement=false; this.activeCommandId=null;
            clearTimeout(this.moveTimeout); this.unlockTabs(); this.updateListUI();
            this.renderVectorialSafety();
            LogSystem.add('ERROR',reason);
        },
        async stopAndReset() {
            if (this.logicalSteps.length > 0 && !confirm("¿Borrar la ruta actual y reiniciar el origen?")) return;
            try {
                const mission=await WSControl.api('/api/v1/missions',{method:'DELETE'});
                if(mission.id!==null||mission.blocked)throw new Error('El backend no confirmó el borrado de la misión');
                this.isRunning = false;
                this.unlockTabs();
                clearTimeout(this.moveTimeout);
                this.waitingForMovement = false;
                this.activeCommandId = null;
                this.queue = [];
                this.logicalSteps = [];
                this.nextLogicalStepId = 1;
                this.currentStepIndex = 0;
                this.currentOrigin = {x: state.telemetria.posX, y: state.telemetria.posY};
                this.clearTelemetryTrail();
                this.updateModeLock();
                LogSystem.add('WARN', 'Ruta detenida y reset confirmado por backend.');
                this.updateListUI();
                this.updateLabels(); this.updatePreview();
                this.renderVectorialSafety();
            } catch(error) {
                LogSystem.add('ERROR',`No se pudo borrar la ruta: ${error.message}`);
            }
        },
        async clearRobotMemory() {
            const box=document.getElementById('route-ockham-confirm-box');
            if(box) box.style.display='none';
            if (!confirm('¿Detener el robot y borrar la misión y su checkpoint de Python?')) return;
            try {
                const result=await WSControl.api('/api/v1/missions/robot-memory',{method:'DELETE'});
                this.isRunning=false; this.waitingForMovement=false; this.activeCommandId=null;
                this.unlockTabs();
                LogSystem.add('INFO','Misión y checkpoint de Python eliminados; se solicitó STOP si había un paso activo.');
                this.currentStepIndex=0; this.updateListUI(); this.drawPlan();
            } catch(error) { LogSystem.add('ERROR',`No se pudo liberar la memoria: ${error.message}`); }
        },
        async ockhamReturn() {
            try {
                const preview=await WSControl.api('/api/v1/missions/return-home',{method:'POST',body:JSON.stringify({preview:true})});
                // Conservar el home de la mision como (0,0) del grafico: asi la
                // traza del retorno se superpone al camino saliente invertido en
                // vez de quedar en un marco relativo a la pose actual.
                this.logicalSteps=[]; this.nextLogicalStepId=1;
                const vectorialReturn=preview.mode==='angular_vectorial';
                this.modeSelect.value=vectorialReturn?'vectorial':'rect';
                let logicalStart=this.currentOrigin;
                this.queue=(preview.planned_points||[]).map((point,index)=>{
                    const end={x:Number(point.x_mm)/10,y:Number(point.y_mm)/10};
                    const id=this.nextLogicalStepId++;
                    const dx=end.x-logicalStart.x,dy=end.y-logicalStart.y;
                    const absoluteAngleDeg=normalizeSignedDegrees(Math.atan2(dy,dx)*180/Math.PI);
                    const previous=this.logicalSteps.length?this.logicalSteps[this.logicalSteps.length-1].absoluteAngleDeg:0;
                    const theta=normalizeSignedDegrees(absoluteAngleDeg-previous);
                    const step=vectorialReturn
                        ? {id,number:index+1,mode:'vectorial',start:logicalStart,end,dx,dy,length:Math.hypot(dx,dy),theta,referenceAngleDeg:previous,absoluteAngleDeg,execution:[],description:'Vector inverso Ockham'}
                        : {id,number:index+1,mode:'rect',start:logicalStart,end,dx,dy,execution:[],description:'Paso inverso Ockham'};
                    this.logicalSteps.push(step); logicalStart=end;
                    return {x:end.x,y:end.y,logicalStepId:id,component:vectorialReturn?'vector':'ockham',desc:vectorialReturn?'Vector inverso Ockham':'Paso inverso Ockham'};
                });
                this.updateModeLock();
                this.currentStepIndex=0;
                this.updateListUI(); this.drawPlan(); this.autoScale();

                const box=document.getElementById('route-ockham-confirm-box');
                const info=document.getElementById('route-ockham-sim-info');
                if(box&&info){
                    box.style.display='block';
                    info.textContent=`Simulación calculada: ${preview.total_steps} pasos enlazados hacia (0, 0) con retorno optimizado (reversa o avance automático) y supervisión de exactitud. Confirma para arrancar.`;
                }
                LogSystem.add('INFO',`Simulación de regreso Ockham lista: ${preview.total_steps} pasos enlazados calculados hacia el origen.`);
            } catch(error) { LogSystem.add('ERROR',`No se pudo simular Ockham: ${error.message}`); }
        },
        async confirmOckhamReturn() {
            const box=document.getElementById('route-ockham-confirm-box');
            if(box) box.style.display='none';
            try {
                const mission=await WSControl.api('/api/v1/missions/return-home',{method:'POST',body:JSON.stringify({preview:false})});
                this.currentStepIndex=mission.current_index||0;
                this.activeCommandId=mission.active_command_id||null;
                this.isRunning=true; this.waitingForMovement=true;
                this.clearTelemetryTrail(); this.lockTabs();
                LogSystem.add('CMD','Ruta Ockham inversa confirmada e iniciada con retorno automático; supervisión de exactitud activa.');
                this.updateListUI(); this.drawPlan();
            } catch(error) { LogSystem.add('ERROR',`No se pudo iniciar regreso Ockham: ${error.message}`); }
        },
        cancelOckhamReturn() {
            const box=document.getElementById('route-ockham-confirm-box');
            if(box) box.style.display='none';
            this.queue=[]; this.logicalSteps=[]; this.currentStepIndex=0;
            this.updateListUI(); this.drawPlan();
            LogSystem.add('INFO','Simulación de regreso Ockham cancelada por el operador.');
        }
    };

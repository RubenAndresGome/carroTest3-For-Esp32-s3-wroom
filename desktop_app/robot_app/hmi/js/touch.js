/* ==============================================
       TOUCH / JOYSTICK
       ============================================== */
    const Touch = {
        wrap:null,svg:null,thumb:null,dirLine:null,arc:null,
        cx:150,cy:150,maxR:108,deadR:18,
        _sendTimer:0, _lastPwmL:0, _lastPwmR:0, _leaseTimer:null, _begun:false,
        _beginPromise:null, recordingId:null, displayedRecordingId:null,
        init() {
            this.wrap=document.getElementById('joystick-wrapper');
            this.svg=document.getElementById('joystick-svg');
            this.thumb=document.getElementById('joy-thumb');
            this.dirLine=document.getElementById('joy-dir-line');
            this.arc=document.getElementById('joy-speed-arc');

            // Soporte universal para Pointer Events (Android WebView, Chrome, Edge)
            if (window.PointerEvent) {
                this.wrap.addEventListener('pointerdown', e => {
                    this.start(e);
                    try { this.wrap.setPointerCapture(e.pointerId); } catch (_) {}
                });
                window.addEventListener('pointermove', e => { if (state.joy.active) this.move(e); });
                window.addEventListener('pointerup', e => {
                    if (state.joy.active) {
                        try { this.wrap.releasePointerCapture(e.pointerId); } catch (_) {}
                        this.end(e);
                    }
                });
                window.addEventListener('pointercancel', e => {
                    if (state.joy.active) {
                        try { this.wrap.releasePointerCapture(e.pointerId); } catch (_) {}
                        this.end(e);
                    }
                });
            } else {
                // Fallback para dispositivos táctiles legacy
                this.wrap.addEventListener('touchstart', e => this.start(e), { passive: false });
                window.addEventListener('touchmove', e => { if (state.joy.active) this.move(e); }, { passive: false });
                window.addEventListener('touchend', e => { if (state.joy.active) this.end(e); });
                window.addEventListener('touchcancel', e => { if (state.joy.active) this.end(e); });
                this.wrap.addEventListener('mousedown', e => this.start(e));
                window.addEventListener('mousemove', e => { if (state.joy.active) this.move(e); });
                window.addEventListener('mouseup', e => { if (state.joy.active) this.end(e); });
            }

            document.getElementById('btn-touch-return').addEventListener('click',()=>this.returnRecording());
            document.getElementById('btn-touch-delete').addEventListener('click',()=>this.deleteRecording());
            document.getElementById('touch-open-area-ack').addEventListener('change',e=>{
                document.getElementById('btn-touch-return').disabled=!this.recordingId||!e.target.checked;
            });
            this.renderRecording(null);
            this.setCapability(false);
        },
        setCapability(enabled) {
            const tab=document.getElementById('tab-control-touch'), help=document.getElementById('touch-unavailable-help'), status=document.getElementById('touch-capability');
            if(!tab)return;
            tab.setAttribute('aria-disabled',enabled?'false':'true');
            tab.classList.toggle('unavailable',!enabled);
            tab.innerHTML = `<svg class="ui-icon" aria-hidden="true" viewBox="0 0 24 24"><rect x="7" y="2" width="10" height="20" rx="2"/><path d="M10 18h4"/>${enabled ? '' : '<path d="M4 4l16 16"/>'}</svg>${enabled ? 'Control Touch' : 'Control Touch (no disponible)'}`;
            tab.title=enabled?'Control manual habilitado por manual_drive_v1.':'No disponible: la telemetría no anuncia manual_drive_v1.';
            if(help)help.textContent=enabled?'Control Touch habilitado por la capacidad manual_drive_v1 anunciada por el robot.':'Control Touch está deshabilitado porque la telemetría no anuncia manual_drive_v1.';
            if(status){status.textContent=enabled?'manual_drive_v1 anunciado: control Touch disponible.':'Esperando telemetría: Touch requiere manual_drive_v1.'; status.classList.toggle('ready',enabled); status.classList.toggle('blocked',!enabled);}
            if(!enabled&&state.joy.active)this.end();
        },
        _pos(e) {
            const t = (e.touches && e.touches[0]) || (e.changedTouches && e.changedTouches[0]);
            const cx = t ? t.clientX : e.clientX;
            const cy = t ? t.clientY : e.clientY;
            const r = this.svg.getBoundingClientRect();
            const s = r.width > 0 ? 300 / r.width : 1;
            return { x: (cx - r.left) * s, y: (cy - r.top) * s };
        },
        _sendManual(pwmL, pwmR, dirL, dirR, force=false) {
            const l = dirL==='rev' ? -pwmL : dirL==='stop' ? 0 : pwmL;
            const r = dirR==='rev' ? -pwmR : dirR==='stop' ? 0 : pwmR;
            this._lastPwmL = l; this._lastPwmR = r;
            const now = Date.now();
            if (!force && now - this._sendTimer < 80) return;
            this._sendTimer = now;
            if(!this._begun)return;
            WSControl.manual('drive',{left:l,right:r}).catch(error=>{
                if(error.status===409&&state.joy.active)this.end();
                LogSystem.add('ERROR',`Control Touch: ${error.message}`);
            });
        },
        start(e) {
            if (e.cancelable) e.preventDefault();
            if (state.joy.active) {
                // Si había un arrastre previo colgado, limpiar estado antes de la nueva captura
                this.end();
            }
            if(!state.manualDriveSupported || !Calibration.calibrated || !state.connected || state.activeTab!=='touch')return LogSystem.add('WARN','Control Touch requiere la capacidad manual_drive_v1 anunciada por telemetría.');
            if(!['IDLE','MANUAL'].includes(state.estado))return LogSystem.add('WARN',`Joystick bloqueado mientras el robot está en ${state.estado}.`);
            if(e.touches && e.touches.length > 1)return;
            state.joy.active=true; this.wrap.classList.add('active');
            document.body.classList.add('is-touch-interacting');
            this.move(e);
            this._beginPromise=WSControl.manual('start').then(recording=>{
                this.recordingId=recording.id;
                document.getElementById('touch-recording-status').textContent='Grabando pose y encoders a 10 Hz…';
                if(state.joy.active){
                    this._begun=true;
                    this._sendManual(Math.abs(this._lastPwmL),Math.abs(this._lastPwmR),this._lastPwmL<0?'rev':this._lastPwmL>0?'fwd':'stop',this._lastPwmR<0?'rev':this._lastPwmR>0?'fwd':'stop',true);
                    clearInterval(this._leaseTimer);
                    this._leaseTimer=setInterval(()=>{
                        if(state.joy.active)this._sendManual(Math.abs(this._lastPwmL),Math.abs(this._lastPwmR),this._lastPwmL<0?'rev':this._lastPwmL>0?'fwd':'stop',this._lastPwmR<0?'rev':this._lastPwmR>0?'fwd':'stop',true);
                    },100);
                }
                return recording;
            }).catch(error=>{
                state.joy.active=false;this._begun=false;this._resetVis();
                document.body.classList.remove('is-touch-interacting');
                clearInterval(this._leaseTimer);this._leaseTimer=null;
                LogSystem.add('ERROR',`No se pudo iniciar Control Touch: ${error.message}`);
                return null;
            });
        },
        move(e) {
            if(!state.joy.active)return;
            if(e.cancelable) e.preventDefault();
            if(e.touches && e.touches.length > 1){this.end();return;}
            const p=this._pos(e); let dx=p.x-this.cx, dy=p.y-this.cy, dist=Math.sqrt(dx*dx+dy*dy);
            if(dist>this.maxR){dx=(dx/dist)*this.maxR;dy=(dy/dist)*this.maxR;}
            const tx=this.cx+dx, ty=this.cy+dy;
            this.thumb.setAttribute('cx',tx);this.thumb.setAttribute('cy',ty);
            this.wrap.querySelector('.joy-thumb-outer').setAttribute('cx',tx);this.wrap.querySelector('.joy-thumb-outer').setAttribute('cy',ty);
            this.dirLine.setAttribute('x2',tx);this.dirLine.setAttribute('y2',ty);
            const cd=Math.min(dist,this.maxR), nx=cd>0?dx/this.maxR:0, ny=cd>0?dy/this.maxR:0;
            let mag=cd/this.maxR; if(cd<this.deadR)mag=0;
            const ang=Math.atan2(dx,-dy)*180/Math.PI, fwd=-ny, turn=nx;
            let lM=fwd+turn, rM=fwd-turn, ma=Math.max(Math.abs(lM),Math.abs(rM),0.001);
            if(ma>1){lM/=ma;rM/=ma;}
            const rL=Math.round(lM*mag*230), rR=Math.round(rM*mag*230);
            const dL=rL>2?'fwd':rL<-2?'rev':'stop', dR=rR>2?'fwd':rR<-2?'rev':'stop';
            const pL=Math.min(Math.abs(rL),230), pR=Math.min(Math.abs(rR),230);
            Object.assign(state.joy,{nx,ny,magnitude:mag,angle:ang,pwmL:pL,pwmR:pR,dirL:dL,dirR:dR});
            const circ=2*Math.PI*118;
            this.arc.setAttribute('stroke-dasharray',`${mag*circ*0.75} ${circ}`);
            this.arc.setAttribute('transform',`rotate(${ang-90} ${this.cx} ${this.cy})`);
            this._bars(pL,pR,dL,dR);
            document.getElementById('joy-speed-val').textContent=`${Math.round(mag*100)}%`;
            document.getElementById('joy-dir-val').textContent=this._dirName(ang,mag);
            this._sendManual(pL, pR, dL, dR);
        },
        end(e) {
            const debeDetener = state.joy.active || this._leaseTimer !== null;
            state.joy.active=false; this.wrap.classList.remove('active'); this._resetVis();
            document.body.classList.remove('is-touch-interacting');
            clearInterval(this._leaseTimer); this._leaseTimer=null;
            this._bars(0,0,'stop','stop');
            document.getElementById('joy-speed-val').textContent='0%';
            document.getElementById('joy-dir-val').textContent='EN REPOSO';
            this._lastPwmL = 0; this._lastPwmR = 0;
            this._begun=false;
            const begin=this._beginPromise;this._beginPromise=null;
            if(debeDetener){
                // Corte inmediato de motores sin esperar round-trip HTTP
                WSControl.manual('drive',{left:0,right:0}).catch(()=>{});
                Promise.resolve(begin).catch(()=>null)
                    .then(()=>WSControl.manual('stop'))
                    .then(recording=>this.renderRecording(recording))
                    .catch(error=>LogSystem.add('ERROR',`No se pudo cerrar Control Touch: ${error.message}`));
            }
        },
        renderRecording(recording) {
            const canvas=document.getElementById('touch-recording-preview'),ctx=canvas.getContext('2d');
            const width=canvas.width,height=canvas.height,pad=24;
            ctx.clearRect(0,0,width,height);ctx.fillStyle='#09111b';ctx.fillRect(0,0,width,height);
            ctx.strokeStyle='rgba(120,145,170,.22)';ctx.lineWidth=1;
            for(let x=pad;x<width;x+=80){ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,height);ctx.stroke();}
            for(let y=pad;y<height;y+=40){ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(width,y);ctx.stroke();}
            const samples=recording?.samples||[],points=recording?.points||[],segments=recording?.compiled_segments||[];
            const all=samples.length?samples:points;
            if(all.length){
                const xs=all.map(p=>Number(p.x_mm)),ys=all.map(p=>Number(p.y_mm));
                let minX=Math.min(...xs),maxX=Math.max(...xs),minY=Math.min(...ys),maxY=Math.max(...ys);
                if(maxX===minX){maxX+=1;minX-=1;} if(maxY===minY){maxY+=1;minY-=1;}
                const scale=Math.min((width-2*pad)/(maxX-minX),(height-2*pad)/(maxY-minY));
                const map=p=>({x:pad+(Number(p.x_mm)-minX)*scale,y:height-pad-(Number(p.y_mm)-minY)*scale});
                const draw=(list,color,dash,w)=>{if(list.length<2)return;ctx.save();ctx.strokeStyle=color;ctx.lineWidth=w;ctx.setLineDash(dash);ctx.beginPath();list.forEach((p,i)=>{const q=map(p);i?ctx.lineTo(q.x,q.y):ctx.moveTo(q.x,q.y);});ctx.stroke();ctx.restore();};
                draw(samples,'#00b0ff',[],2); draw(points,'#d05cff',[],3);
                if(segments.length){const physical=[{x_mm:segments[0].start_x_mm,y_mm:segments[0].start_y_mm},...segments];draw(physical,'#ffab00',[8,6],2);}
            }
            document.getElementById('touch-recording-samples').textContent=String(recording?.sample_count??samples.length);
            document.getElementById('touch-recording-vectors').textContent=String(recording?.logical_steps?.length||0);
            document.getElementById('touch-recording-segments').textContent=`${segments.length} / 256`;
            document.getElementById('touch-recording-tolerance').textContent=recording?.simplification_tolerance_mm!=null?`${Number(recording.simplification_tolerance_mm).toFixed(1)} mm`:'—';
            const valid=recording?.status==='completed'&&segments.length>0;
            this.displayedRecordingId=recording?.id||null;
            this.recordingId=valid?recording.id:null;
            document.getElementById('touch-open-area-ack').checked=false;
            document.getElementById('btn-touch-return').disabled=true;
            document.getElementById('btn-touch-delete').disabled=!recording?.id;
            document.getElementById('touch-recording-status').textContent=!recording?'Sin grabación todavía.':valid?'Vista previa lista: morado conceptual, cian medido y ámbar X→Y físico. Confirma el área para regresar.':`Grabación no ejecutable: ${recording.invalid_reason||recording.status}`;
        },
        async returnRecording() {
            if(!this.recordingId||!document.getElementById('touch-open-area-ack').checked)return;
            try{
                await WSControl.api(`/api/v1/touch-recordings/${this.recordingId}/acknowledge-open-area`,{method:'POST',body:'{}'});
                const mission=await WSControl.api(`/api/v1/touch-recordings/${this.recordingId}/return`,{method:'POST',body:'{}'});
                RouteManager.currentOrigin={x:state.telemetria.posX,y:state.telemetria.posY};
                RouteManager.logicalSteps=[];
                RouteManager.queue=(mission.planned_points||[]).map((point,index)=>({
                    x:Number(point.x_mm)/10,y:Number(point.y_mm)/10,
                    logicalStepId:index+1,component:point.component||'return',desc:'Regreso Touch en reversa'
                }));
                RouteManager.onMission(mission); TabManager.switch('rutas');RouteManager.autoScale();
                LogSystem.add('CMD','Regreso Touch iniciado en reversa sobre los pasos X→Y compilados.');
            }catch(error){LogSystem.add('ERROR',`No se pudo iniciar el regreso Touch: ${error.message}`);}
        },
        async deleteRecording() {
            const id=this.displayedRecordingId;
            if(!id||!confirm('¿Eliminar esta grabación Touch?'))return;
            try{await WSControl.api(`/api/v1/touch-recordings/${id}`,{method:'DELETE'});this.recordingId=null;this.renderRecording(null);}
            catch(error){LogSystem.add('ERROR',`No se pudo eliminar la grabación: ${error.message}`);}
        },
        _resetVis() {
            this.thumb.setAttribute('cx',this.cx);this.thumb.setAttribute('cy',this.cy);
            this.wrap.querySelector('.joy-thumb-outer').setAttribute('cx',this.cx);this.wrap.querySelector('.joy-thumb-outer').setAttribute('cy',this.cy);
            this.dirLine.setAttribute('x2',this.cx);this.dirLine.setAttribute('y2',this.cy);
            const c=2*Math.PI*118;this.arc.setAttribute('stroke-dasharray',`0 ${c}`);
        },
        _bars(pL,pR,dL,dR) {
            const sc=100/230;
            const fL=document.getElementById('bar-left-fill'), fR=document.getElementById('bar-right-fill');
            if(dL==='fwd'){const h=pL*sc;fL.setAttribute('y',120-h);fL.setAttribute('height',h);fL.setAttribute('class','bar-fill-fwd');}
            else if(dL==='rev'){const h=pL*sc;fL.setAttribute('y',120);fL.setAttribute('height',h);fL.setAttribute('class','bar-fill-rev');}
            else{fL.setAttribute('y',120);fL.setAttribute('height',0);}
            if(dR==='fwd'){const h=pR*sc;fR.setAttribute('y',120-h);fR.setAttribute('height',h);fR.setAttribute('class','bar-fill-fwd');}
            else if(dR==='rev'){const h=pR*sc;fR.setAttribute('y',120);fR.setAttribute('height',h);fR.setAttribute('class','bar-fill-rev');}
            else{fR.setAttribute('y',120);fR.setAttribute('height',0);}
            document.getElementById('bar-left-val').textContent=pL;
            document.getElementById('bar-right-val').textContent=pR;
            document.getElementById('bar-left-dir').textContent=dL;
            document.getElementById('bar-right-dir').textContent=dR;
        },
        _dirName(a,m) {
            if(m<0.05)return'EN REPOSO';
            if(a>-22.5&&a<=22.5)return'AVANCE';if(a>22.5&&a<=67.5)return'AVANCE + DER';
            if(a>67.5&&a<=112.5)return'DERECHA';if(a>112.5&&a<=157.5)return'REVERSA + DER';
            if(a>157.5||a<=-157.5)return'REVERSA';if(a>-157.5&&a<=-112.5)return'REVERSA + IZQ';
            if(a>-112.5&&a<=-67.5)return'IZQUIERDA';if(a>-67.5&&a<=-22.5)return'AVANCE + IZQ';return'EN REPOSO';
        }
    };

/* ==============================================
       SISTEMA DE LOGS Y UI S3 (TELEMETRIA, MOTORES)
       ============================================== */
    const LogSystem = {
        entries: [], container: null, emptyMsg: null,
        init() { this.container = document.getElementById('log-container'); this.emptyMsg = document.getElementById('log-empty'); },
        updateCount() {
            const count = document.getElementById('log-count');
            count.textContent = this.entries.length;
            count.setAttribute('aria-label', `${this.entries.length} registros`);
        },
        add(level, message) {
            const now = new Date();
            const ts = now.toTimeString().slice(0,8) + '.' + String(now.getMilliseconds()).padStart(3,'0');
            this.entries.push({ timestamp:ts, level, message });
            if (this.entries.length > CONFIG.LOG_MAX) this.entries.shift();
            if (this.emptyMsg) this.emptyMsg.style.display = 'none';
            const levelKey = level.toLowerCase();
            const div = document.createElement('div');
            div.className = `log-entry log-${levelKey}`;
            const time = document.createElement('time');
            time.className = 'log-time';
            time.textContent = ts;
            const badge = document.createElement('span');
            badge.className = `log-level ll-${levelKey}`;
            badge.textContent = level;
            const detail = document.createElement('span');
            detail.className = 'log-msg';
            detail.textContent = message;
            div.append(time, badge, detail);
            if (!state.logFilters[levelKey]) div.style.display = 'none';
            this.container.appendChild(div);
            if (this.container.scrollHeight - this.container.scrollTop - this.container.clientHeight < 50)
                this.container.scrollTop = this.container.scrollHeight;
            while (this.container.children.length > CONFIG.LOG_MAX + 5)
                if (this.container.children[0] !== this.emptyMsg) this.container.removeChild(this.container.children[0]);
            this.updateCount();
        },
        clear() {
            this.entries = [];
            while (this.container.children.length > 1) this.container.removeChild(this.container.lastChild);
            if (this.emptyMsg) this.emptyMsg.style.display = 'block';
            this.updateCount();
        },
        setFilter(l, a) { state.logFilters[l] = a; this.container.querySelectorAll(`.log-entry.log-${l}`).forEach(e => { e.style.display = a ? '' : 'none'; }); },
        downloadTXT() {
            if (!this.entries.length) return;
            const h = ['========','Robot BETA Logs','========',`Descarga: ${new Date().toLocaleString()}`,`Entradas: ${this.entries.length}`,`Estado: ${state.estado}`,'========',''].join('\n');
            const b = new Blob([h + this.entries.map(e => `[${e.timestamp}] [${e.level.padEnd(5)}] ${e.message}`).join('\n')], { type:'text/plain;charset=utf-8' });
            const a = document.createElement('a'); a.href = URL.createObjectURL(b);
            a.download = `robot_logs_${new Date().toISOString().slice(0,19).replace(/:/g,'-')}.txt`;
            document.body.appendChild(a); a.click(); document.body.removeChild(a);
        }
    };

    const MotorViz = {
        fl:{side:'left'}, fr:{side:'right'}, bl:{side:'left'}, br:{side:'right'}, lastTime:0,
        init() {
            for (const s of ['fl','fr','bl','br']) {
                const m = this[s]; m.angle = 0;
                m.el = document.getElementById(`rotor-${s}`);
                m.svg = document.getElementById(`svg-${s}`);
                m.rpm = document.getElementById(`rpm-${s}`);
                m.pwm = document.getElementById(`pwm-${s}`);
                m.msl = document.getElementById(`msl-${s}`);
                this._ticks(`ticks-${s}`, 24, 78, 88);
            }
            this.lastTime = performance.now();
            requestAnimationFrame(t => this.loop(t));
        },
        _ticks(gId, n, rI, rO) {
            const g = document.getElementById(gId);
            for (let i = 0; i < n; i++) {
                const a = (i*360/n)*Math.PI/180, mj = i%(n/4)===0, r1 = mj ? rI-2 : rI+2;
                const l = document.createElementNS('http://www.w3.org/2000/svg','line');
                l.setAttribute('x1',100+r1*Math.sin(a)); l.setAttribute('y1',100-r1*Math.cos(a));
                l.setAttribute('x2',100+rO*Math.sin(a)); l.setAttribute('y2',100-rO*Math.cos(a));
                l.setAttribute('class', mj ? 'tick-major' : 'tick-minor'); g.appendChild(l);
            }
        },
        loop(now) {
            const dt = Math.min((now-this.lastTime)/1000, 0.1); this.lastTime = now;
            const useJoy = state.activeTab === 'touch' && state.joy.active;

            let currentMotorState = { left: {pwm:0, dir:'stop'}, right: {pwm:0, dir:'stop'} };

            if (useJoy) {
                currentMotorState = { left:{pwm:state.joy.pwmL, dir:state.joy.dirL}, right:{pwm:state.joy.pwmR, dir:state.joy.dirR} };
            } else if (state.connected) {
                const pwmL = state.telemetria.pwmL || 0, pwmR = state.telemetria.pwmR || 0;
                currentMotorState = {
                    left:  { pwm: Math.abs(pwmL), dir: pwmL>0?'fwd':pwmL<0?'rev':'stop' },
                    right: { pwm: Math.abs(pwmR), dir: pwmR>0?'fwd':pwmR<0?'rev':'stop' }
                };
            } else {
                if (state.estado === 'AVANZANDO') currentMotorState = { left:{pwm:150, dir:'fwd'}, right:{pwm:150, dir:'fwd'} };
                else if (state.estado === 'GIRANDO') currentMotorState = { left:{pwm:150, dir:'fwd'}, right:{pwm:150, dir:'rev'} };
            }

            for (const s of ['fl','fr','bl','br']) {
                const m = this[s], {pwm, dir} = currentMotorState[m.side];
                m.pwm.textContent = `PWM: ${Math.round(pwm)}`;
                if (pwm > 0 && dir !== 'stop') {
                    const rpm = CONFIG.MIN_RPM_VISUAL + (pwm/230)*(CONFIG.MAX_RPM_VISUAL-CONFIG.MIN_RPM_VISUAL);
                    m.angle += (rpm*360/60)*dt*(dir==='fwd'?1:-1); m.angle %= 360;
                    m.el.setAttribute('transform',`rotate(${m.angle},100,100)`);
                    m.rpm.textContent = `${Math.round(rpm)} RPM`;
                    m.svg.classList.remove('active-green','active-orange','active-red');
                    m.el.parentElement.classList.remove('state-reverse','state-error');
                    if (dir==='fwd') m.svg.classList.add('active-green');
                    else { m.svg.classList.add('active-orange'); m.el.parentElement.classList.add('state-reverse'); }
                    m.msl.className = 'motor-state-label '+(dir==='fwd'?'msl-forward':'msl-reverse');
                    m.msl.textContent = dir==='fwd'?'AVANCE':'REVERSA';
                } else {
                    m.rpm.textContent = '0 RPM';
                    m.svg.classList.remove('active-green','active-orange','active-red');
                    m.el.parentElement.classList.remove('state-reverse','state-error');
                    m.msl.className='motor-state-label msl-idle'; m.msl.textContent='DETENIDO';
                }
            }
            requestAnimationFrame(t => this.loop(t));
        }
    };

    const EncoderSpeed = {
        cmPerPulse: (Math.PI * 6.6) / 20,
        alpha: 0.3,
        last: null,
        updatedAt: 0,
        values: { fl:0, fr:0, bl:0, br:0 },
        init() { setInterval(() => this.render(), 500); },
        update(t) {
            const now = performance.now();
            const counts = { fl:t.pfl||0, fr:t.pfr||0, bl:t.pbl||0, br:t.pbr||0 };
            if (this.last) {
                const dt = (now - this.last.time) / 1000;
                if (dt >= 0.05 && dt <= 2) {
                    for (const wheel of ['fl','fr','bl','br']) {
                        const delta = counts[wheel] - this.last.counts[wheel];
                        const sidePwm = wheel.endsWith('l') ? t.pwmL : t.pwmR;
                        const sign = sidePwm < 0 ? -1 : sidePwm > 0 ? 1 : 0;
                        const raw = delta >= 0 ? (delta * this.cmPerPulse / dt) * sign : 0;
                        this.values[wheel] = this.alpha * raw + (1 - this.alpha) * this.values[wheel];
                    }
                } else {
                    this.values = { fl:0, fr:0, bl:0, br:0 };
                }
            }
            this.last = { time:now, counts };
            this.updatedAt = Date.now();
            this.render();
        },
        render() {
            const t = state.telemetria;
            const fresh = state.connected && Date.now() - this.updatedAt <= 2000;
            const list = document.getElementById('encoder-live-list');
            if (!list) return;
            list.classList.toggle('stale', !fresh);
            for (const wheel of ['fl','fr','bl','br']) {
                const sidePwm = wheel.endsWith('l') ? t.pwmL : t.pwmR;
                const dir = sidePwm > 0 ? 'fwd' : sidePwm < 0 ? 'rev' : 'stop';
                document.getElementById(`encoder-row-${wheel}`).dataset.dir = dir;
                document.getElementById(`encoder-pulses-${wheel}`).textContent = `${t['p'+wheel] || 0} pulsos`;
                document.getElementById(`encoder-speed-${wheel}`).innerHTML = fresh ? `${Math.abs(this.values[wheel]).toFixed(1)} <small>cm/s</small>` : `-- <small>cm/s</small>`;
            }
            const meanSpeed = Object.values(this.values).reduce((sum,v)=>sum+Math.abs(v),0) / 4;
            document.getElementById('touch-speed-value').textContent = fresh ? meanSpeed.toFixed(1) : '--';
        }
    };

    const Telemetry = {
        _process(t) {
            state.telemetria = { ...state.telemetria, ...t };
            
            const ne = t.estado || 'IDLE';
            if (ne !== state.estado) {
                FSMDisplay.update(ne); 
                LogSystem.add('INFO',`FSM: ${state.estado} -> ${ne}`);
                state.estadoAnterior = state.estado; state.estado = ne;
                if (ne === 'IDLE') {
                    if (typeof RouteManager !== 'undefined') RouteManager.onFsmIdle();
                }
            }
            if (typeof RouteManager !== 'undefined') {
                RouteManager.updateTelemetryMap(t.posX, t.posY, t.orientacion);
            }
            this._uiTelem(); this._uiPos();
        },
        _uiTelem() {
            const t = state.telemetria;
            document.getElementById('telem-dist').textContent=`${t.distancia.toFixed(1)} cm`;
            document.getElementById('telem-angle').textContent=`${t.angulo.toFixed(1)}°`;
            document.getElementById('telem-orient').textContent=`${t.orientacion.toFixed(2)} rad`;
            document.getElementById('telem-enc-l').textContent=`F:${t.pfl} B:${t.pbl}`;
            document.getElementById('telem-enc-r').textContent=`F:${t.pfr} B:${t.pbr}`;
            document.getElementById('telem-pwm-l').textContent=`${t.pwmL}`;
            document.getElementById('telem-pwm-r').textContent=`${t.pwmR}`;
            document.getElementById('telem-clients').textContent=state.connected?'1':'0';
            document.getElementById('pulses-fl').textContent=`Pulsos: ${t.pfl || 0}`;
            document.getElementById('pulses-fr').textContent=`Pulsos: ${t.pfr || 0}`;
            document.getElementById('pulses-bl').textContent=`Pulsos: ${t.pbl || 0}`;
            document.getElementById('pulses-br').textContent=`Pulsos: ${t.pbr || 0}`;
            const angleZ = Number.isFinite(t.angulo) ? t.angulo : 0;
            document.getElementById('gyro-angle').textContent=`${angleZ.toFixed(1)}°`;
            document.getElementById('gyro-x').textContent=`${angleZ.toFixed(1)}°`;
            document.getElementById('gyro-y').textContent=`${(t.orientacion * 180 / Math.PI).toFixed(1)}°`;
            document.getElementById('gyro-dial').style.setProperty('--gyro-angle', `${angleZ}deg`);
            document.getElementById('touch-angle-value').textContent=`${angleZ.toFixed(1)}°`;
            document.getElementById('touch-live-state').textContent=state.estado;
            document.getElementById('chart-status-x').textContent=t.posX.toFixed(1);
            document.getElementById('chart-status-y').textContent=t.posY.toFixed(1);
            document.getElementById('chart-status-angle').textContent=`${angleZ.toFixed(1)}°`;
            document.getElementById('chart-status-state').textContent=state.estado;
            document.getElementById('route-live-x').textContent=`${t.posX.toFixed(1)} cm`;
            document.getElementById('route-live-y').textContent=`${t.posY.toFixed(1)} cm`;
            document.getElementById('route-live-yaw').textContent=`${angleZ.toFixed(1)}°`;
            document.getElementById('route-live-state').textContent=state.estado;
            document.getElementById('route-live-dist').textContent=`${t.distancia.toFixed(1)} cm`;
            document.getElementById('route-live-pwm').textContent=`${t.pwmL} / ${t.pwmR}`;
            const target=t.target||{}, control=t.driveControl||{};
            document.getElementById('route-live-error').textContent=target.absolute
                ? `Av ${Number(target.longitudinal_error_cm||0).toFixed(1)} · L ${Number(target.lateral_error_cm||0).toFixed(1)} · D ${Number(target.distance_error_cm||0).toFixed(1)} cm · #${Number(target.endpoint_attempt||0)}${target.finish_reason?` · ${target.finish_reason}`:''}`
                : 'paso relativo';
            document.getElementById('route-live-control').textContent=`${Number(control.p||0).toFixed(1)} / ${Number(control.i||0).toFixed(1)} / ${Number(control.d||0).toFixed(1)}`;
            document.getElementById('route-live-heading').textContent=`${Number(control.dynamic_heading_deg||0).toFixed(1)}° / ${Number(control.heading_error_deg||0).toFixed(1)}°`;
            document.getElementById('route-live-assist').textContent=`${Number(control.encoder_pwm||0).toFixed(1)} / ${Number(control.right_compensation||1).toFixed(2)} · yaw ${control.heading_brake_side||'none'}`;
            const motion=t.motion||{};
            const rampaReversa=motion.reverse_ramp_active?` · rampa R ${Number(motion.reverse_ramp_elapsed_ms||0)} ms`:'';
            document.getElementById('route-live-motion').textContent=`${motion.effective_mode||motion.requested_mode||'forward'} · trayecto ${Number(motion.travel_heading_deg||0).toFixed(1)}° · chasis ${Number(motion.body_heading_deg||0).toFixed(1)}° · final ${Number(motion.final_heading_deg||0).toFixed(1)}°${rampaReversa}`;
            document.getElementById('route-live-brake').textContent=`pred ${Number(motion.brake_prediction_cm||0).toFixed(1)} · arrastre ${Number(motion.coast_cm||0).toFixed(1)} cm · asent. ${Number(motion.settle_elapsed_ms||0)} ms`;
            const recovery=t.recovery||{};
            document.getElementById('route-live-endpoint').textContent=`${recovery.decision||'none'} · error ${Number(recovery.distance_cm||0).toFixed(1)} / mín ${Number(recovery.min_distance_cm||0).toFixed(1)} cm`;
            document.getElementById('route-live-recovery').textContent=recovery.pivot_avoided
                ? `${recovery.direction||'reverse'} · sin pivote`
                : `${recovery.direction||'forward'} · pivote sólo si hace falta`;
            document.getElementById('route-live-fl').textContent=t.pfl || 0;
            document.getElementById('route-live-fr').textContent=t.pfr || 0;
            document.getElementById('route-live-bl').textContent=t.pbl || 0;
            EncoderSpeed.update(t);
            if (typeof BoostHUD !== 'undefined') BoostHUD.update(t);
        },
        _uiPos() {
            const t = state.telemetria;
            document.getElementById('pos-dot').setAttribute('cx',t.posX);
            document.getElementById('pos-dot').setAttribute('cy',-t.posY);
            const hl=25;
            document.getElementById('pos-heading').setAttribute('x1',t.posX);
            document.getElementById('pos-heading').setAttribute('y1',-t.posY);
            document.getElementById('pos-heading').setAttribute('x2',t.posX+hl*Math.sin(t.orientacion));
            document.getElementById('pos-heading').setAttribute('y2',-t.posY-hl*Math.cos(t.orientacion));
            state.trail.push({x:t.posX,y:-t.posY});
            if (state.trail.length>CONFIG.TRAIL_MAX) state.trail.shift();
            if (state.trail.length%3===0) document.getElementById('pos-trail').setAttribute('points',state.trail.map(p=>`${p.x},${p.y}`).join(' '));
            document.getElementById('pos-x').textContent=t.posX.toFixed(1);
            document.getElementById('pos-y').textContent=t.posY.toFixed(1);
        },
        _ui() {
            document.getElementById('conn-dot').classList.toggle('ok',state.connected);
            document.getElementById('conn-text').textContent=state.connected?'Conectado':'Desconectado';
            EncoderSpeed.render();
        }
    };

    const FSMDisplay = {
        update(s) {
            document.querySelectorAll('.fsm-node').forEach(n => {
                n.classList.remove('active','active-reverse','active-fault','active-estop');
                n.removeAttribute('aria-current');
                if (n.dataset.state===s) {
                    n.classList.add(s==='REVERSE'?'active-reverse':s==='ESTOP'?'active-estop':s==='FALLO'?'active-fault':'active');
                    n.setAttribute('aria-current', 'step');
                }
            });
            const el=document.getElementById('telem-state'); el.textContent=s;
            el.className='telem-value state-val '+({INIT:'tv-init',IDLE:'tv-idle',AVANZANDO:'tv-advancing',GIRANDO:'tv-turning',ESPERANDO:'tv-calibrating',TERMINADO:'tv-init',FALLO:'tv-fault',ESTOP:'tv-estop'}[s]||'tv-idle');
        }
    };

    const BoostHUD = {
        burstDurationMs: 0,
        cooldownRemainingMs: 0,
        lastTime: performance.now(),

        update(t) {
            const now = performance.now();
            const dt = Math.min(now - this.lastTime, 200);
            this.lastTime = now;

            const pwmL = Math.abs(t.pwmL || 0);
            const pwmR = Math.abs(t.pwmR || 0);
            const maxPwm = Math.max(pwmL, pwmR);
            const isBurst = maxPwm > 242;

            if (isBurst) {
                this.burstDurationMs = Math.min(80, this.burstDurationMs + dt);
                this.cooldownRemainingMs = 100;
            } else {
                this.burstDurationMs = Math.max(0, this.burstDurationMs - dt * 0.6);
                if (this.cooldownRemainingMs > 0) {
                    this.cooldownRemainingMs = Math.max(0, this.cooldownRemainingMs - dt);
                }
            }

            const burstBar = document.getElementById('boost-burst-bar');
            const burstTime = document.getElementById('boost-burst-time');
            const cooldownStatus = document.getElementById('boost-cooldown-status');
            if (burstBar) {
                const burstPct = Math.min(100, (this.burstDurationMs / 80) * 100);
                burstBar.style.width = `${burstPct.toFixed(0)}%`;
                burstBar.classList.toggle('bursting', isBurst);
                if (burstTime) burstTime.textContent = `${this.burstDurationMs.toFixed(0)} / 80 ms`;
                if (cooldownStatus) {
                    cooldownStatus.textContent = this.cooldownRemainingMs > 0
                        ? `Cooldown: ${(this.cooldownRemainingMs / 1000).toFixed(1)}s`
                        : 'Cooldown listo';
                    cooldownStatus.style.color = this.cooldownRemainingMs > 0 ? 'var(--warn)' : 'var(--accent)';
                }
            }

            const diffVal = document.getElementById('boost-diff-val');
            const leftBar = document.getElementById('boost-left-bar');
            const rightBar = document.getElementById('boost-right-bar');
            const leftLabel = document.getElementById('boost-left-label');
            const rightLabel = document.getElementById('boost-right-label');

            if (diffVal) {
                const diff = Math.abs(pwmL - pwmR);
                diffVal.textContent = `Δ ${diff.toFixed(0)} PWM`;
                if (leftBar) leftBar.style.width = `${Math.min(100, (pwmL / 255) * 100).toFixed(0)}%`;
                if (rightBar) rightBar.style.width = `${Math.min(100, (pwmR / 255) * 100).toFixed(0)}%`;
                if (leftLabel) leftLabel.textContent = `L: ${pwmL} PWM`;
                if (rightLabel) rightLabel.textContent = `R: ${pwmR} PWM`;
            }

            const p1 = document.getElementById('boost-phase-1');
            const p2 = document.getElementById('boost-phase-2');
            const p3 = document.getElementById('boost-phase-3');
            if (p1 && p2 && p3) {
                const isTurning = state.estado === 'GIRANDO';
                const angleZ = Number.isFinite(t.angulo) ? Math.abs(t.angulo) : 0;
                if (isTurning && isBurst) {
                    p1.classList.add('active'); p2.classList.remove('active'); p3.classList.remove('active');
                } else if (isTurning) {
                    p1.classList.remove('active'); p2.classList.add('active'); p3.classList.remove('active');
                } else {
                    p1.classList.remove('active'); p2.classList.remove('active'); p3.classList.add('active');
                }
            }
        }
    };

    window.BoostHUD = BoostHUD;

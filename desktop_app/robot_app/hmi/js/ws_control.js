/* ==============================================
       SISTEMA DE WEBSOCKET (Sustituye HTTP Fetch)
       ============================================== */
    const WSControl = {
        events: null,
        lastRtosKey: '',
        lastYawRecenterCount: 0,
        appToken: document.querySelector('meta[name="app-token"]')?.content || '',
        get flaskMode() { return Boolean(this.appToken); },
        init() {
            if (!this.flaskMode) {
                LogSystem.add('ERROR','Esta HMI debe ejecutarse desde desktop_app; el navegador no puede conectarse directamente al robot.');
                return;
            }
            this.connectBackend();
        },
        async api(path, options={}) {
            const headers = { 'Content-Type':'application/json', ...(options.headers||{}) };
            if (this.appToken) headers['X-App-Token'] = this.appToken;
            let controller = null;
            let timeoutId = null;
            const fetchOptions = { ...options, headers };
            if (options.timeoutMs && !options.signal) {
                controller = new AbortController();
                fetchOptions.signal = controller.signal;
                timeoutId = setTimeout(() => controller.abort(), options.timeoutMs);
            }
            try {
                const response = await fetch(path, fetchOptions);
                const data = await response.json().catch(()=>({}));
                if (!response.ok) {
                    const error = new Error(data.error || `HTTP ${response.status}`);
                    error.status = response.status; error.data = data;
                    throw error;
                }
                return data;
            } finally {
                if (timeoutId) clearTimeout(timeoutId);
            }
        },
        async closeApplication(force=false) {
            const button=document.getElementById('btn-close-app');
            button.disabled=true;
            try {
                const result=await this.api('/api/v1/app/close',{
                    method:'POST', body:JSON.stringify({force})
                });
                LogSystem.add(force?'WARN':'INFO',force
                    ? 'Cierre forzado registrado; la parada física no fue confirmada.'
                    : 'STOP confirmado y SQLite guardado. Cerrando aplicación...');
                if(this.events)this.events.close();
                button.textContent='Cierre seguro completado';
                if(window.RobotHost&&typeof window.RobotHost.closeApp==='function') {
                    window.RobotHost.closeApp();
                }
                return result;
            } catch(error) {
                button.disabled=false;
                if(error.status===409&&confirm('No se confirmó STOP. ¿Forzar el cierre dejando constancia en SQLite?')) {
                    return this.closeApplication(true);
                }
                LogSystem.add('ERROR',`Cierre bloqueado: ${error.message}`);
                return null;
            }
        },
        connectBackend() {
            const receive = event => {
                try { this.consumeBackend(JSON.parse(event.data)); }
                catch (error) { LogSystem.add('ERROR', `Evento local inválido: ${error.message}`); }
            };
            this.events = new EventSource('/api/v1/events');
            this.events.addEventListener('snapshot', receive);
            this.events.onmessage = receive;
            this.events.onerror = () => LogSystem.add('WARN', 'Canal SSE local interrumpido; el navegador reintentará.');
            this.api('/api/v1/status').then(snapshot => this.consumeBackend(snapshot)).catch(error => LogSystem.add('ERROR', error.message));
        },
        consumeBackend(message) {
            if (message.type) {
                if (message.type === 'telemetry') this.applyBackendTelemetry(message.payload);
                else if (message.type === 'connection') ConnectionPanel.applyConnection(message.payload);
                else if (message.type === 'session') { ConnectionPanel.applySession(message.payload); ConnectionPanel.loadHistory(); }
                else if (message.type === 'mission') RouteManager.onMission(message.payload);
                else if (message.type === 'config') RouteManager.applyConfig(message.payload);
                else if (message.type === 'robot_event') {
                    LogSystem.add('INFO', `Robot: ${JSON.stringify(message.payload)}`);
                    Calibration.onRobotEvent(message.payload);
                    RouteManager.onRobotEvent(message.payload);
                }
                else if (message.type === 'command') LogSystem.add('CMD', `${message.payload.name}: ${message.payload.status}`);
                return;
            }
            if (message.connection) ConnectionPanel.applySnapshot(message);
            if (message.telemetry) this.applyBackendTelemetry(message.telemetry);
            if (message.mission) RouteManager.onMission(message.mission);
        },
        applyBackendTelemetry(raw) {
            const capabilities=raw.capabilities||raw.features||[];
            const manualDrive=Array.isArray(capabilities)
                ? capabilities.includes('manual_drive_v1')
                : capabilities.manual_drive_v1===true || raw.manual_drive_v1===true || raw.manual_drive==='manual_drive_v1';
            state.manualDriveSupported=manualDrive;
            if(typeof Touch!=='undefined')Touch.setCapability(manualDrive);
            const telemX = Number(raw.x_mm || 0) / 10;
            const telemY = Number(raw.y_mm || 0) / 10;
            if (robotChart) { robotChart.data.datasets[1].data[0] = {x:telemX,y:telemY}; robotChart.update('none'); }
            const dx = telemX - (this._lastPos ? this._lastPos.x : 0);
            const dy = telemY - (this._lastPos ? this._lastPos.y : 0);
            if (Math.abs(dx) > .05 || Math.abs(dy) > .05) this._orientacionEstimada = Math.atan2(dy,dx);
            this._lastPos = {x:telemX,y:telemY};
            const yaw = Number(raw.heading_deg ?? raw.yaw_deg ?? 0);
            const reportedFault=raw.fault&&typeof raw.fault==='object'?raw.fault:{};
            const firmwareState = String(raw.state || (reportedFault.active?reportedFault.state:'') || '').toLowerCase();
            const activePhase = String(raw.active_command_name || '').toLowerCase();
            const uiState = firmwareState === 'listo' ? 'IDLE'
                : firmwareState === 'manual' ? 'MANUAL'
                : firmwareState === 'desarmado' ? 'INIT'
                : firmwareState === 'calibrando' ? 'ESPERANDO'
                : firmwareState === 'ejecutando' ? (activePhase.includes('avance') ? 'AVANZANDO' : 'GIRANDO')
                : firmwareState === 'estop' ? 'ESTOP'
                : ['fallo','fault','safe_stop'].includes(firmwareState) ? 'FALLO' : 'INIT';
            const pulses = raw.pulses || {};
            const pwmRaw = raw.pwm || {};
            const stall = raw.stall_accumulated_ms || {}, fusion = raw.encoder_fusion || {};
            const health = raw.encoder_health || {}, motorControl = raw.motor_control || {};
            const pwmPhysical8 = motorControl.physical || {};
            const pwm = {
                left: pwmPhysical8.left_8bit ?? pwmRaw.left ?? 0,
                right: pwmPhysical8.right_8bit ?? pwmRaw.right ?? 0
            };
            const interlockL = motorControl.interlock_left || {}, interlockR = motorControl.interlock_right || {};
            const rtos=raw.rtos||{}, stackMin=rtos.stack_min_free_bytes||{}, timing=rtos.control_timing||{};
            const turnRequestedMode=String(raw.turn_requested_mode||'AUTO');
            const turnMode=String(raw.turn_mode||'PIVOT');
            const turnSign=Number(raw.turn_attempt_sign??raw.turn_fixed_sign??0);
            const requested=Number(raw.turn_requested_deg||0), physical=Number(raw.turn_physical_target_deg||0);
            const powerL=raw.turn_left_energized?'L ON':'L OFF', powerR=raw.turn_right_energized?'R ON':'R OFF';
            document.getElementById('telem-turn-attempt').textContent=`${Number(raw.turn_attempt||0)} / ${Number(raw.turn_attempt_max||1)} · pedido ${turnRequestedMode} → efectivo ${turnMode} · ${powerL}/${powerR} · signo ${turnSign} · objetivo ${requested.toFixed(1)}°/${physical.toFixed(1)}°`;
            const cardinalOk=raw.route_precise_aligned?'cardinal ✓':'cardinal —';
            document.getElementById('telem-retry-pause').textContent=`${raw.move_phase||'OFF'} · ${raw.alignment_stage||'OFF'} · giro ${Number(raw.turn_attempt||0)}/${Number(raw.turn_attempt_max||7)} (${Number(raw.retry_pause_remaining_ms||0)}ms) · avance ${Number(raw.drive_attempt||0)}/${Number(raw.drive_attempt_max||7)} (${Number(raw.drive_retry_pause_remaining_ms||0)}ms) · ${raw.autonomous_recovery_reason||'none'} · objetivo ${Number(raw.heading_target_deg??raw.cardinal_target_deg??0).toFixed(1)}° · error ${Number(raw.heading_error_deg||0).toFixed(1)}° / pred ${Number(raw.heading_predicted_error_deg||0).toFixed(1)}° · control ${Number(raw.heading_control_pwm||0).toFixed(0)}/${Number(raw.encoder_control_pwm||0).toFixed(0)} · recuperación angular ${Number(raw.heading_recovery_attempt||0)}/${Number(raw.heading_recovery_attempt_max||7)} · yaw ${raw.turn_yaw_authority?'autoridad':'sin autoridad'} ${Number(raw.turn_imu_no_response_ms||0)}ms / signo ${Number(raw.turn_direction_mismatch_ms||0)}ms · restante eje ${Number(raw.route_axis_remaining_cm||0).toFixed(1)}cm · tol ±${Number(raw.turn_tolerance_deg||0).toFixed(1)}° · ${cardinalOk}`;
            const yawRecenterCount=Number(raw.imu_yaw_recenter_count||0);
            document.getElementById('telem-yaw-model').textContent=`${yaw.toFixed(2)}° / ${Number(raw.imu_yaw_unwrapped_deg||0).toFixed(2)}° · recentrados ${yawRecenterCount}`;
            document.getElementById('telem-turn-control').textContent=`${raw.turn_braking_phase||'OFF'} · progreso ${Number(raw.turn_progress_deg||0).toFixed(2)}° · error ${Number(raw.turn_error_deg||0).toFixed(2)}° · predicho ${Number(raw.turn_predicted_error_deg||0).toFixed(2)}° · gyro ${Number(raw.turn_gyro_deg_s||0).toFixed(1)}°/s · PWM ${Number(raw.turn_pwm_target_8bit||0)}/255 (${Number(raw.turn_pwm_target||0)}/1023)`;
            document.getElementById('route-live-arc').textContent=`${Number(raw.turn_arc_path_cm||0).toFixed(1)} cm`;
            document.getElementById('route-live-turn-xy').textContent=`${Number(raw.turn_translation_x_cm||0).toFixed(1)} / ${Number(raw.turn_translation_y_cm||0).toFixed(1)} cm`;
            const anti=raw.anti_friction||{};
            const antiText=anti.active ? ` · ANTIFRICCIÓN ${Number(anti.pulse_index||0)}/${Number(anti.pulse_total||7)} ${anti.pulse_on?'ON':'OFF'} · ${Number(anti.target_8bit||0)}/255 (${Number(anti.target_percent||0).toFixed(1)}%)` : '';
            document.getElementById('telem-stall').textContent=`ruedas ${Number(stall.fl||0)} / ${Number(stall.fr||0)} / ${Number(stall.bl||0)} / ${Number(stall.br||0)} ms · lados ${Number(fusion.left_no_progress_ms||0)} / ${Number(fusion.right_no_progress_ms||0)} ms · fuentes ${fusion.left_sources||'none'} / ${fusion.right_sources||'none'} · confiables ${Number(fusion.left_reliable_count||0)}L/${Number(fusion.right_reliable_count||0)}R · ${fusion.estimator||'sin estimador'}${antiText}`;
            document.getElementById('telem-interlock').textContent=`${interlockL.state||'OFF'} (${Number(interlockL.energized_sign||0)}→${Number(interlockL.pending_sign||0)}) / ${interlockR.state||'OFF'} (${Number(interlockR.energized_sign||0)}→${Number(interlockR.pending_sign||0)})`;
            document.getElementById('telem-cal-pulse').textContent=`${health.fl||'unknown'}/${health.fr||'unknown'}/${health.bl||'unknown'}/${health.br||'unknown'} · pivot continuo · pausa por sobrepaso 300 ms · verificación en rango 300 ms`;
            const stackText=`stack ${stackMin.web??'—'} / ${stackMin.control??'—'} bytes · período ${Number(timing.last_period_us||0)} / ${Number(timing.target_period_us||10000)} µs · jitter máx ${Number(timing.max_jitter_us||0)} µs · ciclo máx ${Number(timing.max_cycle_duration_us||0)} µs · muestra máx ${Number(timing.max_sample_age_us||0)} µs · deadlines ${Number(timing.missed_deadlines||0)} · reset ${rtos.reset_reason||'unknown'}${rtos.stack_warning?' · STACK BAJO':''}`;
            document.getElementById('telem-rtos').textContent=stackText;
            const terminal=raw.last_terminal||{};
            document.getElementById('telem-last-stop').textContent=terminal.type?`${terminal.type}: ${terminal.detail||terminal.reason||'sin detalle'}`:'ninguno';
            const resetReason=String(rtos.reset_reason||'unknown');
            const resetPeligroso=!['power_on','software','unknown'].includes(resetReason);
            const rtosKey=`${resetReason}:${rtos.stack_warning?1:0}:${rtos.task_creation_failure||''}`;
            if(rtosKey!==this.lastRtosKey){
                LogSystem.add(rtos.stack_warning||rtos.task_creation_failure||resetPeligroso?'ERROR':'INFO',`RTOS: ${stackText}${resetReason==='brownout'?' · CAÍDA DE VOLTAJE: el ESP32 reinició y detuvo motores':''}${rtos.task_creation_failure?` · ${rtos.task_creation_failure}`:''}`);
                this.lastRtosKey=rtosKey;
            }
            if(yawRecenterCount>this.lastYawRecenterCount){
                LogSystem.add('INFO',`Yaw MPU recentrado en reposo por múltiplos de 360° (#${yawRecenterCount}).`);
            }
            this.lastYawRecenterCount=yawRecenterCount;
            Telemetry._process({
                distancia:totalDistance, angulo:yaw, posX:telemX, posY:telemY,
                orientacion:yaw*Math.PI/180, estado:uiState,
                pfl:Number(pulses.fl||0), pfr:Number(pulses.fr||0), pbl:Number(pulses.bl||0), pbr:Number(pulses.br||0),
                pwmL:Number(pwm.left||0), pwmR:Number(pwm.right||0),
                target:raw.target||{}, driveControl:raw.drive_control||{}, motion:raw.motion||{}, recovery:raw.recovery||{}
            });
            if(typeof RouteManager!=='undefined')RouteManager.applyVectorialTelemetry(raw);
            Calibration.applyTelemetry(raw);
            ConnectionPanel.applyIdentity(raw);
        },
        async send(obj) {
            if (!this.flaskMode) throw new Error('desktop_app no está disponible');
            const cmd = String(obj.cmd || '').toLowerCase();
            if (!CANONICAL_CONFIG.SUPPORTED_COMMANDS.has(cmd)) {
                LogSystem.add('WARN',`El protocolo canónico ${CANONICAL_CONFIG.PROTOCOL_NAME} no admite '${cmd}'; usa pasos o rutas atómicas.`);
                return null;
            }
            const payload = {...obj}; delete payload.cmd;
            const endpoint = cmd === 'estop' ? '/api/v1/estop' : '/api/v1/commands';
            const body = cmd === 'estop' ? {} : {name:cmd,payload};
            const apiOptions = {method:'POST',body:JSON.stringify(body)};
            if (cmd === 'estop') apiOptions.timeoutMs = 2000;
            try { return await this.api(endpoint, apiOptions); }
            catch(error) { LogSystem.add('ERROR',error.message); return null; }
        },
        manual(path, payload={}) {
            return this.api(`/api/v1/manual/${path}`, {method:path==='drive'?'PUT':'POST', body:JSON.stringify(payload)});
        },
        sendMove(x, y) {
            if (isNaN(x) || isNaN(y)) { LogSystem.add('ERROR', "Coordenadas invalidas."); return false; }
            const result = this.api('/api/v1/missions',{method:'POST',body:JSON.stringify({points:[{x_mm:x*10,y_mm:y*10}]})})
                .catch(error=>{LogSystem.add('ERROR',error.message);return null;});
            LogSystem.add('CMD', `WS -> Mover a X:${x.toFixed(1)}, Y:${y.toFixed(1)}`);
            return result;
        }
    };

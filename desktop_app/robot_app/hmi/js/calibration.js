const Calibration = {
        calibrated:false, commandId:null, commandPending:false, rearmCommandId:null, rearmPending:false,
        selfTestCommandId:null, selfTestPassed:true, robotState:'', mpuReady:false, allowedCommands:null,
        applyingSurface:false, applySurfaceTimeout:null,
        lastRaw:{}, lastTurnMode:'PIVOT_CONTINUOUS', lastRequestedTurnMode:'AUTO', lastPowerKey:'',
        lastRobotState:'', lastTerminalKey:'', lastBrakingPhase:'OFF', lastAlignmentStage:'OFF',
        lastDegradedMode:null, lastEncoderHealthKey:'', lastAntiFrictionKey:'',
        init() {
            document.body.classList.add('calibration-locked');
            document.getElementById('btn-calibrate').disabled=true;
            document.getElementById('btn-self-test').disabled=true;
            document.getElementById('self-test-state').textContent='INTEGRADA EN FIRMWARE';
            document.getElementById('btn-calibrate').addEventListener('click',async()=>{
                if(!state.connected) return LogSystem.add('WARN','Conecta primero con el carrito.');
                if(!this.canCalibrate()) return LogSystem.add('WARN',this.calibrationBlockReason());
                
                const autoNameInput = document.getElementById('surface-auto-name');
                const desiredName = autoNameInput?.value?.trim() || null;
                if (typeof SurfaceManager !== 'undefined') {
                    SurfaceManager.pendingAutoSaveName = desiredName;
                }

                this.commandPending=true;
                this.updateAvailability();
                const calPanel = document.getElementById('cal-physical-panel');
                if (calPanel) calPanel.style.display = 'block';
                const toggleCalBtn = document.getElementById('btn-toggle-cal-diagnostics');
                if (toggleCalBtn) toggleCalBtn.textContent = '✕ Ocultar Diagnósticos';
                if(typeof RouteManager !== 'undefined') {
                    if(RouteManager.isRunning) {
                        RouteManager.failRoute('Calibración solicitada: ruta activa descartada.');
                    }
                    RouteManager.queue = [];
                    RouteManager.logicalSteps = [];
                    RouteManager.currentStepIndex = 0;
                    RouteManager.updateListUI();
                    RouteManager.drawPlan();
                }
                try {
                    await WSControl.api('/api/v1/missions',{method:'DELETE'}).catch(()=>{});
                    LogSystem.add('INFO', desiredName 
                        ? `Iniciando calibración física para registrar "${desiredName}"...`
                        : 'Enviando comando de calibración al robot...');
                    const result=await WSControl.send({cmd:'calibrate'});
                    if(!result){ 
                        this.commandPending=false; 
                        if (typeof SurfaceManager !== 'undefined') SurfaceManager.pendingAutoSaveName = null;
                        this.updateAvailability(); 
                        return; 
                    }
                    this.commandId=result.id;
                    this.setState('CALIBRACIÓN SOLICITADA',false);
                } catch(err) {
                    this.commandPending=false;
                    if (typeof SurfaceManager !== 'undefined') SurfaceManager.pendingAutoSaveName = null;
                    this.updateAvailability();
                    LogSystem.add('ERROR','Error enviando calibracion: ' + err.message);
                }
            });
            document.getElementById('btn-clear-fault').addEventListener('click',async()=>{
                if(!this.canRearm()) return LogSystem.add('WARN','El rearme sólo está disponible conectado y en FALLO o ESTOP.');
                this.rearmPending=true;
                this.updateAvailability();
                LogSystem.add('CMD',`Solicitando rearme desde ${this.robotState}...`);
                const result=await WSControl.send({cmd:'clear_fault'});
                if(!result){ this.rearmPending=false; this.updateAvailability(); return; }
                this.rearmCommandId=result.id;
            });
        },
        normalizeRobotState(value) {
            const current=String(value||'').trim().toUpperCase();
            return ['FAULT','SAFE_STOP'].includes(current)?'FALLO':current;
        },
        hasAllowed(command) {
            return !Array.isArray(this.allowedCommands) || this.allowedCommands.includes(command);
        },
        mpuStatus(raw) {
            const mpu=raw && typeof raw.mpu==='object' && raw.mpu ? raw.mpu : {};
            const present=raw.mpu_present ?? mpu.present;
            const stale=raw.mpu_stale ?? mpu.stale;
            const calibrated=raw.mpu_calibrated ?? mpu.calibrated;
            const i2cOk=raw.i2c_ok ?? mpu.i2c_ok;
            if(present!==true)return {ready:false,reason:present===false?'MPU no detectada.':'Esperando diagnóstico de la MPU.'};
            if(i2cOk===false)return {ready:false,reason:'Bus I²C de la MPU con fallo.'};
            if(stale!==false)return {ready:false,reason:stale===true?'Telemetría MPU obsoleta.':'Esperando vigencia de la MPU.'};
            if(calibrated!==true)return {ready:false,reason:'MPU presente, pero aún no está calibrada.'};
            return {ready:true,reason:'MPU apta para calibración física.'};
        },
        canCalibrate() {
            return state.connected && ['DESARMADO','LISTO'].includes(this.robotState) && this.mpuReady &&
                !this.commandPending && !this.rearmPending && this.hasAllowed('calibrate');
        },
        canRearm() {
            return state.connected && ['FALLO','ESTOP'].includes(this.robotState) && !this.rearmPending &&
                this.hasAllowed('clear_fault');
        },
        calibrationBlockReason() {
            if(!state.connected)return 'Conecta primero con el carrito.';
            if(this.commandPending)return 'Ya existe un comando de calibración pendiente.';
            if(this.rearmPending)return 'Espera a que termine el rearme.';
            if(['FALLO','ESTOP'].includes(this.robotState))return `El robot está en ${this.robotState}; rearma el sistema antes de calibrar.`;
            if(!['DESARMADO','LISTO'].includes(this.robotState))return `Calibración no permitida durante ${this.robotState||'un estado desconocido'}.`;
            const mpu=this.mpuStatus(this.lastRaw||{});
            if(!mpu.ready)return mpu.reason;
            const pcnt=this.lastRaw&&typeof this.lastRaw.pcnt_init==='object'?this.lastRaw.pcnt_init:{};
            if(pcnt.ready===false)return 'Inicialización PCNT incompleta; revisa el diagnóstico de encoders y reinicia.';
            if(!this.hasAllowed('calibrate'))return 'El firmware no habilita calibración en este instante.';
            return 'Calibración temporalmente no disponible.';
        },
        updateAvailability() {
            const calibrate=document.getElementById('btn-calibrate');
            calibrate.disabled=!this.canCalibrate();
            calibrate.title=calibrate.disabled?this.calibrationBlockReason():'Iniciar calibración física';
            const rearm=document.getElementById('btn-clear-fault');
            const faultState=['FALLO','ESTOP'].includes(this.robotState);
            rearm.hidden=!faultState;
            rearm.disabled=!this.canRearm();
            document.getElementById('btn-clear-fault-label').textContent=this.rearmPending?'Rearmando…':`Rearmar ${this.robotState||'sistema'}`;
            const applyBtn=document.getElementById('btn-apply-surface');
            if(applyBtn){
                applyBtn.disabled=faultState||!this.mpuReady;
                if(faultState){
                    applyBtn.title=`El robot está en ${this.robotState}. Rearme el sistema antes de aplicar superficie.`;
                }else if(!this.mpuReady){
                    applyBtn.title='MPU no listo para inyectar calibración';
                }else{
                    applyBtn.title='Aplicar superficie seleccionada al robot';
                }
            }
        },
        showCause(reason,visible) {
            const node=document.getElementById('calibration-cause');
            node.textContent=visible?`Causa exacta: ${reason||'sin causa reportada'}`:'';
            node.classList.toggle('visible',Boolean(visible));
        },
        setState(label,ready,tone='') {
            this.calibrated=ready;
            const node=document.getElementById('calibration-state');
            node.textContent=label;
            node.classList.toggle('ready',ready);
            node.classList.toggle('fault',tone==='fault');
            node.classList.toggle('estop',tone==='estop');
            document.body.classList.toggle('calibration-locked',!ready);
            this.updateAvailability();
        },
        valueAt(container,key,index,fallback=undefined) {
            if(Array.isArray(container))return container[index] ?? fallback;
            if(container && typeof container==='object')return container[key] ?? container[key.toUpperCase()] ?? fallback;
            return fallback;
        },
        renderDiagnostics(raw) {
            const diag=raw && typeof raw.calibration_diagnostics==='object' && raw.calibration_diagnostics
                ? raw.calibration_diagnostics : {};
            const phase=diag.phase ?? raw.active_command_name ?? '—';
            const ramp=diag.ramp_level ?? diag.ramp_step ?? diag.pasoRampa;
            const rampTotal=diag.ramp_level_count ?? diag.ramp_total ?? diag.pasosRampaTotal;
            const pwm10=diag.pwm_10bit ?? diag.pwm_target ?? diag.pwmObjetivo;
            const pwm8=diag.pwm_8bit ?? (Number.isFinite(Number(pwm10))?Math.round(Number(pwm10)*255/1023):undefined);
            const pwmLeft8=diag.pwm_left_8bit, pwmRight8=diag.pwm_right_8bit;
            const attempt=diag.attempt ?? diag.retry_attempt;
            const attemptMax=diag.attempt_max ?? diag.retry_attempt_max;
            document.getElementById('cal-diag-phase').textContent=String(phase||'—').toUpperCase();
            document.getElementById('cal-diag-ramp').textContent=ramp===undefined?'—':`${ramp}/${rampTotal??'—'}`;
            document.getElementById('cal-diag-pwm').textContent=pwmLeft8!==undefined||pwmRight8!==undefined
                ? `L/R ${pwmLeft8??'—'}/${pwmRight8??'—'} de 255`
                : pwm10===undefined?'—':`${pwm8??'—'}/255 · ${pwm10}/1023`;
            document.getElementById('cal-diag-attempt').textContent=attempt===undefined?'—':`${attempt}/${attemptMax??'—'}`;
            const history=raw&&typeof raw.torque_history==='object'&&raw.torque_history?raw.torque_history:{};
            const historyCount=Number(history.record_count||0);
            const historyStatus=String(history.status||'sin datos');
            const basePosL=Number(history.base_positive_left_8bit??history.base_positive_8bit??140);
            const basePosR=Number(history.base_positive_right_8bit??history.base_positive_8bit??140);
            const baseNegL=Number(history.base_negative_left_8bit??history.base_negative_8bit??140);
            const baseNegR=Number(history.base_negative_right_8bit??history.base_negative_8bit??140);
            document.getElementById('cal-diag-history').textContent=historyCount
                ? `${historyCount}/10 · base + L/R ${basePosL}/${basePosR} · − L/R ${baseNegL}/${baseNegR} · ${historyStatus}`
                : `0/10 · base 140 · ${historyStatus}`;
            const pcnt=raw&&typeof raw.pcnt_init==='object'&&raw.pcnt_init?raw.pcnt_init:{};
            const pcntNames=['fl','fr','bl','br'];
            const pcntFailures=pcntNames.filter(name=>pcnt[name]&&pcnt[name].initialized===false)
                .map(name=>`${name.toUpperCase()}:${pcnt[name].failed_stage||'unknown'}(${Number(pcnt[name].error_code||0)})`);
            document.getElementById('cal-diag-pcnt').textContent=pcnt.ready===true
                ? 'FL/FR/BL/BR listos'
                : pcnt.ready===false ? `FALLO · ${pcntFailures.join(' · ')||'sin detalle'}` : 'esperando telemetría';
            const deltas=diag.encoder_delta ?? diag.deltas ?? diag.deltaEncoders;
            const responding=diag.encoder_responding ?? diag.encoder_responds ?? diag.encoderResponde;
            const isolated=diag.encoder_isolated ?? diag.isolated ?? diag.encoderAislado;
            const trusted=diag.trusted;
            ['fl','fr','bl','br'].forEach((wheel,index)=>{
                const delta=this.valueAt(deltas,wheel,index);
                const responds=this.valueAt(responding,wheel,index);
                const isIsolated=Boolean(this.valueAt(isolated,wheel,index,false));
                const isTrusted=this.valueAt(trusted,wheel,index);
                let status='unknown',label='sin datos';
                if(isIsolated || isTrusted===false){status='isolated';label='aislado';}
                else if(responds===true || isTrusted===true){status='ok';label='responde';}
                else if(responds===false){status='unknown';label='sin señal en esta fase';}
                document.getElementById(`cal-diag-delta-${wheel}`).textContent=`Δ ${delta===undefined?'—':String(delta)}`;
                document.getElementById(`cal-diag-status-${wheel}`).textContent=label;
                document.getElementById(`cal-diag-${wheel}`).dataset.status=status;
            });
            const silence=Number(diag.encoder_silence_ms||0), silenceLimit=Number(diag.encoder_silence_limit_ms||500);
            const silenceLabel=diag.gyro_without_encoders===true?` · IMU sin PCNT ${silence}/${silenceLimit} ms`:'';
            const responseCount=Number(diag.pcnt_response_count||0);
            const authority=diag.authority==='imu_primary_bilateral_pcnt'
                ? 'MPU principal · PCNT bilateral'
                : diag.authority==='imu_primary_one_pcnt'?'MPU principal · firmware anterior':'Autoridad sin identificar';
            const wait=String(diag.wait_reason||'esperando datos');
            const pivot=diag.pivot_pwm&&typeof diag.pivot_pwm==='object'?diag.pivot_pwm:{};
            const leftTicks=Number(diag.left_ticks||0), rightTicks=Number(diag.right_ticks||0);
            const ratio=Number(diag.tick_ratio);
            const ratioLabel=Number.isFinite(ratio)?ratio.toFixed(2):'—';
            const yawDelta=Number(diag.yaw_phase_delta||0);
            const quality=String(diag.pivot_quality||'unverified');
            const balance=String(diag.balance_adjustment||'—');
            const translation=Number(diag.estimated_translation_cm||0);
            const torque=diag.torque_detected===true?'torque detectado':'buscando torque';
            const pivotState=diag.pivot_validated===true?'pivote validado':'pivote pendiente';
            const directionLabel=diag.encoder_direction_verified===false?' · dirección PCNT no verificable':'';
            document.getElementById('cal-diag-sides').textContent=`${authority} · ${torque} · ${pivotState} · calidad ${quality} · balance ${balance} · ticks L/R ${leftTicks}/${rightTicks} (ratio ${ratioLabel}) · Δyaw ${yawDelta.toFixed(1)}° · traslación estimada ${translation.toFixed(2)} cm · PCNT ${responseCount}/4 · espera ${wait} · PWM lógico L/R ${pivot.logical_left??'—'}/${pivot.logical_right??'—'}${silenceLabel}${directionLabel}`;
        },
        applyTelemetry(raw) {
            this.lastRaw=raw||{};
            this.selfTestPassed=true;
            const intento=Number(raw.turn_attempt||0), max=Number(raw.turn_attempt_max||0);
            const requestedMode=String(raw.turn_requested_mode||'AUTO');
            const turnMode=String(raw.turn_mode||'PIVOT');
            if(requestedMode!==this.lastRequestedTurnMode){
                LogSystem.add('INFO',`Método de giro solicitado: ${requestedMode}.`);
                this.lastRequestedTurnMode=requestedMode;
            }
            if(turnMode!==this.lastTurnMode){
                if(turnMode==='PIVOT_CONTINUOUS') LogSystem.add('INFO','Pivot simple activado; sólo pausa 300 ms cuando sobrepasa el objetivo.');
                this.lastTurnMode=turnMode;
            }
            const powerKey=`${raw.turn_left_energized?1:0}:${Number(raw.turn_left_power_started_ms||0)}:${raw.turn_right_energized?1:0}:${Number(raw.turn_right_power_started_ms||0)}`;
            if(powerKey!==this.lastPowerKey && (raw.turn_left_energized||raw.turn_right_energized)){
                LogSystem.add('INFO',`Energía física de giro: L=${raw.turn_left_energized?'ON':'OFF'} desde ${Number(raw.turn_left_power_started_ms||0)} ms; R=${raw.turn_right_energized?'ON':'OFF'} desde ${Number(raw.turn_right_power_started_ms||0)} ms.`);
            }
            this.lastPowerKey=powerKey;
            const brakingPhase=String(raw.turn_braking_phase||'OFF');
            if(brakingPhase!==this.lastBrakingPhase && brakingPhase!=='OFF'){
                LogSystem.add('INFO',`Control angular ${brakingPhase}: error=${Number(raw.turn_error_deg||0).toFixed(2)}°, predicho=${Number(raw.turn_predicted_error_deg||0).toFixed(2)}°, PWM objetivo=${Number(raw.turn_pwm_target_8bit||0)}/255.`);
            }
            this.lastBrakingPhase=brakingPhase;
            const alignmentStage=String(raw.alignment_stage||'OFF');
            if(alignmentStage!==this.lastAlignmentStage && alignmentStage!=='OFF'){
                LogSystem.add('INFO',`Alineación ${alignmentStage}: fase=${raw.move_phase||'OFF'}, objetivo=${Number(raw.heading_target_deg||0).toFixed(1)}°, error=${Number(raw.heading_error_deg||0).toFixed(1)}°, tolerancia=±${Number(raw.turn_tolerance_deg||0).toFixed(1)}°.`);
            }
            this.lastAlignmentStage=alignmentStage;
            const encoderHealth=raw.encoder_health||{};
            const encoderHealthKey=['fl','fr','bl','br'].map(k=>String(encoderHealth[k]||'unknown')).join('/');
            const encoderWarning=['fl','fr','bl','br'].some(k=>{
                const status=String(encoderHealth[k]||'unknown');
                return !['healthy','ok'].includes(status);
            });
            const degraded=Boolean(raw.degraded_mode);
            if(this.lastDegradedMode!==null && degraded!==this.lastDegradedMode){
                if(degraded) LogSystem.add('WARN',`Modo degradado activo: ${encoderHealthKey}. La ruta continúa porque queda al menos un encoder confiable por lado.`);
                else LogSystem.add('INFO',`Fusión de encoders restaurada: ${encoderHealthKey}.`);
            } else if(degraded && encoderHealthKey!==this.lastEncoderHealthKey){
                LogSystem.add('WARN',`Salud de encoders en modo degradado: ${encoderHealthKey}.`);
            } else if(encoderWarning && encoderHealthKey!==this.lastEncoderHealthKey){
                LogSystem.add('WARN',`Encoder sin respuesta estable: ${encoderHealthKey}. Se mantiene vigilancia antes de excluirlo.`);
            }
            this.lastDegradedMode=degraded;
            this.lastEncoderHealthKey=encoderHealthKey;
            const antiState=raw.anti_friction||{};
            const antiKey=`${Boolean(antiState.active)}:${Number(antiState.pulse_index||0)}:${Boolean(antiState.pulse_on)}`;
            if(antiState.active && antiKey!==this.lastAntiFrictionKey){
                LogSystem.add('WARN',`Antifricción ${Number(antiState.pulse_index||0)}/${Number(antiState.pulse_total||7)} ${antiState.pulse_on?'ON':'OFF'}: PWM ${Number(antiState.target_8bit||0)}/255 (${Number(antiState.target_percent||0).toFixed(1)}%).`);
            } else if(!antiState.active && this.lastAntiFrictionKey.startsWith('true') && antiState.movement_confirmed){
                LogSystem.add('INFO','Antifricción terminada: movimiento confirmado en ambos lados.');
            }
            this.lastAntiFrictionKey=antiKey;
            const fault=raw.fault&&typeof raw.fault==='object'?raw.fault:{};
            const robotState=this.normalizeRobotState(raw.state||(fault.active?fault.state:''));
            this.robotState=robotState;
            this.allowedCommands=Array.isArray(raw.allowed_commands)?raw.allowed_commands.map(value=>String(value).toLowerCase()):null;
            this.mpuReady=this.mpuStatus(raw).ready;
            this.renderDiagnostics(raw);
            const terminal=raw.last_terminal||{};
            const diagnostics=raw.calibration_diagnostics&&typeof raw.calibration_diagnostics==='object'?raw.calibration_diagnostics:{};
            const terminalReason=String(fault.detail||terminal.detail||terminal.reason||diagnostics.reason||raw.finish_reason||'sin causa reportada');
            const terminalKey=`${terminal.id||''}:${terminal.type||''}:${terminalReason}`;
            if(['FALLO','ESTOP'].includes(robotState) &&
               (robotState!==this.lastRobotState || terminalKey!==this.lastTerminalKey)) {
                LogSystem.add('ERROR',`Robot pasó a ${robotState}: ${terminalReason}. PWM físico detenido por seguridad.`);
                this.lastTerminalKey=terminalKey;
            }
            this.lastRobotState=robotState;
            const intentoTexto=intento&&max?` — GIRO ${intento}/${max} · ${turnMode}`:'';
            if(robotState==='ESTOP'){
                this.commandPending=false;
                this.setState('ESTOP — REARME REQUERIDO',false,'estop');
                this.showCause(terminalReason,true);
            } else if(robotState==='FALLO'){
                this.commandPending=false;
                this.setState('FALLO — REARME REQUERIDO',false,'fault');
                this.showCause(terminalReason,true);
            } else {
                this.rearmPending=false;
                this.rearmCommandId=null;
                this.showCause('',false);
                const isCalibrated = Boolean(raw.calibrated ?? raw.cal);
                if(isCalibrated){
                    this.applyingSurface=false;
                    this.setState('LISTO — RUTAS HABILITADAS',true);
                }
                else if(this.applyingSurface) {
                    // Esperando que la telemetría del ESP32 confirme el estado LISTO tras set_calibration
                }
                else if(robotState==='CALIBRANDO') {
                    this.setState(`${raw.state} — ${Math.round((raw.command_progress||0)*100)}%${intentoTexto}`,false);
                    const calPanel = document.getElementById('cal-physical-panel');
                    if (calPanel && calPanel.style.display === 'none') {
                        calPanel.style.display = 'block';
                        const toggleCalBtn = document.getElementById('btn-toggle-cal-diagnostics');
                        if (toggleCalBtn) toggleCalBtn.textContent = '✕ Ocultar Calibración';
                    }
                }
                else if(['DESARMADO','LISTO'].includes(robotState) && this.mpuReady)this.setState('LISTO PARA CALIBRAR',false);
                else this.setState(this.mpuStatus(raw).reason.toUpperCase(),false);
            }
        },
        onRobotEvent(event) {
            if(event.id===this.rearmCommandId){
                if(event.type==='queued'||event.type==='accepted'||event.type==='progress')return;
                this.rearmPending=false;
                this.rearmCommandId=null;
                if(event.type==='completed'){
                    this.rearmPending=true;
                    this.setState('REARME ACEPTADO — ESPERANDO TELEMETRÍA',false);
                    LogSystem.add('INFO','Rearme aceptado; esperando confirmación del nuevo estado del robot.');
                } else {
                    const reason=event.detail||event.reason||'rearme rechazado';
                    this.showCause(reason,true);
                    LogSystem.add('ERROR',`No se pudo rearmar: ${reason}.`);
                    this.updateAvailability();
                }
                return;
            }
            if(event.id===this.selfTestCommandId){
                if(event.type==='queued'||event.type==='accepted'||event.type==='progress')return;
                const ok=event.type==='completed';
                this.selfTestPassed=ok;
                document.getElementById('self-test-state').textContent=ok?'APROBADA':`FALLO — ${event.detail||event.reason||'sin detalle'}`;
                document.getElementById('btn-self-test').disabled=false;
                this.updateAvailability();
                LogSystem.add(ok?'INFO':'ERROR',ok?'Autoevaluación lógica aprobada.':`Autoevaluación falló: ${event.detail||event.reason||'sin detalle'}`);
                return;
            }
            if(event.id!==this.commandId)return;
            if(event.type==='queued'||event.type==='accepted'||event.type==='progress')return;
            this.commandPending=false;
            this.commandId=null;
            if(event.type==='completed'){
                this.setState('LISTO — CONTROLES HABILITADOS',true);
                LogSystem.add('INFO','Calibración física completada; rutas habilitadas.');
                if (typeof SurfaceManager !== 'undefined') {
                    SurfaceManager.handleCalibrationCompleted();
                }
            }
            if(event.type==='fault'||event.type==='rejected'){
                if (typeof SurfaceManager !== 'undefined') {
                    SurfaceManager.pendingAutoSaveName = null;
                }
                const reason=event.detail||event.reason||'calibración rechazada';
                this.setState(this.calibrated?'LISTO — CALIBRACIÓN RECHAZADA':`CALIBRACIÓN FALLIDA — ${reason}`,this.calibrated,'fault');
                this.showCause(reason,true);
                LogSystem.add('ERROR',`No se pudo calibrar: ${reason}.`);
            }
            this.updateAvailability();
        }
    };

    const SurfaceManager = {
        surfaces: [],
        activeSurfaceId: null,
        pendingAutoSaveName: null,

        init() {
            const toggleBtn = document.getElementById('btn-toggle-surface-manager');
            const form = document.getElementById('surface-manager-form');
            if (toggleBtn && form) {
                toggleBtn.addEventListener('click', () => {
                    const isHidden = form.style.display === 'none' || !form.style.display;
                    form.style.display = isHidden ? 'block' : 'none';
                    toggleBtn.textContent = isHidden ? '✕ Cerrar Editor' : 'Modo Experto (PWM)';
                });
            }

            const toggleCalBtn = document.getElementById('btn-toggle-cal-diagnostics');
            const calPanel = document.getElementById('cal-physical-panel');
            if (toggleCalBtn && calPanel) {
                toggleCalBtn.addEventListener('click', () => {
                    const isHidden = calPanel.style.display === 'none' || !calPanel.style.display;
                    calPanel.style.display = isHidden ? 'block' : 'none';
                    toggleCalBtn.textContent = isHidden ? '✕ Ocultar Diagnósticos' : 'Diagnóstico MPU/PCNT';
                });
            }

            const autoNameInput = document.getElementById('surface-auto-name');
            const calibrateLabel = document.getElementById('btn-calibrate-label');
            if (autoNameInput && calibrateLabel) {
                autoNameInput.addEventListener('input', () => {
                    const val = autoNameInput.value.trim();
                    calibrateLabel.textContent = val 
                        ? `Calibrar y Registrar "${val}"`
                        : 'Calibrar físicamente (Giro ±25°)';
                });
            }

            const saveCurrentAsBtn = document.getElementById('btn-save-current-as-surface');
            if (saveCurrentAsBtn) {
                saveCurrentAsBtn.addEventListener('click', () => this.saveCurrentLiveSurface());
            }

            const select = document.getElementById('select-surface');
            if (select) {
                select.addEventListener('change', () => {
                    this.onSurfaceSelect(select.value);
                });
            }

            const applyBtn = document.getElementById('btn-apply-surface');
            if (applyBtn) {
                applyBtn.addEventListener('click', () => this.applySelected());
            }

            const saveBtn = document.getElementById('btn-save-surface');
            if (saveBtn) {
                saveBtn.addEventListener('click', () => this.saveCurrent());
            }

            const captureBtn = document.getElementById('btn-capture-surface');
            if (captureBtn) {
                captureBtn.addEventListener('click', () => this.captureFromTelemetry());
            }

            const deleteBtn = document.getElementById('btn-delete-surface');
            if (deleteBtn) {
                deleteBtn.addEventListener('click', () => this.deleteSelected());
            }

            this.loadSurfaces();
        },

        async loadSurfaces(selectId = null) {
            if (!WSControl.flaskMode) return;
            try {
                const surfaces = await WSControl.api('/api/v1/calibration/surfaces');
                this.surfaces = Array.isArray(surfaces) ? surfaces : [];
                const select = document.getElementById('select-surface');
                if (!select) return;
                
                select.innerHTML = '';
                if (this.surfaces.length === 0) {
                    const opt = document.createElement('option');
                    opt.value = '';
                    opt.textContent = 'Sin superficies guardadas';
                    select.appendChild(opt);
                    return;
                }

                this.surfaces.forEach(s => {
                    const opt = document.createElement('option');
                    opt.value = s.id;
                    const pwmP = s.pwm_pos ?? s.pwm_positive_8bit ?? 155;
                    const pwmN = s.pwm_neg ?? s.pwm_negative_8bit ?? 155;
                    opt.textContent = `${s.name} (PWM: ${pwmP}/${pwmN})`;
                    select.appendChild(opt);
                });

                const targetId = selectId || this.activeSurfaceId || this.surfaces[0].id;
                select.value = targetId;
                this.onSurfaceSelect(select.value);
            } catch (err) {
                LogSystem.add('ERROR', `Error cargando superficies: ${err.message}`);
            }
        },

        onSurfaceSelect(id) {
            const surface = this.surfaces.find(s => s.id === id);
            if (!surface) return;
            
            const nameInput = document.getElementById('surface-name');
            const descInput = document.getElementById('surface-desc');
            const pwmPosInput = document.getElementById('surface-pwm-pos');
            const pwmNegInput = document.getElementById('surface-pwm-neg');
            const candPosSelect = document.getElementById('surface-cand-pos');
            const candNegSelect = document.getElementById('surface-cand-neg');

            if (nameInput) nameInput.value = surface.name || '';
            if (descInput) descInput.value = surface.description || '';
            const pwmP = surface.pwm_pos ?? surface.pwm_positive_8bit ?? 155;
            const pwmN = surface.pwm_neg ?? surface.pwm_negative_8bit ?? 155;
            const candP = surface.cand_pos ?? surface.positive_polarity ?? 1;
            const candN = surface.cand_neg ?? surface.negative_polarity ?? -1;

            if (pwmPosInput) pwmPosInput.value = pwmP;
            if (pwmNegInput) pwmNegInput.value = pwmN;
            if (candPosSelect) candPosSelect.value = String(candP);
            if (candNegSelect) candNegSelect.value = String(candN);
        },

        async applySelected() {
            if (['FALLO', 'ESTOP'].includes(Calibration.robotState || Calibration.lastRobotState)) {
                LogSystem.add('WARN', `El robot está en ${Calibration.robotState || Calibration.lastRobotState}. Presiona "Rearmar" antes de aplicar una superficie.`);
                return;
            }
            const select = document.getElementById('select-surface');
            const id = select ? select.value : null;
            if (!id) {
                LogSystem.add('WARN', 'Selecciona una superficie primero.');
                return;
            }
            const applyBtn = document.getElementById('btn-apply-surface');
            if (applyBtn) applyBtn.disabled = true;

            try {
                LogSystem.add('INFO', `Inyectando superficie ${id} al robot...`);
                const result = await WSControl.api(`/api/v1/calibration/surfaces/${encodeURIComponent(id)}/apply`, {
                    method: 'POST',
                    body: '{}'
                });
                this.activeSurfaceId = id;
                const info = document.getElementById('surface-active-info');
                if (info && result.surface) {
                    const pP = result.surface.pwm_pos ?? result.surface.pwm_positive_8bit ?? 155;
                    const pN = result.surface.pwm_neg ?? result.surface.pwm_negative_8bit ?? 155;
                    info.textContent = `· Perfil: ${result.surface.name} (PWM ${pP}/${pN})`;
                }
                Calibration.commandId = result.command_id;
                Calibration.applyingSurface = true;
                clearTimeout(Calibration.applySurfaceTimeout);
                Calibration.applySurfaceTimeout = setTimeout(() => {
                    Calibration.applyingSurface = false;
                    Calibration.updateAvailability();
                }, 3500);
                Calibration.setState('LISTO — SUPERFICIE APLICADA', true);
                LogSystem.add('INFO', `Superficie "${result.surface?.name || id}" aplicada y calibrada en ESP32.`);
            } catch (err) {
                LogSystem.add('ERROR', `Fallo al aplicar superficie: ${err.message}`);
            } finally {
                if (applyBtn) applyBtn.disabled = false;
            }
        },

        async saveCurrentLiveSurface() {
            const autoNameInput = document.getElementById('surface-auto-name');
            const name = autoNameInput?.value?.trim();
            if (!name) {
                LogSystem.add('WARN', 'Escribe el nombre del piso antes de guardar.');
                if (autoNameInput) autoNameInput.focus();
                return;
            }

            const raw = Calibration.lastRaw || {};
            const history = raw.torque_history || {};
            const diag = raw.calibration_diagnostics || {};
            const pwmPos = Math.max(140, Math.min(247, Number(history.base_positive_left_8bit ?? history.base_positive_8bit ?? diag.pwm_8bit ?? 155)));
            const pwmNeg = Math.max(140, Math.min(247, Number(history.base_negative_left_8bit ?? history.base_negative_8bit ?? 155)));

            try {
                const payload = {
                    name,
                    description: 'Guardado desde calibración activa',
                    pwm_pos: pwmPos,
                    pwm_neg: pwmNeg,
                    cand_pos: 1,
                    cand_neg: -1
                };
                const saved = await WSControl.api('/api/v1/calibration/surfaces', {
                    method: 'POST',
                    body: JSON.stringify(payload)
                });
                LogSystem.add('INFO', `Superficie "${saved.name}" registrada en SQLite (PWM: ${pwmPos}/${pwmNeg}).`);
                if (autoNameInput) autoNameInput.value = '';
                const calibrateLabel = document.getElementById('btn-calibrate-label');
                if (calibrateLabel) calibrateLabel.textContent = 'Calibrar físicamente (Giro ±25°)';
                await this.loadSurfaces(saved.id);
            } catch (err) {
                LogSystem.add('ERROR', `Error guardando superficie: ${err.message}`);
            }
        },

        async handleCalibrationCompleted() {
            const name = this.pendingAutoSaveName;
            this.pendingAutoSaveName = null;
            if (!name) return;

            const raw = Calibration.lastRaw || {};
            const history = raw.torque_history || {};
            const diag = raw.calibration_diagnostics || {};
            const pwmPos = Math.max(140, Math.min(247, Number(history.base_positive_left_8bit ?? history.base_positive_8bit ?? diag.pwm_8bit ?? 155)));
            const pwmNeg = Math.max(140, Math.min(247, Number(history.base_negative_left_8bit ?? history.base_negative_8bit ?? 155)));

            try {
                const payload = {
                    name,
                    description: 'Calibrado físicamente por el robot',
                    pwm_pos: pwmPos,
                    pwm_neg: pwmNeg,
                    cand_pos: 1,
                    cand_neg: -1
                };
                const saved = await WSControl.api('/api/v1/calibration/surfaces', {
                    method: 'POST',
                    body: JSON.stringify(payload)
                });
                LogSystem.add('INFO', `Calibración finalizada: Superficie "${saved.name}" registrada en SQLite (PWM: ${pwmPos}/${pwmNeg}).`);
                const autoNameInput = document.getElementById('surface-auto-name');
                if (autoNameInput) autoNameInput.value = '';
                const calibrateLabel = document.getElementById('btn-calibrate-label');
                if (calibrateLabel) calibrateLabel.textContent = 'Calibrar físicamente (Giro ±25°)';
                await this.loadSurfaces(saved.id);
            } catch (err) {
                LogSystem.add('ERROR', `Error registrando calibración automática en SQLite: ${err.message}`);
            }
        },

        async saveCurrent() {
            const name = document.getElementById('surface-name')?.value?.trim();
            const desc = document.getElementById('surface-desc')?.value?.trim() || '';
            const pwmPos = Number(document.getElementById('surface-pwm-pos')?.value);
            const pwmNeg = Number(document.getElementById('surface-pwm-neg')?.value);
            const candPos = Number(document.getElementById('surface-cand-pos')?.value);
            const candNeg = Number(document.getElementById('surface-cand-neg')?.value);

            if (!name) {
                LogSystem.add('WARN', 'El nombre de la superficie es obligatorio.');
                return;
            }
            if (!Number.isFinite(pwmPos) || pwmPos < 140 || pwmPos > 247 ||
                !Number.isFinite(pwmNeg) || pwmNeg < 140 || pwmNeg > 247) {
                LogSystem.add('WARN', 'PWM debe estar entre 140 y 247.');
                return;
            }
            if (candPos === candNeg || ![-1, 1].includes(candPos) || ![-1, 1].includes(candNeg)) {
                LogSystem.add('WARN', 'Las polaridades deben ser opuestas (+1 y -1).');
                return;
            }

            try {
                const payload = {
                    name,
                    description: desc,
                    pwm_pos: pwmPos,
                    pwm_neg: pwmNeg,
                    cand_pos: candPos,
                    cand_neg: candNeg
                };
                const saved = await WSControl.api('/api/v1/calibration/surfaces', {
                    method: 'POST',
                    body: JSON.stringify(payload)
                });
                LogSystem.add('INFO', `Superficie guardada en SQLite: ${saved.name} (ID: ${saved.id})`);
                await this.loadSurfaces(saved.id);
            } catch (err) {
                LogSystem.add('ERROR', `Error guardando superficie: ${err.message}`);
            }
        },

        captureFromTelemetry() {
            const raw = Calibration.lastRaw || {};
            const history = raw.torque_history || {};
            const diag = raw.calibration_diagnostics || {};
            
            const pwmPos = Number(history.base_positive_left_8bit ?? history.base_positive_8bit ?? diag.pwm_8bit ?? 155);
            const pwmNeg = Number(history.base_negative_left_8bit ?? history.base_negative_8bit ?? 155);

            const pwmPosInput = document.getElementById('surface-pwm-pos');
            const pwmNegInput = document.getElementById('surface-pwm-neg');
            if (pwmPosInput) pwmPosInput.value = Math.max(140, Math.min(247, pwmPos));
            if (pwmNegInput) pwmNegInput.value = Math.max(140, Math.min(247, pwmNeg));

            const nameInput = document.getElementById('surface-name');
            if (nameInput && !nameInput.value) {
                nameInput.value = `Captura ${new Date().toLocaleTimeString([], {hour: '2-digit', minute:'2-digit'})}`;
            }
            LogSystem.add('INFO', `Valores capturados de telemetría: PWM+ ${pwmPos}, PWM− ${pwmNeg}`);
        },

        async deleteSelected() {
            const select = document.getElementById('select-surface');
            const id = select ? select.value : null;
            if (!id) {
                LogSystem.add('WARN', 'Selecciona una superficie para borrar.');
                return;
            }
            const surface = this.surfaces.find(s => s.id === id);
            const name = surface ? surface.name : id;
            if (!confirm(`¿Eliminar la superficie "${name}" de SQLite?`)) return;

            try {
                await WSControl.api(`/api/v1/calibration/surfaces/${encodeURIComponent(id)}`, {
                    method: 'DELETE'
                });
                LogSystem.add('INFO', `Superficie "${name}" eliminada de SQLite.`);
                if (this.activeSurfaceId === id) {
                    this.activeSurfaceId = null;
                    const info = document.getElementById('surface-active-info');
                    if (info) info.textContent = '· Perfil: —';
                }
                await this.loadSurfaces();
            } catch (err) {
                LogSystem.add('ERROR', `Error eliminando superficie: ${err.message}`);
            }
        }
    };

    const ConnectionPanel = {
        state:'stopped',
        init() {
            document.getElementById('btn-connect').addEventListener('click',()=>this.change('connect'));
            document.getElementById('btn-reconnect').addEventListener('click',()=>this.change('reconnect'));
            document.getElementById('btn-disconnect').addEventListener('click',()=>this.change('disconnect'));
            this.loadHistory();
        },
        async change(action) {
            if (!WSControl.flaskMode) { LogSystem.add('WARN','La gestión explícita está disponible en la aplicación Flask.'); return; }
            try {
                const host=document.getElementById('robot-host').value.trim();
                if (action !== 'disconnect') await WSControl.api('/api/v1/config/robot',{method:'PUT',body:JSON.stringify({robot_host:host})});
                const snapshot=await WSControl.api(`/api/v1/connection/${action}`,{method:'POST',body:'{}'});
                this.applySnapshot(snapshot);
                LogSystem.add('INFO',`${action.toUpperCase()} solicitado a Flask.`);
            } catch(error) { LogSystem.add('ERROR',error.message); }
        },
        applySnapshot(snapshot) {
            document.getElementById('robot-host').value=snapshot.robot_host||'192.168.4.1';
            if(typeof RouteManager!=='undefined')RouteManager.applyCapabilities(snapshot.capabilities||{});
            this.applyConnection(snapshot.connection||{});
            this.applySession({recording:snapshot.recording,session_id:snapshot.session_id});
            document.getElementById('protocol-version').textContent=snapshot.connection?.state==='stopped' ? '—' : (snapshot.connection?.protocol||'—');
            if(snapshot.telemetry)this.applyIdentity(snapshot.telemetry);
        },
        applyConnection(connection) {
            this.state=connection.state||'stopped';
            state.connected=this.state==='connected';
            const label={stopped:'DESCONECTADO',connecting:'CONECTANDO',connected:'CONECTADO',backoff:'REINTENTANDO'}[this.state]||this.state.toUpperCase();
            document.getElementById('connection-panel-state').textContent=label;
            const detail=connection.detail||(state.connected?'WebSocket y protocolo robot-s3-steps-v3 activos.':'Selecciona ROBOT_S3_LOCAL en el Wi‑Fi del dispositivo antes de conectar.');
            document.getElementById('connection-panel-detail').textContent=detail;
            document.getElementById('connection-panel-dot').classList.toggle('ok',state.connected);
            if((this.state === 'stopped' || this.state === 'backoff') && typeof RouteManager !== 'undefined') {
                if(RouteManager.isRunning) {
                    RouteManager.failRoute(`Conexión con el robot interrumpida (${label}); ruta cancelada por seguridad.`);
                }
                RouteManager.unlockTabs();
                if(this.state === 'stopped') {
                    RouteManager.queue = [];
                    RouteManager.logicalSteps = [];
                    RouteManager.currentStepIndex = 0;
                    RouteManager.updateListUI();
                    RouteManager.drawPlan();
                }
            }
            Calibration.updateAvailability();
            Telemetry._ui();
        },
        applySession(session) {
            document.getElementById('active-session').textContent=session.recording ? `#${session.session_id}` : '—';
            if(!session.recording && this.state === 'stopped' && typeof RouteManager !== 'undefined') {
                if(RouteManager.isRunning) {
                    RouteManager.failRoute('Sesión detenida; ruta cancelada.');
                }
                RouteManager.queue = [];
                RouteManager.logicalSteps = [];
                RouteManager.currentStepIndex = 0;
                RouteManager.updateListUI();
                RouteManager.drawPlan();
            }
        },
        applyIdentity(telemetry) {
            if(telemetry.robot_id)document.getElementById('robot-id').textContent=telemetry.robot_id;
            if(telemetry.firmware_version)document.getElementById('firmware-version').textContent=telemetry.firmware_version;
            if(telemetry.protocol)document.getElementById('protocol-version').textContent=telemetry.protocol;
        },
        esc(value) { const node=document.createElement('span'); node.textContent=value??'—'; return node.innerHTML; },
        async loadHistory() {
            if(!WSControl.flaskMode)return;
            try {
                const rows=await WSControl.api('/api/v1/sessions');
                const body=document.getElementById('session-history');
                if(!rows.length){body.innerHTML='<tr><td colspan="8" class="session-empty">Sin sesiones registradas</td></tr>';return;}
                const localDay=row=>{
                    if(!row.started_local_day)return '—';
                    const offset=Number(row.started_utc_offset_min);
                    if(!Number.isFinite(offset))return row.started_local_day;
                    const sign=offset>=0?'+':'−', absolute=Math.abs(offset);
                    return `${row.started_local_day} · UTC${sign}${String(Math.floor(absolute/60)).padStart(2,'0')}:${String(absolute%60).padStart(2,'0')}`;
                };
                body.innerHTML=rows.map(row=>`<tr><td>#${row.id}</td><td>${this.esc(localDay(row))}</td><td>${this.esc(row.started_at)}</td><td>${this.esc(row.ended_at||'ACTIVA')}</td><td>${this.esc(row.robot_id||'pendiente')}</td><td>${row.samples}</td><td>${this.esc(row.disconnect_reason||'—')}</td><td><a href="/api/v1/sessions/${row.id}/telemetry.csv">CSV</a><a href="/api/v1/sessions/${row.id}.json">JSON</a></td></tr>`).join('');
            } catch(error){LogSystem.add('ERROR',`Historial: ${error.message}`);}
        }
    };

/* ==============================================
       INIT GENERAL
       ============================================== */
    function init() {
        LogSystem.init(); MotorViz.init(); EncoderSpeed.init(); FSMDisplay.update('INIT');
        Calibration.init(); SurfaceManager.init(); TabManager.init(); ConnectionPanel.init(); initChart(); Touch.init();
        if (typeof RouteManager !== 'undefined') RouteManager.init();
        document.querySelectorAll('.route-cinema, .route-cinema-floating-close').forEach(button=>button.addEventListener('click',()=>RouteManager.toggleCinema(button.dataset.card)));
        document.addEventListener('keydown', (e) => {
            if (e.key === 'Escape' || e.key === 'Esc') {
                const activeCinema = document.querySelector('.route-map-card.cinema-mode');
                if (activeCinema) {
                    RouteManager.toggleCinema(activeCinema.id);
                }
            }
        });
        document.addEventListener('visibilitychange',()=>{if(document.hidden)Touch.end();});
        window.addEventListener('blur',()=>Touch.end());
        window.addEventListener('pagehide',()=>Touch.end());

        // Algunos teclados numéricos Android (incluido Samsung) no muestran
        // el signo menos. Este control permite introducir valores negativos
        // sin depender de la distribución concreta del teclado virtual.
        document.querySelectorAll('[data-toggle-sign]').forEach(button=>button.addEventListener('click',()=>{
            const input = document.getElementById(button.dataset.toggleSign);
            const parsed = Number.parseFloat(input.value);
            input.value = Number.isFinite(parsed) ? String(parsed === 0 ? -1 : -parsed) : '-1';
            input.dispatchEvent(new Event('input', {bubbles:true}));
            input.focus();
        }));

        // E‑STOP
        document.getElementById('btn-estop').addEventListener('click', function() {
            this.classList.add('btn-flash');
            setTimeout(() => this.classList.remove('btn-flash'), 300);
            if (typeof RouteManager !== 'undefined') {
                if (RouteManager.isRunning) {
                    RouteManager.failRoute('Parada de emergencia (E-STOP) activada por el operador: ruta cancelada.');
                }
                RouteManager.unlockTabs();
            }
            WSControl.send({cmd:"estop"});
            LogSystem.add('ERROR', 'E-STOP enviado al robot');
        });
        document.getElementById('btn-close-app').addEventListener('click',()=>WSControl.closeApplication(false));

        // Testing: Aplicar PWM (Bloqueado por protocolo canónico de pasos)
        document.getElementById('btn-apply-pwm').disabled = true;
        document.getElementById('btn-apply-pwm').title = `No disponible en ${CANONICAL_CONFIG.PROTOCOL_NAME}. El hardware DRV8833 exige control cerrado por MPU.`;
        document.getElementById('btn-apply-pwm').addEventListener('click', function() {
            LogSystem.add('WARN', `Control PWM directo bloqueado: el protocolo ${CANONICAL_CONFIG.PROTOCOL_NAME} protege los DRV8833 exigiendo control en lazo cerrado.`);
            this.classList.add('btn-flash'); setTimeout(()=>this.classList.remove('btn-flash'),250);
        });

        document.querySelectorAll('.dir-btn').forEach(b=>b.addEventListener('click',function(){
            const m = this.dataset.motor;
            document.querySelectorAll(`.dir-btn[data-motor="${m}"]`).forEach(el => el.classList.remove('active-fwd','active-rev','active-stop'));
            this.classList.add(this.dataset.dir==='fwd'?'active-fwd':this.dataset.dir==='rev'?'active-rev':'active-stop');
            state.motorDir[m] = this.dataset.dir;
        }));

        document.querySelectorAll('[data-cmd]').forEach(b=>b.addEventListener('click',function(){
            const c=this.dataset.cmd;
            const dist=parseFloat(document.getElementById('cmd-dist').value)||50;
            const angle=parseFloat(document.getElementById('cmd-angle').value)||90;
            this.classList.add('btn-flash'); setTimeout(()=>this.classList.remove('btn-flash'),250);

            if(c==='avance'){
                WSControl.send({cmd:'step',heading:0,cm:dist,drive_mode:'forward'});
            } else if(c==='retroceso'){
                WSControl.send({cmd:'step',heading:180,cm:dist,drive_mode:'reverse'});
            } else if(c==='giro-izq'){
                WSControl.send({cmd:'turn_to',heading:(Number(state.telemetria.angulo||0)-angle+360)%360});
            } else if(c==='giro-der'){
                WSControl.send({cmd:'turn_to',heading:(Number(state.telemetria.angulo||0)+angle)%360});
            } else if(c==='parada'){
                WSControl.send({cmd:"stop"});
                LogSystem.add('CMD', 'Parada inmediata enviada');
            } else if(c==='reset'){
                WSControl.send({cmd:"reset_pose"});
                LogSystem.add('CMD', 'Reset pose enviado');
            }
        }));

        document.querySelectorAll('.log-filter-btn').forEach(b=>b.addEventListener('click',function(){
            this.classList.toggle('active');
            this.setAttribute('aria-pressed', this.classList.contains('active') ? 'true' : 'false');
            LogSystem.setFilter(this.dataset.filter, this.classList.contains('active'));
        }));
        
        document.getElementById('btn-download-log').addEventListener('click',()=>LogSystem.downloadTXT());
        document.getElementById('btn-clear-log').addEventListener('click',()=>{LogSystem.clear();LogSystem.add('INFO','Log limpiado');});
        document.getElementById('btn-purge-db-logs').addEventListener('click', async () => {
            const select = document.getElementById('select-purge-days');
            const days = parseInt(select.value, 10);
            const label = select.options[select.selectedIndex].text;
            if (!confirm(`¿Confirmas eliminar los registros de la Base de Datos con antigüedad: ${label}?`)) return;
            try {
                const token = document.querySelector('meta[name="app-token"]').content;
                const res = await fetch('/api/v1/sessions/cleanup', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json', 'X-App-Token': token },
                    body: JSON.stringify({ days })
                });
                const data = await res.json();
                if (res.ok) {
                    LogSystem.add('INFO', `Limpieza de Base de Datos exitosa: ${data.message}`);
                    if (typeof ConnectionPanel !== 'undefined' && ConnectionPanel.loadHistory) ConnectionPanel.loadHistory();
                } else {
                    LogSystem.add('ERROR', `Error al limpiar BD: ${data.error || 'error desconocido'}`);
                }
            } catch (err) {
                LogSystem.add('ERROR', `Error al conectar con backend para limpieza: ${err.message}`);
            }
        });

        WSControl.init();
    }

    if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', init); 
    else init();

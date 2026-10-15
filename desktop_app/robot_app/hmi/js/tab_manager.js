/* ==============================================
       TAB MANAGER
       ============================================== */
    const TabManager = {
        init() {
            const tabs=[...document.querySelectorAll('.tab-btn')];
            tabs.forEach((button,index)=>{
                button.addEventListener('click',()=>{
                    if(button.getAttribute('aria-disabled')==='true')return;
                    const tabId = button.dataset.tab;
                    const winId = button.getAttribute('data-win') || (tabId === 'boost' ? 'tab-boost' : `tab-${tabId}`);
                    
                    // Comportamiento de barra de tareas estilo Windows
                    if (typeof WindowManager !== 'undefined' && WindowManager.mode !== 'tabs') {
                        const win = WindowManager.windows.get(winId);
                        if (win) {
                            if (win.isMinimized) {
                                WindowManager.restoreWindow(winId);
                                WindowManager.bringToFront(winId);
                                if (WindowManager.mode === 'tiling') {
                                    win.el.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
                                }
                            } else if (win.el.classList.contains('is-focused')) {
                                WindowManager.minimizeWindow(winId);
                            } else {
                                WindowManager.bringToFront(winId);
                                if (WindowManager.mode === 'tiling') {
                                    win.el.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
                                }
                            }
                            return;
                        }
                    } else if (typeof WindowManager !== 'undefined' && WindowManager.mode === 'tabs') {
                        const win = WindowManager.windows.get(winId);
                        if (win && win.isMinimized) {
                            WindowManager.restoreWindow(winId);
                            this.switch(tabId);
                            return;
                        } else if (win && state.activeTab === tabId && !win.isMinimized) {
                            WindowManager.minimizeWindow(winId);
                            return;
                        }
                    }
                    this.switch(tabId);
                });
                button.addEventListener('keydown',event=>{
                    let targetIndex=null;
                    if(event.key==='ArrowRight')targetIndex=(index+1)%tabs.length;
                    if(event.key==='ArrowLeft')targetIndex=(index-1+tabs.length)%tabs.length;
                    if(event.key==='Home')targetIndex=0;
                    if(event.key==='End')targetIndex=tabs.length-1;
                    if(targetIndex===null)return;
                    event.preventDefault();
                    tabs[targetIndex].focus();
                    tabs[targetIndex].scrollIntoView({block:'nearest',inline:'nearest'});
                });
            });
        },
        switch(id) {
            const requestedTab=document.querySelector(`.tab-btn[data-tab="${id}"]`);
            if(!requestedTab || requestedTab.getAttribute('aria-disabled')==='true')return;
            if(!Calibration.calibrated && id!=='conexion')return LogSystem.add('WARN','Completa primero la calibración física de torque y giro.');
            if (typeof RouteManager !== 'undefined' && RouteManager.isRunning && id !== 'rutas') return;
            if (state.activeTab === 'touch' && id !== 'touch') Touch.end();
            state.activeTab=id;
            document.querySelectorAll('.tab-btn').forEach(b=>{
                const a=b.dataset.tab===id;
                b.classList.toggle('active',a);
                if (a) b.classList.remove('is-minimized');
                b.setAttribute('aria-selected',a);
                b.tabIndex=a?0:-1;
            });
            document.querySelectorAll('.tab-content').forEach(c=>{
                const isActive = c.id===`tab-${id}` || c.id===id;
                c.classList.toggle('active', isActive);
                if (isActive) {
                    const isFlexMode = typeof WindowManager !== 'undefined' && (WindowManager.mode === 'floating' || WindowManager.mode === 'tiling');
                    c.style.display = isFlexMode ? 'flex' : 'block';
                } else if (typeof WindowManager === 'undefined' || WindowManager.mode === 'tabs') {
                    c.style.display = 'none';
                }
            });
            if (typeof WindowManager !== 'undefined') {
                const winId = id.startsWith('tab-') ? id : `tab-${id}`;
                const win = WindowManager.windows.get(winId) || WindowManager.windows.get(id);
                if (win) {
                    win.isMinimized = false;
                    win.el.style.display = (WindowManager.mode === 'tiling' || WindowManager.mode === 'floating') ? 'flex' : 'block';
                    if (WindowManager.mode === 'floating' && WindowManager._applyWindowRect) {
                        WindowManager._applyWindowRect(win);
                    } else if (WindowManager.mode === 'tiling') {
                        win.el.style.height = '100%';
                        win.el.style.width = win.tilingWidth ? `${win.tilingWidth}px` : '';
                    }
                    WindowManager.bringToFront(win.id);
                }
            }
            if (id === 'rutas' && typeof RouteManager !== 'undefined') RouteManager.resizeCharts();
        }
    };

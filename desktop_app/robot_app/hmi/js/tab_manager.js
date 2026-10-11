/* ==============================================
       TAB MANAGER
       ============================================== */
    const TabManager = {
        init() {
            const tabs=[...document.querySelectorAll('.tab-btn')];
            tabs.forEach((button,index)=>{
                button.addEventListener('click',()=>{
                    if(button.getAttribute('aria-disabled')==='true')return;
                    this.switch(button.dataset.tab);
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
            document.querySelectorAll('.tab-btn').forEach(b=>{const a=b.dataset.tab===id;b.classList.toggle('active',a);b.setAttribute('aria-selected',a);b.tabIndex=a?0:-1;});
            document.querySelectorAll('.tab-content').forEach(c=>c.classList.toggle('active',c.id===`tab-${id}`));
            if (id === 'rutas' && typeof RouteManager !== 'undefined') RouteManager.resizeCharts();
        }
    };

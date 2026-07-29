# Roadmap de Mejoras y Auditoría de Sistema

Este documento recopila la evaluación de hallazgos en la arquitectura actual del sistema (Firmware ESP32-S3, HMI Web y Backend Python) y establece el mapa de ruta para evoluciones futuras.

---

## 1. Evaluación y Filtrado de Hallazgos

### A. Hallazgos Descartados (Obsoletos / No Aplican)
Los siguientes puntos pertenecen a versiones antiguas del firmware o protocolos heredados ya reemplazados en `robot-s3-steps-v2`:
- **Watchdog de 6000 ms vs 450 ms**: La arquitectura actual del súper-ciclo a 100 Hz utiliza lazo continuo con `DRIVE_STALL_MS = 6000 ms` e interlocks universales por lado. No se aplican cortes arbitrarios de 450 ms que interrumpan la búsqueda de torque.
- **Reevaluación de Encoders**: Las rutinas de reevaluación de encoders fueron consolidadas en el control de deltas filtradas sobre PCNT.
- **Protocolo de Misiones Legacy (`mission_upload` / `mission_start`)**: En el firmware actual, las misiones se ejecutan como secuencias paso a paso de alta precisión a través de comandos `step`. La base de datos SQLite histórica de ADB contiene registros de comandos deprecados.

---

### B. Hallazgos Confirmados en Backend y HMI (Roadmap de Corrección)
1. **Descomposición del Último Paso de Regreso a 0° en HMI (`index.html`)**:
   - *Diagnóstico*: En `ockhamReturn()`, la comprobación `currentYaw > 3.5` compara la unidad de orientación de la telemetría contra un valor escalar ambiguo y envía comandos de micro-desplazamientos de 0.5–1 cm con heading 0°, lo que puede causar zumbidos innecesarios de motores o alternancia prematura de fase.
   - *Acción en Roadmap*: Unificar la reorientación final como un comando exclusivo de giro sin vector de desplazamiento.

2. **Acumulación de Error Geométrico en Tramos Ortogonales**:
   - *Diagnóstico*: Los segmentos subsiguientes se calculan desde la posición planeada ideal en lugar de la pose real integrada en telemetría.
   - *Acción en Roadmap*: Implementar compensación de pose real en el backend antes de calcular la distancia del siguiente paso.

3. **Reconciliación de Eventos y Cola WebSocket**:
   - *Diagnóstico*: En transmisiones simultáneas entre `Task_Web` y eventos del súper-ciclo, la cola de eventos puede llegar a omitir notificaciones terminales en clientes Web lentos.
   - *Acción en Roadmap*: Incorporar mutex en `enviarJSON()` y activar el polling de reconciliación en la capa Python/HMI.

---

## 2. Propuesta Arquitectónica: Ruta Ockham Inversa $f^{-1}$

### Concepto Matemático de Inversión de Pasos
Actualmente, el regreso al origen en la Ruta Ockham calcula una trayectoria ortogonal directa basada en las coordenadas finales $(X, Y)$. Se propone una alternativa basada en la inversión de la función de trayectoria ejecutada:

$$\text{Ruta Directa: } f(\text{pasos}) = \{p_1, p_2, p_3, \dots, p_n\}$$

$$\text{Ruta Ockham Inversa: } f^{-1}(\text{pasos}) = \{p_n^{-1}, p_{n-1}^{-1}, \dots, p_2^{-1}, p_1^{-1}\}$$

### Mecanismo de Ejecución:
1. **Pila de Pasos (`Step Stack`)**: Cada paso ejecutado exitosamente se almacena en una pila con su distancia $d_i$ y su rumbo $\theta_i$.
2. **Desplazamiento Inverso**: Al solicitar "Regresar al Inicio (Ockham)", el sistema desapila los pasos en orden inverso ($p_n \to p_1$).
3. **Inversión Vectorial**:
   - Para cada paso $p_i = (\theta_i, d_i)$, se ejecuta la maniobra especular:
     $$\theta_{\text{retorno}} = (\theta_i + 180^\circ) \bmod 360^\circ$$
     $$\text{distancia} = d_i$$
4. **Resultado**: El robot desanda exactamente el mismo camino físico por el que avanzó, sortea obstáculos previamente librados y concluye en el origen $(0,0)$ orientado exactamente a $0.0^\circ$ (Norte cardinal).

---

## 3. Estado de Aceptación del Sistema Actual (MVP)

- **Firmware ESP32-S3**: Aprobado y verificado. Compilación limpia (RAM 15.1%, Flash 41.8%).
- **Límites Eléctricos del DRV8833**: Aprobados (Tope 90% avance, 97% giro; tiempo muerto universal de 250 ms al invertir sentido).
- **Giro y Orientación**: Rampa adaptativa para vencer fricción en macro-giros + ráfagas de micro-pulsos (50 ms ON / 100 ms OFF) en $<5^\circ$ con precisión flotante `fabsf(gyro_z)`.
- **Rutas Ortogonales Basales**: Funcionamiento verificado en pruebas continuas.

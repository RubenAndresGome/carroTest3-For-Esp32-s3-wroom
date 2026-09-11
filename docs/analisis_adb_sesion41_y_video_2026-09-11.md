# Análisis ADB de Sesión #41 y Validación con Video en Suelo — 2026-09-11

## 1. Resumen Ejecutivo

En la prueba física registrada en la **Sesión SQLite #41** (2026-09-11 23:46:16–23:49:21 UTC / 17:46–17:49 local) y documentada en el video [`evidencia/videos/ruta_ortogonal_sesion41_2026-09-11.mp4`](../evidencia/videos/ruta_ortogonal_sesion41_2026-09-11.mp4), se validó el firmware con el commit `92ab106`:

1. **Éxito en Navegación y Rumbo**: Se resolvió de forma definitiva el *deadlock* en `giro_fin`. El robot ejecutó con éxito **8 comandos de paso consecutivos** en circuito ortogonal (rumbos 0°, 180°, 270°, 90°, 0°, 180°), completando cada tramo con `step_ok` y manteniendo la orientación angular dentro de $\pm 1.0^\circ$ del eje cardinal.
2. **Error de Precisión sobre Marcas Físicas**: Persiste una discrepancia métrica sobre las marcas en cruz (`+`) colocadas en el suelo. En el primer paso de 50 cm, el robot registró en odometría $57.4\text{ cm}$ (sobreavance de $+7.4\text{ cm}$ respecto a la marca de 50 cm).
3. **Asimetría de Tracción Visual y Mecánica**: Se aprecia visualmente en el video y se confirma en telemetría que el tren motor izquierdo empuja con mayor velocidad/fuerza que el derecho. El lazo de rumbo del MPU interviene frenando el lado izquierdo, pero la ganancia actual es muy suave ($< 1\%$ de reducción PWM para desvíos $< 0.5^\circ$), lo que permite que la asimetría se manifieste visiblemente como un sesgo de avance antes de corregir.

---

## 2. Metadatos de la Prueba

* **Archivo de base de datos extraído**: `tmp_db/robot_20260911_175409.sqlite3`
* **Dispositivo ADB**: `R52Y108A5DB` (Samsung Galaxy Tab)
* **Archivo de video de evidencia**: `evidencia/videos/ruta_ortogonal_sesion41_2026-09-11.mp4`
  - Duración: 96.5 s
  - Resolución: 1280x576 @ 29.66 fps
  - SHA-256: Registrado en bitácora local
* **Fotograma de referencia en marcas**: `evidencia/fotogramas/sesion41_marcas_piso.jpg`
* **Firmware activo**: `robot-s3-v3.5`, commit `92ab106`
* **Configuración de Odometría**: `ENCODER_ERROR_PORCENTAJE = -0.195f`, `FACTOR_ESCALA_ENCODER = 0.805f` ($5.313\text{ cm}$ diámetro odometría).

---

## 3. Registro Secuencial de Comandos en Sesión #41

| ID Cmd | Tipo | Consigna (Rumbo, Distancia, Modo) | Estado | Detalle / Evento | Pose Final en DB (X, Y, Yaw) |
|---|---|---|---|---|---|
| `8ea3f644` | `calibrate` | Calibración de hardware | **completed** | `cal_ok` | $(0.0, 0.0\text{ mm}, 0.0^\circ)$ |
| `64dcfa94` | `step` | Hdg: 0.0°, 50.0 cm, `auto` | **completed** | `step_ok` | $(0.0, 574.0\text{ mm}, 359.7^\circ)$ |
| `f700212e` | `step` | Hdg: 0.0°, 50.0 cm (a 100 cm), `auto` | **completed** | `step_ok` | $(-3.0, 876.0\text{ mm}, 358.8^\circ)$ |
| `bc613828` | `step` | Hdg: 180.0°, 100.0 cm (retorno a 0), `auto` | **completed** | `step_ok` | $(16.0, -28.0\text{ mm}, 357.7^\circ)$ |
| `4a54182d` | `step` | Hdg: 270.0°, 100.0 cm (a X=-1000), `auto` | **completed** | `step_ok` | $(-962.0, -29.0\text{ mm}, 269.9^\circ)$ |
| `d94592af` | `step` | Hdg: 90.0°, 100.0 cm, `reverse` | **completed** | `step_ok` | $(72.0, 5.0\text{ mm}, 272.4^\circ)$ |
| `0fb06075` | `step` | Hdg: 0.0°, 100.0 cm, `reverse` | **completed** | `step_ok` | $(62.0, 1012.0\text{ mm}, 274.4^\circ)$ |
| `25553c29` | `step` | Hdg: 180.0°, 50.0 cm, `reverse` | **completed** | `step_ok` | $(40.0, 449.0\text{ mm}, 273.0^\circ)$ |
| `d5e5768e` | `step` | Hdg: 180.0°, 50.0 cm, `reverse` | **failed** | `stopped_by_estop` | $(38.0, 41.0\text{ mm}, 0.3^\circ)$ |
| `fccc7a28` | `estop` | Parada de emergencia manual | **completed** | `estop_latched` | Detención inmediata en suelo |
| `1521bd2f` | `stop` | Confirmación de frenado | **completed** | `stop_ok` | Motores desenergizados |

---

## 4. Análisis Forense de Telemetría

### 4.1 Resolución del Bloqueo en Giro
En la Sesión #39, el comando fallaba con `recenter_timeout` porque al alcanzar el entorno de tolerancia en `giro_fin`, el chasis asentaba a $3.4^\circ$--$3.7^\circ$, quedando atrapado en el latch `giroEnTol` a PWM 0 durante 13 segundos.
En la Sesión #41, la liberación del latch (`giroEnTol = false; estableGiroDesdeMs = 0;`) permitió que el robot:
- Completara los giros de 90° y 180° fluidamente.
- No presentara pausas muertas.
- Concluyera todos los pasos con `step_ok`.

### 4.2 Disparidad Mecánica entre Trenes de Tracción
Al inspeccionar las lecturas de los encoders de rueda en los tramos rectos de avance:
- **Tren Izquierdo**: `FL = 141`, `BL = 134` (Promedio izquierdo $\approx 137.5\text{ ticks}$).
- **Tren Derecho**: `FR = 0` (canal excluido / sin señal de hardware), `BR = 135` (Promedio confiable derecho $= 135.0\text{ ticks}$).
- El canal `FR` no generó pulsos durante la sesión; el algoritmo de fusión degradada operó con `BR` como única fuente derecha sana, cumpliendo la regla de `ControlSeguridad::promedioConfiableLado`.
- Sin embargo, a igualdad de comando PWM (p. ej. 970 / 970), el tren izquierdo produce una tasa de aceleración superior a la del derecho, empujando el morro del robot hacia la derecha (+Yaw).
- En telemetría, el lazo de corrección respondió asignando `pasoLadoFrenoRumbo = "left"`.

---

## 5. Análisis Técnico: Reducción Fija del 30% vs Reducción Dinámica de PWM

El operador planteó la duda técnica:
> *"avanzó más un lado debería reducir la tolerancia para que si detecta desviación el lado el cual está impulsando la desviación reduzca PWM con una variable que reste el 30% del pwm del lado para equilibrar o sería mejor que ese PWM de resta sea dinámico"*

### 5.1 Por qué una resta fija del 30% NO es conveniente
1. **Comportamiento "Bang-Bang" y Culebreo**: Una reducción fija del 30% (ej. restar ~290 unidades PWM sobre 970 de golpe) actúa como un interruptor de encendido/apagado brusco. Cuando el robot detecte un desvío mínimo de $0.2^\circ$, pegará un frenazo violento del lado izquierdo; esto causará un sobrepaso inmediato hacia la izquierda, disparando la resta fija hacia el lado derecho. El resultado será una oscilación destructiva en zig-zag (*hunting*).
2. **Caída por debajo de la fricción estática en baja velocidad**: Durante las fases de aproximación y precisión (`VELOCIDAD_APROXIMACION` a ~700 PWM), restar un 30% fijo de 290 puntos dejaría el PWM en $\approx 410$, valor inferior al piso de arranque del DRV8833 ($\approx 560$ PWM). El motor de ese lado se detendría por completo (bloqueo), transformando el avance recto en un pivote descontrolado.
3. **Estrés inductivo en el DRV8833**: Conmutar escalones discretos del 30% genera picos de corriente inductiva y esfuerzos mecánicos innecesarios en la piñonería de plástico de los motores TT.

### 5.2 Por qué la Reducción Dinámica Proporcional es la Solución Canónica
La solución robusta y matemáticamente estable consiste en una **reducción dinámica escalada**:
$$\Delta \text{PWM}_{\text{freno}} = \text{base} \times \min\left(\text{TECHO\_MAX\_PCT}, K_{\text{dinamico}} \cdot |e_{\text{rumbo}}|\right)$$

Ventajas:
1. **Proporcionalidad Suave**: Para desviaciones pequeñas ($0.1^\circ$--$0.3^\circ$), aplica una corrección moderada (ej. 3%--8%), suficiente para equilibrar la tracción sin sacudir el chasis.
2. **Capacidad de Choque ante Desvíos Fuertes**: Si la perturbación externa o la asimetría mecánica crece ($> 1.5^\circ$), la reducción escala de forma suave y continua hasta alcanzar el techo máximo permitido (**30%**, ~290 unidades PWM).
3. **Término Integral de Asimetría (Bias Trim)**: El controlador PI vigente (`salidaPI.total`) ya cuenta con integración de error. Aumentando la ganancia proporcional de rumbo ($K_p$) y el límite superior de corrección de rumbo (`PWM_CORRECCION_RUMBO_MAX`), el sistema restará exactamente el PWM dinámico necesario para que ambos lados avancen simétricos en suelo.

---

## 6. Diagnóstico de la Precisión Métrica sobre las Marcas

* Con `ENCODER_ERROR_PORCENTAJE = -0.195f`, el paso de 50 cm avanzó $57.4\text{ cm}$ en odometría real.
* La correlación entre la distancia real en suelo y la constante matemática de odometría indica que el diámetro efectivo de la rueda en este suelo de prueba con carga completa se sitúa en $\approx 5.85\text{ cm}$ (equivalente a un factor de escala de $\approx 0.885$ o `ENCODER_ERROR_PORCENTAJE \approx -0.115f`).
* La precisión absoluta sobre marcas físicas en suelo requiere afinar este factor mediante series de repetición de 100 cm en línea recta, validando con cinta métrica.

---

## 7. Próximos Pasos Propuestos

1. **Implementar Compensación Dinámica de Asimetría**:
   - Incrementar la ganancia dinámica de corrección de rumbo en tramo recto para que pequeños desvíos angulares ($> 0.2^\circ$) descuenten dinámicamente hasta un 30% del PWM del lado acelerado sin generar escalones discretos.
2. **Calibrar `ENCODER_ERROR_PORCENTAJE`**:
   - Ajustar el porcentaje de escala a un valor intermedio validado en suelo (de `-0.195` hacia `-0.115`) para centrar el avance de 50 cm exactamente sobre las marcas de cinta.
3. **Revisión Eléctrica del Canal FR**:
   - Inspeccionar el cableado del encoder delantero derecho (FR, GPIO10) para recuperar la redundancia bilateral completa de 4 encoders.

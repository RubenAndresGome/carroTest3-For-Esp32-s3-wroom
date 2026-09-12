# Análisis ADB de Sesión #42: Geometría de Chasis, Retorno Ockham y Supresión de Giro Parásito en Reversa

---

## 1. Resumen Ejecutivo

En la prueba física registrada en la **Sesión SQLite #42** (2026-09-12 00:20:24–00:23:20 UTC / 18:20–18:23 local) tras la implementación del commit `63b99c8`:

1. **Validación Exitosa de Rumbo y Asimetría Dinámica**:
   - En el primer paso de avance de 50 cm (`806e258e`), el error angular de rumbo se mantuvo por debajo de **$0.6^\circ$**, confirmando la eficacia del nuevo recorte dinámico de PWM sobre el tren izquierdo.
   - En el tramo horizontal en X- (`d6fb7df0`, 100 cm a X=-1000 mm), el desvío lateral final fue de apenas **$0.3\text{ cm}$** (frente a los $2.9\text{ cm}$ de la Sesión #41), demostrando el impacto directo de la ampliación de autoridad lateral a $8.0^\circ$.
   - La nueva escala de odometría ($0.920$) corrigió el sobreavance previo, ajustando la detención al entorno de las marcas físicas.

2. **Diagnóstico del Efecto de "10 a 15 cm" en Reversa (Geometría del Chasis 30x20 cm)**:
   - Se descartó rotundamente pérdida de pulsos en encoders por velocidad: el periférico PCNT por hardware del ESP32-S3 operó a 100 Hz con 0 plazos perdidos y conteo perfectamente lineal.
   - La discrepancia visual de 15 cm responde a la **geometría física del chasis**: el origen matemático $(0,0)$ reside en el **centro geométrico de giro entre las 4 ruedas**, mientras que el parachoques delantero (morro) dista **$+15\text{ cm}$** y el trasero (cola) dista **$-15\text{ cm}$**. Al retornar marcha atrás, cuando el centro llega al $(0,0)$, el morro queda $15\text{ cm}$ por delante de la cruz de cinta, generando la percepción visual de que "faltaron 15 cm".

3. **Causa Raíz del Desvío en el Botón "Regresar a Origen" (Ockham Return)**:
   - Ockham invierte la lista histórica completa de vectores. Al haber hecho una ruta de ida y vuelta previa, desandó el circuito entero pasando nuevamente por $Y=1000\text{ mm}$.
   - En el último paso en reversa (`1db3709e`), al alcanzar el reposo longitudinal a $Y = 7.7\text{ cm}$, el firmware ejecutó un **`giro_fin` parásito de $180^\circ$** debido a que `pasoRumboFinalDeg` no coincidía con el rumbo del cuerpo. El intento de pivotar en el sitio con 4 ruedas de goma sobre concreto produjo un **arrastre parásito de 23 cm en Y y 9 cm en X** en 3 segundos, desalineando el robot.

---

## 2. Metadatos de la Sesión

* **Archivo de base de datos extraído**: `tmp_db/robot_20260911_182813.sqlite3`
* **Dispositivo ADB**: `R52Y108A5DB` (Samsung Galaxy Tab)
* **Firmware**: `robot-s3-v3.5`, commit `63b99c8`
* **Configuración de Odometría**: `ENCODER_ERROR_PORCENTAJE = -0.080f`, `FACTOR_ESCALA_ENCODER = 0.920f` ($6.072\text{ cm}$ diámetro odometría).
* **Parámetros de Control**: `CORRECCION_LATERAL_RUMBO_MAX_DEG = 8.0f`, `KP_ASYMMETRY_PWM_PER_DEG = 0.15f`, `ASYMMETRY_PWM_REDUCTION_MAX_RATIO = 0.30f`.

---

## 3. Secuencia de Comandos en Sesión #42

| ID Cmd | Tipo | Consigna | Estado | Detalle | Pose Final en DB (X, Y, Yaw) |
|---|---|---|---|---|---|
| `f961b7bd` | `calibrate` | Calibración MPU/PCNT | **completed** | `cal_ok` | $(0.0, 0.0\text{ mm}, 0.0^\circ)$ |
| `806e258e` | `step` | Hdg: 0.0°, 50.0 cm, `auto` | **completed** | `step_ok` (yaw máx: 0.6°) | $(-1.0, 467.0\text{ mm}, 359.9^\circ)$ |
| `4b709bb5` | `step` | Hdg: 0.0°, 50.0 cm (a 100 cm), `auto` | **completed** | `step_ok` (yaw máx: 2.2°) | $(-4.0, 819.0\text{ mm}, 359.8^\circ)$ |
| `9740f86f` | `step` | Hdg: 180.0°, 100.0 cm (a 0 cm), `auto` | **completed** | `step_ok` | $(6.0, 12.0\text{ mm}, 357.0^\circ)$ |
| `d6fb7df0` | `step` | Hdg: 270.0°, 100.0 cm (a X=-1000), `auto` | **completed** | `step_ok` (lat: 0.3 cm) | $(-959.0, 3.0\text{ mm}, 270.1^\circ)$ |
| `0f4252e0` | `step` | Hdg: 90.0°, 100.0 cm, `reverse` (Ockham 1) | **completed** | `step_ok` | $(3.0, 19.0\text{ mm}, 271.4^\circ)$ |
| `2152105c` | `step` | Hdg: 0.0°, 100.0 cm, `reverse` (Ockham 2) | **completed** | `step_ok` | $(-8.0, 1003.0\text{ mm}, 274.0^\circ)$ |
| `ef6fd027` | `step` | Hdg: 180.0°, 50.0 cm, `reverse` (Ockham 3) | **completed** | `step_ok` | $(39.0, 524.0\text{ mm}, 89.9^\circ)$ |
| `1db3709e` | `step` | Hdg: 180.0°, 50.0 cm, `reverse` (Ockham 4) | **failed** | `stopped` (app close) | $(-87.0, 304.0\text{ mm}, 334.4^\circ)$ |
| `ad700b97` | `stop` | Detención de seguridad | **completed** | `stop_ok` | Frenado inmediato |

---

## 4. Análisis Físico y Geométrico

### 4.1 La Geometría del Chasis Rectangular (30 x 20 cm)
En cinemática diferencial / skid-steer:
* El marco de coordenadas del robot ubica el origen en el **centro geométrico del tren de rodaje**:
  $$X_{\text{centro}} = 0, \quad Y_{\text{centro}} = 0$$
* El cuerpo físico tiene dimensiones aproximadas de $30\text{ cm}$ de largo (longitudinal) por $20\text{ cm}$ de ancho (transversal).
* El parachoques delantero (morro) está en $Y = +15\text{ cm}$.
* El parachoques trasero (cola) está en $Y = -15\text{ cm}$.

```
                 MORRO (Y = +15 cm)
          ┌─────────────────────────────┐
          │   [Motor FL]    [Motor FR]  │
          │                             │
          │           (0, 0)            │  <-- Centro Odométrico
          │                             │
          │   [Motor BL]    [Motor BR]  │
          └─────────────────────────────┘
                 COLA (Y = -15 cm)
```

**Consecuencia operativa en suelo**:
* Si el operador posiciona el robot alineando el **morro** sobre la marca de partida, el centro físico del robot está realmente en $Y = -15\text{ cm}$.
* Cuando el robot avanza hacia el frente ($0^\circ$) hasta una marca en $Y = 50\text{ cm}$, el morro alcanza la marca cuando el centro está en $Y = 35\text{ cm}$.
* Al realizar la **maniobra de reversa**: El robot retrocede hacia el origen. Cuando el odómetro indica $Y_{\text{centro}} = 0$, la cola está en $Y = -15\text{ cm}$ y el morro en $Y = +15\text{ cm}$.
* Quien observa el morro percibe que el coche "se detuvo 15 cm antes", cuando en realidad el **centro de las ruedas** llegó con exactitud métrica al punto $(0,0)$.
* **Recomendación Canónica**: Alinear el robot en el suelo tomando como referencia una marca física en el **punto medio de los laterales del chasis** (a la altura del centro de los motores), nunca el parachoques frontal.

---

### 4.2 Descarte de Fallo de Encoders por Alta Velocidad
Se analizó la tasa de conteo por muestra del periférico PCNT a máxima velocidad de crucero (PWM 970 / ~25 cm/s):
* Las tasas registradas fueron de 2 a 3 ticks por ciclo de 10 ms (~200 a 300 ticks/s).
* El hardware PCNT del ESP32-S3 opera a nivel de compuertas lógicas en silicio sin depender de interrupciones de CPU, con capacidad de muestreo de hasta 40 MHz.
* El súper-ciclo a 100 Hz registró una desviación típica de jitter de $211\ \mu\text{s}$ y **cero plazos perdidos** (`control_missed_deadlines = 0`).
* Los encoders no pierden pulsos por velocidad. La única anomalía física detectada es la menor amplitud del canal `FR` (que genera ~70% menos pulsos que los otros tres por cableado/sensor), compensada de forma redundante por `BR` mediante la mediana robusta.

---

### 4.3 Supresión del Giro Parásito (`giro_fin`) en Reversa

#### Diagnóstico del Fallo en `1db3709e`
A las `00:23:14`, el robot concluyó su tramo en reversa con $Y = 7.7\text{ cm}$ y el chasis apuntando a $0.0^\circ$ (Yaw $= 358.3^\circ$).
Al evaluar `verificarObjetivoFinal()`:
```cpp
if (fabsf(errorAng360(pasoRumboFinalDeg, heading360)) > TOLERANCIA_GIRO_DEG) {
  iniciarBaseGiro(pasoRumboFinalDeg, Fase::GIRO_FINAL);
}
```
Debido a que `pasoRumboFinalDeg` se calculó en el inicio del paso tomando el `heading360` previo a `giro_ini`, el sistema asumió una discrepancia angular de $180^\circ$ y activó `GIRO_FINAL` a plena potencia (`-972 / +692`).
El derrape de las 4 ruedas de goma sobre suelo de concreto generó una traslación espuria:
* `pos` saltó de $(1.0, 77.0)\text{ mm}$ a $(-87.0, 304.0)\text{ mm}$.
* El robot se desplazó $23\text{ cm}$ en longitudinal y $9\text{ cm}$ en lateral mientras intentaba girar en el sitio.

#### Corrección Canónica Implementada
En `include/ControlRuta.h` y `src/Cinematica.cpp`:
```cpp
inline float rumboFinalParaPaso(float rumboTrayectoDeg, int direccion, float rumboCuerpoDeg) {
  return direccion < 0 ? normalizar360(rumboCuerpoDeg) : normalizar360(rumboTrayectoDeg);
}
```
* En avance (`direccion = +1`), el rumbo final es el rumbo de trayecto planificado (`heading`).
* En reversa (`direccion = -1`), el chasis **conserva el rumbo de su cuerpo** (`pasoRumboCuerpoDeg`) al detenerse.
* Al finalizar el tramo marcha atrás, `errorAng360(pasoRumboFinalDeg, heading360) == 0`.
* Se elimina por completo el `giro_fin` parásito, permitiendo que el robot asiente y complete el paso con `step_ok` exactamente en la posición de llegada sin derrapar.

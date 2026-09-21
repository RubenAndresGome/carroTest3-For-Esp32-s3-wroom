# Modelos Matemáticos y de Control del Sistema (Versión Simplificada)

Este documento resume de forma analítica, intuitiva y simplificada todos los **modelos matemáticos, de filtrado, cinemática y leyes de control** implementados en el robot móvil 4WD diferencial/skid-steer sobre el ESP32-S3 (`carroTest3-For-Esp32-s3-wroom`).

---

## 1. Convención de Coordenadas y Geometría

El robot opera en un plano 2D continuo con convención cardinal/aeronáutica estándar:

```
                  FRENTE (+Y, Yaw θ = 0°)
          ┌───────────────────────────────────┐
          │   [Motor FL]          [Motor FR]  │
          │                                   │
IZQUIERDA │               (0,0)               │ DERECHA (+X, Yaw θ = +90°)
(-X)      │              Centro               │
          │                                   │
          │   [Motor BL]          [Motor BR]  │
          └───────────────────────────────────┘
                  ATRÁS (-Y, Yaw θ = 180°)
```

* **Eje longitudinal ($Y$)**: Frente $+Y$, Atrás $-Y$.
* **Eje transversal ($X$)**: Derecha $+X$, Izquierda $-X$.
* **Ángulo de Rumbo (Yaw, $\theta$)**: $0^\circ$ en $+Y$, avanza positivamente en sentido horario (dextrógiro) hacia $+X$ ($+90^\circ$).

---

## 2. Odometría y Cinemática Diferencial 4WD

A pesar de ser un chasis de 4 ruedas motrices (4WD), el robot se modela cinemáticamente como un **vehículo diferencial equivalente** agrupando las ruedas izquierdas ($FL, BL$) y derechas ($FR, BR$):

### 2.1. Escala de Pulsos a Distancia
Cada pulso de los encoders de ranura (PCNT hardware, 40 PPR) equivale a un desplazamiento lineal:

$$C_{\text{tick}} = \frac{\pi \cdot D_{\text{rueda}}}{\text{PPR}} \approx \frac{3.1416 \cdot 6.80\text{ cm}}{40} \approx 0.534\text{ cm/pulso}$$

### 2.2. Desplazamiento Lineal por Lado
$$\Delta s_L = \Delta \text{ticks}_L \cdot C_{\text{tick}}$$
$$\Delta s_R = \Delta \text{ticks}_R \cdot C_{\text{tick}}$$

### 2.3. Cinemática Directa de Pose
El avance del centro del chasis ($\Delta s$) y su actualización en el marco global $(X, Y)$ para un rumbo $\theta$:

$$\Delta s = \frac{\Delta s_L + \Delta s_R}{2}$$

$$\begin{cases}
X_{k} = X_{k-1} + \Delta s \cdot \sin(\theta_k) \\
Y_{k} = Y_{k-1} + \Delta s \cdot \cos(\theta_k)
\end{cases}$$

> **Nota de diseño**: La orientación $\theta$ no se calcula con la diferencia $(\Delta s_R - \Delta s_L)$ de los encoders debido al alto patinaje en chasis 4WD rígidos. La **IMU (MPU6050)** es la autoridad angular exclusiva.

---

## 3. Estimación Robusta de Ticks y Rechazo de Fallos

Para evitar que la suciedad en una ranura óptica o un falso contacto distorsione la odometría, se implementa un filtro de **mediana y descarte de anomalías** (`estimacionRobustaTicksAvance`):

1. **Cálculo de la mediana**: Entre los 4 canales $\{FL, FR, BL, BR\}$, se calcula la mediana $\text{med}$.
2. **Criterio de Outlier**: Un canal $i$ se descarta si su lectura difiere significativamente de la mediana:
   $$\frac{|\Delta \text{ticks}_i - \text{med}|}{\max(1, |\text{med}|)} > 0.25 \quad \text{y} \quad |\Delta \text{ticks}_i - \text{med}| \ge 5\text{ ticks}$$
3. **Promedio Filtrado**: La distancia recorrida se calcula únicamente promediando los canales que resultaron coherentes:
   $$\Delta \text{ticks}_{\text{efectivo}} = \frac{1}{N_{\text{coherentes}}} \sum_{i \in \text{coherentes}} \Delta \text{ticks}_i$$

---

## 4. Orientación y Filtrado IMU (MPU6050)

El giróscopo en el eje $Z$ mide la velocidad angular $\omega_z$ a $100\text{ Hz}$ ($\Delta t = 0.01\text{ s}$):

### 4.1. Calibración y Compensación
$$\omega_{z,\text{corregido}} = (\omega_z - \text{offset}_z) \cdot \text{Polaridad} \quad (\text{Polaridad} = -1.0)$$

### 4.2. Filtro Promedio Móvil y Zona Muerta
Se atenúa el ruido mediante una ventana de 8 muestras, aplicando un umbral estricto para evitar la deriva en reposo (drift):

$$\omega_{z,\text{filtrado}} = \begin{cases} 
0.0, & \text{si } |\omega_{z,\text{prom}}| < 0.005\text{ rad/s} \\ 
\omega_{z,\text{prom}}, & \text{en otro caso} 
\end{cases}$$

### 4.3. Integración Discreta del Ángulo
$$\theta_k = \theta_{k-1} + \left(\omega_{z,\text{filtrado}} \cdot \text{factor\_escala} \cdot \Delta t \cdot \frac{180}{\pi}\right)$$

---

## 5. Control de Avance Recto y Seguimiento de Trayectoria

Durante el avance rectilíneo hacia una meta, el robot regula simultáneamente su **distancia longitudinal**, su **desvío lateral** y su **orientación**:

```
      Ruta Deseada  ───────►────────────────────────────────► Meta
                              ▲
                              │ Error Lateral (e_lat)
                              ▼
                        [ Robot ] (Rumbo θ)
```

### 5.1. Descomposición de Errores de Trayectoria
Para un segmento con rumbo planificado $\theta_{\text{obj}}$ que parte de $(X_0, Y_0)$ hasta $(X_f, Y_f)$:

* Vector unitario de la directriz: $\vec{u} = (\sin\theta_{\text{obj}}, \cos\theta_{\text{obj}})$
* Vector unitario normal (perpendicular): $\vec{n} = (\cos\theta_{\text{obj}}, -\sin\theta_{\text{obj}})$
* Desplazamiento desde el origen: $\Delta X = X - X_0, \quad \Delta Y = Y - Y_0$

$$\begin{aligned}
e_{\text{longitudinal}} &= D_{\text{planificada}} - (\Delta X \cdot \sin\theta_{\text{obj}} + \Delta Y \cdot \cos\theta_{\text{obj}}) \\
e_{\text{lateral}} &= \Delta X \cdot \cos\theta_{\text{obj}} - \Delta Y \cdot \sin\theta_{\text{obj}}
\end{aligned}$$

### 5.2. Corrección Proporcional Lateral
El desvío lateral genera una consigna angular correctiva para reintegrar el robot a la línea recta:

$$\theta_{\text{corrección}} = \text{limitar}\left(-e_{\text{lateral}} \cdot K_{\text{lat}}, -\theta_{\text{max\_corr}}, +\theta_{\text{max\_corr}}\right)$$

### 5.3. Controlador PD de Rumbo en Marcha
Con el error angular total $e_\theta = (\theta_{\text{obj}} + \theta_{\text{corrección}}) - \theta_{\text{actual}}$:

$$\text{PWM}_{\text{corrección}} = \text{limitar}\left(e_\theta \cdot K_p - \omega_{z,\text{filtrado}} \cdot K_d, -\text{PWM}_{\text{max\_corr}}, +\text{PWM}_{\text{max\_corr}}\right)$$

* $K_p = 4.0$: Fuerza de alineación proporcional.
* $K_d = 12.0$: Amortiguamiento derivativo con giroscopio para evitar sacudidas.

### 5.4. Modulación Motriz
$$\begin{cases}
\text{PWM}_L = \text{PWM}_{\text{base}} - \text{PWM}_{\text{corrección}} \\
\text{PWM}_R = \text{PWM}_{\text{base}} + \text{PWM}_{\text{corrección}}
\end{cases}$$

---

## 6. Control de Giro Puro (Pivote) y Frenado Activo

Las rotaciones se realizan con giro sobre el eje central (pivote puro: motores izquierdos y derechos en sentidos opuestos).

### 6.1. Control Híbrido de 3 Zonas
Para un error angular $e_\theta = \theta_{\text{meta}} - \theta$:

1. **Zona de Potencia ($|e_\theta| > 15^\circ$)**:
   Aplica rampa de par progresiva para romper inercia estática:
   $$\text{PWM} = \min(247, \text{PWM}_{\text{arranque}} + \Delta \text{par})$$
2. **Zona de Desaceleración Lineal ($4^\circ \le |e_\theta| \le 15^\circ$)**:
   $$\text{PWM}(e_\theta) = \text{PWM}_{\text{mínimo}} + (\text{PWM}_{\text{máx}} - \text{PWM}_{\text{mínimo}}) \cdot \frac{|e_\theta|}{15^\circ}$$
3. **Zona de Micro-Pulsos Finitos ($|e_\theta| < 4^\circ$)**:
   Micro-pulsos de torque suave ($\Delta t_{\text{on}} = 45\text{ ms}$, $\text{PWM} = \text{PWM}_{\text{calibrado}} + 5$), seguidos de reposo con freno activo para asentar el chasis sin sobrepasar el ángulo.

### 6.2. Frenado Activo Predictivo por Inercia (Back-EMF DRV8833)
Basado en las ecuaciones de física rotacional clásica ($v^2 = 2 \cdot a \cdot d$):

$$\Delta \theta_{\text{freno}} = \frac{\omega_z^2}{2 \cdot \alpha_{\text{frenado}}}$$

* $\omega_z$: Velocidad angular actual en $^\circ/\text{s}$.
* $\alpha_{\text{frenado}} = 2400^\circ/\text{s}^2$: Tasa de desaceleración lograda al cortocircuitar los devanados del motor (Back-EMF: IN1=1, IN2=1).

**Regla de Decisión**:
$$\text{Si } |e_\theta| \le \Delta \theta_{\text{freno}} \implies \text{Activar Freno Activo Inmediato}$$

Esto disipa la energía cinética remanente de forma exacta y deposita el chasis en $0^\circ$ o $90^\circ$ sin oscilaciones ni latigazos.

---

## 7. Compensación de Traslación Parásita (ICR)

Debido a que el centro de masas del robot no coincide perfectamente con el centro geométrico y los 4 puntos de apoyo en suelo hiperestático resbalan con fricciones asimétricas, un giro pivote puro induce un desplazamiento lateral no deseado.

Se modela mediante el **Centro Instantáneo de Rotación (ICR)** $(x_{\text{ICR}}, y_{\text{ICR}})$:

$$\begin{pmatrix} \Delta x_{\text{chasis}} \\ \Delta y_{\text{chasis}} \end{pmatrix} = \begin{pmatrix} -y_{\text{ICR}} \cdot \Delta \theta_{\text{rad}} \\ x_{\text{ICR}} \cdot \Delta \theta_{\text{rad}} \end{pmatrix}$$

Rotando este sesgo al marco inercial global:

$$\begin{pmatrix} \Delta X_{\text{parásito}} \\ \Delta Y_{\text{parásito}} \end{pmatrix} = \begin{pmatrix} \cos\theta & \sin\theta \\ -\sin\theta & \cos\theta \end{pmatrix} \begin{pmatrix} \Delta x_{\text{chasis}} \\ \Delta y_{\text{chasis}} \end{pmatrix}$$

Esta corrección se descuenta de la odometría global para mantener impecables las coordenadas de navegación.

---

## 8. Límites y Protecciones Eléctricas del DRV8833

| Parámetro | Valor Límite | Propósito Físico |
| :--- | :--- | :--- |
| **PWM Máximo Continuo** | $242 / 255$ (~$95\%$) | Protección térmica continua de etapas H-Bridge. |
| **PWM Máximo en Giros** | $247 / 255$ (~$97\%$) | Par suficiente para romper estricción en superficies gomosas/alfombras. |
| **Ráfagas al 100% (Burst)** | $\le 80\text{ ms}$ | Desenclavamiento inicial de cajas reductoras TT sin sobrecalentamiento. |
| **Tiempo Muerto de Inversión** | $250\text{ ms}$ | Pausa obligatoria al cambiar dirección para extinguir corrientes de retorno y evitar corto por conducción cruzada (*shoot-through*). |
| **Zona Muerta Dinámica** | $\le 180 / 255$ | Piso dinámico solo en arranque; nunca se suma sobre crucero. |
| **Watchdog de Avance** | $450\text{ ms}$ | Corte preventivo inmediato ante falta de pulsos de encoder. |
| **Watchdog de Giro** | $2.5\text{ s}$ | Corte de seguridad ante rotor bloqueado (stall) durante rotación. |

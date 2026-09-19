# Arquitectura de Control Cinemático y Control de Rumbo

Este documento describe la arquitectura de control, cinemática, odometría y protecciones de potencia implementadas en el firmware modular del ESP32-S3 (`src/Cinematica.cpp`, `src/PoseEstimator.cpp`, `include/Config.h` y `src/Motores.cpp`).

---

## 1. Desacoplamiento de Parámetros de Giro y Avance

Para evitar sobrepasos por inercia en maniobras de rotación y asegurar fuerza suficiente en rectas, el firmware desacopla los parámetros de potencia y tiempos entre ambas maniobras:

### Giro Pivote (`GIRO`)

- **Control Híbrido y Rampa Adaptativa**: El giro inicia con una rampa suave de torque. Mientras no se confirme el movimiento físico mediante giroscopio y encoders, el torque escala en rampa adaptativa hasta alcanzar **247/255** (~97%), venciendo la fricción estática de la caja reductora TT en superficies difíciles sin gatillar falsos stalls.
- **Aproximación Fina (Micro-pulsos)**: Cuando el error angular es $< 5^\circ$, el sistema conmuta a un régimen de micro-pulsos intermitentes (`TURN_PULSE_ON_MS` / `TURN_PULSE_OFF_MS`) para evaluar la inercia, integrar el IMU con alta fidelidad y evitar sobrepasos.
- **Tolerancias y Finalización**: El margen durante navegación general se ubica en **3.5°** (`TOLERANCIA_GIRO_DEG`), mientras que la alineación fina y calibración fuerzan una precisión $< 1.0^\circ$ antes de confirmarse.
- **Tiempos Límite (Watchdogs por fase)**: El sistema implementa timeouts según la fase: calibración (20 s), giros regulares (12 s o watchdog de giro de 2.5 s tras alcanzar torque) y avance (watchdog de 450 ms por falta de ticks).

### Avance Rectilíneo (`AVANCE`)

- **PWM Máximo Continuo**: **242/255** (~95% de potencia continua sostenida).
- **Régimen de Ráfagas al 100%**: Autorizado exclusivamente para transitorios acotados (`PWM_BURST_MAX_MS = 80 ms`) para vencer la inercia de arranque o desenclave de reductores TT. Al agotarse la ráfaga, `Motores.cpp` repliega el PWM a 242/255 y exige `PWM_BURST_COOLDOWN_MS` de enfriamiento.
- **Zona Muerta Dinámica**: Actúa como **piso dinámico** solo por debajo de `PWM_DEADBAND_UMBRAL_CRUCERO` (180/255); nunca se suma sobre crucero.
- **`VELOCIDAD_MINIMA_RECTO`**: **140/255**.
- **`DRIVE_STALL_MS`**: Watchdog dinámico con corte preventivo de 450 ms ante ausencia de ticks.

---

## 2. Odometría y Estimación de Pose (40 PPR)

- **Resolución Física**: 40 pulsos por revolución (40 PPR, relación 2:1 física por flancos de encoder de ranura).
- **Filtro Anti-rebote Hardware**: Configuración del periférico PCNT con filtro de rechazo de ruido de 1023 ciclos de reloj para suprimir inductancia de los motores TT.
- **Aislamiento en Giros**: La odometría lineal se desactiva estrictamente durante las maniobras de pivote rotacional para evitar que el resbalamiento de las ruedas contamine las coordenadas globales $(X, Y)$.
- **Simetría y Polaridad**: `PWM_FORWARD_POLARITY = -1` compensa la inversión mecánica de 180° de los reductores enfrentados en el chasis 4WD.

---

## 3. Lazo de Corrección Dinámica de Rumbo en Marcha

Durante el avance recto en `controlarAvance()`, el firmware ejecuta un lazo PD continuo que disminuye cualquier desviación angular en movimiento sin detener el robot:

$$\text{ctrlRumbo} = \text{constrain}(e_{\text{rumbo}} \cdot K_p - G_z \cdot K_d, -\text{PWM}_{\text{max\_corr}}, +\text{PWM}_{\text{max\_corr}})$$

- **Término Proporcional ($K_p = 4.0$)**: Corrige desviaciones angulares respecto al rumbo objetivo.
- **Término Derivativo ($K_d = 12.0$)**: Utiliza la velocidad angular real del giróscopo ($G_z$) para amortiguar oscilaciones e impedir sacudidas bruscas.
- **Compensación Diferencial**: Se modula dinámicamente la potencia de la rueda opuesta al error para reorientar el chasis mientras se desplaza a velocidad crucero.

---

## 4. Orquestación Ortogonal de Pasos y Retorno Ockham

1. **Pasos Exclusivamente Frontales**:
   - Cada tramo se compone de una alineación previa por pivote puro hacia `rumboObjetivoDeg` seguida de una traslación hacia adelante ($\le 200\text{ cm}$).
   - Se eliminó el avance en reversa física ciega para garantizar tracción simétrica y predictibilidad odimétrica.
2. **Retorno Ockham**:
   - Para regresar al origen, Python invierte la secuencia de vectores en orden cronológico inverso.
   - El retorno ejecuta cada segmento como avance frontal orientado y finaliza con `turn_to(0.0°)`, restableciendo la orientación cardinal inicial.

---

## 5. Protecciones Eléctricas y Mecánicas del DRV8833

- **Tope de PWM Crucero Sostenido**: **242/255** (~95%).
- **Tope de PWM Giros Sostenido**: **247/255** (~97%).
- **Ráfaga Máxima Transitoria**: **255/255** durante máximo **80 ms**.
- **Tiempo Muerto de Inversión Universal**: **250 ms** obligatorios en `Motores.cpp` (`PWM_DIRECTION_PAUSE_MS`) al invertir el sentido de polaridad. Aplica a joystick, giro autónomo y calibración.
- **Afinidad de Núcleos**: `Task_Web` en Core 0; súper-ciclo síncrono de 100 Hz en Core 1 consumiendo la misma muestra temporal.

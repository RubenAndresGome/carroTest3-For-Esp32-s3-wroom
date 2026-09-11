# Arquitectura de Control Cinemático y Control de Rumbo

Este documento describe la arquitectura de control, cinemática y desacoplamiento de parámetros implementada en el firmware modular del ESP32-S3 (`src/Cinematica.cpp`, `include/Config.h` y `src/Motores.cpp`).

---

## 1. Desacoplamiento de Parámetros de Giro y Avance

Para evitar sobrepasos por inercia en maniobras de rotación y asegurar fuerza suficiente en rectas, el firmware desacopla los parámetros de potencia y tiempos entre ambas maniobras:

### Giro Pivote (`GIRO`)

- **Control Híbrido y Rampa Adaptativa**: El giro inicia con una rampa suave de torque. Mientras no se confirme el movimiento físico mediante giroscopio y encoders, el torque escala en rampa adaptativa hasta alcanzar el **247/255** (~97%), venciendo la fricción estática de la caja reductora o superficies difíciles sin gatillar falsos stalls.
- **Aproximación Fina (Micro-pulsos)**: Cuando el error angular es < 5°, el sistema transiciona a un régimen de micro-pulsos intermitentes (`TURN_PULSE_ON_MS` / `TURN_PULSE_OFF_MS`) para evaluar la inercia, integrar el IMU con alta fidelidad y evitar sobrepasos.
- **Tolerancias y Finalización**: El margen durante navegación general se ubica alrededor de **3.5°** (`TOLERANCIA_GIRO_DEG`), pero la alineación fina y calibración final fuerzan una precisión < 1.0° antes de darse por completada. El signo de giro sigue continuamente el error real.
- **Tiempos Límite (Watchdogs por fase)**: El sistema implementa timeouts según la fase: calibración (20s), giros regulares (12s) y avance (7s).

### Avance Rectilíneo (`AVANCE`)

- **`VELOCIDAD_BASE_RECTO`**: **230/255** (~90% de potencia, tope global de seguridad según `AGENTS.md`).
- **`VELOCIDAD_APROXIMACION`**: **180/255**.
- **`VELOCIDAD_MINIMA_RECTO`**: **140/255**.
- **`DRIVE_STALL_MS`**: Watchdog dinámico según fase (generalmente 7s en recorrido).

---

## 2. Lazo de Corrección Dinámica de Rumbo en Marcha

Durante el avance recto en `controlarAvance()`, el firmware ejecuta un lazo PD continuo que disminuye cualquier desviación angular en movimiento sin detener el robot:

$$\text{ctrlRumbo} = \text{constrain}(e_{\text{rumbo}} \cdot K_p - G_z \cdot K_d, -\text{PWM}_{\text{max\_corr}}, +\text{PWM}_{\text{max\_corr}})$$

- **Término Proporcional ($K_p = 4.0$)**: Corrige desviaciones angulares.
- **Término Derivativo ($K_d = 12.0$)**: Utiliza la velocidad angular real del giroscopio ($G_z$) para amortiguar oscilaciones e impedir giros bruscos.
- **Compensación de Lado**: Se reduce dinámicamente la potencia de la rueda opuesta al error para reorientar el chasis mientras rueda.

---

## 3. Inicialización Segura y Resiliencia en Frío

Para garantizar que el robot pueda rotar en lazo cerrado de forma simétrica desde el momento del arranque (incluso si no se ha ejecutado la rutina de calibración manual en la sesión actual):

- **Mapeo de pivote v3.5**: `+yaw` (derecha) usa `L+ / R-`; `-yaw` usa `L- / R+`. La polaridad por motor es fija y no se auto-invierte durante calibración.
- **Mínimos de Torque por Defecto**: `pwmMinGiroPos = 148` (~58%), `pwmMinGiroNeg = 148` (~58%).
- **Efecto**: el MPU6050 valida el signo y los encoders sólo validan movimiento bilateral, con guardas de deriva y desbalance.

---

## 4. Orquestación Ortogonal de Pasos y Alineación Final

1. **Durante el Paso**:
   - Cada paso ortogonal se ejecuta sobre su rumbo propio (`rumboObjetivoDeg`).
   - Al completar la distancia solicitada, el robot verifica su rumbo contra `rumboObjetivoDeg`. Si está dentro de `TOLERANCIA_GIRO_DEG` (3.5°), el paso se completa de forma continua sin giros innecesarios a cero.
2. **Cierre de Misión**:
   - Al finalizar el último paso de una secuencia o ruta, el sistema ejecuta una alineación cardinal final hacia `0.0°` (`mission_completed_zero_aligned`), dejando el robot reorientado hacia el Norte cardinal.

---

## 5. Protecciones Eléctricas y Mecánicas Respetadas

- **Tope de PWM Global**: **230/255** (~95%).
- **Tiempo Muerto de Inversión**: **250 ms** obligatorios en `Motores.cpp` (`PWM_DIRECTION_PAUSE_MS`) al cambiar de sentido de giro.
- **Core Allocation**: `Task_Web` en Core 0; súper-ciclo de control síncrono a 100 Hz en Core 1.

# Catálogo de Fallos Físicos, Telemetría y Firmware

Esta referencia correlaciona los fenómenos observables en las grabaciones de video con las firmas de telemetría SQLite (`consultar_db.py`) y las secciones críticas del firmware del robot ESP32-S3.

---

## 1. Matriz de Síntomas Visuales vs Telemetría vs Causa Raíz

| Síntoma Observable en Video | Firma en Telemetría (`consultar_db.py`) | Componente Responsable | Causa Raíz Frecuente |
|---|---|---|---|
| **Patinaje sobre suelo**<br>Ruedas giran rápido pero el chasis no rota ni avanza. | Ticks de encoder suben exponencialmente; `gyro_z < 0.12 rad/s` o `yaw_deg` no incrementa. Evento `cal_rotation_not_confirmed`. | Físico / Mecánico (`src/Estado.cpp`) | Superficie demasiado lisa (azulejo brillante), neumáticos sin agarre o centro de masa mal balanceado. **No es falta de PWM.** |
| **Giro en sentido opuesto**<br>El chasis pivota en sentido antihorario cuando debía ser horario. | `gyro_z < 0` durante rampa inicial `CAL_A`. Evento `cal_yaw_sign_mismatch`. | `include/Configuracion.h` / `src/Motores.cpp` | Bornera de motor invertida en polaridad, o polaridad global de avance `PWM_FORWARD_POLARITY` alterada. |
| **Pivote excéntrico con traslación (*Drift*)**<br>El robot no gira sobre su centro; se desplaza $>1\text{ cm}$ lateral o longitudinalmente. | Odometría $(X,Y)$ se aleja de $(0,0)$ durante calibración. Evento `cal_origin_drift`. | `src/Cinematica.cpp` / Físico | Asimetría de fricción entre tren izquierdo y derecho, o una rueda atascada que actúa como eje de compás. |
| **Bloqueo unilateral**<br>Solo las ruedas de un lado giran; el otro lado permanece inmóvil. | Ticks estancados en lado afectado; PWM sube en rampa hasta 247/255. Evento `cal_stall_left` o `cal_stall_right`. | Conexión DRV8833 / `src/Motores.cpp` | Cable de motor desconectado en bornera, GPIO no conmuta, o reductor TT trabado mecánicamente. |
| **Apagón / Parpadeo y reinicio súbito**<br>Robot frena instantáneamente; los LEDs parpadean y el robot queda desarmado. | Nueva sesión en tabla `sessions`; `reset_reason=power_on` o `brownout`; estado `desarmado`, `cal=false`. | Eléctrico / `src/Motores.cpp` | Caída de tensión en línea de 3.3V/5V al invertir sentido de motores sin respetar el tiempo muerto universal de 250 ms, o ausencia de condensador electrolítico $\ge 100\,\mu\text{F}$ en VMOT. |
| **Oscilación / Temblores en parada**<br>El robot sobrepasa el ángulo objetivo y hace micro-pulsos adelante/atrás continuos. | Yaw oscila alrededor del setpoint con cambios de signo repetidos. | `include/Configuracion.h` (`TURN_PULSE_ON_MS`) | Inercia del chasis superior a la esperada; pulsos de afinamiento demasiado largos para el peso del robot. |
| **Ruedas de un mismo lado en sentidos opuestos**<br>Por ejemplo: FL avanza pero BL retrocede en el tren izquierdo. | Ticks de un encoder avanzan y el otro retrocede en el mismo tren motriz. El carro vibra sin avanzar. | Cableado de borneras | Violación de la regla de simetría 4WD de `AGENTS.md` (motores FL/FR enfrentados a BL/BR). Cable naranja/negro invertido en una de las borneras. |

---

## 2. Puntos de Inspección en Código Fuente de Firmware

Cuando la evidencia del video apunte a un fallo de control lógico:

### A. Tiempos muertos y control de motores: `src/Motores.cpp`
- **Función:** `Motores::aplicarVelocidades(int pwmIzq, int pwmDer)`
- **Invariante:** Debe respetarse una pausa mínima de 250 ms al pasar de avance a retroceso para proteger los puentes H DRV8833 y evitar caídas de tensión (brownout).
- **Límites de PWM:** Avance $\le 242/255$; Giros y calibración $\le 247/255$.

### B. Dogma de Calibración: `src/Estado.cpp`
- **Secuencia:**
  1. `CAL_A`: Rampa positiva de 140 a 247 hasta confirmar movimiento con `fabsf(gyro_z) >= 0.12 rad/s`.
  2. `CAL_VALIDAR_25`: Giro a $+25^\circ \pm 2.5^\circ$.
  3. `CAL_PAUSA`: 2.5 s de reposo.
  4. `CAL_B`: Giro en sentido opuesto.
  5. `CAL_PAUSA_RETORNO`: 2.5 s de reposo.
  6. `CAL_RETORNO`: Regreso exacto al yaw inicial $0^\circ$.

### C. Mapeo físico de pines y polaridades: `include/Configuracion.h`
- Pines de motores:
  - FL: FWD=GPIO7, REV=GPIO6
  - BL: FWD=GPIO4, REV=GPIO5
  - FR: FWD=GPIO18, REV=GPIO17
  - BR: FWD=GPIO16, REV=GPIO15
- Pines de encoders (PCNT):
  - FL=GPIO11, FR=GPIO10, BL=GPIO12, BR=GPIO13
- Pines I2C IMU MPU6050:
  - SDA=GPIO8, SCL=GPIO9
- Polaridad global: `PWM_FORWARD_POLARITY = -1`

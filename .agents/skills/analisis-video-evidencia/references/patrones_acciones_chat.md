# Análisis de Patrones y Acciones Repetidas en el Chat

Esta referencia metodológica describe cómo analizar el historial de la conversación, detectar bucles de depuración estériles y utilizar la evidencia del video para romper los ciclos repetitivos de prueba y error.

---

## 1. Naturaleza de los Bucles de Chat en Robótica

Durante el desarrollo de robots móviles, los desarrolladores y los asistentes de IA caen frecuentemente en bucles cognitivos repetitivos debido a la falta de información visual directa:

```
┌────────────────────────────────────────────────────────┐
│             BUCLE CLÁSICO DE PRUEBA Y ERROR            │
├────────────────────────────────────────────────────────┤
│ 1. Usuario prueba robot ───► Falla físicamente         │
│ 2. Usuario escribe en chat: "Sigue sin girar bien"     │
│ 3. Agente conjetura a ciegas: "Subamos el PWM a 240"   │
│ 4. Usuario prueba de nuevo ──► Vuelve a fallar         │
│ 5. Agente: "Aumentemos el timeout del watchdog"        │
│ 6. Repetición indefinida sin solucionar la causa raíz  │
└────────────────────────────────────────────────────────┘
```

El video físico actúa como el **disruptor del bucle**, transformando suposiciones subjetivas en hechos observables irrefutables.

---

## 2. Los 4 Patrones de Acciones Repetidas Más Frecuentes

### Patrón 1: El Bucle de "Falta de Fuerza / Subir PWM"
- **Acción repetida en chat:** Cada vez que el robot no completa un giro o avance, el usuario o agente proponen aumentar el PWM o reducir rampas.
- **Lo que revela el video:**
  - *Caso A:* Las ruedas están patinando velozmente sobre el suelo. El motor tiene torque de sobra; el fallo es falta de adherencia (fricción estática superada). Subir el PWM empeora el patinaje.
  - *Caso B:* Las ruedas no giran porque los cables de alimentación del motor están flojos o la bornera DRV8833 tiene un falso contacto. Subir el PWM en software no tiene ningún efecto físico.

### Patrón 2: El Bucle de "Inversión Ciega de Signos"
- **Acción repetida en chat:** Cambiar alternativamente `+` y `-` en factores cinemáticos, ganancias PID o polaridades de avance/giro en turnos sucesivos (*"probemos con yaw negativo"*, *"ahora probemos invirtiendo L y R"*).
- **Lo que revela el video:**
  - Muestra el vector real de rotación del chasis respecto a la geometría 4WD.
  - Permite verificar visualmente la simetría de montaje descrita en `AGENTS.md`: motores delanteros enfrentados a traseros. Si las ruedas de un mismo lado giran en direcciones opuestas (una adelante y otra atrás), hay un cruce de cableado en la bornera física, no un error de signo en el algoritmo cinemático.

### Patrón 3: El Bucle de "Ampliar Watchdogs / Ignorar Guards"
- **Acción repetida en chat:** Cuando el firmware aborta con `cal_stall_left`, `cal_rotation_not_confirmed` o `watchdog timeout`, se propone aumentar los tiempos de espera a 5 o 10 segundos.
- **Lo que revela el video:**
  - El robot se encuentra físicamente trabado contra un obstáculo o la IMU perdió comunicación (I2C colgado). Ampliar el timeout solo somete a los motores DRV8833 a sobrecorriente destructiva sin resolver el atasco.

### Patrón 4: El Bucle de "Desincronización de Comandos"
- **Acción repetida en chat:** El usuario envía múltiples comandos sucesivos en la HMI porque el robot "no responde", generando acumulación de órdenes o carreras de estado.
- **Lo que revela el video:**
  - El robot aún estaba completando la rampa de deceleración o el tiempo muerto de 250 ms del comando anterior cuando recibió la nueva orden.

---

## 3. Protocolo de Auditoría de Transcripción y Chat

Antes de emitir un veredicto o modificar código:

1. **Revisar los últimos 3 a 5 turnos del chat:**
   - Extraer todas las hipótesis previas formuladas.
   - Listar los cambios de código previamente sugeridos y su resultado.
2. **Construir la Tabla de Falsificación de Acciones:**

| Turno Chat | Acción / Hipótesis Previa | Resultado Reportado | Veredicto del Video | Razón de la Falsación |
|---|---|---|---|---|
| Turno N-2 | Aumentar PWM a 220 por "falta de fuerza" | Siguió sin rotar | **FALSA** | Ruedas giraron a alta velocidad patinando en losa; no faltaba torque. |
| Turno N-1 | Invertir signo de yaw en `Cinematica.cpp` | Giro descontrolado | **FALSA** | El signo de la IMU era correcto; el error era que el motor BL giraba en sentido inverso por bornera invertida. |

3. **Cierre de Ciclo:**
   - Informar al usuario explícitamente:
     > *"Al revisar los turnos anteriores del chat, notamos que se intentó subir el PWM y cambiar los signos de yaw. Sin embargo, la inspección del video en el segundo 00:04 demuestra que el motor BL gira en sentido opuesto a FL. Por lo tanto, no se debe modificar el software de control, sino corregir la polaridad del cableado del motor BL en la bornera."*

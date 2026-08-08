# Rutina de recuperación de endpoint y reversa

## Propósito

Una ruta no debe convertir un residuo pequeño de odometría en una secuencia de
giros repetidos. Esta rutina conserva el avance de la misión, hace una
corrección acotada cuando todavía es razonable y deja evidencia suficiente para
distinguir una recuperación normal de una calibración pendiente.

No reemplaza la calibración física de diámetro efectivo, encoders, IMU ni el
control de deslizamiento. Un `step_ok_endpoint_soft` sólo significa que la ruta
continúa de forma segura; no certifica que el robot haya llegado con precisión
metrológica.

## Decisión al terminar un paso

Al alcanzar la distancia de mando, el firmware frena, deja asentar la muestra y
compara la pose estimada contra el objetivo absoluto del tramo.

1. Si el endpoint está dentro de la tolerancia, termina con `step_ok`.
2. Si el residuo euclidiano es menor de 13 cm, publica
   `step_ok_endpoint_soft` y `recovery.decision=soft_complete`. No inventa un
   pivote ni bloquea los pasos siguientes; la HMI debe marcar que hay
   calibración pendiente.
3. Si el residuo es mayor, calcula el vector real hacia el objetivo y puede
   intentar como máximo dos recuperaciones. Cada intento queda publicado con su
   dirección, distancia y motivo.
4. Si no converge en ese límite, termina seguro con `fault/endpoint_not_reached`.
   No hay bucle infinito de recuperación.

## Recuperación con avance o reversa

Con `drive_mode=auto`, la dirección efectiva se elige usando el ángulo entre el
rumbo actual y el vector hacia el objetivo.

- Si el objetivo queda claramente detrás (umbral de 135°), se selecciona
  `reverse_no_pivot`: ambos lados retroceden, se conserva el yaw del chasis y
  se evita un giro artificial de 180°.
- En los demás casos se usa avance hacia el objetivo y sólo se hace pivot si el
  error angular real lo necesita.
- Un `turn_to` explícito sigue siendo una orden de orientación distinta: puede
  girar incluso si la recuperación previa eligió reversa.

La inversión de polaridad conserva el tiempo muerto universal de 250 ms. En
reversa el PWM no salta al crucero: se inicia en el torque de precisión y sube
progresivamente durante 900 ms. Esto reduce el sobreimpulso inicial y permite
que el lazo de rumbo actúe desde el inicio.

## Corrección de rumbo durante reversa

El PID de yaw sigue trabajando con el error de rumbo real. Como el vehículo se
mueve hacia atrás, frenar el lado equivocado aumenta el error: la rutina invierte
el lado físico de frenado para que la misma corrección angular siga reduciendo el
error. La telemetría publica `drive_control.heading_brake_side` y la condición
de rampa de reversa para verificar esa decisión sin osciloscopio.

## Telemetría y análisis posterior

Durante cada tramo se deben conservar `target`, `motion`, `drive_control` y
`recovery`. Los campos más útiles son:

- `motion.effective_mode`, `reverse_ramp_active` y
  `reverse_ramp_elapsed_ms`;
- `drive_control.heading_brake_side`, PWM izquierdo/derecho y error de yaw;
- `recovery.decision`, intentos, distancia residual y si se evitó el pivot;
- `command_run_id`, que separa ejecuciones nuevas aunque el ESP32 reinicie la
  secuencia numérica `seq`.

Para una base nueva, usar `python consultar_db.py --trace-command <prefijo-id>`.
El trazador agrupa por `run_id`; las bases históricas sin ese campo se muestran
como aproximación y no se deben usar para atribuir de forma concluyente eventos
con el mismo `seq`.

## Criterio de aceptación

La recuperación es un mecanismo de continuidad, no una autorización para operar
sin medición. Antes de aceptar una calibración se requieren recorridos repetidos
en los cuatro rumbos cardinales, evidencia SQLite/vídeo sincronizada y los
límites definidos en [validacion_sistema_final.md](validacion_sistema_final.md).
Si la rutina se activa repetidamente, se debe medir el error físico y ajustar la
calibración; no aumentar el máximo de intentos ni relajar el fallo terminal.

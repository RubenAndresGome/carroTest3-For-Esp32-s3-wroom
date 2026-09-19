# Contrato `robot-s3-steps-v3` entre Python y ESP32-S3

Python es el único propietario de la misión y del WebSocket. El ESP32 conserva memoria corta: acepta una maniobra, la ejecuta con sus protecciones locales y reporta el resultado antes de recibir la siguiente.

## Negociación

```json
{"cmd":"hello","session":"0123456789abcdef","seq":0}
```

```json
{"evt":"hello_ack","protocol":"robot-s3-steps-v3","session":"0123456789abcdef","state":"listo","last_seq":4,"calibrated":true}
```

Un cambio de proceso crea una sesión nueva. Antes de habilitar controles, Python envía `stop` si SQLite contenía trabajo no terminal; nunca repite automáticamente ese trabajo.

## Maniobras atómicas

Avance relativo compatible con control manual:

```json
{"cmd":"step","heading":90.0,"cm":50.0,"seq":5}
```

Avance de misión con waypoint absoluto (campos aditivos):

```json
{"cmd":"step","heading":90.0,"cm":50.0,"drive_mode":"auto","target_x_mm":1500.0,"target_y_mm":0.0,"seq":5}
```

`target_x_mm` y `target_y_mm` se aceptan sólo como par finito dentro de
±100 000 mm. Python los toma del waypoint almacenado, nunca de la pose ya
desviada. El ESP32 los convierte a centímetros, mantiene los `step` antiguos
sin objetivo como relativos y exige para una misión error lateral y euclidiano
≤5 cm, yaw ≤3° estable durante 300 ms. Si la distancia longitudinal termina
sin llegar al punto, un error de menos de 13 cm termina como
`completed/step_ok_endpoint_soft`: no ordena un giro ni una corrección que la
inercia no pueda repetir, deja la calibración pendiente y permite continuar al
siguiente waypoint absoluto. Para un error mayor, intenta hasta dos
recuperaciones acotadas y después reporta `fault/endpoint_not_reached`.

`drive_mode` es opcional y acepta `forward`, `reverse` o `auto`. Las rutas
canónicas envían `auto`: si el rumbo de trayecto está a 135° o más detrás del
yaw actual, el ESP32 aplica reversa a ambos lados y conserva el yaw, sin
convertir el paso en un giro de 180°. Si un cliente omite el campo se mantiene
la semántica histórica `forward`.

Giro absoluto sin traslación:

```json
{"cmd":"turn_to","heading":0.0,"seq":6}
```

`turn_to` usa el mismo pivot continuo, rampa, watchdog, detección de sobrepaso e interlock eléctrico que los giros de `step`. Su terminal correcto es:

```json
{"evt":"completed","seq":6,"detail":"turn_ok"}
```

## Calibración supervisada (`calibrate`)

El comando `calibrate` ejecuta el Dogma Canónico de Calibración en 5 fases usando el MPU6050 como autoridad angular única:

```json
{"cmd":"calibrate","seq":7}
```

Eventos de ciclo de calibración:
- `progress`: Reporta fase actual (`CAL_CUENTA_REGRESIVA` 5.0 s para reposo y estabilización de sesgo MPU, `CAL_A` búsqueda de torque positivo, `CAL_VALIDAR_25` giro a +25°, `CAL_PAUSA` reposo 2.5 s, `CAL_B` búsqueda opuesta, `CAL_PAUSA_RETORNO` reposo 2.5 s, `CAL_RETORNO` regreso exacto a yaw inicial).
- `completed`: `{"evt":"completed","seq":7,"detail":"cal_ok"}` al consolidar el retorno con pose (X=0, Y=0) y yaw reseteados.
- `fault`:
  - `cal_connection_lost`: Desconexión de WebSocket durante calibración; Core 0 notifica inmediatamente y Core 1 frena a PWM 0.
  - `cal_stall_left` / `cal_stall_right`: Atasco detectado si no hay deltas de encoder o giro angular en la rampa.
  - `cal_no_gyro_rotation`, `cal_pivot_asymmetric`, `cal_return_timeout`.

Se conservan además `stop`, `estop`, `clear_fault`, `set_comp` y `reset_pose`. Todos llevan un `seq` positivo salvo `hello`. Las tramas JSON WebSocket admiten hasta 7168 bytes tanto en firmware como en el backend Python.

## Idempotencia y cierre

- `seq <= last_seq` devuelve `already_done` y no repite movimiento.
- Una reconexión dentro del mismo proceso conserva `session` y puede reconciliar el paso activo.
- Un reinicio de proceso abandona la misión persistida, falla comandos no terminales y solicita una parada de reconciliación.
- La aplicación solo autoriza su cierre normal después de recibir `completed/stop_ok` cuando había movimiento.

## Telemetría de convergencia

El evento `telemetry` publica `target` con objetivo, errores longitudinal,
lateral y euclidiano, intento de endpoint y razón de fin; `drive_control`
incluye rumbo dinámico, error angular, P/I/D, integral, corrección por encoder,
corrección lateral y compensación derecha aplicada. Estos objetos quedan dentro
de `payload_json` de SQLite para mantener compatibilidad con bases previas.

La telemetría también incluye `encoder_health` y `encoder_fusion`. Si un canal
FL/FR/BL/BR permanece fuera de la mediana robusta durante la ventana de
auditoría, se publica como `excluded`; la pose, la distancia y la corrección de
rumbo usan los canales restantes. La misión sólo continúa si queda al menos un
encoder confiable por lado. `encoder_fusion.distance_scale_factor` publica la
fórmula efectiva `1 + encoder_error_pct` aplicada al valor nominal por pulso.

El objeto `motion` publica modo solicitado y efectivo, rumbo de trayecto,
rumbo del chasis, rumbo final, distancia restante, freno previsto, arrastre de
inercia y tiempo de asentamiento. El PID y la odometría usan la misma escala
por pulso. Tras soltar PWM, la fase `asentando` integra los pulsos residuales y
sólo verifica el objetivo cuando ya no hay nuevos ticks ni giro.

En reversa, `motion.reverse_ramp_elapsed_ms` y `reverse_ramp_active` muestran
la rampa de 900 ms antes de crucero. `drive_control.heading_brake_side` indica
el lado que el PID frenó para corregir yaw. Cada ejecución publica un
`command_run_id` distinto aun si el cliente reinicia `seq`; los eventos llevan
el mismo `run_id` para trazar una maniobra sin mezclarla con otra en SQLite.

`recovery` no omite decisiones: publica `decision`, `distance_cm`, `direction`,
`pivot_avoided` y `min_distance_cm`. Si el punto está detrás, una recuperación
amplia usa `reverse_no_pivot` para conservar la orientación del chasis; si el
error es corto publica `soft_complete` y el terminal no bloqueante indicado.

## Garantías de seguridad eléctrica y límites de potencia

El protocolo y firmware aseguran la integridad del puente H DRV8833 en todas las transiciones:
- **Límite de avance continuo:** 242/255 (~95%) para evitar sobrecalentamiento y saturación en los devanados de los motores TT.
- **Límite de giro continuo:** 247/255 (~97%) para garantizar torque suficiente de pivote sobre superficies de alta fricción.
- **Régimen de ráfagas acotadas:** PWM 255/255 solo permitido durante transitorios de despegue (kickstart) por un máximo de `PWM_BURST_MAX_MS = 80 ms`, exigiendo `PWM_BURST_COOLDOWN_MS = 250 ms` de enfriamiento.
- **Tiempo muerto universal:** 250 ms en `Motores.cpp:aplicarVelocidades()` al invertir sentido de marcha o rotación, previniendo corrientes de shoot-through en los MOSFETs.
- **Zona muerta como piso dinámico:** 140/255 activa únicamente por debajo de `PWM_DEADBAND_UMBRAL_CRUCERO` (180/255); nunca se adiciona sobre crucero.

## Odometría de 40 PPR y aislamiento en pivote

- **Resolución efectiva:** 40 pulsos por revolución (disco de 20 ranuras con lectura de flancos de subida y bajada por canal TXS0108E).
- **Filtro hardware PCNT:** 1023 ciclos de reloj APB para supresión total de transitorios espurios inducidos por escobillas de los motores.
- **Aislamiento en giro:** Durante pivote puro (`Giro.cpp`), la integración de distancia longitudinal se congela para evitar lecturas de arrastre parasitario causadas por el patinaje diagonal del chasis 4WD.


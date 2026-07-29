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

Avance con rumbo absoluto:

```json
{"cmd":"step","heading":90.0,"cm":50.0,"seq":5}
```

Giro absoluto sin traslación:

```json
{"cmd":"turn_to","heading":0.0,"seq":6}
```

`turn_to` usa el mismo pivot continuo, rampa, watchdog, detección de sobrepaso e interlock eléctrico que los giros de `step`. Su terminal correcto es:

```json
{"evt":"completed","seq":6,"detail":"turn_ok"}
```

Se conservan `calibrate`, `stop`, `estop`, `clear_fault`, `set_comp` y `reset_pose`. Todos llevan un `seq` positivo salvo `hello`.

## Idempotencia y cierre

- `seq <= last_seq` devuelve `already_done` y no repite movimiento.
- Una reconexión dentro del mismo proceso conserva `session` y puede reconciliar el paso activo.
- Un reinicio de proceso abandona la misión persistida, falla comandos no terminales y solicita una parada de reconciliación.
- La aplicación solo autoriza su cierre normal después de recibir `completed/stop_ok` cuando había movimiento.

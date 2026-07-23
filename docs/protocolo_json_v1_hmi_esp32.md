# Contrato JSON V1 entre HMI Python y ESP32-S3

## Responsabilidades

- `desktop_app` es el único cliente WebSocket. Conserva misión, segmentación,
  interfaz, validación e historial SQLite.
- El ESP32-S3 conserva PCNT, MPU6050, pose, PWM, lazos de control, límites,
  E-STOP y watchdogs.
- El navegador nunca abre un WebSocket al carrito: usa HTTP y SSE contra
  Flask en `127.0.0.1`.
- Cada segmento autónomo mide como máximo 2000 mm. Python divide tramos más
  largos y envía el siguiente únicamente después del `completed` correlacionado.

## Conexión y saludo obligatorio

El enlace es `ws://192.168.4.1/ws`. Ambos extremos envían un saludo y ningún
comando se procesa antes de validarlo:

```json
{"v":1,"type":"hello","protocol":"robot-s3-json-v1","role":"controller"}
{"v":1,"type":"hello_ack","protocol":"robot-s3-json-v1"}
```

No existe salida heredada. Un protocolo diferente cierra la conexión.
Solamente una computadora puede controlar el robot simultáneamente.

## Comandos

Todos los comandos usan un UUID hexadecimal de 32 caracteres:

```json
{"v":1,"type":"command","id":"0123456789abcdef0123456789abcdef","name":"move","payload":{"x_mm":500,"y_mm":0}}
```

Comandos admitidos:

- `self_test {}`: autoevaluación lógica opcional y sin movimiento.
- `calibrate {}`: recentra pose y rumbo; no ejecuta movimientos de calibración.
- `move {x_mm,y_mm}`: waypoint cartesiano absoluto.
- `drive {distance_mm}`: distancia relativa firmada, entre -2000 y 2000 mm.
- `turn {angle_deg}`: giro relativo firmado, entre -360 y 360 grados.
- `manual {l,r}`: PWM lógico limitado globalmente de -230 a 230; lease
  independiente de 400 ms.
- `test_pwm {l,r,dur_ms}`: prueba limitada a ±230 y 10–1500 ms.
- `stop`, `estop`, `clear_fault` y `reset_pose`.

El AP y la telemetría arrancan mientras la MPU realiza su ajuste estático. Al
recibir una muestra válida, el firmware pasa automáticamente de `UNCALIBRATED`
a `IDLE`; no ejecuta una calibración motriz ni exige `self_test`. Antes de
completar ese ajuste se rechaza cualquier movimiento; `stop`, `estop` y rearme
continúan disponibles.

## Resultados, idempotencia y reconexión

```json
{"v":1,"type":"queued","id":"0123456789abcdef0123456789abcdef","cmd":"move"}
{"v":1,"type":"progress","id":"0123456789abcdef0123456789abcdef","progress":0.5}
{"v":1,"type":"completed","id":"0123456789abcdef0123456789abcdef","detail":"move_completed"}
```

También existen `accepted`, `rejected`, `fault` y `error`. Un `rejected`
incluye `reason`. Reenviar el UUID activo devuelve `queued`; reenviar el último
UUID terminal reproduce su resultado sin repetir el movimiento.

Una orden autónoma continúa de forma segura si se desconecta la computadora.
El saludo y la telemetría incluyen `active_command_id` y `last_terminal` para
que Python se reconcilie al regresar. Una misión se detiene ante `rejected`,
`fault` o `error`; nunca salta automáticamente un waypoint.

## Heartbeat y control manual

Python envía cada 200 ms:

```json
{"v":1,"type":"heartbeat","ts_ms":123456789}
```

El `heartbeat_ack` diagnostica el enlace, pero no renueva el joystick. Mientras
el operador mantiene el control, el navegador repite `manual` cada 100 ms,
incluso si el PWM no cambió, y envía cero al soltar. La ausencia de renovación
durante 400 ms provoca una parada segura en el ESP32.

## Telemetría

El ESP32 publica a 10 Hz un objeto `telemetry` con pose en mm/grados, pulsos de
los cuatro encoders, velocidades, PWM, MPU, salud de encoders, modo degradado,
estado de autoevaluación y calibración, objetivo, progreso, comando activo y
último resultado.
Python normaliza la muestra para SSE y conserva hasta 5 muestras por segundo en
SQLite.

`Task_Web` permanece en Core 0 y nunca ejecuta control de tiempo real. Core 1
ejecuta un único súper-ciclo síncrono de 100 Hz en el `loop()` nativo: lee MPU
y PCNT, actualiza pose y seguridad, calcula cinemática y aplica PWM usando la
misma muestra. No existe cola entre sensores y motores.

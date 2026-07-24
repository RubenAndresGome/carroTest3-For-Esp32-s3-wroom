# Contrato `robot-s3-steps-v2` entre HMI Python y ESP32-S3

## Responsabilidades

- Python es el único propietario del WebSocket y conserva misión, sesión,
  segmentación, SQLite e interfaz. Windows y Android no deben controlar el
  mismo robot simultáneamente.
- El ESP32 conserva PCNT, MPU6050, pose, límites, E-STOP, watchdogs, cinemática
  y PWM. Sólo ejecuta un paso atómico cada vez.
- El navegador usa HTTP/SSE contra Flask en `127.0.0.1`; nunca abre el
  WebSocket del robot.

## Saludo, sesión e idempotencia

El enlace es `ws://192.168.4.1/ws`. Python inicia siempre con:

```json
{"cmd":"hello","session":"0123456789abcdef","seq":0}
```

El firmware confirma la sesión y su checkpoint:

```json
{"evt":"hello_ack","protocol":"robot-s3-steps-v2","session":"0123456789abcdef","state":"listo","last_seq":4,"calibrated":true}
```

Python asigna enteros positivos y crecientes. Si `seq <= last_seq`, el ESP32
responde `already_done` y no repite movimiento. Ante reinicio del ESP32 durante
un paso no confirmado, Python bloquea la misión para impedir una repetición
ciega.

## Comandos

```json
{"cmd":"calibrate","seq":1}
{"cmd":"step","heading":90.0,"cm":150.0,"seq":2}
{"cmd":"stop","seq":3}
{"cmd":"estop","seq":4}
{"cmd":"clear_fault","seq":5}
{"cmd":"set_comp","factor":0.95,"seq":6}
{"cmd":"reset_pose","seq":7}
```

`heading` es absoluto en `[0,360)`: 0° apunta a +Y y 90° a +X. `cm` está entre
0.5 y 200. Cada `step` gira, avanza y termina alineado a 0°. Python acepta sólo
tramos ortogonales con tolerancia transversal de 1 mm y divide recorridos
largos en segmentos de hasta 2 m.

No existen en este protocolo `manual`, `test_pwm`, `drive`, `turn`,
`mission_upload`, heartbeat ni UUID de firmware.

## Eventos y telemetría

```json
{"evt":"accepted","seq":2,"detail":"accepted"}
{"evt":"progress","seq":2,"pct":0.45,"detail":"avance"}
{"evt":"completed","seq":2,"detail":"step_ok"}
{"evt":"already_done","seq":2,"detail":"already_done"}
{"evt":"fault","seq":2,"detail":"stall_FR"}
```

La telemetría de 10 Hz incluye estado, yaw, pose en cm, cuatro encoders, PWM,
sesión, `last_seq`, paso/fase/progreso, calibración, compensación y diagnósticos
RTOS. Python la normaliza a mm para el HMI y SQLite.

`Task_Web` permanece en Core 0. El `loop()` nativo de Core 1 es el único
súper-ciclo de control síncrono a 100 Hz; no hay tarea ni cola intermedia entre
sensores, pose, seguridad, cinemática y PWM.

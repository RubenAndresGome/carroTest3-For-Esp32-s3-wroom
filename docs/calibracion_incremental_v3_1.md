# Calibración incremental `robot-s3-v3.1`

## Objetivo

La versión `v3.1` corrige la falta de observabilidad de la calibración sin
debilitar las protecciones del DRV8833. La búsqueda de torque vuelve a usar
pasos de `5/255` cada `250 ms`, desde `140/255` hasta `247/255`. El corte por
lado se mantiene acumulativo y se activa después de `800 ms` al PWM máximo.

Los niveles de la rampa no son reintentos completos. Repetir ciclos de stall a
97 % aumentaría el riesgo eléctrico. La telemetría publica el nivel actual y el
total para que la HMI muestre el progreso real.

## Máquina de estados

| Estado | Operaciones relevantes |
|---|---|
| `DESARMADO` | calibrar, reset de pose, compensación, STOP y E-STOP |
| `CALIBRANDO` | STOP y E-STOP |
| `LISTO` | calibrar, paso, giro, reset de pose, compensación, STOP y E-STOP |
| `EJECUTANDO` | STOP y E-STOP |
| `FALLO` | rearme, STOP y E-STOP |
| `ESTOP` | rearme, STOP y E-STOP |

`clear_fault` sólo reporta `fault_cleared` si la transición salió realmente de
`FALLO/ESTOP`. Un rearme sin fallo activo o con salidas de motor no disponibles
se rechaza. Después del rearme se vacía la cola para exigir una orden nueva.

E-STOP terminaliza primero el comando interrumpido con `stopped_by_estop` y
después confirma el propio comando con `estop_latched`.

## Encoders durante calibración

La aceptación conserva el promedio obligatorio de ambos encoders por lado:

```text
izquierda = (FL + BL) / 2
derecha   = (FR + BR) / 2
```

Un encoder individual sin respuesta se identifica cuando su pareja demuestra
movimiento suficiente. Al confirmar la calibración se conserva como
`EXCLUDED`, por lo que la navegación posterior usa la fuente restante del
lado. Nunca se acepta un lado completo sin pulsos.

## Contrato de diagnóstico

`calibration_diagnostics` añade, sin cambiar `robot-s3-steps-v3`:

- fase, nivel y total de la rampa;
- PWM solicitado en 8 y 10 bits;
- delta, respuesta y aislamiento de `FL`, `FR`, `BL`, `BR`;
- promedio, validez y stall acumulado de cada lado;
- umbrales de ticks y tiempo de stall.

La telemetría también publica `fault`, `allowed_commands`, estado del MPU y
`last_terminal`. Todos son campos aditivos y compatibles con clientes previos.

## Aceptación

Las pruebas de host y la compilación demuestran lógica y compatibilidad, pero
no validan corriente ni cableado. Antes de operar en suelo:

1. Elevar las ruedas.
2. Limitar la fuente a `0.5 A` o usar fusible de `1 A`.
3. Verificar en la HMI los cuatro deltas durante la rampa.
4. Medir corriente en VMOT y confirmar menos de `1 A` sostenido.
5. Sólo entonces realizar una prueba de calibración completa y una ruta corta.

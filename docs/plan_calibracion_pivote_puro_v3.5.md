# Plan aplicado: calibración por pivote puro v3.5

## Dogma físico

La hoja [`evidencia/hardware/cableado_dogmatico_milestone_2026-09-08.jpeg`](../evidencia/hardware/cableado_dogmatico_milestone_2026-09-08.jpeg) es la fuente inmutable de GPIO, colores y conectores. La dirección lógica se resuelve individualmente en firmware:

| Rueda | Avance lógico | Reversa lógica |
|---|---:|---:|
| FL | GPIO6 | GPIO7 |
| BL | GPIO4 | GPIO5 |
| FR | GPIO17 | GPIO18 |
| BR | GPIO15 | GPIO16 |

`+Y` es frente, `+X` es derecha y yaw positivo es horario. Por tanto, `+yaw`
usa `L+ / R-` y `-yaw` usa `L- / R+`. No se vuelve a aplicar una inversión
global ni se intercambian GPIO para corregir la gráfica.

Los encoders se colocan a 45° respecto al chasis y sólo prueban magnitud,
balance y que existe al menos una fuente PCNT sana por lado. El MPU6050 decide
el signo y el ángulo.

## Guard de calibración

1. Reposo y baseline de MPU, pulsos y pose durante 5 s.
2. Búsqueda fija de torque en `+yaw`; al primer tick bilateral se congela la
   rampa. El MPU debe sostener `gyro_z >= 0.12 rad/s` durante 100 ms.
3. Se detiene con signo contrario, falta de yaw, desbalance persistente o
   deriva de origen superior a 1 cm. Los motivos son explícitos:
   `cal_yaw_sign_mismatch`, `cal_rotation_not_confirmed`,
   `cal_pivot_unbalanced` y `cal_origin_drift`.
4. Se valida +25°, se reposa 2.5 s, se valida el pivote contrario y se retorna
   al yaw inicial. Al terminar se conservan X/Y previos y sólo se reanclan
   pulsos y referencias angulares.

La capacidad `calibration_pivot_guard_v1` se anuncia por WebSocket. Firmware y
HMI antiguos no pueden iniciar `calibrate`; STOP y E-STOP permanecen siempre
disponibles.

## Aceptación física

Con VMOT apagado durante la carga, probar primero con ruedas elevadas y corriente
limitada. Confirmar que +yaw hace girar las ruedas izquierdas hacia delante y
las derechas hacia atrás, que -yaw invierte el patrón, que la deriva total es
menor de 1 cm y que la telemetría ADB refleja el mismo signo. Detener la prueba
ante una sola rueda incorrecta o cualquier traslación visible.

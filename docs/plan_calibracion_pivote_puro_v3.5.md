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

## Configuración de Encoders Ópticos LM393 (FC-03) y PCNT

Conforme al principio físico de los sensores ópticos de ranura LM393 con disco
perforado de 20 ranuras (Damián, 2026), cada ranura que cruza el haz infrarrojo
produce exactamente un ciclo digital completo: un flanco de subida al abrirse la
ranura y un flanco de bajada al obstruirse.

Para garantizar exactitud métrica sin duplicar conteos espurios por rebotes en los
flancos de bajada, el hardware PCNT del ESP32-S3 se configura con:
- `pos_mode = PCNT_COUNT_INC` (incremento en flanco de subida).
- `neg_mode = PCNT_COUNT_DIS` (ignorar flanco de bajada para completar 1 ranura = 1 tick).
- `filter_value = 1023` (filtro de rechazo de transitorios y ruido de alta frecuencia).
- `ENCODER_PPR = 20` pulsos por revolución.

## Guard de calibración y autoridad MPU

1. Reposo y baseline de MPU, pulsos y pose durante 5 s (`CAL_CUENTA`).
2. Rampa de búsqueda de torque continua en **10 bits nativos**: de 560 (~55 % duty)
   a 990 (~97 % duty) con pasos de 10 unidades (~1 % duty) cada 150 ms.
3. Al detectarse movimiento bilateral inicial, la rampa se congela y se establece
   una línea base de ticks relativos (`ticksBasePruebaRotacionCal`).
4. **MPU como autoridad directora única**: La rotación se confirma si:
   - Velocidad angular filtrada `gyro_z >= 0.08 rad/s` en signo correcto sostenida durante 100 ms, O
   - Desplazamiento angular acumulado $|\Delta\text{yaw}| \ge 0.5^\circ$ en sentido correcto.
5. Se detiene con signo contrario (`cal_yaw_sign_mismatch`), desbalance
   persistente >350 ms (`cal_pivot_unbalanced`), exceder 10 ticks relativos sin
   confirmación de MPU en 350 ms (`cal_rotation_not_confirmed`) o deriva de
   origen >5.2 cm (`cal_origin_drift`).
6. Se valida +25°, se reposa 2.5 s, se valida el pivote contrario (`CAL_B`) y se
   retorna al yaw de origen original (`yawOrigenCalDeg`). Al terminar se
   conservan X/Y previos y sólo se reanclan pulsos y referencias angulares.

La capacidad `calibration_pivot_guard_v1` se anuncia por WebSocket. Firmware y
HMI antiguos no pueden iniciar `calibrate`; STOP y E-STOP permanecen siempre
disponibles.

## Referencias (APA 7.ª edición)

- Damián, J. (2026, 22 de febrero). *Encoder y Arduino. Tutorial sobre el módulo sensor de velocidad IR con el comparador LM393 (Encoder FC-03)*. Electrogeek. https://www.electrogeekshop.com/encoder-y-arduino-tutorial-sobre-el-modulo-sensor-de-velocidad-ir-con-el-comparador-lm393-encoder-fc-03/

## Aceptación física

Con VMOT apagado durante la carga, probar primero con ruedas elevadas y corriente
limitada. Confirmar que +yaw hace girar las ruedas izquierdas hacia delante y
las derechas hacia atrás, que -yaw invierte el patrón, que la deriva total es
menor de 5.2 cm y que la telemetría ADB refleja el mismo signo. Detener la prueba
ante una sola rueda incorrecta o cualquier traslación visible.

# Análisis ADB de revisión de ejes — 2026-09-11

## Resultado

La primera extracción ADB correspondió a la **sesión SQLite #34**, iniciada el
2026-09-11 22:28:47. La segunda extracción posterior añadió la sesión #35,
iniciada a las 22:32:40. El firmware se identifica
como `robot-s3-v3.5`, el protocolo anunciado es `robot-s3-steps-v3` y las
capacidades incluyen `calibration_pivot_guard_v1`. La #34 terminó en
`failed / cal_stall_left` a las 22:29:03; el operador confirmó después que los
encoders estaban apagados, lo que explica sus ceros. La #35 completó calibración
y avance, pero terminó una reversa en `failed / endpoint_not_reached`.

La evidencia principal es consistente y acotada: el MPU reportó un pivote
positivo de 0.2° a 157.6° mientras el PWM llegó a `990/-990` (247/255), pero
los cuatro encoders y los acumulados de ambos lados permanecieron en cero. El
guard quedó en `waiting_bilateral_ticks`, con `left.average=0`, `right.average=0`
y `stall_ms=790`, y frenó en `cal_stall_left`; no se observó un fallo de signo
del yaw. La SQLite muestra la falta de confirmación PCNT y la confirmación
operativa identifica la causa de #34 como encoders apagados. La #35 demuestra
que, al encenderlos, aparecen ticks, por lo que el fallo de #34 no debe
atribuirse a un bug de firmware o gateway.

## Extracción y procedencia

- Checkout: rama `codex/milestone-funcional-1.0-ejes-ortogonales`.
- Comando ejecutado: `powershell -NoProfile -ExecutionPolicy Bypass -File android_app\\extraer_db_tablet.ps1`.
- ADB: dispositivo autorizado `R52Y108A5DB`.
- Ruta nueva: `tmp_db/robot_20260911_163141.sqlite3`.
- Fecha/hora de extracción local: 2026-09-11 16:31:41–16:31:44 (America/Mexico_City).
- Las marcas de la SQLite son `TEXT` sin offset. Se conservan literalmente como
  `22:28:47`–`22:29:03`; al compararlas con la extracción local de `16:31`
  (America/Mexico_City, UTC−06:00) parecen estar en UTC, pero la base no
  contiene una declaración de zona y esta conversión se trata como hipótesis de
  presentación, no como dato almacenado.
- SHA-256 nueva: `85D2109BFC1C7A9ECE868AD874609A15E009F57DCE2D48CFF16D7C8BD23DA523`.
- Segunda extracción posterior: `tmp_db/robot_20260911_163639.sqlite3`, SHA-256
  `FBA93DB1B3A21994F67252E4A3AA80D048A47FD4D0F617BE991C361088138766`.
- Tercera extracción canónica: `tmp_db/robot_20260911_164322.sqlite3`, 31,621,120 bytes, SHA-256
  `994B687008809D7BBE5C7CFFC4BA620AA834398EAAEBDA04345D0AABCDA3342F`.
- Copia previa conservada sin sobrescribir: `tmp_db/robot.sqlite3`, 30,683,136 bytes,
  SHA-256 `2444946F796073B9710EB87C3BB6A4F4AFAFA8632C714A0C821CBD0CBA92CF21`.

La primera base contiene 34 sesiones, 98 comandos, 937 eventos y 7,073 muestras
de telemetría. La segunda y tercera contienen 35 sesiones, 102 comandos, 970 eventos y
7,218 muestras; #35 es nueva respecto de la primera y todas las bases previas
se conservaron.

Metadatos observados: sesión #34 tiene `firmware_version=robot-s3-v3.5`,
`protocol=negotiating` en la tabla de sesiones y anuncia dentro del JSON
`protocol=robot-s3-steps-v3`, `capabilities=[manual_drive_v1,
motion_supervision_v1, route_axis_recenter_v1, calibration_pivot_guard_v1]`.
No hay hash de build, commit ni número de placa disponible en la SQLite;
`robot_id` está vacío.

## Secuencia de la sesión #34

| Hora | Estado/fase | Evidencia |
|---|---|---|
| 22:28:47 | conexión | `ws://192.168.4.1/ws`, firmware `robot-s3-v3.5`; sensores MPU/I2C presentes y PCNT inicializado. |
| 22:28:51 | accepted / `cal` | Comando `640ee1f510df4c0591e577b50aafc948`, `run_id=1`, `seq=1`; PWM 0, yaw 0°, cuatro encoders 0. |
| 22:28:56 | `cal_a` | inicia rampa con `direction_candidate=1`, `expected_yaw_sign=1`, PWM `560/-560`; todavía sin ticks. |
| 22:28:58 | `cal_a` | yaw 0.2°, `gyro_z=0.0085 rad/s`; encoders `[0,0,0,0]`. |
| 22:29:01 | `cal_a` | yaw 22.7° y luego 48.7°; gyro 0.94–1.13 rad/s; PWM 900–930; encoders siguen en cero. |
| 22:29:02 | `cal_a` | PWM `990/-990`, yaw 102.8°; `stall_ms=20`, sin ticks. |
| 22:29:03 | `cal_a` → fallo | Muestras intermedias llegan a yaw 150.0° y la muestra terminal a 162.6°; gyro 1.19–1.21 rad/s, `verification_ms=680–790`; ambos lados siguen `average=0`, `ok=false`; fault `cal_stall_left`; PWM final 0/0. |

La pose permaneció en `(x,y)=(0,0)` durante todo el intento. El yaw final no
volvió a cero porque la calibración terminó antes de las fases `cal_mas_25`,
`cal_pausa`, `cal_b` y `cal_retorno`. La telemetría declara `mpu_present=1`,
`mpu_stale=0`, `i2c_ok=1`, `enc_trusted=[true,true,true,true]` y
`pcnt_all_initialized=true`; esas banderas prueban disponibilidad/configuración,
no que hayan llegado flancos durante el pivote.

## Sesión posterior #35: calibración y pasos

La segunda extracción confirma que la prueba continuó. Los comandos completos
son: calibrate `184281f41e8c4a4ea18f056e33ca2dec`, `run_id=1/seq=1`, `cal_ok`
(22:32:41–22:33:04); step avance `21e8acd77bcb48b380ba217559710339`,
`run_id=2/seq=1`, objetivo `(0,1000) mm`, `step_ok` (22:33:27–22:33:31); y
step reversa `eef30d4640b04ce8be5a11f26ab5c56e`, `run_id=3/seq=1`, objetivo
`(8,-15) mm`, `endpoint_not_reached` (22:33:35–22:33:39), seguido de stop
`676cc8cee33f47d499ff7831f84417bc` (`stop_ok`).

Durante la calibración #35 los ticks reaparecen: `telemetry.id=7102` tiene
`pfl=2,pfr=0,pbl=2,pbr=2`; `id=7126` tiene `18,0,18,22`; y al completar
(`id=7168`) `41,0,36,54`. Esto confirma que los encoders estaban activos en
#35, aunque FR siguió en cero.

El avance de 100 cm terminó correctamente tras `realign_pause`/`realign_axis`:
`id=7170` muestra `(0,33) mm`, y `id=7176` `(3,164) mm` con ticks laterales.
La primera anomalía del paso reversa es `axis_cross_low` con `realign_pause` en
`id=7195` (pose `(8,942) mm`, PWM `0/0`, `left_delta=5.5`, `right_delta=5`).
Después continuó en sentido reversa. El lado derecho quedó degradado: desde
`id=7193`, `pfr=0` mientras FL/BL/BR aumentaban; el JSON marca
`right_reliable_count=1`, `encoder_fusion.warning=true` y `deviation_pct.fr=100`.
La muestra terminal `id=7218` (`22:33:39`, `seq=545`) queda en `(48,-48) mm`,
yaw `357°`, PWM `0/0`, `longitudinal_error=-3.336 cm`,
`lateral_error=-3.962 cm`, distancia restante `3.727 cm` y
`finish_reason=endpoint_not_reached`. La SQLite prueba ese estado terminal y
la degradación FR; no permite por sí sola decidir si la orden de reversa o la
supervisión de endpoint debe cambiarse.

## Contraste con los casos previos

- **Guard de progreso firmado hacia endpoint:** no aparece en la sesión #34,
  porque no hubo `step`. En la sesión #30 sí hay evidencia de un retorno fallido:
  un `step` de 70 cm con `heading=180` terminó `endpoint_not_reached`; el
  registro muestra poses negativas y repetidos giros/anti-fricción antes de
  detenerse. La base no conserva literalmente el ejemplo textual
  `Y120→116, destino 100`; por tanto no se debe afirmar ese caso exacto desde
  ADB.
- **Pérdida de acumulado al copiar baseline:** no es demostrable de forma
  aislada en #34. En la sesión #33, un `step` de 50 cm entró en la fase
  `recenter_turn` en la fila cruda `telemetry.id=6984` (`20:59:31`, pose
  `(-23,260)`, lateral `-2.299 cm`, `rejoin=(0,35.958)`), pasó a
  `recenter_drive` en `id=7012` (`20:59:35`) y terminó con
  `recenter_diverging` en `id=7021` (`20:59:36`, pose `(-22,256)`). Esto es
  evidencia del fallo histórico de `recenter_diverging` en firmware anterior;
  no reproduce ni demuestra el bug nuevo de cruce de endpoint, ni permite
  concluir que una variable concreta se haya copiado o perdido. Los valores de
  `rejoin` son del diagnóstico emitido por firmware y no deben reinterpretarse
  como un salto físico instantáneo ni como prueba de superar un límite lateral.
- **Reloj de 30 s renovado por intento:** la sesión #34 no inicia reintentos;
  `attempt=1`, `attempt_max=21`. La marca `progress` de `cal_a` se repite 15
  veces hasta el fallo, pero no contiene un reloj de reintento independiente.
  No hay evidencia ADB suficiente para confirmar o refutar la renovación del
  reloj.
- **`CAL_GUARD` 11→21:** #34 anuncia `attempt_max=21`, coincidente con
  `CAL_GUARD_MAX_ATTEMPTS` en `include/Config.h:247` (alias de
  `ROUTE_RECOVERY_MAX_ATTEMPTS`). Esto es observable; la sesión no explica el
  cambio histórico ni autoriza ampliarlo. El fallo ocurrió en el primer intento,
  así que el valor 21 no fue la causa temporal de este aborto.

## Lectura causal y límites

La primera transición anómala observable es la aparición de yaw/gyro mientras
`encoder_delta` y ambos promedios laterales siguen en cero. La transición
terminal ocurre después de alcanzar PWM máximo y acumular el límite de 800 ms;
el código activo incrementa cada acumulador cuando el lado no está confirmado y
evalúa primero el izquierdo (`src/Cinematica.cpp:403-412`). Por eso el texto
`cal_stall_left` es compatible con que ambos lados estén sin ticks; no prueba que
el lado izquierdo sea el único defectuoso.

La correlación con la fuente es independiente del dato visual: `src/Red.cpp`
serializa los pulsos crudos y el delta de calibración; `src/Cinematica.cpp`
genera `waiting_bilateral_ticks` y acumula el stall por lado; y
`src/Sensores.cpp` pausa, lee y limpia PCNT. En #34 esos datos y el guard
confirman la ausencia de ticks que explicó el operador; en #35 los mismos campos
confirman ticks activos y degradación específica de FR.

Observado en el JSON crudo del firmware, además de las columnas derivadas:
`calibration_diagnostics.encoder_delta=[0,0,0,0]`, ambos promedios laterales
`0`, `encoder_fusion.left_delta=0`, `right_delta=0`, y
`pcnt_all_initialized=true`/`pcnt_per_side_ready=true`. Por ello no se trata
solamente de columnas que el gateway pudiera haber omitido. El guard interno
también emite `waiting_bilateral_ticks`; esto prueba que el controlador no vio
la condición bilateral requerida en esas muestras. El operador confirmó que
los encoders estaban apagados en #34 y #35 muestra ticks al encenderlos; la
explicación de #34 queda cerrada. Observado: movimiento
angular por MPU, PWM simétrico de pivote, ausencia de ticks en cuatro ruedas,
guard `waiting_bilateral_ticks`, frenado seguro y firmware/protocolo
identificados. Inferido y priorizado: la confirmación bilateral por PCNT fue la
frontera que bloqueó la calibración con encoders apagados. La sesión #35 deja
como evidencia separada un FR sin ticks y degradación derecha durante los
pasos; no se atribuye aún a un bug concreto.

## Evidencia reproducible exportada

El script `tmp_db/evidencia_adb_20260911.py` ejecuta consultas SQLite de solo
lectura contra la extracción y genera `tmp_db/evidencia_<base>.txt`. El export
incluye comandos y filas de #34, #35 y #33, con timestamps, pose, yaw, PWM y
subobjetos crudos de diagnóstico. Para reproducir la segunda extracción:

```powershell
py -3 .\tmp_db\evidencia_adb_20260911.py .\tmp_db\robot_20260911_163639.sqlite3
```

## Validaciones ejecutadas

Se ejecutaron dos extracciones canónicas, `py -3 consultar_db.py --db ...` en modo
resumen, `--commands 30`, `--events 40`, `--telemetry 30` y `--search calib`,
además de consultas SQLite de esquema y de las sesiones #30–#35. No se lanzaron
controladores, motores, firmware ni HMI. La validación ADB confirma artefacto y
telemetría almacenada; no sustituye validación física.

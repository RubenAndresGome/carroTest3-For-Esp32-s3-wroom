# Plan de corrección de regresiones del Milestone funcional 1.0

## Objetivo y estado

Recuperar un Milestone que detenga siempre el movimiento ante E-STOP, STOP o
pérdida de supervisión; calibre sin trasladar físicamente el centro del chasis;
y ejecute `+Y` al frente y `+X` a la derecha mediante pivotes centrados.

### Estado de implementación (2026-09-10)

- Implementado: lease global de supervisión de 300 ms/1.5 s para manual,
  calibración y rutas; desconexión o vencimiento produce
  `control_connection_lost`, PWM cero, cola cancelada y fallo enclavado.
- Implementado: pivote bilateral con MPU como autoridad angular y corrección
  PCNT por promedio de fuentes confiables; una fuente sana por lado es válida.
- Implementado: odometría durante pivote y diagnóstico de traslación X/Y del
  centro. La calibración sólo acepta `cal_ok` si vuelve al yaw y queda dentro
  de 3 cm del origen; ya no oculta deriva con un `reset()` ciego.
- Implementado: sincronización de contadores al fijar origen, eliminación del
  `delay(50)` dentro del súper-ciclo y firmware `robot-s3-v3.4`.
- Validado en host: 36/36 pruebas nativas, 61/61 pruebas Python, compilación
  ESP32-S3 y validador modular. Pendiente exclusivamente la aceptación física
  controlada y el diagnóstico eléctrico de los encoders frontales.

**Estado actual: implementación validada en host, todavía no aceptada en
suelo.** Este documento no autoriza movimiento, flasheo ni cambios de GPIO.

## No negociables

1. La [hoja física dogmática](../evidencia/hardware/cableado_dogmatico_milestone_2026-09-08.jpeg)
   fija entradas/salidas, GPIO y colores. No se altera para corregir software.
2. `+Y` es frente, `+X` es derecha y el yaw positivo es horario.
3. `PWM_FORWARD_POLARITY` es la única transformación global de sentido.
4. Un encoder sano por lado permite estimar distancia en modo degradado; perder
   un lado completo detiene el movimiento.
5. MPU es autoridad angular. PCNT permanece como fuente de rueda; no se sustituye
   por `attachInterrupt`.
6. Core 0 recibe red y sólo publica solicitudes; Core 1 aplica seguridad y PWM
   en el súper-ciclo síncrono de 100 Hz.
7. E-STOP, interlock de 250 ms, límites PWM y watchdogs no se debilitan.
8. No se activa código de `archive/legacy/` ni archivos `.cpp.disabled`.
9. Todo `.cpp` productivo permanece en `src/`. Los ensayos físicos aprobados
   sólo se compilan desde `archive/firmware_tests/` mediante su script de
   staging; nunca se convierten accidentalmente en firmware normal.

## Línea base de evidencia

- Video `video_2026-09-09_23-12-49.mp4`: durante `CAL_A` y
  `CAL_MAS_25` el chasis abandona el origen físico aunque la pose publicada
  permanezca `(0,0)`.
- SQLite/ADB, sesión 15: calibración `cal_ok`, FL y FR excluidos; final
  FL/FR/BL/BR `322/0/223/365`.
- SQLite/ADB, sesión 16: calibración detenida en 35 %, PWM hasta `990/-990`,
  yaw final 287.7° y deltas aproximados BL/BR `1063/1665`; terminó por pérdida
  de conexión, no por una parada segura.
- SQLite/ADB, sesión 13: tramo `+X` terminó en `(34.5,9.4) cm` para objetivo
  `(30,0) cm`, con `final_endpoint_out_of_tolerance` y contadores
  `214/18/315/1659`.
- Los STOP que llegaron al firmware finalizaron `stop_ok`; en las sesiones
  bloqueadas no se persistió un comando E-STOP. El toque se perdió antes del
  firmware.
- ADB registró cierre forzado de la aplicación con aproximadamente 1.1 GB RSS;
  no existe evidencia de ANR formal.

## Primera cadena causal confirmada

1. El pivote manda magnitudes PWM iguales y opuestas, pero no sincroniza el
   recorrido real de ambos lados.
2. Un encoder por lado confirma “hay movimiento”, aunque BL y BR avancen con
   magnitudes muy distintas.
3. MPU confirma rotación, pero no traslación del centro; durante calibración la
   pose no revela esa deriva.
4. Al terminar, `PoseGlobal.reset()` borra la evidencia lógica sin demostrar
   retorno físico al origen.
5. Si HMI/backend se bloquea o desaparece, la desconexión WebSocket sólo detiene
   el modo manual; calibración y rutas pueden continuar.

## P0 — Parada independiente de la rutina

### Firmware

- Convertir `WS_EVT_DISCONNECT` en una solicitud de seguridad general, no sólo
  manual. Core 0 marca la solicitud y Core 1 frena antes de ejecutar otra fase.
- Interrumpir `CALIBRANDO`, `EJECUTANDO` y `MANUAL`; vaciar comandos pendientes,
  conservar el fallo y publicar `control_connection_lost` para la secuencia
  interrumpida.
- Incorporar un lease explícito de supervisión para todo movimiento autónomo.
  El backend lo renueva; su vencimiento frena aun si TCP no notifica el cierre.
- Mantener E-STOP por encima de STOP, calibración, ruta, reintentos y restauración
  de sensores.

### Backend, HMI y Android

- Hacer que E-STOP y detener una ruta en ejecución no esperen confirmaciones ni
  renderizado. La confirmación de borrar una ruta sólo aplica cuando está inactiva.
- Cancelar primero el movimiento y después actualizar misión, gráfica e historial.
- En cierre normal, exigir terminal `stop_ok`/`estop_latched`; en cierre forzado,
  el lease del firmware es la defensa principal.
- Documentar que el corte físico de VMOT es la única parada independiente de
  WebView, Python, Wi-Fi y ESP32.

### Criterio de aceptación P0

- Cada uno de estos eventos produce PWM cero dentro del límite definido y no
  reanuda fases: E-STOP, STOP, WebSocket cerrado, backend terminado, HMI cerrada
  y lease vencido.
- Una orden antigua o reconexión nunca reanuda el movimiento cancelado.

## P1 — Verdad eléctrica sin modificar el dogma

- Añadir pruebas de compilación que fijen exactamente los doce GPIO dogmáticos:
  ocho canales de motor y cuatro encoders.
- Mantener separados `pwmLogico` y `pwmElectrico`; telemetría debe publicar ambos.
- Con ruedas elevadas, probar por separado FL, BL, FR y BR con pulsos breves
  positivos y negativos. Registrar sentido físico, corriente y ticks.
- Mantener `PWM_FORWARD_POLARITY=-1` mientras las cuatro pruebas coincidan. Una
  discrepancia individual se corrige físicamente en su terminal, nunca mediante
  otro cambio global ni permutando GPIO.

## P1 — Calibración sin traslación del origen

- Conservar la secuencia angular +25° y retorno al yaw inicial, pero compartir
  con las rutas un único controlador de pivote centrado.
- Reconstruir recorrido firmado por lado usando el signo ordenado y los ticks de
  las fuentes confiables. Un encoder por lado sigue siendo suficiente.
- Regular PWM izquierdo y derecho de forma independiente para igualar magnitud
  de recorrido, con MPU como autoridad del ángulo.
- Añadir límite persistente de desequilibrio y un timeout total corto. Pulsos en
  ambos lados no deben ocultar un pivote que se está trasladando.
- Calcular y publicar deriva longitudinal estimada, balance por lado, yaw inicial
  y yaw final antes de cualquier reset.
- Ejecutar `PoseGlobal.reset()` sólo después de aceptar balance, deriva y yaw;
  nunca usar el reset como prueba de retorno.
- Con FL/FR ausentes, permitir BL/BR en degradado, pero exigir balance bilateral.

### Criterio de aceptación de calibración

- Final angular dentro de la tolerancia configurada, sin timeout ni inversión
  pendiente.
- Deriva odométrica compatible con cero y centro físico dentro de 2 cm de la
  marca inicial en la prueba de suelo controlada.
- Diagnóstico conserva qué encoder participó en cada sentido; no declara sano
  un canal que sólo produjo un pulso aislado.

## P1 — Pivote de ruta y PID

- Bloquear la fase de avance hasta que el pivote esté asentado y balanceado.
- Aplicar el mismo sincronizador bilateral de calibración a `GIRO_INICIAL`,
  `GIRO_FINAL`, recuperación y `turn_to`.
- Ante desequilibrio persistente, fallar el paso; no continuar mediante
  `endpoint_soft` ni compensarlo con PID de avance.
- Validar primero la tabla lógica del PID con Ki=0: error yaw positivo/negativo,
  avance/reversa y los cuatro rumbos cardinales. Después habilitar integral y
  corrección lateral de forma gradual.
- Mantener la decisión geométrica en la capa lógica; verificar físicamente qué
  lado debe reducirse antes de ajustar ganancias.

## P2 — HMI, memoria y capacidad de detener

- Acotar todas las colecciones en memoria: eventos, logs, telemetría, trazas de
  mapa, puntos de Chart.js y colas SSE.
- Separar ingestión de telemetría de renderizado; mostrar a 10–15 FPS como máximo
  y suspender gráficas invisibles.
- Medir heap/RSS e input latency durante una sesión de 30 minutos. La memoria
  debe alcanzar una meseta y los botones de parada deben conservar respuesta.
- Añadir una prueba Android que pulse E-STOP durante calibración y ruta mientras
  llegan telemetría y eventos a máxima tasa.

## P2 — Encoders frontales

- Confirmar primero inicialización PCNT y después nivel crudo/pulsos por canal;
  PCNT configurado no prueba continuidad eléctrica.
- Probar FL y FR individualmente con rueda elevada, revisar separación del cable
  de motor, umbral LM393 y traducción B5→A5/B6→A6.
- Registrar ausencia, señal atascada y ruido como estados distintos.
- Un doble conductor hacia un mismo GPIO sigue siendo una sola fuente; no se
  contabiliza como dos encoders.
- La restauración readmite un canal únicamente después de una ventana estable y
  actualiza su máscara; no reinicia ni reanuda una calibración o ruta cancelada.

## Matriz mínima bug → prueba

| Bug | Prueba automática | Evidencia física/ADB |
|---|---|---|
| Rutina continúa sin backend | desconexión y lease durante cada fase | PWM llega a cero y aparece motivo terminal |
| E-STOP no llega | API/HMI→gateway→firmware con cola ocupada | comando persistido y `estop_latched` |
| Calibración se traslada | simulación de ticks bilaterales desiguales | marca de suelo y deriva ≤2 cm |
| Pivote sale del punto | balance firmado por lado | giro ±90° sobre cruz marcada |
| PID corrige al lado incorrecto | tabla de signos cardinal/reversa | pulsos cortos y ruta +Y/−Y/+X/−X |
| Un encoder por lado | combinaciones FL/BL y FR/BR | distancia con regla y máscara publicada |
| HMI se congela | carga sostenida y colecciones acotadas | memoria en meseta y toque E-STOP registrado |

## Secuencia de implementación y rollback

1. Commit P0 exclusivamente de parada/desconexión/lease.
2. Commit que congela el dogma en pruebas y telemetría; sin cambiar GPIO.
3. Commit de pivote bilateral y calibración observable.
4. Commit de PID por etapas y rutas cardinales.
5. Commit de límites de memoria y pruebas HMI/Android.

Cada etapa debe poder revertirse sin retirar las protecciones P0. No mezclar
ganancias, GPIO y seguridad en un mismo commit.

## Validación antes de suelo

- Pruebas nativas de motores, calibración, pivote, PID, reversa, sensores
  degradados, STOP, E-STOP, desconexión y lease.
- Suite Python de gateway, API, misión, persistencia y cierre.
- Compilación PlatformIO y `scripts/firmware/validar_firmware_modular.ps1`.
- Validación de HMI y empaquetado Android desde la fuente canónica.
- `git diff --check` y árbol limpio salvo los cambios previstos.

Sólo después: VMOT apagado durante carga, ruedas elevadas, fuente limitada,
medición de corriente y prueba individual por rueda. La prueba en suelo comienza
con una cruz de origen, E-STOP accesible y tramos cortos; compilar no demuestra
seguridad eléctrica ni retorno físico.

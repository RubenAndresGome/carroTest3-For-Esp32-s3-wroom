# Auditoría del estado actual

**Corte auditado:** `7a69eac`  
**Alcance:** `src/`, `include/`, `desktop_app/`, `android_app/`, scripts activos,
protocolo v3, SQLite y evidencia audiovisual.  
**Exclusiones del conteo activo:** `archive/`, archivos `.disabled`, dependencias
`vendor`, pruebas, salidas `build/dist/.pio` y código generado. Su existencia y
su función histórica sí se revisan en el apartado correspondiente.

## Dictamen ejecutivo

La separación de responsabilidades es correcta en su intención: Python posee
la misión y el historial; el ESP32 posee control en tiempo real, PWM, E-STOP y
watchdogs. El sistema compila y dispone de protecciones importantes, pero la
documentación no puede declarar todavía una operación resiliente ni una
seguridad eléctrica aceptada.

Los riesgos principales son la pérdida de memoria de protocolo cuando reinicia
el ESP32, el límite de cinco reconexiones del backend, el reinicio de `seq` a 1
dentro de la misma sesión y la posibilidad de perder eventos terminales al
llenarse la cola. Además, las reglas operativas escritas contradicen varias
constantes reales del firmware.

## Resumen cuantitativo

El catálogo regenerable [`catalogo_funciones.json`](catalogo_funciones.json)
identifica 54 archivos operativos, 460 funciones/prototipos, 26 tipos, 18 rutas
HTTP y un posible ciclo nominal de dependencias. Los diagramas por carpeta están
en [`uml/`](uml/README.md); la escala, esfuerzos y riesgos aceptados están en
[`hallazgos_y_riesgos.md`](hallazgos_y_riesgos.md).

| Subsistema | Funciones/prototipos detectados | Propietario operativo |
|---|---:|---|
| `firmware/src` | 106 | ESP32, Core 0 y súper-ciclo Core 1 |
| `firmware/include` | 55 | Contratos y configuración de firmware |
| `backend/python` | 118 | Misiones, validación, SQLite y gateway |
| `hmi/canonica` | 74 | Interacción del operador |
| `frontend/typescript` | 3 | Base Vite aislada; no es la HMI desplegada |
| `android/kotlin` | 28 | WebView, servicio y ciclo de vida Android |
| `android/python` | 4 | Waitress/Flask embebido |
| `herramientas/scripts` | 72 | Construcción, validación, consulta y descomponedor HTML independiente |

## Hallazgos

### A-01 — Alta: la reconexión automática se detiene

`RobotGateway.MAX_CONNECT_ATTEMPTS` vale 5. Al agotar los intentos, la hebra
queda esperando indefinidamente una acción manual de reconexión. Esto contradice
el requisito de tolerar cortes accidentales de Wi-Fi prolongados.

- Evidencia: `desktop_app/robot_app/gateway.py`, líneas 35 y 164–190.
- Efecto: una misión queda detenida aunque el AP reaparezca después.
- Recomendación: reintento indefinido con backoff acotado, estado `offline`
  explícito y reactivación inmediata mediante evento de red Android.

### A-02 — Alta: la sesión corta no sobrevive al reinicio del ESP32

`sessionId`, `ultimoSeqCompletado`, calibración y pose residen en RAM. Un reboot
devuelve `last_seq=0`, `cal=false` y pose cero incluso si Python conserva su
`controller_session`.

- Evidencia: `src/Red.cpp`, líneas 58–72; `src/Estado.cpp`.
- [Evidencia SQLite resumida](../evidencia/incidentes/2026-07-30_reinicio_y_calibracion.md):
  en la sesión revisada, la misma sesión Python pasó de
  `cal=true,last_seq=1` a `cal=false,last_seq=0` después de perder conexión.
- Efecto: no es posible decidir automáticamente si un paso se ejecutó parcial o
  totalmente antes del reinicio.
- Recomendación: publicar `boot_id` monotónico; persistir un journal mínimo de
  comando activo/terminal o exigir resolución operativa antes de reanudar.

### A-03 — Alta: `seq=1` borra la memoria de idempotencia

Python reinicia `_next_seq` a 1 al activar una misión y el firmware interpreta
cualquier `seq==1` como reinicio de `ultimoSeqCompletado`. El identificador UUID
de Python no viaja en los comandos `step/turn_to`.

- Evidencia: `desktop_app/robot_app/services.py`, activación de misión;
  `src/Red.cpp`, líneas 98–104; `src/Cinematica.cpp`, líneas 689 y 725.
- Efecto: un paquete atrasado o duplicado de una misión anterior podría abrir
  de nuevo la ventana de secuencias dentro de la misma sesión.
- Recomendación: secuencia monotónica durante toda la sesión y `command_id` o
  `step_id` obligatorio en el protocolo.

### A-04 — Alta: un evento terminal puede perderse silenciosamente

`encolarEvento()` modifica `ultimoSeqCompletado` y limpia `seqActivo` antes de
llamar a `xQueueSend()`, pero ignora si la cola de ocho elementos está llena.

- Evidencia: `src/Eventos.cpp`, líneas 6–30.
- Efecto: el robot puede terminar y quedar libre mientras Python conserva el
  comando como activo hasta reconciliación o timeout.
- Recomendación: reservar capacidad para terminales, reemplazar progreso antes
  de descartar un terminal y telemetrizar contador de pérdidas.

### A-05 — Alta: especificación y firmware contradicen límites operativos

Las reglas vigentes describen 450 ms de stall en avance, 2.5 s en giro, 800 ms
de calibración máxima, tolerancia de ±2° y micro-pulsos finos. El código usa,
respectivamente, 6000 ms, 4000 ms, 1000 ms, 3° y no contiene
`TURN_PULSE_ON_MS/TURN_PULSE_OFF_MS`. La rampa también está configurada con
intervalo de 500 ms y paso aproximado de 13/255.

- Evidencia: `include/Config.h`, líneas 62, 78–112.
- Efecto: las pruebas de aceptación escritas no prueban el producto compilado.
- Recomendación: decidir una única especificación aprobada y hacer que una
  prueba automática compare constantes documentadas contra firmware.

### A-06 — Alta operativa: calibración con IMU activa y encoders en cero

La [evidencia SQLite de la prueba](../evidencia/incidentes/2026-07-30_reinicio_y_calibracion.md)
registró giro por IMU, cuatro
encoders en cero y fallo `cal_stall_left`.

- Efecto: el robot se mueve, pero no puede confirmar movimiento por lado ni
  completar calibración de forma confiable.
- Recomendación: prueba con ruedas elevadas, inspección de GPIO/PCNT y conectores,
  alimentación limitada y registro simultáneo de cada encoder.

### A-07 — Media: módulo de misión firmware compilado pero desconectado

`src/Mision.cpp` implementa persistencia y ejecución de rutas completas, pero
`main.cpp` no llama a `procesarMisionAutonoma()` ni inicializa ese módulo. La
arquitectura operativa usa pasos atómicos propiedad de Python.

- Efecto: dos modelos incompatibles permanecen en fuentes activas y elevan el
  riesgo de documentación o mantenimiento incorrectos.
- Recomendación: moverlo a referencia histórica o integrarlo únicamente tras
  una decisión arquitectónica explícita.

### A-08 — Media: sesiones SQLite se fragmentan durante cada `BACKOFF`

`RobotService._on_connection_state()` cierra la sesión SQLite tanto al detenerse
como al entrar en backoff. La sesión de protocolo sí continúa, pero una caída
breve produce varias sesiones históricas.

- Efecto: dificulta correlacionar una sola misión física y sus reconexiones.
- Recomendación: mantener una sesión lógica hasta cierre del proceso/operador y
  registrar cortes de transporte como eventos internos.

### A-09 — Media: pruebas internas de firmware no se ejecutan

Existen `validarInterlockMotores()` y `validarLogicaDiagnosticoRTOS()`, pero no
son invocadas por el arranque ni por una suite host. Además, una aserción del
interlock espera el límite de avance aunque el nivel bajo admite el límite de
giro.

- Efecto: una compilación verde no demuestra esas invariantes.
- Recomendación: extraer lógica pura a pruebas host o ejecutar un self-test de
  arranque con motores desenergizados.

### A-10 — Media: dos frontends con distinto grado de vigencia

La HMI efectiva es un HTML/JavaScript monolítico en
`desktop_app/robot_app/hmi/index.html`; `desktop_app/frontend/src` contiene una
base TypeScript mínima que no reemplaza esa interfaz.

- Efecto: la compilación frontend no cubre la mayor parte de la HMI operativa.
- Recomendación: migración gradual de módulos HMI a TypeScript y pruebas de sus
  flujos críticos.

### A-11 — Baja: mensajes de preparación mezclan causas distintas

La creación de misión puede fallar por transporte, handshake, reconciliación de
arranque, telemetría obsoleta, falta de calibración o estado distinto de
`listo`. La HMI no siempre presenta estas causas como estados separados.

- Efecto: el operador interpreta un reboot o una calibración perdida como “no
  conectado”.
- Recomendación: códigos de error estables y guía contextual de recuperación.

### A-12 — Brecha de aceptación: seguridad eléctrica no demostrada

Los videos muestran movimiento y operación de HMI, pero no muestran medición de
corriente, fuente limitada, fusible ni rotor bloqueado. No debe afirmarse
seguridad eléctrica hasta completar el protocolo físico.

### A-13 — Alta: runtimes SQLite afectados por el fallo WAL-reset de 2026

El runtime Python de pruebas carga SQLite 3.43.1 y la biblioteca arm64 incluida
en el APK reporta 3.50.4. La documentación oficial de SQLite indica que el
fallo raro de reinicio de WAL afecta 3.7.0–3.51.2 y se corrige en 3.51.3, con
backports 3.44.6 y 3.50.7. `Database` abre una conexión independiente por
transacción y puede escribir desde hilos distintos, por lo que no se puede
descartar la precondición de escritores/checkpoints concurrentes.

- Evidencia local: `desktop_app/.test-venv` → SQLite 3.43.1; extracción de
  `lib/arm64-v8a/libsqlite3_python.so` del APK → cadena 3.50.4;
  `desktop_app/robot_app/database.py`, líneas 17–42.
- Referencia primaria: [SQLite — Write-Ahead Logging, sección 11](https://sqlite.org/wal.html#the_wal_reset_bug).
- Efecto: riesgo improbable pero posible de corrupción en la bitácora que se
  usa para reconciliar cierre, comandos y misiones.
- Recomendación: actualizar a SQLite 3.51.3+, 3.50.7 o un runtime con backport;
  serializar escrituras/checkpoints y añadir una prueba concurrente de estrés.

## Código histórico, pruebas y herramientas no operativas

- `src/Mision.cpp` es fuente activa y compilada, pero no gobierna la misión
  desplegada: `main.cpp` no invoca su máquina de ruta. Python envía una sola
  maniobra atómica por vez. Esta diferencia está registrada en A-07.
- Los `*.cpp.disabled` son ensayos físicos aprobados o candidatos archivados.
  No entran al firmware normal; se compilan sólo mediante el script de staging
  documentado en `CONTRIBUTING.md`.
- `archive/legacy/` conserva arquitecturas y HMI anteriores como referencia;
  no se usa para inferir el comportamiento del binario actual.
- `SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html` es una
  herramienta independiente para transformar puntos en vectores. Sus funciones
  aparecen bajo `herramientas/scripts`, pero el archivo no controla el robot.
- `consultar_db.py` es la interfaz CLI canónica de lectura de SQLite; no forma
  parte del proceso de control.
- Las pruebas automatizadas se documentan como subsistema de validación, no
  como funciones operativas del robot, y por eso se excluyen del catálogo
  activo.

## Fortalezas verificadas

- Propiedad única del WebSocket en Python.
- Core 0 dedicado a red y Core 1 al súper-ciclo de 100 Hz.
- PCNT para los cuatro encoders.
- PWM en cero como primera operación controlable de `setup()`.
- Interlock por lado al invertir polaridad.
- E-STOP procesado con prioridad y frenado local.
- Validación finita y rangos de payload antes de encolar comandos.
- SQLite en WAL, `synchronous=FULL` y transacciones explícitas.
- Token local para mutaciones HTTP, `TRUSTED_HOSTS` y CSP.
- Cuarentena de misiones al reiniciar el proceso Python.
- Cierre seguro que espera `stop_ok` antes de autorizar salida normal.

## Matriz de decisión para reconexión

| Situación | Evidencia disponible | Conducta segura |
|---|---|---|
| Sólo cae WebSocket; mismo `boot_id` y `last_seq` | Estado del ESP32 conservado | Reconciliar y reenviar únicamente el comando no confirmado con la misma identidad. |
| Reinicia Python | SQLite conoce misión previa, pero se pierde propietario | Abandonar misión, fallar comandos y enviar STOP de reconciliación. |
| Reinicia ESP32 antes del paso | Nuevo `boot_id`, sin comando activo | Recalibrar y permitir replanificación desde pose verificada. |
| Reinicia ESP32 durante un paso | No se conoce distancia parcial | Frenar, bloquear y exigir decisión/medición; no repetir automáticamente. |
| Desaparece el AP por tiempo prolongado | Sin telemetría fresca | Mantener misión pausada y reintentar transporte indefinidamente sin afirmar estado físico. |

## Criterios pendientes para aceptación

1. Resolver A-01 a A-05 antes de declarar tolerancia completa a desconexión.
2. Resolver A-06 antes de nuevas rutas en suelo.
3. Ejecutar prueba de corriente obligatoria.
4. Correlacionar video, telemetría y medición física en una misma sesión.
5. Verificar ruta `Y+100,Y+50,X+190,X-190`, retorno Ockham y yaw final.
6. Actualizar/verificar SQLite según A-13 antes de usar la bitácora como prueba
   única de recuperación tras cierres abruptos.

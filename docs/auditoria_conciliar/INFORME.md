# Auditoría Conciliar — revisión crítica de la casuística nivel 2

- **Fecha:** 2026-08-23
- **Herramienta:** `concilio-salamanca` v1.1.0 (ejecutada desde `D:\DocumentosWin\mde-concilio-de-salamanca`)
- **Casuística:** `--agents escolasticos --mode escolastico`
- **Motor:** `--provider deepseek --model deepseek-v4-flash-vision-exp`
- **Configuración:** hasta 4 llamadas por archivo, `--audit-level 2`, `--token-budget 6000–8000`
- **Alcance declarado:** firmware (`src/*.cpp`) y backend (`desktop_app/robot_app/*.py`)
- **Artefactos revisados:** 17 ficheros `*_l2.md` y `smoke_l2_cinematica.md`

Este informe conserva los veredictos emitidos, pero no los adopta como diagnóstico. Cada
afirmación concreta se contrastó con sus llamadores, validadores y rutas operativas.

## Resultado ejecutivo

- El recuento bruto es correcto: **15 `CONDENA` y 2 `ABSUELVE`**.
- Sólo cuatro artefactos contienen afirmaciones concretas: `main`, `Mision`, `Motores` y
  `Estado`. Los otros **13** terminan en una síntesis electoral y dicen expresamente que se
  revise un «ledger compacto» no incluido; por sí solos no aportan un hallazgo reproducible.
- Los **17** artefactos declaran `Analisis estatico: 0 lineas`. Por ello, el veredicto electoral
  no sustituye evidencia de código, independientemente de que sea condena o absolución.
- Hay contradicción PnC explícita en **11 de 17** dictámenes. El smoke añade otra.
- El smoke es inválido como auditoría de `Cinematica.cpp`: reporta 0 líneas de C++, pero aplica
  un veto por `eval(`. Eso diagnostica un problema de entrada/clasificación del arnés, no del
  firmware.
- Los Markdown crudos presentan texto mojibake (`c≤digo`, `Sφntesis`, etc.); conviene conservarlos
  como evidencia original y corregir la codificación en futuras ejecuciones.

## Estado de remediación

Las deudas defendibles de esta revisión se corrigieron en el árbol activo:

- `src/Estado.cpp` exige `Secrets.h`, valida por compilación los límites de SSID/clave y ya no
  cae silenciosamente en la plantilla pública. Los scripts de staging generan credenciales
  sintéticas dentro de `.pio` sin copiar el secreto local.
- `src/Motores.cpp` eliminó el fallback al canal 0, valida el mapa completo antes de escribir,
  verifica cada `ledcSetup()`, pone a cero cada canal inmediatamente y conserva una máscara para
  apagar también configuraciones parciales. Devuelve un fallo que `Cinematica.cpp` convierte en
  parada enclavada; `CLEAR_FAULT` no puede rearmar el sistema mientras los motores no estén listos.
- `src/Cinematica.cpp` rechaza rumbo/distancia no finitos en sus APIs públicas. Además se corrigió
  un defecto encontrado durante la revisión: cuando `objetivoAbsoluto` es falso, los ceros de un
  `ComandoRed {}` ya no se interpretan como destino espacial `(0,0)`.
- `Mision.cpp`/`Mision.h` se retiraron de `src/` e `include/` y se conservan sólo en
  `archive/legacy/firmware_mission/`. Python queda como único propietario operativo de misión.

Los dictámenes `*_l2.md` se mantienen intactos como evidencia cruda; este apartado registra la
remediación posterior y no reescribe sus veredictos.

## Resumen de veredictos y valor probatorio

| Archivo | Veredicto | Llamadas LLM | Resultado del contraste |
|---|---|---:|---|
| `firmware_main` | CONDENA | 2 | Hallazgos concretos no confirmados |
| `firmware_Mision` | CONDENA | 2 | Sin desbordamiento confirmado; dos debilidades latentes |
| `firmware_Motores` | CONDENA | 2 | Inicialización confirmada; fallback latente |
| `firmware_Estado` | CONDENA | 3 | Diagnóstico errado; se identifica un fallback inseguro distinto |
| `firmware_Eventos` | CONDENA | 3 | Síntesis sin evidencia expuesta |
| `firmware_PoseEstimator` | CONDENA | 2 | Síntesis sin evidencia expuesta |
| `firmware_Red` | CONDENA | 4 | Síntesis sin evidencia expuesta |
| `firmware_Seguridad` | CONDENA | 1 | Síntesis sin evidencia expuesta |
| `firmware_Sensores` | CONDENA | 1 | Síntesis sin evidencia expuesta |
| `python_services` | CONDENA | 2 | Síntesis sin evidencia expuesta |
| `python_domain` | CONDENA | 2 | Síntesis sin evidencia expuesta |
| `python_database` | CONDENA | 2 | Síntesis sin evidencia expuesta |
| `python_gateway` | CONDENA | 2 | Síntesis sin evidencia expuesta |
| `python_web` | CONDENA | 2 | Síntesis sin evidencia expuesta |
| `python_compaction` | CONDENA | 2 | Síntesis sin evidencia expuesta |
| `python_app_factory` | ABSUELVE | 4 | Síntesis sin evidencia expuesta |
| `python_config` | ABSUELVE | 4 | Síntesis sin evidencia expuesta |

Las dos absoluciones tampoco prueban ausencia de defectos: proceden del mismo mecanismo electoral
sin diagnóstico expuesto.

## Contraste de los cuatro dictámenes sustantivos

### 1. `main.cpp` — NaN/rangos de `ComandoRed` y antigüedad de sensores

**Afirmación del concilio:** los campos de `ComandoRed` pueden contener NaN o valores fuera de
rango, y el lazo usa muestras temporalmente incoherentes.

**Contraste:**

- `src/Red.cpp:78-81` define `leerFloatFinito()` y exige un tipo numérico y `isfinite()`.
- `src/Red.cpp:129-146` rechaza `heading`, distancia y objetivos no finitos; además limita las
  coordenadas absolutas antes de encolar el comando.
- `src/Cinematica.cpp:1069-1076` vuelve a comprobar distancia y objetivos y normaliza el rumbo.
  La validación está repartida entre frontera de red y controlador, pero existe.
- `src/main.cpp:97-99` toma una muestra síncrona inmediatamente antes de consumirla. La secuencia
  evita reprocesar una muestra sin avance.
- `src/Sensores.cpp:253-264` asigna timestamp y secuencia en cada adquisición. Para la IMU,
  `mpu_stale` se calcula en `src/Sensores.cpp:205-234`; `src/main.cpp:110-120` frena y entra en
  fallo si la IMU queda ausente u obsoleta durante movimiento.
- `src/main.cpp:171-175` usa la edad de muestra para diagnóstico RTOS, no como la única defensa
  de frescura.

**Dictamen revisado:** **falso positivo**. Como defensa en profundidad, las APIs públicas de
`Cinematica.cpp` podrían rechazar por sí mismas todo argumento no finito, pero no se demostró una
ruta operativa que introduzca NaN en `ComandoRed` ni una muestra obsoleta sin reacción segura.

### 2. `Mision.cpp` — buffers y `atoi()`

**Afirmación del concilio:** hay desbordamiento o ausencia de terminación nula y `atoi()` puede
producir comportamiento indefinido.

**Contraste:**

- `src/Mision.cpp:43-44`, `104-105`, `123-124` y `181-182` terminan explícitamente los destinos.
- `src/Mision.cpp:70-71` sólo copia cadenas persistidas cuya longitud es exactamente 32. Con un
  destino de 33 bytes no hay desbordamiento. Es cierto que esas dos sentencias no escriben
  explícitamente el byte 32: hoy permanece en cero por la inicialización estática y por las demás
  escrituras del módulo. Es un invariante frágil, no la vulnerabilidad afirmada.
- `atoi()` devuelve 0 para una cadena no numérica y el código aplica un fallback. El riesgo real
  aparece si una cadena decimal excede `int`, porque `atoi()` no permite detectar el error de
  rango. También pierde silenciosamente la identidad de IDs alfanuméricos.
- La búsqueda de llamadores confirma que `inicializarPersistenciaMision()`,
  `cargarMisionAutonoma()`, `iniciarMisionAutonoma()` y `procesarMisionAutonoma()` no son invocados
  por el firmware operativo. La arquitectura activa envía pasos atómicos desde Python; este hecho
  ya consta en `docs_markdowns/auditoria_estado_actual.md` (A-07).

**Dictamen revisado:** **no se confirma desbordamiento**. Sobreviven dos deudas latentes en un
módulo desconectado: conversión no estricta con `atoi()` (`src/Mision.cpp:52,134`) y terminación
nula implícita en las restauraciones de `src/Mision.cpp:70-71`. Si se reactiva el módulo, deben
corregirse y probarse antes de conectarlo a red o movimiento.

### 3. `Motores.cpp` — inicialización PWM y fallback de canal

**Afirmación del concilio:** se escribe PWM sin garantía de `ledcSetup()` y un pin desconocido cae
al canal 0.

**Contraste:**

- `src/main.cpp:140` llama `setup_Motores()` durante `setup()`, antes de crear tareas y antes del
  superciclo.
- `src/Motores.cpp:193-199` configura y adjunta los ocho canales, y después los frena.
- `canalParaPin()` sí devuelve 0 si no encuentra un pin (`src/Motores.cpp:84-86`). Sin embargo, es
  una función `static` y todos sus llamadores actuales pasan constantes incluidas en el mismo
  `MOTOR_PINS`; no se encontró una ruta de entrada externa hacia ella.

**Dictamen revisado:** la falta de inicialización es **falso positivo**. El fallback es una
**debilidad latente de fail-safe**, no un bug activo demostrado. Conviene devolver `-1`, omitir
toda escritura del par cuando el mapeo sea inválido y registrar/enclavar un fallo; también puede
eliminarse el lookup pasando canales explícitos o validando unicidad del mapa en compilación.

### 4. `Estado.cpp` — secretos y fallback de configuración

**Afirmación del concilio:** si falta `Secrets.h`, las macros quedan indefinidas y falla la
compilación; además, las credenciales se aceptan como entrada externa sin validar.

**Contraste:**

- La afirmación de compilación es falsa: `src/Estado.cpp:2-6` incluye `Secrets.h` cuando existe y,
  en caso contrario, incluye `Secrets.example.h`, que define ambas constantes.
- Las credenciales son constantes de compilación, no una entrada de red ni memoria mutable por
  el usuario durante ejecución. `WiFi.softAP()` comprueba el arranque y `src/Red.cpp:41-46` trata
  su fallo.
- Sí existe un riesgo distinto y verificable: el fallback compila con el SSID y la contraseña
  públicos de `include/Secrets.example.h`. Si se omite por error el archivo local, el AP puede
  arrancar con credenciales conocidas en vez de fallar de forma explícita.
- `include/Secrets.h` está correctamente ignorado por Git; se comprobó su presencia local sin
  leer ni exponer su contenido.

**Dictamen revisado:** el diagnóstico original es **falso**, pero revela una debilidad real de
configuración **fail-open**. Para una compilación destinada al robot, debe exigirse `Secrets.h`
mediante inclusión obligatoria o `#error`; el archivo de ejemplo no debería ser fallback de
producción. Pueden añadirse `static_assert` de longitud para SSID y clave.

## Hallazgos que sobreviven al contraste

No se confirmó un defecto activo en las cuatro acusaciones concretas. Sí sobreviven estas deudas
de robustez, ordenadas por relevancia operativa:

1. **`Estado.cpp:2-6` — fallback de credenciales conocido.** Riesgo de desplegar un AP con valores
   públicos si falta la configuración local. Recomendación: fallar la compilación.
2. **`Motores.cpp:84-100` — canal 0 como fallback.** Ruta actualmente interna e inalcanzable con
   el mapa presente, pero contraria a fail-safe. Recomendación: error explícito y cero PWM.
3. **`Mision.cpp:52,134` — `atoi()` sin conversión estricta.** Riesgo latente/semántico y de rango
   si se reactiva el módulo desconectado.
4. **`Mision.cpp:70-71` — byte nulo implícito.** No hay desbordamiento actual; hacer explícito el
   terminador elimina una dependencia innecesaria del estado previo.

## Verificación del árbol actual

- `platformio run`: **SUCCESS** para `esp32-s3-devkitc-1` (RAM 15.3 %, flash 42.4 %).
- `scripts/firmware/validar_firmware_modular.ps1`: **SUCCESS** con `Secrets.h` sintético en staging.
- `platformio test -e pruebas_control_ruta_nativas`: **20 pruebas aprobadas**.
- `desktop_app/.test-venv/Scripts/python.exe -m unittest discover -s tests -v`:
  **44 pruebas aprobadas**.
- `scripts/documentacion/validar_enlaces.py`: **733 destinos locales válidos**.
- El ensayo histórico `test_encoders_funcion_only_aprobado` compila desde staging sin copiar
  credenciales locales. `test_mpu6050_angulo_unicamente_aprobado` conserva un fallo previo por la
  constante obsoleta `THETA_ERROR_DEG`; no se alteró el ensayo físico aprobado.

Estas validaciones demuestran que el árbol actual compila y que la suite automatizada del backend
pasa. No prueban las ramas desconectadas de `Mision.cpp`, el fallback cuando falta el secreto local
ni la seguridad eléctrica/física del robot.

## Evaluación del método

La casuística produjo formato, votos y detección de desacuerdo, pero esta ejecución no es una
auditoría de programa completo. Sus principales límites observados son:

1. análisis por archivo sin cierre sobre llamadores ni fronteras de validación;
2. 13 decisiones sin evidencia detallada accesible;
3. presupuesto agotado en los 17 dictámenes;
4. metadato de análisis estático de 0 líneas en todos los artefactos;
5. smoke contaminado por un hallazgo `eval(` incompatible con el C++ objetivo;
6. salida con codificación dañada.

Reejecutar con `--token-budget 12000` sólo tiene valor si además se corrigen la ingestión del
código, la codificación y la exportación del ledger/evidencias. Aumentar tokens por sí solo no
resuelve los falsos positivos interarchivo ni vuelve auditables las síntesis electorales.

---
*Revisión crítica contra la fuente canónica; los `*_l2.md` se conservan como evidencia cruda.*

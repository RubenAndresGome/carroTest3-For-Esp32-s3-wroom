---
name: analisis-video-evidencia
description: Usa grabaciones de video como evidencia física forense para analizar y corregir fallos del robot ESP32-S3 a partir de acciones o discusiones repetidas en el chat. Solicita obligatoriamente la ubicación y el nombre del archivo de video, correlaciona la verdad visual del video con las acciones/comandos registrados en la conversación, la telemetría SQLite/ADB y el código de firmware, rompiendo bucles de prueba y error.
---

# Análisis de Video como Evidencia para Corrección de Errores del Robot

Esta skill proporciona una metodología forense y multimodal para diagnosticar y corregir fallos físicos y lógicos en el robot ESP32-S3 utilizando grabaciones de video como verdad fundamental (*ground truth*). Se enfoca especialmente en desenredar y resolver problemas cuando en el chat se han repetido acciones, quejas cíclicas o intentos fallidos de depuración.

---

## 0. Requerimiento Obligatorio: Ubicación y Nombre del Video

> [!IMPORTANT]
> **REGLA DE ORO DE LA SKILL**:
> El agente **NUNCA debe asumir ni inventar** la ubicación ni el nombre del video. Si el usuario no proporcionó explícitamente ambos datos en el mensaje actual o en el contexto inmediato, el agente **DEBE solicitar obligatoriamente al usuario:**
> 1. **Ubicación (Ruta o Carpeta)**: e.g., `evidencia/videos/`, `C:\Videos\Robot\`, o ruta relativa/absoluta.
> 2. **Nombre exacto del archivo**: e.g., `ensayo_giro_calibracion.mp4`, `ejes_invertidos_2026-09-08.mp4`, etc.

### Protocolo de solicitud inicial
Si falta cualquiera de los dos datos, responde directamente al usuario con una solicitud clara y estructurada antes de intentar cualquier diagnóstico:

```markdown
Para analizar el fallo del robot con evidencia visual, necesito los datos del video:
- **Ubicación (carpeta o ruta):** (ejemplo: `evidencia/videos/` o ruta absoluta)
- **Nombre del archivo:** (ejemplo: `mi_ensayo.mp4`)

Por favor confírmalos para iniciar la inspección forense.
```

### Verificación de accesibilidad del archivo
Una vez proporcionados la ubicación y el nombre:
1. Construye la ruta completa (`os.path.join(ubicacion, nombre)` o ruta absoluta normalizada).
2. Verifica su existencia en disco mediante `find_by_name`, `list_dir` o comandos de shell.
3. Si el archivo no existe en la ruta dada:
   - Notifica al usuario la ruta buscada.
   - Si existen videos en la carpeta estándar del repositorio (`evidencia/videos/`), muéstralos como sugerencia amigable sin asumir arbitrariamente ninguno.
4. Si el archivo existe, procede inmediatamente con el flujo de análisis.

---

## 1. Detección y Análisis de Acciones Repetidas en el Chat

El mayor obstáculo en la depuración robótica son los **bucles de prueba y error en el chat**, donde usuario y asistente repiten acciones sin llegar a la causa raíz.

Antes de mirar el código, analiza el historial de la conversación (o `transcript.jsonl`):

1. **Identificar la acción o síntoma repetido**:
   - ¿Qué comando se ha ejecutado una y otra vez? (ej. `calibrate`, `step(x, y)`, joystick libre).
   - ¿Qué frase o síntoma describe el usuario repetidamente? (ej. *"se desvía a la derecha"*, *"se detiene en el segundo giro"*, *"una rueda no tiene fuerza"*, *"falla en CAL_B"*).
2. **Catalogar hipótesis previamente quemadas**:
   - ¿Qué explicaciones ya se dieron en el chat? (ej. *"le falta PWM"*, *"el timeout del watchdog es corto"*, *"el sensor MPU está invertido"*).
   - ¿Qué modificaciones ya se intentaron y **no funcionaron**?
   - **Prohibición**: NUNCA vuelvas a proponer un ajuste que ya fracasó en turnos anteriores sin una refutación matemática o evidencia visual nueva.
3. **Identificar la brecha de percepción (Chat vs Realidad)**:
   - Frecuentemente el relato del usuario en el chat difiere de la física real:
     - Chat dice: *"El motor izquierdo no tiene fuerza y se apaga"*. $\rightarrow$ Video puede mostrar: *La rueda gira a toda velocidad patinando sobre losa pulida (pérdida de tracción, no falta de torque).*
     - Chat dice: *"Gira hacia el lado incorrecto"*. $\rightarrow$ Video puede mostrar: *Giro inicial correcto, pero sobrepaso inercial masivo y rebote sin frenado regenerativo.*
   - Registra explícitamente esta discrepancia. Para detalles metodológicos, consulta [references/patrones_acciones_chat.md](references/patrones_acciones_chat.md).

---

## 2. Inspección Multimodal del Video (Verdad Física)

El modelo cuenta con capacidad multimodal nativa para procesar archivos de video e imágenes directamente.

### Vía Primaria: Inspección Multimodal Directa con `view_file`
1. Llama a `view_file` especificando la ruta absoluta del archivo de video:
   ```json
   {
     "AbsolutePath": "c:\\Users\\IK\\Documents\\Codex\\carroTest3-For-Esp32-s3-wroom\\evidencia\\videos\\archivo.mp4",
     "toolAction": "Viewing video file",
     "toolSummary": "Multimodal video analysis"
   }
   ```
2. Analiza visualmente la secuencia física completa y elabora una **cronología visual ($t_{video}$)**:
   - **$t_{0}$ (Inicio)**: Estado de reposo, posición inicial del chasis y LEDs visibles.
   - **$t_{mov}$ (Arranque)**: ¿Qué ruedas giran primero? (FL, FR, BL, BR). ¿Hay cabeceo del chasis?
   - **$t_{maniobra}$ (Desarrollo)**:
     - Sentido de rotación de cada una de las 4 ruedas (hacia adelante, atrás o detenida).
     - Comportamiento del chasis: ¿Rotación pura sobre el centro, pivote asimétrico sobre una rueda, traslación parásita (*drift*) o bloqueo?
     - Interacción con la superficie: ¿Patinaje visible sobre azulejo/alfombra? ¿Resistencia o enganche de cables?
   - **$t_{fallo}$ (Momento del fallo)**:
     - ¿El robot se frena en seco, tiembla, oscila o sigue empujando sin girar?
     - ¿Se apagan los LEDs del ESP32 o parpadean (reinicio por caída de tensión / brownout)?
   - **$t_{fin}$ (Estado terminal)**: Posición final, ángulo alcanzado respecto a la referencia de inicio.

### Vía Secundaria / Forense: Herramienta CLI de Fotogramas
Si se requiere documentar fotogramas clave, medir distancias visuales pixel a pixel o generar hojas de contacto, ejecuta el script auxiliar con el entorno del proyecto:
```powershell
& 'desktop_app\.test-venv\Scripts\python.exe' .agents\skills\analisis-video-evidencia\scripts\inspeccionar_video.py --video "<ruta_completa_del_video>" --contact-sheet --frames 24
```
Esto genera muestras en `evidencia/fotogramas/<nombre_video>/` que pueden enlazarse en los informes o inspeccionarse individualmente. Consulta [references/guia_extraccion_video.md](references/guia_extraccion_video.md).

---

## 3. Triangulación de Evidencia (Video $\leftrightarrow$ Telemetría ADB $\leftrightarrow$ Firmware)

Nunca diagnostiques basándote sólo en el video ni sólo en el código. Cruza los tres pilares:

```
                  ┌────────────────────────┐
                  │  VIDEO (Física Real)   │
                  │ Ruedas, Yaw visual,    │
                  │ Patinaje, LEDs, Drift  │
                  └───────────┬────────────┘
                              │
                              ▼
┌─────────────────────────────┴─────────────────────────────┐
│          CORRELACIÓN TEMPORAL (t_video ≈ t_telemetría)     │
└─────────────┬─────────────────────────────┬───────────────┘
              │                             │
              ▼                             ▼
┌───────────────────────────┐ ┌─────────────────────────────┐
│  TELEMETRÍA ADB / SQLITE  │ │     FIRMWARE ESP32-S3       │
│  consultar_db.py:         │ │  Core 1 (100 Hz):           │
│  • ticks PCNT por rueda   │ │  • Dogma MPU6050 yaw        │
│  • yaw integrado, gyro_z  │ │  • PWM_FORWARD_POLARITY     │
│  • PWM L/R aplicado       │ │  • Deadband 250 ms          │
│  • Evento / motivo guardia│ │  • Watchdog 2.5s / 450ms    │
└───────────────────────────┘ └─────────────────────────────┘
```

### Consultar la base SQLite extraída por ADB
Si se cuenta con `tmp_db/robot.sqlite3` o la base de la prueba:
```powershell
& 'desktop_app\.test-venv\Scripts\python.exe' consultar_db.py --commands 10
& 'desktop_app\.test-venv\Scripts\python.exe' consultar_db.py --events 15
& 'desktop_app\.test-venv\Scripts\python.exe' consultar_db.py --telemetry 20
& 'desktop_app\.test-venv\Scripts\python.exe' consultar_db.py --search "calib"
```

### Tabla de Sincronización Temporal
Construye una matriz comparativa entre lo que se ve en el video y lo que registró el sistema:

| Marca $t_{video}$ | Acción física visible en video | Estado Firmware / Cmd | Ticks Encoders (FL/FR/BL/BR) | IMU Yaw / `gyro_z` | PWM L / R | Veredicto |
|---|---|---|---|---|---|---|
| `00:03.2` | Arranque rampa de giro | `CAL_A` | Subiendo en ambos lados | `gyro_z > 0.12` | 140 $\rightarrow$ 170 | Coherente |
| `00:05.1` | Ruedas izquierdas giran, derechas trabadas | `CAL_A` | L sube, R estancado | `yaw` no incrementa | 220 / 220 | Fallo mecánico o eléctrico en tren derecho |
| `00:07.5` | Carro se detiene en seco | `FAILED (cal_stall_right)` | Inmóvil | `0.0` | 0 / 0 | Guardia disparada correctamente por firmware |

Para el mapeo completo de firmas y síntomas, consulta [references/catalogo_fallos_fisicos.md](references/catalogo_fallos_fisicos.md).

---

## 4. Matriz de Falsificación de Hipótesis

Para romper definitivamente el ciclo de chat repetido, evalúa explícitamente cada hipótesis previa y la nueva contra la evidencia observable:

1. **Hipótesis A**: *¿Es falta de PWM / torque?*
   - **Falsación**: Si en el video las ruedas giran y deslizan sobre el suelo, la hipótesis es **FALSA** (el motor tiene torque suficiente, el problema es fricción o tracción).
2. **Hipótesis B**: *¿Es fallo de polaridad o cableado invertido?*
   - **Falsación**: Si las ruedas delanteras y traseras de un mismo lado giran en sentidos opuestos, la polaridad de bornera o GPIO está cruzada respecto a la simetría 4WD descrita en `AGENTS.md`.
3. **Hipótesis C**: *¿Es fallo del sensor IMU MPU6050 vs Encoders?*
   - **Falsación**: Si en el video el chasis gira físicamente en sentido horario (+Y hacia +X), pero en telemetría el yaw decrece negativamente, hay discrepancia de signo MPU (`cal_yaw_sign_mismatch`).
4. **Hipótesis D**: *¿Es reinicio por caída de tensión (brownout)?*
   - **Falsación**: Si en el video se aprecia un apagón momentáneo de los LEDs o en telemetría aparece `reset_reason=power_on` con pérdida de calibración al invertir sentido, se violó el tiempo muerto de 250 ms o falta desacoplo en VMOT.

---

## 5. Plan de Corrección y Cierre Verificable

Una vez identificada la causa raíz con certeza empírica:

1. **Respetar las Reglas Canónicas de `AGENTS.md`**:
   - PWM máximo de avance: 242/255; giro autónomo: 247/255.
   - Tiempo muerto universal de inversión: 250 ms obligatorio en `Motores.cpp`.
   - Core 1 exclusivo a 100 Hz; no agregar colas ni tareas intermedias.
   - Dogma de calibración inmutable (MPU como autoridad angular única, retorno estricto a 0°).
   - Simetría 4WD: motores delanteros orientados hacia atrás, traseros hacia adelante; compensación por hardware y `PWM_FORWARD_POLARITY = -1`.
2. **Implementar el cambio mínimo transversal**:
   - Modifica únicamente el archivo responsable (`src/Motores.cpp`, `src/Cinematica.cpp`, `src/Estado.cpp`, etc.).
   - No aumentes valores de PWM o watchdogs a ciegas si el problema es cinemático o físico.
3. **Informe al Usuario**:
   - Presenta la causa demostrada citando marcas de tiempo exactas del video ($t_{video}$) y correlación con la base de datos o logs.
   - Explica por qué las acciones o hipótesis repetidas anteriormente en el chat eran incorrectas.
   - Detalla la corrección implementada y los pasos para verificarla físicamente (con ruedas elevadas primero, conforme a la prueba de corriente de `AGENTS.md`).

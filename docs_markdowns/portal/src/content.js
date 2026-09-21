export const findings = [
  { id: "A-01", severity: "Alta", title: "Reconexión automática finita", detail: "El gateway se detiene después de cinco intentos y exige una acción manual aunque el AP vuelva a estar disponible.", source: "desktop_app/robot_app/gateway.py:35,164-190" },
  { id: "A-02", severity: "Alta", title: "Sesión volátil ante reboot del ESP32", detail: "El firmware pierde sesión, last_seq, calibración y pose al reiniciar; Python no puede conocer el avance parcial.", source: "src/Red.cpp:58-72 · src/Estado.cpp" },
  { id: "A-03", severity: "Alta", title: "seq=1 reinicia la ventana idempotente", detail: "Cada misión vuelve a secuencia 1 dentro de la misma sesión y el UUID Python no viaja en step/turn_to.", source: "services.py:_activate_mission · Red.cpp:98-104" },
  { id: "A-04", severity: "Alta", title: "Evento terminal descartable", detail: "encolarEvento ignora el resultado de xQueueSend después de limpiar el comando activo.", source: "src/Eventos.cpp:6-30" },
  { id: "A-05", severity: "Alta", title: "Reglas y constantes contradictorias", detail: "Stalls, tolerancia, rampa y aproximación fina escritos no coinciden con Config.h y Cinematica.cpp.", source: "AGENTS.md · include/Config.h" },
  { id: "A-06", severity: "Resuelto", title: "Encoders y filtrado PCNT verificados", detail: "Resolución 40 PPR (escala 2:1), filtro hardware PCNT 1023 ciclos, aislamiento en giro y corroboración bilateral validados en taller.", source: "src/Encoders.cpp · SQLite Sesión 15622" },
  { id: "A-07", severity: "Resuelto", title: "Mision.cpp retirado del firmware activo", detail: "El módulo histórico de misión completa vive fuera del árbol activo; Python conserva la propiedad de misión y el ESP32 ejecuta pasos atómicos.", source: "archive/legacy/firmware_mission/Mision.cpp · src/main.cpp" },
  { id: "A-08", severity: "Media", title: "Sesiones SQLite fragmentadas", detail: "Cada estado BACKOFF cierra la sesión histórica aunque la sesión de protocolo siga viva.", source: "services.py:_on_connection_state" },
  { id: "A-09", severity: "Media", title: "Autopruebas de firmware no ejecutadas", detail: "Las validaciones puras del interlock y RTOS existen, pero no forman parte de una puerta automática.", source: "src/Motores.cpp · src/DiagnosticoRTOS.cpp" },
  { id: "A-10", severity: "Media", title: "Dos frontends con cobertura desigual", detail: "La HMI activa sigue siendo HTML/JS monolítico; la suite TypeScript cubre sólo una fachada mínima.", source: "hmi/index.html · frontend/src" },
  { id: "A-11", severity: "Baja", title: "Errores de preparación demasiado genéricos", detail: "Transporte, handshake, reboot, telemetría obsoleta y calibración perdida pueden parecer el mismo problema.", source: "services.py:_require_ready_robot" },
  { id: "A-12", severity: "Pendiente", title: "Seguridad eléctrica no demostrada", detail: "Los videos no sustituyen las mediciones de corriente exigidas antes de operar en suelo.", source: "docs/validacion_sistema_final.md" },
  { id: "A-13", severity: "Alta", title: "SQLite vulnerable al fallo WAL-reset 2026", detail: "El runtime de pruebas usa SQLite 3.43.1 y el APK arm64 3.50.4; ambas preceden las correcciones oficiales 3.51.3/3.50.7.", source: "database.py · SQLite WAL §11" },
];

export const diagrams = {
  context: `flowchart LR
    O["Operador"] --> H["HMI local"]
    H <-->|"HTTP + SSE"| P["Backend Python"]
    P --> D[("SQLite")]
    P <-->|"WebSocket v3"| E["ESP32-S3"]
    E --> M["DRV8833 + motores"]
    S["Encoders PCNT + MPU6050"] --> E`,
  runtime: `flowchart TB
    subgraph CORE0["Core 0 · comunicaciones"]
      WS["Task_Web"] --> RX["parsearMensaje"]
      EV["drenarEventos"] --> WS
      TM["telemetría 10 Hz"] --> WS
    end
    CQ[["colaComandos · 4"]]
    EQ[["colaEventos · 8"]]
    subgraph CORE1["Core 1 · súper-ciclo 100 Hz"]
      CMD["procesarComandos"] --> CINE["Cinemática"]
      SNS["PCNT + MPU"] --> POSE["PoseEstimator"]
      SNS --> SAFE["Seguridad"]
      POSE --> CINE
      CINE --> PWM["Motores + interlock"]
      SAFE --> PWM
    end
    RX --> CQ --> CMD
    CINE --> EQ --> EV`,
  mission: `sequenceDiagram
    autonumber
    actor U as Operador
    participant H as HMI
    participant P as RobotService
    participant G as Gateway
    participant E as ESP32
    participant D as SQLite
    U->>H: Ejecutar ruta
    H->>P: POST /missions
    P->>P: validar y subdividir ≤200 cm
    P->>D: persistir active_mission
    loop un tramo a la vez
      P->>G: step(heading,cm,seq)
      G->>E: JSON v3
      E-->>P: accepted/progress
      E-->>P: completed(step_ok)
      P->>D: completar y avanzar
    end
    P->>D: last_completed_route=available`,
  calibration: `sequenceDiagram
    autonumber
    actor U as Operador
    participant H as HMI
    participant P as RobotService
    participant E as ESP32
    participant S as MPU + PCNT
    participant M as Motores
    U->>H: Recalibrar
    H->>P: POST /commands calibrate
    P->>E: calibrate(seq)
    E-->>P: accepted
    Note over E,S: Fase 1: CAL_CUENTA_REGRESIVA 5.0 s (sesgo MPU)
    loop Búsqueda A (torque positivo)
      S-->>E: gyro_z >= 0.12 rad/s + ticks bilaterales
      E->>M: rampa PWM 140 a 247
      E-->>P: progress(cal/cal_a)
    end
    Note over E,M: Fase 2: CAL_VALIDAR_25 (giro +25 deg)
    E->>M: pivote fino +25 deg (asentamiento 600 ms)
    Note over E,M: Fase 3: CAL_PAUSA (reposo 2.5 s) y CAL_B
    loop Búsqueda B (torque opuesto)
      S-->>E: gyro_z opuesto + ticks bilaterales
      E->>M: rampa opuesta protegida
      E-->>P: progress(cal/cal_b)
    end
    Note over E,M: Fase 4: CAL_PAUSA_RETORNO (reposo 2.5 s)
    Note over E,M: Fase 5: CAL_RETORNO (regreso a yaw original)
    E->>M: pivote hacia yaw original
    alt retorno exitoso
      E-->>P: completed(cal_ok)
      Note over E: PoseGlobal=(0,0), yaw=0
      P-->>H: listo / rutas habilitadas
    else atasco o pérdida socket
      E->>M: PWM = 0
      E-->>P: fault(cal_stall_left/right/cal_connection_lost)
      P-->>H: fallo / parada segura
    end`,
  returnHome: `sequenceDiagram
    autonumber
    actor U as Operador
    participant H as HMI
    participant P as RobotService
    participant D as SQLite
    participant E as ESP32
    U->>H: Regresar por Ockham
    H->>P: POST /missions/return-home
    P->>D: leer last_completed_route=available
    P->>P: invertir vectores en orden inverso
    P->>D: return_state=in_progress
    loop cada vector inverso
      P->>E: step(heading,cm,seq)
      E-->>P: completed(step_ok)
    end
    P->>E: turn_to(heading=0,seq)
    E-->>P: completed(turn_ok)
    P->>D: return_state=completed
    P-->>H: misión terminada`,
  reconnect: `sequenceDiagram
    autonumber
    participant P as Python
    participant G as Gateway
    participant E as ESP32
    E--xG: desconexión
    G->>P: BACKOFF
    loop máximo 5 actualmente
      G->>E: reconectar
    end
    E-->>G: socket + hello_ack
    alt mismo estado y last_seq confirma
      P->>P: avanzar sin repetir
    else listo y paso no confirmado
      P->>E: reenviar misma identidad
    else reboot o estado incompatible
      P->>P: bloquear misión
      P->>E: stop
    end`,
  robotReboot: `sequenceDiagram
    autonumber
    participant P as Python
    participant E as ESP32
    participant D as SQLite
    E--xP: reboot / socket perdido
    Note over E: RAM vuelve a session="", last_seq=0, cal=false, pose=0
    P->>E: hello(session conservada)
    E-->>P: hello_ack(state=desarmado,last_seq=0)
    alt había paso activo
      P->>D: stage=blocked
      P->>D: error=robot_restarted_mid_step
      P->>E: stop
      Note over P,E: No repetir distancia parcial desconocida
    else no había paso activo
      P-->>P: exigir calibración y pose verificable
    end`,
  pythonRestart: `sequenceDiagram
    autonumber
    participant P as Nuevo proceso Python
    participant D as SQLite
    participant E as ESP32
    P->>D: cargar active_mission
    P->>D: no terminal → abandoned + blocked
    P->>D: queued/sent/acknowledged → failed
    P->>D: cerrar sesiones huérfanas
    P->>P: crear controller_session nueva
    P->>E: hello(session nueva)
    E-->>P: hello_ack
    P->>E: stop de reconciliación
    E-->>P: completed(stop_ok)
    P->>D: app_restart_stop_confirmed
    Note over P,E: La misión anterior nunca se reenvía`,
  close: `sequenceDiagram
    autonumber
    actor U as Operador
    participant H as HMI
    participant P as RobotService
    participant E as ESP32
    participant D as SQLite
    U->>H: Cerrar aplicación
    H->>P: POST /app/close
    P->>P: bloquear comandos nuevos
    opt movimiento posible
      P->>E: stop(seq)
      E-->>P: completed(stop_ok)
    end
    P->>D: abandonar misión y cerrar sesión
    P-->>H: safe_to_close=true`,
  states: `stateDiagram-v2
    [*] --> DESARMADO
    DESARMADO --> CALIBRANDO: calibrate
    CALIBRANDO --> LISTO: cal_ok
    CALIBRANDO --> FALLO
    LISTO --> EJECUTANDO: step / turn_to
    EJECUTANDO --> LISTO: step_ok / turn_ok
    EJECUTANDO --> FALLO
    LISTO --> ESTOP: estop
    EJECUTANDO --> ESTOP: estop
    FALLO --> DESARMADO: clear_fault sin calibración
    FALLO --> LISTO: clear_fault calibrado`,
  database: `erDiagram
    SETTINGS {
      TEXT key PK
      TEXT value_json
      TEXT updated_at
    }
    SESSIONS {
      INTEGER id PK
      TEXT started_at
      TEXT ended_at
      TEXT firmware_version
      TEXT robot_id
      TEXT protocol
      TEXT disconnect_reason
    }
    COMMANDS {
      TEXT id PK
      INTEGER session_id FK
      TEXT command_type
      TEXT payload_json
      TEXT status
      TEXT error
    }
    EVENTS {
      INTEGER id PK
      INTEGER session_id FK
      TEXT kind
      TEXT severity
      TEXT payload_json
    }
    TELEMETRY {
      INTEGER id PK
      INTEGER session_id FK
      INTEGER seq
      REAL received_at
      TEXT state
      REAL yaw_deg
      INTEGER pwm_l
      INTEGER pwm_r
    }
    SESSIONS ||--o{ COMMANDS : registra
    SESSIONS ||--o{ EVENTS : agrupa
    SESSIONS ||--o{ TELEMETRY : contiene`,
  android: `flowchart TB
    A["MainActivity"] -->|"inicia"| S["RobotBackendService"]
    S --> C["Chaquopy / mobile_entry.py"]
    C --> W["Waitress 127.0.0.1"]
    W --> F["Flask + HMI canónica"]
    A --> V["WebView local"]
    V -->|"HTTP/SSE + X-App-Token"| F
    F -->|"propietario único WS"| E["ESP32 (IP del robot)"]
    N["Notificación Detener"] -->|"mismo cierre seguro"| F
    F -->|"safe_to_close"| B["RobotHost bridge restringido"]
    B --> S
    B --> R["finishAndRemoveTask"]`,
  safety: `flowchart TD
    C["Comando de movimiento"] --> V["Validación Python"]
    V --> Q["Cola de comandos"]
    Q --> S["Máquina de estados ESP32"]
    S --> I["Interlock 250 ms por lado"]
    I --> L["Límites PWM"]
    L --> M["DRV8833 + motores"]
    E["E-STOP"] --> Z["PWM cero inmediato"]
    W["Watchdogs / IMU / PCNT"] --> Z
    F["Fallo de control"] --> Z
    Z --> M
    P["Fusible + capacitores + fuente limitada"] -.-> M
    N["Prueba física de corriente"] -.-> M`,
  validation: `flowchart LR
    S["Fuente"] --> FW["PlatformIO firmware"]
    S --> FM["staging firmware modular"]
    S --> PY["unitarias Python"]
    S --> TS["frontend Vite"]
    S --> HM["validador HMI"]
    S --> APK["Gradle APK"]
    S --> DOC["catálogo + portal"]
    FW --> G{"Puerta integral"}
    FM --> G
    PY --> G
    TS --> G
    HM --> G
    APK --> G
    DOC --> G
    G --> PH["prueba física supervisada"]`,
};

export const manuals = [
  {
    title: "Preparación segura",
    steps: [
      "Elevar las ruedas y mantener VMOT apagado durante boot o carga de firmware.",
      "Confirmar capacitores, fusible o polyfuse y fuente limitada a 0.5 A.",
      "Encender el ESP32 y comprobar PWM izquierdo/derecho en cero.",
      "Conectar únicamente Windows o Android al SSID del robot; nunca ambos.",
      "Abrir la HMI y verificar protocolo robot-s3-steps-v3 y telemetría fresca.",
    ],
  },
  {
    title: "Calibración",
    steps: [
      "Mantener libre el radio de giro y confirmar que las ruedas no toquen obstáculos.",
      "Pulsar Recalibrar una sola vez y observar la cuenta regresiva.",
      "Comprobar giro de validación, pausa y retorno independiente a yaw 0.",
      "Aceptar únicamente completed/cal_ok y estado listo.",
      "Ante cal_stall_left/right, detener y revisar ticks por encoder antes de reintentar.",
    ],
  },
  {
    title: "Ruta y retorno Ockham",
    steps: [
      "Crear puntos en milímetros; Python los convierte en tramos ortogonales de máximo 200 cm.",
      "Revisar la previsualización y confirmar que no exista otra misión activa.",
      "Ejecutar y observar un único comando step activo por vez.",
      "Esperar stage=completed antes de solicitar Regresar por Ockham.",
      "El retorno invierte vectores y termina con turn_to(0°); no repetirlo si ya fue consumido.",
    ],
  },
  {
    title: "Desconexión y recuperación",
    steps: [
      "No mover físicamente el robot mientras la telemetría esté ausente.",
      "Si vuelve el mismo estado/last_seq, dejar que Python reconcilie el paso atómico.",
      "Si cal=false, last_seq retrocede o la pose vuelve a cero, tratarlo como reboot del ESP32.",
      "Un reboot durante un paso exige bloquear la misión; no repetir automáticamente una distancia desconocida.",
      "La versión actual requiere pulsar Conectar después de agotar cinco intentos.",
    ],
  },
  {
    title: "Cierre seguro",
    steps: [
      "Usar Cerrar aplicación, no finalizar el proceso desde el selector de tareas.",
      "La HMI bloquea comandos nuevos y envía stop cuando existe movimiento posible.",
      "Cerrar sólo después de safe_to_close=true y stop_confirmed=true cuando se requería parada.",
      "Si no llega stop_ok, mantener la aplicación abierta o confirmar conscientemente el cierre forzado.",
      "Después del cierre, comprobar sesión SQLite terminada y PWM físico cero.",
    ],
  },
];

const formatTime = (seconds) => `${String(Math.floor(seconds / 60)).padStart(2, "0")}:${String(Math.floor(seconds % 60)).padStart(2, "0")}`;
const makeFrames = (folder, duration, phases) => Array.from({ length: 24 }, (_, index) => {
  const seconds = (index + 0.5) * duration / 24;
  const phase = phases[Math.min(phases.length - 1, Math.floor(index * phases.length / 24))];
  return [formatTime(seconds), `fotogramas/${folder}/fotograma_${String(index + 1).padStart(2, "0")}.jpg`, `${phase} · muestra ${String(index + 1).padStart(2, "0")}`];
});

export const glossary = [
  ["PWM", "Ciclo útil que regula la energía media enviada al puente H."],
  ["PCNT", "Periférico de hardware que cuenta flancos de los encoders."],
  ["IMU", "Sensor inercial; el MPU6050 aporta acelerómetro y giroscopio."],
  ["Yaw", "Orientación horizontal estimada alrededor del eje vertical."],
  ["Watchdog", "Vigilancia temporal que detiene ante falta de progreso."],
  ["Interlock", "Intervalo obligatorio a PWM cero antes de invertir polaridad."],
  ["Sesión", "Identidad corta del proceso controlador compartida con el ESP32."],
  ["seq", "Orden e identidad idempotente de un comando dentro de la sesión."],
  ["WAL", "Bitácora anticipada de SQLite que requiere checkpoint y manejo coherente."],
  ["CC", "Complejidad ciclomática léxica usada para priorizar revisión."],
];

export const evidence = [
  {
    title: "Servidor y robot",
    src: "videos/servidor_y_robot_ruta_ortogonal.mp4",
    duration: "03:43",
    hash: "239F02EE…1626A",
    thumbnail: "fotogramas/miniatura_server_y_robot.webp",
    frames: makeFrames("server_y_robot", 223.866667, ["HMI y servidor", "Robot en área", "Preparación de ruta", "Ejecución", "Retorno Ockham", "Estado final"]),
  },
  {
    title: "Ruta Y+100, Y+50, X+190, X−190",
    src: "videos/ruta_ortogonal_y100_y50_x190_xmenos190.mp4",
    duration: "01:28",
    hash: "D10C633E…A30D3",
    thumbnail: "fotogramas/miniatura_ruta_ortogonal.webp",
    frames: makeFrames("ruta_ortogonal", 88.533333, ["Inicio", "Tramo Y+100", "Tramo Y+50", "Giro y X+190", "X−190", "Corte final"]),
  },
];

export const archifyMaps = [
  {
    id: "sistema_contexto",
    title: "Contexto del Sistema",
    file: "archify/sistema_contexto.html",
    type: "Architecture",
    description: "Límites arquitectónicos entre Operador, HMI/Backend, Android, ESP32-S3 y Planta Motriz.",
  },
  {
    id: "firmware_superciclo",
    title: "Súper-Ciclo 100 Hz Firmware",
    file: "archify/firmware_superciclo.html",
    type: "Architecture",
    description: "Pipeline síncrono Core 1 (PCNT/MPU, Pose, Seguridad, Cinemática, PWM) y Core 0 (Task_Web).",
  },
  {
    id: "calibracion_dogma",
    title: "Dogma Canónico de Calibración",
    file: "archify/calibracion_dogma.html",
    type: "Sequence",
    description: "Secuencia inmutable en 5 fases con cuenta regresiva 5.0 s, búsqueda A, validación +25°, búsqueda B y retorno a yaw 0.",
  },
  {
    id: "mision_navegacion",
    title: "Misión y Navegación Ortogonal",
    file: "archify/mision_navegacion.html",
    type: "Workflow",
    description: "Descomposición ortogonal de tramos ≤200 cm, lazo cerrado heading PID y retorno Ockham seguro.",
  },
  {
    id: "fsm_ciclo_vida",
    title: "Ciclo de Vida de Comandos (FSM)",
    file: "archify/fsm_ciclo_vida.html",
    type: "Lifecycle",
    description: "Transiciones de estado de comandos robot-s3-steps-v3 con ventanas de idempotencia seq/last_seq.",
  },
  {
    id: "seguridad_electrica",
    title: "Seguridad Eléctrica DRV8833",
    file: "archify/seguridad_electrica.html",
    type: "Architecture",
    description: "Techos de PWM (242/247), ráfagas kickstart (80 ms), zonas muertas y tiempo muerto universal (250 ms).",
  },
  {
    id: "sqlite_datos",
    title: "Esquema SQLite y Persistencia",
    file: "archify/sqlite_datos.html",
    type: "Architecture",
    description: "Base relacional de sesiones, comandos, eventos, telemetría y perfiles de fricción de superficies.",
  },
  {
    id: "android_arquitectura",
    title: "Arquitectura Android y Chaquopy",
    file: "archify/android_arquitectura.html",
    type: "Architecture",
    description: "Contenedor nativo APK con Foreground Service, puente Chaquopy Python, servidor Waitress y WebView HMI.",
  },
  {
    id: "validacion_puertas",
    title: "Puertas de Calidad y Validación",
    file: "archify/validacion_puertas.html",
    type: "Workflow",
    description: "Pipeline de validación multifacética: PlatformIO, Pytest, Vite HMI, Chaquopy APK y Catálogo documental.",
  },
];

export const archifyTabMap = {
  mission: { file: "archify/mision_navegacion.html", title: "Misión y Navegación Ortogonal (Workflow)" },
  calibration: { file: "archify/calibracion_dogma.html", title: "Dogma Canónico de Calibración (Sequence)" },
  returnHome: { file: "archify/mision_navegacion.html", title: "Retorno Ockham Seguro (Workflow)" },
  reconnect: { file: "archify/fsm_ciclo_vida.html", title: "Recuperación de Red y FSM (Lifecycle)" },
  robotReboot: { file: "archify/fsm_ciclo_vida.html", title: "Reinicio ESP32 y Reconciliación (Lifecycle)" },
  pythonRestart: { file: "archify/fsm_ciclo_vida.html", title: "Reinicio Python y Sesión (Lifecycle)" },
  close: { file: "archify/fsm_ciclo_vida.html", title: "Cierre Seguro y Parada (Lifecycle)" },
  states: { file: "archify/fsm_ciclo_vida.html", title: "Ciclo de Vida y Estados del Robot (Lifecycle)" },
  database: { file: "archify/sqlite_datos.html", title: "Esquema SQLite y Persistencia (Architecture)" },
  android: { file: "archify/android_arquitectura.html", title: "Arquitectura Android y Puente Chaquopy (Architecture)" },
  safety: { file: "archify/seguridad_electrica.html", title: "Protección Eléctrica DRV8833 (Architecture)" },
  validation: { file: "archify/validacion_puertas.html", title: "Puertas de Calidad y Validación (Workflow)" },
};

export const calibrationDetails = {
  mpuConfig: [
    { param: "Interfaz I2C", value: "GPIO 8 (SDA) / GPIO 9 (SCL)", note: "Bus I2C síncrono en Core 1 a 400 kHz." },
    { param: "Escala Acelerómetro", value: "±8 g", note: "Rango dinámico para detección de impactos y sacudidas." },
    { param: "Escala Giroscopio", value: "±500 °/s", note: "Sensibilidad de 65.5 LSB/(°/s) para giros rápidos y micro-pulsos." },
    { param: "Filtro Digital Pasa-Bajos (DLPF)", value: "21 Hz", note: "Atenúa ruidos mecánicos de reductores TT y motorreductores." },
    { param: "Tara y Offset Z", value: "256 muestras en reposo", note: "Calcula el sesgo estático (bias) con el chasis inmóvil." },
    { param: "Polaridad Angular", value: "-1.0 (invertida)", note: "Compensa el montaje físico invertido del chip MPU respecto a la convención canónica (+Y = 0°, dextrógiro)." },
    { param: "Filtro Promedio Móvil", value: "Ventana de 8 muestras", note: "Suavizado digital síncrono ejecutado en cada muestra del súper-ciclo a 100 Hz." },
    { param: "Zona Muerta (Deadband)", value: "|gyro| < 0.005 rad/s → 0", note: "Elimina integración espuria y deriva angular parásita en reposo." },
  ],
  phases: [
    {
      id: "fase1",
      name: "1. CAL_CUENTA_REGRESIVA (5.0 s)",
      desc: "Reposo absoluto en suelo. Permite estabilización térmica, elimina transitorios de encendido y calcula la tara del giróscopo MPU6050 (offset Z) con 256 lecturas.",
      tag: "Estabilización",
    },
    {
      id: "fase2",
      name: "2. CAL_A: Búsqueda Torque Polaridad Positiva",
      desc: "Rampa adaptativa de PWM desde 140 hasta 247/255 (2/255 cada 20 ms). Requiere ticks bilaterales en encoders PCNT y confirmación sostenida de gyro_z >= 0.12 rad/s durante 100 ms. Si el giro inicial resulta negativo, invierte candidatoCal con pausa de 750 ms hasta validar sentido dextrógiro.",
      tag: "Torque Positivo",
    },
    {
      id: "fase3",
      name: "3. CAL_VALIDAR_25: Giro y Asentamiento",
      desc: "Pivote fino hasta alcanzar yawInicio + 25° (±2.5°) con reposo de asentamiento de 600 ms para registrar la respuesta inercial y fricción estática del terreno.",
      tag: "Validación +25°",
    },
    {
      id: "fase4",
      name: "4. CAL_PAUSA & CAL_B: Torque Polaridad Opuesta",
      desc: "Reposo de 2.5 s y búsqueda de par mínimo en sentido opuesto. Valida movimiento bilateral y comprueba que candidatoGiroPos != candidatoGiroNeg.",
      tag: "Torque Negativo",
    },
    {
      id: "fase5",
      name: "5. CAL_PAUSA_RETORNO & CAL_RETORNO: Vuelta a Cero",
      desc: "Reposo de 2.5 s y retorno cerrado hacia el yawInicioCalDeg original. Al estabilizarse, resetea la odometría de PoseGlobal (X=0, Y=0) y el yaw del IMU, transicionando a estado LISTO.",
      tag: "Retorno y Cero",
    },
  ],
  subsystems: [
    {
      title: "Encoders y Periférico PCNT",
      icon: "activity",
      desc: "Resolución efectiva de 40 PPR (reducción 2:1 por software sobre disco de 20 ranuras). El ESP32 utiliza el periférico de hardware PCNT con filtro de desparasitado configurado en 1023 ciclos de reloj APB (~12.8 µs), descartando rebotes y ruido electromagnético de los motores.",
    },
    {
      title: "Sensor de Actividad y Monitor de Batería",
      icon: "circuit-board",
      desc: "Supervisión continua de la fuente de alimentación lógica y de potencia VMOT. Previene caídas bruscas de tensión (brownout) durante picos de par y bloquea comandos de avance si la tensión de celda cae por debajo de los umbrales seguros.",
    },
    {
      title: "Detección de Atascos (Stalls) y Watchdogs",
      icon: "alert-triangle",
      desc: "En giros autónomos, el watchdog de 2.5 s por lado se arma únicamente tras alcanzar el par calibrado. En traslación recta, el corte actúa a 450 ms ante falta de ticks o deriva angular. En caso de atasco, se produce parada segura inmediata (PWM 0/0) y transición al estado FALLO con error cal_stall_left/right.",
    },
    {
      title: "Rearme de Fallos (clear_fault)",
      icon: "shield-check",
      desc: "El comando clear_fault permite restablecer la máquina de estados. Si el robot conserva su calibración en memoria, retorna directamente a LISTO; si la calibración fue invalidada o ocurrió un reinicio, pasa a DESARMADO exigiendo una nueva calibración.",
    },
    {
      title: "Persistencia SQLite e Inyección de Par",
      icon: "database",
      desc: "La tabla calibration_surfaces en robot.sqlite3 almacena los pares calibrados por tipo de superficie (madera, baldosa, alfombra). Permite reinyectar el par óptimo directamente al firmware vía JSON v3 sin necesidad de forzar una nueva calibración física en cada arranque.",
    },
  ],
};


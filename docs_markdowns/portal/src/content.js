export const findings = [
  { id: "A-01", severity: "Alta", title: "Reconexión automática finita", detail: "El gateway se detiene después de cinco intentos y exige una acción manual aunque el AP vuelva a estar disponible.", source: "desktop_app/robot_app/gateway.py:35,164-190" },
  { id: "A-02", severity: "Alta", title: "Sesión volátil ante reboot del ESP32", detail: "El firmware pierde sesión, last_seq, calibración y pose al reiniciar; Python no puede conocer el avance parcial.", source: "src/Red.cpp:58-72 · src/Estado.cpp" },
  { id: "A-03", severity: "Alta", title: "seq=1 reinicia la ventana idempotente", detail: "Cada misión vuelve a secuencia 1 dentro de la misma sesión y el UUID Python no viaja en step/turn_to.", source: "services.py:_activate_mission · Red.cpp:98-104" },
  { id: "A-04", severity: "Alta", title: "Evento terminal descartable", detail: "encolarEvento ignora el resultado de xQueueSend después de limpiar el comando activo.", source: "src/Eventos.cpp:6-30" },
  { id: "A-05", severity: "Alta", title: "Reglas y constantes contradictorias", detail: "Stalls, tolerancia, rampa y aproximación fina escritos no coinciden con Config.h y Cinematica.cpp.", source: "AGENTS.md · include/Config.h" },
  { id: "A-06", severity: "Alta", title: "Calibración sin ticks", detail: "La prueba reciente mostró giro por IMU, cuatro encoders en cero y cal_stall_left.", source: "SQLite de la prueba física" },
  { id: "A-07", severity: "Media", title: "Mision.cpp desconectado", detail: "El módulo de misión completa compila pero no participa en el flujo operativo de pasos propiedad de Python.", source: "src/Mision.cpp · src/main.cpp" },
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
    loop búsqueda de torque
      S-->>E: gyro + ticks por lado
      E->>M: rampa PWM protegida
      E-->>P: progress(cal/cal_a)
    end
    E->>M: validar +25°, reposo y retorno
    alt movimiento confirmado
      E-->>P: completed(cal_ok)
      P-->>H: listo / rutas habilitadas
    else encoder sin progreso
      E->>M: PWM = 0
      E-->>P: fault(cal_stall_left/right)
      P-->>H: fallo / rutas bloqueadas
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

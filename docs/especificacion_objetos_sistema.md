# Especificación de Objetos y Módulos del Sistema (Versión Actual)

Este documento detalla la especificación de clases, estructuras de datos, enumeraciones, módulos y APIs tanto del **Servidor (Python Backend/HMI)** como del **Sistema Autónomo (ESP32-S3 Firmware)** en su versión activa.

---

## 1. Especificación de Objetos del Servidor Python (`desktop_app/` / `android_app/`)

### 1.1 Modelo de Dominio (`desktop_app/robot_app/domain.py`)

#### `ConnectionState` (StrEnum)
Estados posibles de la conexión WebSocket hacia el ESP32:
- `STOPPED`: Conexión inactiva o cerrada explícitamente.
- `CONNECTING`: Intentando establecer enlace Socket hacia `ws://<host>/ws`.
- `CONNECTED`: Enlace activo y respondiendo pings.
- `BACKOFF`: Esperando intervalo de reintento tras pérdida de conexión.

#### `CommandStatus` (StrEnum)
Ciclo de vida de los comandos enviados al ESP32:
- `QUEUED`: Encolado localmente en el servidor.
- `SENT`: Transmitido por WebSocket hacia el ESP32.
- `ACKNOWLEDGED`: Confirmado recibido por el robot.
- `COMPLETED`: Ejecutado exitosamente por el firmware.
- `REJECTED`: Rechazado por el robot (por ejemplo, estado no válido o parámetros fuera de rango).
- `FAILED`: Falló durante ejecución (por ejemplo, atasco de motores o pérdida de MPU).

#### `RobotCommand` (Dataclass Inmutable)
Representa un comando emitido hacia el robot.
- **Campos**:
  - `name: str`: Nombre del comando (`calibrate`, `estop`, `stop`, `reset_pose`, `clear_fault`, `set_comp`, `step`, `move`).
  - `payload: dict[str, Any]`: Diccionario con parámetros validados (`heading`, `cm`, `factor`, etc.).
  - `command_id: str`: UUIDv4 hexadecimal único.
  - `created_at: float`: Timestamp Unix de creación.
  - `seq: int`: Número de secuencia incremental para coincidencia con eventos del firmware.
- **Métodos Clave**:
  - `create(cls, name, payload, seq) -> RobotCommand`: Valida los límites del payload antes de instanciar.
  - `protocol_envelope() -> dict`: Genera el paquete JSON exacto para el protocolo `robot-s3-steps-v3`.

#### `TelemetrySnapshot` (Dataclass Inmutable)
Captura de estado transmitida periódicamente por el ESP32:
- `heading: float`: Orientación normalizada [0.0°, 360.0°).
- `x_cm: float`, `y_cm: float`: Coordenadas odométricas globales en cm.
- `pulsos_fl: int`, `pulsos_fr: int`, `pulsos_bl: int`, `pulsos_br: int`: Conteo de ticks PCNT.
- `pwm_l: int`, `pwm_r: int`: Potencia actual aplicada a los motores [-230, 230].
- `v_bat: float`: Voltaje de alimentación medido en Voltios.
- `status: str`: Estado reportado por el firmware (`DESARMADO`, `CALIBRANDO`, `LISTO`, `EJECUTANDO`, `PAUSADO`, `FALLO`).
- `fase: str`, `progreso: float`: Fase de maniobra activa y porcentaje [0, 100].
- `mpu_ok: bool`: Indicador de comunicación activa I2C con MPU6050.
- `stack_min_bytes: int`: Stack libre mínimo reportado por FreeRTOS (alerta si < 1024 bytes).

#### `MissionPlan` & `OrthogonalRoute`
- **Funcionalidades**:
  - División de tramos largos mediante `split_segment_mm(start, end, max_segment_mm=2000)`.
  - Descomposición geométrica en pasos cardinales rectos (0°, 90°, 180°, 270°) mediante `decompose_to_orthogonal_steps()`.

---

### 1.2 Cliente Gateway WebSocket (`desktop_app/robot_app/gateway.py`)

#### `RobotGateway`
Único propietario del canal WebSocket hacia el ESP32 en el sistema operativo.
- **Atributos de Control**:
  - `_outgoing: PriorityQueue[_Outgoing]`: Cola priorizada (E-STOP prioridad 0 > Control prioridad 1 > Diagnóstico).
  - `_state: ConnectionState`: Estado actual de conexión protegido por `threading.Lock`.
- **Comportamiento**:
  - Reconexión automática con backoff exponencial y jitter aleatorio (hasta 5 reintentos por ciclo).
  - Límite de tamaño de paquete JSON entrante (máximo 4096 bytes).
  - Emisión de callbacks thread-safe: `on_message`, `on_state`, `on_sent`.

---

### 1.3 Servicios y Fachada (`desktop_app/robot_app/services.py`)

#### `RobotFacade`
Fachada principal que orquesta los casos de uso para las interfaces Web y API:
- `connect()`, `disconnect()`: Controlan la conexión a través de `RobotGateway`.
- `send_command(name, payload)`: Genera secuencias, convalida estados y despacha al gateway.
- `start_mission(points)`: Convierte lista de puntos XY en una secuencia de pasos ortogonales y la despacha ordenadamente.

#### `EventHub`
Broker pub-sub en memoria thread-safe:
- Mantiene cola circular de eventos con tamaño máximo (64 elementos).
- Publica en tiempo real eventos del sistema a todas las sesiones Web WebSocket / SSE conectadas.

#### `TelemetryRecorder`
Trabajador asíncrono en segundo plano:
- Procesa muestras de `TelemetrySnapshot` desde una cola sin bloquear hilos HTTP.
- Inserta registros en la base de datos SQLite (`insert_telemetry()`).

---

### 1.4 Base de Datos (`desktop_app/robot_app/database.py`)

Gobernanza de almacenamiento SQLite (`tmp_db/robot.sqlite3`):
- **`sessions`**: Registro de sesiones operativas de encendido / misión.
- **`commands`**: Historial de comandos enviados, payloads, `seq` y estado final (`COMPLETED`, `REJECTED`, `FAILED`).
- **`events`**: Registro de eventos del sistema (alertas, E-STOP, fallos MPU).
- **`telemetry`**: Muestras de pose, yaw, pulsos de encoders y PWM.

---

## 2. Especificación de Objetos del Sistema Autónomo (Firmware ESP32-S3)

### 2.1 Módulos y Clases de C++ (`src/` e `include/`)

#### `Estado` (`include/Estado.h`, `src/Estado.cpp`)
- **Enumeración `EstadoSistema`**: `DESARMADO`, `CALIBRANDO`, `LISTO`, `EJECUTANDO`, `PAUSADO`, `FALLO`.
- **Variables Globales**:
  - `estadoActual`: Estado operativo global.
  - `seqActivo`: ID de secuencia del comando en ejecución.
  - `faseComando`: Nombre textual de la fase activa (`girando`, `avanzando`, etc.).
  - `progresoComando`: Porcentaje de avance [0.0, 100.0].

#### `Motores` (`include/Motores.h`, `src/Motores.cpp`)
Gobernador físico y eléctrico de los motores DC vía DRV8833:
- **`setup_MotorPinsLow()`**: Fuerza GPIOs a `LOW` inmediatamente al entrar a `setup()`.
- **`aplicarVelocidades(int pwmL, int pwmR)`**:
  - Clampa el PWM global al límite seguro **`PWM_SAFE_LIMIT` = 230/255** (~90%).
  - Aplica tiempo muerto universal **`PWM_DIRECTION_PAUSE_MS` = 250 ms** ante cualquier cambio de signo en la velocidad.
- **`frenarMotores()`**: Aplica cortocircuito de freno rápido en ambos puentes H y pone PWM a 0.

#### `Sensores` (`include/Sensores.h`, `src/Sensores.cpp`)
Abstracción de lecturas síncronas de hardware:
- **Encoders PCNT**: Configura 4 unidades hardware de conteo de pulsos de la ESP32 (Front-Left, Front-Right, Back-Left, Back-Right).
- **MPU6050 (I2C)**: Lee tasa giroscópica Z ($G_z$) y calcula integración angular con filtro complementario.
- **`SensorSnapshot`**: Struct con muestra síncrona inmutable tomada en el inicio del súper-ciclo a 100 Hz.

#### `PoseEstimator` (`include/PoseEstimator.h`, `src/PoseEstimator.cpp`)
Odometría diferencial y fusión de orientación:
- **`actualizarOdometria(pulsosFL, pulsosFR, pulsosBL, pulsosBR, soloAvance)`**:
  - Calcula deltas de movimiento con el promedio por lado.
  - Actualiza $X_{\text{global}}$ y $Y_{\text{global}}$ en centímetros.
- **`actualizarOrientacion(deltaZ_rad)`**: Integra variación angular del giroscopio.
- **`recentrarYawIMUEnReposo()`**: Corrección continua de drift angular mientras el robot permanece inmóvil en `LISTO` o `DESARMADO`.

#### `Cinematica` (`include/Cinematica.h`, `src/Cinematica.cpp`)
Controlador cinemático de bucle cerrado:
- **`controlarGiro()`**:
  - Ejecuta maniobra de pivote único continuo.
  - Búsqueda continua de torque desde `140` hasta `247/255` en pasos de 5 cada 250 ms hasta vencer fricción estática.
  - Frenado y corrección inmediata al entrar en la banda de tolerancia **$\pm 3.5^\circ$**.
- **`controlarAvance()`**:
  - Lazo PD dinámico de rumbo: $\text{ctrlRumbo} = \text{constrain}(e_{\text{rumbo}} \cdot 4.0 - G_z \cdot 12.0, -45, +45)$.
  - Mantiene el avance recto sin detener el chasis.
- **`iniciarCalibracion()`**:
  - Ejecuta prueba de simetría angular (+25°, reposo 2.5 s, retorno a 0°).

#### `Seguridad` (`include/Seguridad.h`, `src/Seguridad.cpp`)
Guardián de integridad del robot:
- **`auditarSalud(snap, pwmL, pwmR)`**:
  - Audita pérdida de MPU6050.
  - Verifica atasco de motores por watchdog de encoder (6 s empíricos en avance, 2.5 s en giro, 800 ms acumulados al alcanzar el torque máximo de calibración).
  - Fuerza `FALLO` e inmoviliza el robot ante cualquier anomalía.
- **`forzarEStop()`**: Detención inmediata por software/hardware.

#### `DiagnosticoRTOS` (`include/DiagnosticoRTOS.h`, `src/DiagnosticoRTOS.cpp`)
Monitoreo de rendimiento e hilos FreeRTOS:
- Registra stack libre mínimo de `Task_Web` (Core 0) y `loop()` (Core 1).
- Mide tiempo de ciclo (objetivo 10000 µs), jitter, ciclos perdidos y motivo de reinicio de la CPU.

#### `Red` (`include/Red.h`, `src/Red.cpp`)
Servidor WebSocket e interfaz de comunicación JSON:
- Gestiona punto de acceso Wi-Fi `ROBOT_S3_LOCAL`.
- Procesa paquetes entrantes `robot-s3-steps-v3` y coloca `ComandoRed` en `colaComandos`.
- Lee `colaEventosRed` y transmite eventos y telemetría periódica (cada 100 ms).

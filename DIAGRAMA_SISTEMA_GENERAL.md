# Esquema de Funcionamiento y Diagramas UML del Sistema General

Este documento describe la arquitectura global, flujo de control y esquemas de interacción entre el **Servidor HMI/Backend (Python Desktop / Android)** y el **Sistema Autónomo (Firmware ESP32-S3)**.

---

## 1. Diagrama UML de Componentes del Sistema General

El sistema se compone de dos grandes subsistemas:

1. **Servidor HMI / Backend** (en `desktop_app/` para Windows y `android_app/` para Tablet Samsung): maneja interfaz visual, persistencia en SQLite, descomposición ortogonal de rutas y WebSocket cliente.
2. **Sistema Autónomo ESP32-S3** (en `src/` e `include/`): ejecuta el control en tiempo real a 100 Hz, odometría, filtrado MPU6050, protecciones eléctricas DRV8833 y servidor WebSocket.

```mermaid
flowchart TB
    subgraph S_SERVER["Servidor (Desktop App / Android App)"]
        direction TB
        UI["HMI Interface Web / Socket.IO"]
        Facade["RobotFacade"]
        Dispatcher["CommandDispatcherService"]
        Mission["MissionService"]
        Route["OrthogonalRoute / split_segment_mm"]
        Gateway["RobotGateway (WebSocket Client)"]
        Recorder["TelemetryRecorder"]
        DB[("SQLite Database (tmp_db/robot.sqlite3)")]
        Hub["EventHub"]

        UI --> Facade
        Facade --> Dispatcher
        Facade --> Mission
        Facade --> Recorder
        Mission --> Route
        Dispatcher --> Gateway
        Recorder --> DB
        Gateway --> Hub
        Hub --> UI
    end

    subgraph S_WIFI["Canal Wi-Fi (WebSocket Protocol V2)"]
        Protocol["Protocolo JSON robot-s3-steps-v3"]
    end

    subgraph S_ESP32["Sistema Autónomo ESP32-S3 (Firmware Dual-Core)"]
        subgraph CORE0["Core 0 - Comunicaciones (Task_Web)"]
            TaskWeb["Task_Web (ESP32 Core 0)"]
            RedServer["Red.cpp (WebSocket Server)"]
            TaskWeb --> RedServer
        end

        subgraph QUEUES["FreeRTOS Inter-Task Queues"]
            CmdQueue["colaComandos (Queue size 4)"]
            EvtQueue["colaEventosRed (Queue size 8)"]
        end

        subgraph CORE1["Core 1 - Tiempo Real 100 Hz (loop Super-Ciclo)"]
            CmdProc["procesarComandos()"]
            Cine["Cinematica.cpp (Lazo Abierto/Cerrado)"]
            Sens["Sensores.cpp (PCNT + MPU6050)"]
            Pose["PoseEstimator.cpp (Odometría + Yaw)"]
            Motores["Motores.cpp (PWM DRV8833 <= 230 rectos / <= 247 giros)"]
            Seguridad["Seguridad.cpp (Watchdog + E-STOP)"]

            CmdQueue --> CmdProc
            CmdProc --> Cine
            Sens --> Pose
            Pose --> Cine
            Cine --> Motores
            Seguridad --> Motores
            Cine --> EvtQueue
        end

        RedServer --> CmdQueue
        EvtQueue --> RedServer
    end

    subgraph S_HARDWARE["Hardware Físico y Sensores"]
        DRV["DRV8833 Dual H-Bridge Driver"]
        DC["Motores DC con Reductora"]
        Encoders["Encoders Magnéticos (x4)"]
        MPU["MPU6050 IMU (I2C 400kHz)"]

        Motores --> DRV
        DRV --> DC
        Encoders --> Sens
        MPU --> Sens
    end

    Gateway <--> Protocol
    Protocol <--> TaskWeb
```

---

## 2. Diagrama UML de Despliegue (Hardware & Software)

Describe la asignación física de componentes software sobre el hardware del robot y la estación base / tablet:

```mermaid
flowchart TB
    subgraph NODE_HOST["Estación Base / Tablet (Windows / Android)"]
        subgraph SW_BACKEND["Python Backend (app.py / Chaquopy)"]
            Facade_D["RobotFacade & CommandDispatcher"]
            Recorder_D["TelemetryRecorder & EventHub"]
            Gateway_D["RobotGateway (Propietario Único)"]
        end
        DB_D[("SQLite (robot.sqlite3)")]
        UI_D["Web Frontend (HTML5 / JS / Canvas)"]

        UI_D <-->|"HTTP REST / SSE"| SW_BACKEND
        Recorder_D --> DB_D
    end

    subgraph NODE_WIFI["Red Wi-Fi AP (ROBOT_S3_LOCAL)"]
        WIFI_BUS["Canal WebSocket / IP: 192.168.4.1:80/ws"]
    end

    subgraph NODE_ESP32["Robot Móvil Autónomo ESP32-S3"]
        subgraph C0["ESP32-S3 Core 0 (Comunicaciones)"]
            TaskWeb_D["Task_Web (Stack 8KB)"]
            WSServer_D["AsyncWebSocket Server"]
            TaskWeb_D <--> WSServer_D
        end

        subgraph RTOS_QUEUES["IPC FreeRTOS"]
            Q1["colaComandos"]
            Q2["colaEventosRed"]
        end

        subgraph C1["ESP32-S3 Core 1 (Super-Ciclo 100 Hz)"]
            Loop_D["loop Native Loop (Stack 8KB)"]
            Control_D["Cinematica & PD Yaw Controller"]
            Odometry_D["PoseEstimator (Odometría + Gyro Z)"]
            Safety_D["Seguridad & Protecciones DRV8833"]

            Loop_D --> Control_D
            Loop_D --> Odometry_D
            Loop_D --> Safety_D
        end

        subgraph HW_PERIPHERALS["Periféricos de Hardware"]
            IMU_HW["Módulo MPU6050 (I2C 400kHz)"]
            DRV_HW["Drivers DRV8833 (Dead-Time 250ms, PWM <= 230)"]
            ENC_HW["Encoders Magnéticos (x4 PCNT)"]
        end

        WSServer_D <--> Q1
        WSServer_D <--> Q2
        Q1 <--> Loop_D
        Q2 <--> Loop_D

        C1 -->|"I2C Bus"| IMU_HW
        C1 -->|"GPIO LEDC PWM"| DRV_HW
        ENC_HW -->|"GPIO PCNT Units"| C1
    end

    Gateway_D <--> WIFI_BUS
    WIFI_BUS <--> WSServer_D
```

---

## 3. Diagrama UML de Secuencia (Flujo de Ejecución de Paso Ortogonal y Telemetría)

Ilustra el intercambio asíncrono desde que el usuario solicita un comando de movimiento en el Servidor HMI hasta su procesamiento síncrono a 100 Hz en el ESP32-S3 y la retroalimentación de telemetría:

```mermaid
sequenceDiagram
    autonumber
    actor Usuario as Usuario HMI
    participant Web as Web Frontend (UI)
    participant Facade as RobotFacade (Python)
    participant Gateway as RobotGateway (Python)
    participant Core0 as Task_Web (ESP32 Core 0)
    participant Core1 as loop() 100Hz (ESP32 Core 1)
    participant Hardware as Motores / Encoders / MPU

    Usuario->>Web: Clic en "Ejecutar Paso" (heading=90°, cm=50)
    Web->>Facade: POST /api/command {cmd: "step", heading: 90, cm: 50}
    Facade->>Facade: Crear y Validar RobotCommand(seq=101)
    Facade->>Gateway: send_command(RobotCommand)
    Gateway->>Gateway: Encolar en PriorityQueue (_Outgoing)
    Gateway->>Core0: Send JSON {"cmd":"step", "heading":90, "cm":50, "seq":101}
    Core0->>Core0: deserializarJson() -> ComandoRed struct
    Core0->>Core1: xQueueSend(colaComandos, &cmd)
  
    rect rgb(235, 245, 255)
        note over Core1,Hardware: Súper-ciclo síncrono a 100 Hz (10 ms)
        Core1->>Core1: procesarComandos() -> iniciarPaso(90°, 50cm, seq=101)
        Core1->>Hardware: leerSensoresSincrono() [PCNT Encoders + MPU6050 I2C]
        Hardware-->>Core1: SensorSnapshot (pulsos, deltaZ_rad)
        Core1->>Core1: PoseGlobal.actualizarOdometria()
        Core1->>Core1: controlarMovimiento() -> Giro Pivote Continuo / Avance PD
        Core1->>Hardware: aplicarVelocidades(pwmL, pwmR) [DRV8833 PWM <= 230]
        Core1->>Core0: xQueueSend(colaEventosRed, EVT_PROGRESS)
    end

    Core0->>Gateway: Broadcast JSON Telemetry Snapshot & EVT_PROGRESS
    Gateway->>Facade: on_message(TelemetrySnapshot)
    Facade->>Web: SSE / Socket.IO Telemetry Stream (x, y, heading, status)
    Web->>Usuario: Actualización de Posición e Historial en Vivo

    rect rgb(235, 255, 235)
        note over Core1: Paso Alcanzado con Tolerancia Geométrica
        Core1->>Core0: xQueueSend(colaEventosRed, EVT_COMPLETED, "step_ok")
    end
    Core0->>Gateway: Send Event JSON {"evt":"completed", "seq":101, "msg":"step_ok"}
    Gateway->>Facade: Command Status -> COMPLETED
    Facade->>Web: Event Notify (Comando Completado)
```

---

## 4. Diagrama UML de Máquina de Estados (Firmware ESP32-S3)

Muestra los estados operativos y las transiciones del robot en firmware:

```mermaid
stateDiagram-v2
    [*] --> DESARMADO : Arranque / setup_MotorPinsLow()
  
    DESARMADO --> CALIBRANDO : Comando CMD_CALIBRATION
    CALIBRANDO --> LISTO : Calibración exitosa (+25° yaw, 2.5s reposo, 0° yaw)
    CALIBRANDO --> FALLO : Atascamiento / Pérdida IMU / Watchdog 800ms

    DESARMADO --> LISTO : Transición manual limpia
    LISTO --> EJECUTANDO : Comando CMD_STEP / CMD_MOVE
  
    EJECUTANDO --> EJECUTANDO : Bucle 100 Hz (Giro Pivote Continuo -> Avance PD)
    EJECUTANDO --> LISTO : Fin de Paso / Alineación Cardinal Final
    EJECUTANDO --> PAUSADO : Comando CMD_STOP / Pause
    PAUSADO --> EJECUTANDO : Reanudar Paso
    PAUSADO --> LISTO : Cancelar Paso

    EJECUTANDO --> FALLO : Sensor Invalido / Overcurrent / Stall Watchdog
    LISTO --> FALLO : Fallo Hardware auditado por Seguridad.cpp
  
    state FALLO {
        [*] --> MotoresFrenados : frenarMotores() + PWM=0
        MotoresFrenados --> InterlockActivo : DRV8833 Pause 250ms
    }

    FALLO --> DESARMADO : Comando CMD_CLEAR_FAULT / E-STOP Reset
```

---

## 5. Referencias de Especificación por Componente

Las especificaciones detalladas de clases, funciones y estructuras de datos se encuentran organizadas en las siguientes carpetas del proyecto:

- **Servidor y Backend (Python / HMI)**: [`desktop_app/README.md`](desktop_app/README.md) y [`docs/especificacion_objetos_sistema.md`](docs/especificacion_objetos_sistema.md#1-especificación-de-objetos-del-servidor-python-desktop_app--android_app).
- **Sistema Autónomo (ESP32-S3 Firmware)**: [`src/README.md`](src/README.md), [`include/README.md`](include/README.md) y [`docs/especificacion_objetos_sistema.md`](docs/especificacion_objetos_sistema.md#2-especificación-de-objetos-del-sistema-autónomo-firmware-esp32-s3).
- **Aplicación Android (Tablet Samsung Wrapper)**: [`android_app/README.md`](android_app/README.md).

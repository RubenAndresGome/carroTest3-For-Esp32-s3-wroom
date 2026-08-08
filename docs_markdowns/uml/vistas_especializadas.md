# Vistas especializadas de ejecución y protocolo

Estas vistas complementan el atlas general y los grafos generados por carpeta.
Describen el corte operativo actual; no convierten los riesgos abiertos en
garantías de seguridad física.

## Cadena de adquisición, pose y control

```mermaid
flowchart LR
    PCNT["4 unidades PCNT<br/>pulsos FL/FR/BL/BR"] --> Snapshot["SensorSnapshot único<br/>100 Hz · Core 1"]
    MPU["MPU6050 por I²C<br/>gyro Z + delta yaw"] --> Snapshot
    Snapshot --> Filter["deltas y velocidades filtradas"]
    Snapshot --> Pose["PoseEstimator<br/>x · y · heading360"]
    Snapshot --> Safety["Seguridad<br/>stale · stall · watchdog"]
    Filter --> Motion["Cinemática<br/>giro/avance"]
    Pose --> Motion
    Safety --> Stop["frenarMotores()"]
    Motion --> PWM["aplicarVelocidades()"]
    PWM --> Driver["DRV8833"]
```

La lectura PCNT no se sustituye por interrupciones GPIO. Pose, seguridad y
cinemática consumen la misma muestra para evitar decisiones de ciclos distintos.

## Propiedad de memoria y recursos compartidos

```mermaid
flowchart TB
    subgraph C0["Core 0 · Task_Web · stack 8192"]
        WS["clienteActivo · msgBuf 4097 B"]
        Net["sessionId · ultimoSeqCompletado"]
        Tele["telemetría 10 Hz"]
    end
    subgraph Shared["Cruce controlado entre núcleos"]
        CQ[["colaComandos · 4 ComandoRed"]]
        EQ[["colaEventosRed · 8 EventoRed"]]
        SS["snapshotSensoresControl<br/>seqlock por sequence"]
        Est["estadoActual · seqActivo · PWM aplicado"]
    end
    subgraph C1["Core 1 · loop() nativo · stack 8192"]
        Sensors["PCNT + MPU"]
        Control["pose + seguridad + cinemática"]
        Motors["PWM e interlock"]
    end
    WS --> CQ --> Control
    Sensors --> SS --> Control
    Control --> EQ --> WS
    Control --> Est --> Tele
    Control --> Motors
```

Los valores globales/volátiles hacen visible el estado, pero no sustituyen la
sincronización. Los dos cruces de trabajo explícitos son las colas FreeRTOS; la
copia de sensores usa la secuencia del snapshot.

## Contrato JSON v3, campo por campo

```mermaid
flowchart TB
    Hello["hello<br/>cmd:string='hello'<br/>session:string[1..16]<br/>seq:int=0"] --> HelloAck["hello_ack<br/>evt:string<br/>protocol:'robot-s3-steps-v3'<br/>session:string<br/>state:enum<br/>last_seq:int<br/>calibrated:bool<br/>fault?:string"]
    Step["step<br/>cmd:'step'<br/>heading:number [0,360)<br/>cm:number (0,200]<br/>seq:int > 0"] --> Events
    Turn["turn_to<br/>cmd:'turn_to'<br/>heading:number [0,360)<br/>seq:int > 0"] --> Events
    Other["calibrate/stop/estop/<br/>clear_fault/reset_pose<br/>cmd:string · seq:int > 0<br/>set_comp añade factor:number"] --> Events
    Events["eventos<br/>evt:accepted|rejected|progress|<br/>completed|already_done|fault<br/>seq:int<br/>detail?:string<br/>pct?:number"]
    Telemetry["telemetry (10 Hz)<br/>evt · state · yaw · x · y<br/>pwm_l · pwm_r · enc[4]<br/>degraded · session · last_seq · seq<br/>phase · prog · comp · cal<br/>firmware · protocol · reset_reason<br/>stack_* · tasks_ok · control_*"]
```

`session` identifica el proceso controlador; `seq` ordena maniobras dentro de
esa memoria corta. El UUID `command_id` existe en Python/SQLite, pero no viaja al
firmware, limitación registrada como A-03.

## Cadena de una orden hasta el PWM

```mermaid
sequenceDiagram
    autonumber
    participant H as HMI local
    participant W as web.py
    participant S as RobotService
    participant G as RobotGateway
    participant R as Red/Core 0
    participant C as Cinemática/Core 1
    participant M as Motores/DRV8833
    H->>W: POST autenticado X-App-Token
    W->>S: validar estado, carga y frescura
    S->>G: RobotCommand + seq
    G->>R: JSON WebSocket v3
    R-->>G: accepted(seq)
    R->>C: colaComandos
    loop cada 10 ms
        C->>C: error de rumbo/distancia
        C->>M: PWM limitado + rampa
        M->>M: interlock 250 ms si invierte signo
    end
    C->>R: colaEventosRed(completed/fault)
    R-->>G: evento terminal del mismo seq
    G->>S: resolver comando y persistir
    S-->>H: SSE/estado actualizado
```

## Ciclo nominal detectado por análisis estático

```mermaid
flowchart LR
    rPts["rPts()"] --> rmPt["rmPt()"]
    rmPt --> rPts
```

El ciclo `rPts ↔ rmPt` procede del descomponedor HTML independiente. Es un ciclo
nominal del grafo de llamadas, no evidencia por sí solo de recursión infinita;
debe revisarse con entradas límite cuando se modifique esa herramienta.

## Exportaciones

- [Fuente PlantUML combinada](plantuml/vistas_especializadas.puml)
- [SVG](exportados/vistas_especializadas.svg)
- [PNG](exportados/vistas_especializadas.png)


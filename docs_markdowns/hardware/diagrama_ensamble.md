# Diagrama de ensamble físico actual

> Fuente canónica: `include/Config.h`. Si cambia un GPIO en firmware, este documento y el diagrama interactivo deben actualizarse en el mismo cambio.

## Alimentación

| Fuente | Tensión | Destino |
|---|---:|---|
| B1 | 3.3 V regulados | ESP32-S3 y lado VCCA de los conversores |
| B2 | 4.8 V regulados | MPU6050, LM393 y lado VCCB de los conversores |
| B3 | 7.7 V | VMOT de ambos DRV8833 |

Todas las fuentes y módulos comparten una **tierra estrella**. Nunca conecte 7.7 V a un GPIO o al riel de 3.3 V. Verifique las tensiones con multímetro antes de conectar el ESP32.

## Arquitectura General de Hardware y Conexiones

```mermaid
flowchart TB
    %% Definición de Estilos
    classDef mcu fill:#1e293b,stroke:#38bdf8,stroke-width:2px,color:#ffffff;
    classDef sensor fill:#0f766e,stroke:#2dd4bf,stroke-width:2px,color:#ffffff;
    classDef power fill:#7c2d12,stroke:#fb923c,stroke-width:2px,color:#ffffff;
    classDef driver fill:#831843,stroke:#f472b6,stroke-width:2px,color:#ffffff;
    classDef motor fill:#701a75,stroke:#e879f9,stroke-width:2px,color:#ffffff;
    classDef level fill:#1e3a8a,stroke:#60a5fa,stroke-width:2px,color:#ffffff;
    classDef hmi fill:#14532d,stroke:#4ade80,stroke-width:2px,color:#ffffff;

    subgraph SISTEMA_COMPLETO["ROBOT ESP32-S3 4WD · ARQUITECTURA GENERAL DE HARDWARE Y CONEXIONES"]
        direction TB

        %% Capa de Interfaz y Enlace Inalámbrico
        subgraph CAPA_HMI["1. Capa de Comunicación e Interfaz de Control"]
            direction LR
            HMI["<b>HMI / Panel de Control</b><br/>(Tablet Android / App Python Desktop)<br/>WebSocket Protocol: robot-s3-steps-v3"]:::hmi
            WIFI["<b>Enlace Wi-Fi SoftAP</b><br/>SSID: robot-s3-ap (192.168.4.1)<br/>Puerto 80 / ws://192.168.4.1/ws"]:::hmi
            HMI <-->|Frames JSON ≤ 7168 Bytes| WIFI
        end

        %% Capa de Alimentación Eléctrica
        subgraph CAPA_POWER["2. Sistema de Potencia y Alimentación Aislada"]
            direction LR
            B1["<b>B1 · 3.3V Regulados</b><br/>Lógica ESP32-S3 y TXS0108E (VCCA)"]:::power
            B2["<b>B2 · 4.8V Regulados</b><br/>Sensores LM393, MPU6050 y TXS (VCCB)"]:::power
            B3["<b>B3 · 7.7V Li-Ion (2S)</b><br/>Alimentación Motores VMOT"]:::power
            SWITCH["<b>Interruptor Físico VMOT</b><br/>Doble switch de seguridad"]:::power
            GND_STAR["<b>GND ESTRELLA UNIFICADO</b><br/>(Punto común de referencia a tierra)"]:::power

            B3 --> SWITCH
            B1 --- GND_STAR
            B2 --- GND_STAR
            SWITCH --- GND_STAR
        end

        %% Microcontrolador Central
        subgraph CAPA_MCU["3. Unidad de Procesamiento Central (ESP32-S3 DevKitC-1)"]
            direction TB
            ESP["<b>ESP32-S3 Microcontroller (N8)</b><br/>Xtensa LX7 Dual-Core @ 240 MHz<br/>• Core 0: Task_Web (AsyncTCP / WebSocket / JSON)<br/>• Core 1: loop() Súper-Ciclo Síncrono 100 Hz RTOS"]:::mcu
        end

        %% Capa de Sensores y Adaptación de Nivel
        subgraph CAPA_SENSORES["4. Sensores de Odometría e Inercia (5V / 4.8V)"]
            direction TB

            subgraph TXS_BLOCK["Adaptación de Nivel Lógico (TXS0108E)"]
                TXS_I2C["<b>TXS0108E (I2C)</b><br/>3.3V (VCCA) ↔ 4.8V (VCCB)"]:::level
                TXS_ENC["<b>TXS0108E (Encoders)</b><br/>3.3V (VCCA) ↔ 4.8V (VCCB)"]:::level
            end

            subgraph SENSORES_HARDWARE["Módulos de Sensado"]
                MPU["<b>IMU MPU6050 (6-DOF)</b><br/>I2C Addr: 0x68 (AD0=GND)<br/>Giroscopio Z: Autoridad Angular"]:::sensor
                ENC_FL["<b>Encoder FL (LM393)</b><br/>Rueda Frontal Izq (20 PPR)"]:::sensor
                ENC_FR["<b>Encoder FR (LM393)</b><br/>Rueda Frontal Der (20 PPR)"]:::sensor
                ENC_BL["<b>Encoder BL (LM393)</b><br/>Rueda Trasera Izq (20 PPR)"]:::sensor
                ENC_BR["<b>Encoder BR (LM393)</b><br/>Rueda Trasera Der (20 PPR)"]:::sensor
            end
        end

        %% Capa de Actuadores y Drivers de Potencia
        subgraph CAPA_ACTUADORES["5. Drivers Puente H y Motores de Tracción (4WD)"]
            direction TB

            subgraph DRIVERS_POTENCIA["Puentes H Duales (DRV8833)"]
                DRV_L["<b>DRV8833 Izquierdo</b><br/>Canales FL + BL<br/>Capacitores 100µF + 0.1µF"]:::driver
                DRV_R["<b>DRV8833 Derecho</b><br/>Canales FR + BR<br/>Capacitores 100µF + 0.1µF"]:::driver
            end

            subgraph MOTORES_TT["Motores Reductores DC (TT Amarillos 1:48)"]
                M_FL["<b>Motor FL</b> (Frontal Izq)<br/>Montado con cuerpo hacia ATRÁS"]:::motor
                M_BL["<b>Motor BL</b> (Trasero Izq)<br/>Montado con cuerpo hacia ADELANTE"]:::motor
                M_FR["<b>Motor FR</b> (Frontal Der)<br/>Montado con cuerpo hacia ATRÁS"]:::motor
                M_BR["<b>Motor BR</b> (Trasero Der)<br/>Montado con cuerpo hacia ADELANTE"]:::motor
            end
        end

        %% Interconexiones de Señales y Buses
        WIFI <-->|AsyncTCP Core 0| ESP
        B1 -.->|3.3V Power| ESP
        B1 -.->|VCCA 3.3V| TXS_BLOCK
        B2 -.->|VCCB 4.8V| TXS_BLOCK
        B2 -.->|Alimentación 4.8V| SENSORES_HARDWARE
        SWITCH -.->|VMOT 7.7V Filtrado| DRIVERS_POTENCIA

        %% I2C
        ESP -- "GPIO8 (SDA) / GPIO9 (SCL)" --> TXS_I2C
        TXS_I2C -- "SDA / SCL (4.8V)" --> MPU

        %% Encoders PCNT
        ENC_FL -- "Señal Digital (Verde B5)" --> TXS_ENC
        ENC_FR -- "Señal Digital (Blanco B6)" --> TXS_ENC
        ENC_BL -- "Señal Digital (Negro B2)" --> TXS_ENC
        ENC_BR -- "Señal Digital (Rojo B1)" --> TXS_ENC

        TXS_ENC -- "GPIO11 (PCNT Unit 0)" --> ESP
        TXS_ENC -- "GPIO10 (PCNT Unit 1)" --> ESP
        TXS_ENC -- "GPIO12 (PCNT Unit 2)" --> ESP
        TXS_ENC -- "GPIO13 (PCNT Unit 3)" --> ESP

        %% PWM Motores DRV8833
        ESP -- "GPIO7 (IN3 · FWD+) / GPIO6 (IN4 · REV-)" --> DRV_L
        ESP -- "GPIO4 (IN2 · FWD+) / GPIO5 (IN1 · REV-)" --> DRV_L
        ESP -- "GPIO18 (IN4 · FWD+) / GPIO17 (IN3 · REV-)" --> DRV_R
        ESP -- "GPIO16 (IN2 · FWD+) / GPIO15 (IN1 · REV-)" --> DRV_R

        %% Conexión Borneras Motor
        DRV_L -- "OUT3 (+) Naranja / OUT4 (-) Negro" --> M_FL
        DRV_L -- "OUT2 (+) Naranja / OUT1 (-) Rojo" --> M_BL
        DRV_R -- "OUT4 (+) Naranja / OUT3 (-) Negro" --> M_FR
        DRV_R -- "OUT2 (+) Naranja / OUT1 (-) Negro" --> M_BR
    end
```

## Disposición Mecánica del Chasis 4WD y Simetría

![Geometría del Chasis 4WD y Orientación de Motores](chasis_4wd_orientacion_motores.png)

```
                  FRENTE (+Y, Yaw = 0°)
          ┌───────────────────────────────────┐
          │   [Motor FL]          [Motor FR]  │
          │   (Cuerpo hacia       (Cuerpo hacia│
          │      atrás)              atrás)   │
          │                                   │
IZQUIERDA │        (ESP32-S3 / Baterías)      │ DERECHA (+X, Yaw = 90°)
(-X)      │         Pose (0,0) IMU            │
          │                                   │
          │   [Motor BL]          [Motor BR]  │
          │   (Cuerpo hacia       (Cuerpo hacia│
          │     adelante)           adelante) │
          └───────────────────────────────────┘
                  ATRÁS (-Y, Yaw = 180°)
```

```mermaid
flowchart TB
    subgraph CHASIS["CHASIS 4WD · VISTA SUPERIOR Y ORIENTACIÓN DE MOTORES"]
        direction TB
        subgraph FRENTE_DIR["FRENTE (+Y · Yaw 0°)"]
            direction LR
            FL["<b>Motor FL</b><br/>Frontal Izquierdo<br/><i>(Cuerpo hacia ATRÁS)</i><br/>GPIO7 (+), GPIO6 (-)"]
            FR["<b>Motor FR</b><br/>Frontal Derecho<br/><i>(Cuerpo hacia ATRÁS)</i><br/>GPIO18 (+), GPIO17 (-)"]
        end

        subgraph CENTRO_CHASIS["Plataforma Central / Baterías / ESP32-S3"]
            direction LR
            IZQ_LABEL["<b>IZQUIERDA</b><br/>(-X)"]
            IMU_POSE["<b>IMU MPU6050 & Pose (0,0)</b><br/>SDA: GPIO8 · SCL: GPIO9"]
            DER_LABEL["<b>DERECHA</b><br/>(+X · Yaw +90°)"]
        end

        subgraph ATRAS_DIR["ATRÁS (-Y · Yaw 180°)"]
            direction LR
            BL["<b>Motor BL</b><br/>Trasero Izquierdo<br/><i>(Cuerpo hacia ADELANTE)</i><br/>GPIO4 (+), GPIO5 (-)"]
            BR["<b>Motor BR</b><br/>Trasero Derecho<br/><i>(Cuerpo hacia ADELANTE)</i><br/>GPIO16 (+), GPIO15 (-)"]
        end

        FL --- CENTRO_CHASIS --- BL
        FR --- CENTRO_CHASIS --- BR
    end
```

> **Dogma de Simetría Mecánica 4WD**: Los motores reductores TT delanteros (FL/FR) y traseros (BL/BR) están montados en oposición física ($180^\circ$ enfrentados hacia el interior del chasis). La asignación de pines FWD/REV y la conexión a las borneras del DRV8833 compensa esta orientación física para asegurar que la señal lógica de avance (`FWD`, $+Y$) produzca rotación armónica hacia el frente en todas las ruedas.

## Pinout canónico

### Motores DRV8833

| Rueda | Avance/FWD | Reversa/REV | Puente y entradas |
|---|---:|---:|---|
| FL, frontal izquierda | GPIO7 | GPIO6 | DRV izquierdo IN3 (+) / IN4 (-) |
| BL, trasera izquierda | GPIO4 | GPIO5 | DRV izquierdo IN2 (+) / IN1 (-) |
| FR, frontal derecha | GPIO18 | GPIO17 | DRV derecho IN4 (+) / IN3 (-) |
| BR, trasera derecha | GPIO16 | GPIO15 | DRV derecho IN2 (+) / IN1 (-) |

El firmware genera PWM a 5 kHz y 10 bits (0–1023). La polaridad lógica aprobada está implementada en software; no intercambie GPIO para corregir el sentido de una rueda sin revisar también el cableado y `Config.h`.

### Encoders PCNT

| Encoder | GPIO | PCNT | Identificación física (TXS0108E) |
|---|---:|---:|---|
| FL | GPIO11 | UNIT 0 | Verde (B5 $\to$ A5) |
| FR | GPIO10 | UNIT 1 | Blanco (B6 $\to$ A6) |
| BL | GPIO12 | UNIT 2 | Negro (B2 $\to$ A2) |
| BR | GPIO13 | UNIT 3 | Rojo (B1 $\to$ A1) |

Las salidas LM393 de 4.8 V pasan por el conversor de nivel antes del ESP32. La lectura se hace con PCNT, no con `attachInterrupt`.

### MPU6050

| Señal | ESP32-S3 | Observación |
|---|---:|---|
| SDA | GPIO8 | I2C mediante conversor de nivel |
| SCL | GPIO9 | I2C mediante conversor de nivel |
| Dirección | 0x68 | AD0 en nivel bajo |

El MPU6050 debe fijarse con orientación conocida, lejos de vibración intensa y cableado de potencia. Es un acelerómetro/giroscopio de 6 ejes; no contiene magnetómetro.

## Distribución FreeRTOS

| Núcleo | Ejecución |
|---|---|
| Core 0 | `Task_Web`: AP Wi-Fi, WebSocket/JSON V1 y telemetría |
| Core 1 | `loop()` síncrono a 100 Hz: MPU6050/PCNT → pose/seguridad → navegación → motores |

La computadora ejecuta `desktop_app` y es el único cliente WebSocket. El ESP32 expone `ws://192.168.4.1/ws`; no aloja el HMI activo.

Consulte [manual_ensamble_fisico.md](manual_ensamble_fisico.md) para la secuencia de montaje y [diagrama_interactivo/diagrama.html](diagrama_interactivo/diagrama.html) para inspeccionar cada conexión.

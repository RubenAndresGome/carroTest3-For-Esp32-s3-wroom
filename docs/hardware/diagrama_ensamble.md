# Diagrama de ensamble físico actual

> Fuente canónica: `include/Config.h`. Si cambia un GPIO en firmware, este documento y el diagrama interactivo deben actualizarse en el mismo cambio.

## Alimentación

| Fuente | Tensión | Destino |
|---|---:|---|
| B1 | 3.3 V regulados | ESP32-S3 y lado VCCA de los conversores |
| B2 | 4.8 V regulados | MPU6050, LM393 y lado VCCB de los conversores |
| B3 | 7.7 V | VMOT de ambos DRV8833 |

Todas las fuentes y módulos comparten una **tierra estrella**. Nunca conecte 7.7 V a un GPIO o al riel de 3.3 V. Verifique las tensiones con multímetro antes de conectar el ESP32.

```mermaid
flowchart LR
    B1["B1 · 3.3 V"] --> ESP["ESP32-S3"]
    B1 --> LVI["TXS0108E I2C · VCCA"]
    B1 --> LVE["TXS0108E encoders · VCCA"]
    B2["B2 · 4.8 V"] --> MPU["MPU6050 · 0x68"]
    B2 --> LVIH["TXS0108E I2C · VCCB"]
    B2 --> LVEH["TXS0108E encoders · VCCB"]
    B2 --> ENC["4 × LM393"]
    B3["B3 · 7.7 V"] --> DRVL["DRV8833 izquierdo"]
    B3 --> DRVR["DRV8833 derecho"]
    ESP -- "GPIO8 SDA / GPIO9 SCL" --> LVI
    LVIH --> MPU
    ENC --> LVEH
    LVE -- "GPIO10–13 · PCNT" --> ESP
    ESP -- "GPIO4–7" --> DRVL
    ESP -- "GPIO15–18" --> DRVR
    DRVL --> FL["Motor FL"]
    DRVL --> BL["Motor BL"]
    DRVR --> FR["Motor FR"]
    DRVR --> BR["Motor BR"]
```

## Pinout canónico

### Motores DRV8833

| Rueda | Avance/FWD | Reversa/REV | Puente y entradas |
|---|---:|---:|---|
| FL, frontal izquierda | GPIO6 | GPIO7 | DRV izquierdo IN1 / IN2 |
| BL, trasera izquierda | GPIO4 | GPIO5 | DRV izquierdo IN3 / IN4 |
| FR, frontal derecha | GPIO17 | GPIO18 | DRV derecho IN1 / IN2 |
| BR, trasera derecha | GPIO15 | GPIO16 | DRV derecho IN3 / IN4 |

El firmware genera PWM a 5 kHz y 10 bits (0–1023). La polaridad lógica aprobada está implementada en software; no intercambie GPIO para corregir el sentido de una rueda sin revisar también el cableado y `Config.h`.

### Encoders PCNT

| Encoder | GPIO | PCNT | Identificación física |
|---|---:|---:|---|
| FL | GPIO10 | UNIT 0 | cable rojo |
| FR | GPIO11 | UNIT 1 | cable café |
| BL | GPIO12 | UNIT 2 | cable negro |
| BR | GPIO13 | UNIT 3 | cable blanco |

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

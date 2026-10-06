# Guía Canónica de Reproducibilidad Técnica y Experimental

> **Propósito para Evaluadores y Revisores:**
> Este documento contiene la especificación unívoca para clonar, compilar, flashear, probar y reproducir experimentalmente los resultados del sistema robótico móvil diferencial 4WD basado en **ESP32-S3**.
> Al tratarse de **software embebido de tiempo real**, la ejecución involucra restricciones estrictas de hardware (frecuencias de muestreo, temporización FreeRTOS, periféricos de conteo por hardware, buses I²C y electrónica de potencia). El evaluador no necesita asumir versiones, comandos ni cableados.

---

## 1. Identificación del Código y Control de Versiones

### 1.1. Rama Canónica de Reproducción
Para evaluar el estado funcional más reciente, completo y validado, **utilice siempre la última versión de la rama principal (`main`)**:

```bash
# 1. Clonar el repositorio
git clone https://github.com/RubenAndresGome/carroTest3-For-Esp32-s3-wroom.git

# 2. Acceder al directorio
cd carroTest3-For-Esp32-s3-wroom

# 3. Asegurar la última versión sincronizada de main
git checkout main
git pull origin main
```

### 1.2. Trazabilidad Histórica y Metadatos de Commits Auditados
Para efectos de verificación científica y auditoría técnica cruzada con los informes, los hitos de referencia registrados en el árbol Git son:

| Parámetro | Valor de Referencia | Descripción |
| :--- | :--- | :--- |
| **Repositorio Remoto** | `https://github.com/RubenAndresGome/carroTest3-For-Esp32-s3-wroom.git` | Origen canónico de código y documentación |
| **Rama Principal** | `main` | Rama obligatoria de compilación y despliegue |
| **Commit de Referencia (`main`)** | `e207d678ad52c3c9c9480dc3ecfa450d06179b88` | Hito evaluado: Modelos matemáticos aplicados y plataforma operativa |
| **Commit en Rama de Hito R1** | `939eb989226626388c616be84e92d817a524896a` | Rama `producto_operativo_R1_0.0.0` |
| **Tags del Proyecto** | `Milestone_Funcional_1.0` | Tag del hito funcional auditado |
| **GitHub Pages Activo** | `https://rubenandresgome.github.io/carroTest3-For-Esp32-s3-wroom/` | Portal web documental con artefactos de descarga |

---

## 2. Especificación del Entorno Embebido y Toolchain

El proyecto se gestiona mediante **PlatformIO**. La configuración de compilación, microcontrolador, particiones y bibliotecas está formalizada de manera inmutable en [`platformio.ini`](platformio.ini).

### 2.1. Requisitos de Software en Host
* **PlatformIO Core**: `>= 6.2.0` (disponible como CLI `pio` o mediante la extensión oficial en VS Code).
* **Python**: `3.11.x` o superior (necesario para PlatformIO, scripts ADB y análisis de telemetría).
* **Android Debug Bridge (ADB)**: versión `>= 1.0.41` (incluido en Android SDK Platform-Tools) para extracción de telemetría desde la tablet.

### 2.2. Hardware y Microcontrolador Target
* **SoC / MCU**: Espressif **ESP32-S3-WROOM-1-N16R8** (Xtensa® dual-core 32-bit LX7 @ 240 MHz).
* **Memoria Interna**: 512 KB SRAM + 8 MB Octal PSRAM + 16 MB Quad SPI Flash.
* **Placa de desarrollo**: `esp32-s3-devkitc-1` (44 pines, 2×22).
* **Periféricos de Hardware Requeridos**:
  * **PCNT (Pulse Counter)**: 4 canales de hardware para lectura asíncrona de encoders sin sobrecarga de interrupciones de CPU.
  * **I²C a 400 kHz**: Pines dedicados GPIO 8 (SDA) y GPIO 9 (SCL) con sensor inercial GY-521 (MPU6050).
  * **LEDC (PWM Hardware)**: 8 canales a 20 kHz para control silencioso y lineal de puentes H DRV8833.

### 2.3. Dependencias de Librerías Fijadas (`lib_deps`)
Para garantizar compilaciones 100% deterministas y reproducibles, las versiones de las dependencias externas están congeladas y resueltas como sigue:

```ini
lib_deps =
    ESP32Async/ESPAsyncWebServer @ 3.11.2
    ESP32Async/AsyncTCP @ 3.4.10
    bblanchon/ArduinoJson @ 6.21.5
    adafruit/Adafruit MPU6050 @ 2.2.9
```

**Árbol exacto de dependencias transitivas resueltas:**
```text
Libraries
├── Adafruit MPU6050 @ 2.2.9
│   ├── Adafruit BusIO @ 1.17.4
│   ├── Adafruit GFX Library @ 1.12.6
│   ├── Adafruit SSD1306 @ 2.5.17
│   └── Adafruit Unified Sensor @ 1.1.15
├── ArduinoJson @ 6.21.5
├── AsyncTCP @ 3.4.10
└── ESPAsyncWebServer @ 3.11.2
    └── AsyncTCP @ 3.5.0
```

### 2.4. Flags de Compilación y Distribución de Cores FreeRTOS
La arquitectura de software embebido distribuye la carga entre los dos núcleos de la CPU de forma estricta:

```ini
build_flags =
    -DCORE_DEBUG_LEVEL=0
    -DCONFIG_ASYNC_TCP_RUNNING_CORE=0
    -DCONFIG_ARDUINO_RUNNING_CORE=1
    -DCONFIG_ARDUINO_LOOP_STACK_SIZE=8192
    -Os
```

* **Core 0 (`CONFIG_ASYNC_TCP_RUNNING_CORE=0`)**: Ejecuta exclusivamente la pila de comunicaciones asíncronas TCP, el servidor HTTP/WebSocket (`Task_Web`) y la serialización JSON.
* **Core 1 (`CONFIG_ARDUINO_RUNNING_CORE=1`)**: Ejecuta exclusivamente el **súper-ciclo síncrono y determinista de 100 Hz** en el `loop()` nativo. Integra lectura de PCNT/MPU, estimación de pose EKF/odometría, máquinas de estado de seguridad, cinemática diferencial y generación de PWM sin colas ni mutex bloqueantes intermedias.
* **Stack Size (8192 bytes)**: Evita condiciones de desbordamiento de pila (*stack overflow*) durante maniobras complejas de cinemática y control de torque.

---

## 3. Comandos PlatformIO (`pio`) para Compilación, Flasheo y Pruebas

Todos los comandos se ejecutan desde la raíz del repositorio clonado:

### 3.1. Preparación de Secretos de Red (Paso Previo Obligatorio)
Antes de compilar, cree su archivo local [`include/Secrets.h`](include/Secrets.h) a partir de la plantilla provista:

```bash
# En Linux/macOS:
cp include/Secrets.example.h include/Secrets.h

# En Windows (PowerShell):
Copy-Item include\Secrets.example.h include\Secrets.h
```

Configure en dicho archivo las credenciales de la red Wi-Fi o del Access Point donde se conectará el robot y la HMI.

### 3.2. Inspección de Paquetes y Dependencias
```bash
pio pkg list
```
*(Nota en Windows: si su consola utiliza codificación CP1252, exporte `$env:PYTHONIOENCODING="utf-8"` antes de ejecutar este comando).*

### 3.3. Compilación del Firmware Embebido
Compila la imagen binaria para el ESP32-S3:

```bash
pio run -e esp32-s3-devkitc-1
```
* **Métrica esperada de aceptación:**
  * Estado: `[SUCCESS]`
  * Uso de RAM: $\approx 18.7\%$ (61,312 / 327,680 bytes)
  * Uso de Flash: $\approx 46.6\%$ (915,253 / 1,966,080 bytes)

### 3.4. Flasheo / Carga del Firmware al ESP32-S3
Conecte el microcontrolador mediante cable USB (puerto USB-UART o USB-OTG nativo). Asegúrese de mantener el interruptor de potencia motriz (`VMOT 7.7V`) **apagado**:

```bash
pio run -e esp32-s3-devkitc-1 -t upload
```

### 3.5. Carga del Sistema de Archivos SPIFFS
El robot almacena su historial de torque de 10 calibraciones en una partición SPIFFS declarada en [`partitions.csv`](partitions.csv):

```bash
pio run -e esp32-s3-devkitc-1 -t uploadfs
```
*(Opcionalmente en Windows puede usar el asistente [`scripts/firmware/preparar_spiffs.ps1`](scripts/firmware/preparar_spiffs.ps1) con el parámetro `-ConfirmarVmotDesconectado`).*

### 3.6. Monitor Serial de Diagnóstico
Abre la consola serie a 115,200 baudios para visualizar el arranque de FreeRTOS, calibración inercial y dirección IP asignada:

```bash
pio device monitor -b 115200
```

### 3.7. Ejecución de la Suite de Pruebas Unitarias Nativas (Host)
Permite validar 100% de la lógica matemática, estimación de odometría, máquina de estados y lazos de seguridad **directamente en el host x86_64, sin necesidad de hardware conectado**:

```bash
pio test -e pruebas_control_ruta_nativas
```
* **Resultado de aceptación verificado:**
  * **74 casos de prueba exitosos** (`74 PASSED, 0 FAILED`).
  * Tiempo de ejecución típico: $< 2.5$ segundos.
  * Valida: invarianza rotacional en 4 cuadrantes, corrección de centro instantáneo de rotación (ICR), algoritmo de desacoplo de deslizamiento, máquina de estados de calibración MPU y filtros PCNT.

### 3.8. Pruebas Unitarias Cruzadas en Target ESP32
Para ejecutar las pruebas directamente sobre el microcontrolador físico:

```bash
pio test -e pruebas_control_ruta
```

---

## 4. Extracción Forense y Verificación de Telemetría vía ADB

El subsistema de registro almacena cada muestra de telemetría a 100 Hz, comando emitido y evento de guard en una base de datos local SQLite dentro de la tablet Android (paquete `mx.ik.robots3` / `com.carrotest3.robot`).

### 4.1. Extracción Automatizada en Windows (PowerShell)
Conecte la tablet Android a la PC mediante USB con la depuración USB habilitada y ejecute:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File android_app\extraer_db_tablet.ps1
```
El script extrae la base de datos interna hacia `tmp_db/robot.sqlite3` y genera una copia con timestamp en `tmp_db/robot_YYYYMMDD_HHMMSS.sqlite3`.

### 4.2. Extracción Automatizada en Linux / macOS (Bash)
```bash
bash scripts/pull_telemetry.sh
```

### 4.3. Extracción Manual Directa con ADB CLI
Si prefiere extraerla manualmente sin scripts envolventes:

```bash
# 1. Verificar reconocimiento del dispositivo
adb devices

# 2. Extraer la base de datos interna mediante run-as
adb exec-out run-as mx.ik.robots3 cat files/robot_s3/robot.sqlite3 > tmp_db/robot.sqlite3

# 3. Comprobar integridad física del archivo
sqlite3 tmp_db/robot.sqlite3 "PRAGMA integrity_check;"
```

### 4.4. Comandos de Inspección y Análisis Estadístico
El repositorio provee herramientas canónicas en Python para interrogar la base de datos extraída:

* **Resumen general de tablas y sesiones:**
  ```bash
  python consultar_db.py
  ```
* **Últimos 15 comandos con su estado, fase y payload:**
  ```bash
  python consultar_db.py --commands 15
  ```
* **Últimos 20 eventos del sistema y transiciones de seguridad:**
  ```bash
  python consultar_db.py --events 20
  ```
* **Muestras de telemetría a 100 Hz (pose X/Y, yaw e índices PWM):**
  ```bash
  python consultar_db.py --telemetry 10
  ```
* **Filtrado forense por palabra clave (e.g., calibraciones o fallos):**
  ```bash
  python consultar_db.py --search "calib"
  ```
* **Análisis Cuantitativo Experimental e Intervalos de Confianza al 95%:**
  ```bash
  python tools/analyze_telemetry.py
  ```
  Genera el cálculo matemático formal de error longitudinal en avance, error angular en pivote y tasa de éxito operacional sobre las muestras reales del robot.

---

## 5. Reglas Críticas de Seguridad y Operación de Hardware Embebido

Al reproducir físicamente los ensayos con el robot en banco o en suelo, respete estrictamente los siguientes dogmas y protecciones eléctricas:

1. **Aislamiento de Rieles de Alimentación:**
   * **Riel de Potencia Motriz (`7.7V`)**: 2 baterías de litio en paralelo alimentan los puentes H DRV8833 a través de un switch físico de corte (`VMOT`). **Mantenga VMOT apagado** durante el boot, carga de firmware por USB o reinicios para evitar disparos accidentales en el transitorio del Boot ROM.
   * **Riel Digital y Sensores (`5V`)**: PowerBank independiente conectado a `5VIN` del ESP32-S3, al riel alto `VCCB` del conversor de nivel lógico y a los cuatro comparadores LM393. Esto aísla los transitorios de consumo de los motores del microcontrolador.
   * **Riel Lógico de Precisión (`3.3V`)**: Proviene del regulador interno del ESP32-S3 (`3V3`) hacia el MPU6050 y la referencia baja `VCCA` del conversor de nivel.
2. **Filtrado de Ruido Back-EMF y Desacoplo:**
   * Capacitor cerámico de $100\text{ nF / 20V}$ soldado directamente entre los terminales de cada uno de los 4 motores TT.
   * Capacitor electrolítico de $\ge 100\ \mu\text{F}$ en la bornera de alimentación de cada módulo DRV8833.
3. **Límites de PWM en Firmware ([`include/Config.h`](include/Config.h) y [`src/Motores.cpp`](src/Motores.cpp)):**
   * **Avance sostenido máximo**: $242/255$ ($\approx 95\%$). Prohibido el 100% sostenido.
   * **Giro autónomo sostenido máximo**: $247/255$ ($\approx 97\%$).
   * **Ráfagas de arranque (Kickstart)**: autorizadas a $255/255$ únicamente durante transitorios de $80\text{ ms}$ (`PWM_BURST_MAX_MS`), tras lo cual el firmware repliega el PWM y exige un período de enfriamiento (`PWM_BURST_COOLDOWN_MS`).
   * **Tiempo muerto universal**: $250\text{ ms}$ forzados al invertir sentido de giro en cualquier motor para proteger los MOSFETs del puente H.
4. **Dogma de Calibración Canónica Inmutable:**
   La calibración autónoma ejecuta una máquina de 5 fases supervisada por el giróscopo MPU6050:
   1. Reposo y estabilización inercial de $5.0\text{ s}$.
   2. Búsqueda de torque positivo (`CAL_A`) con rampa y verificación de $\omega_z > 0.12\text{ rad/s}$.
   3. Validación de giro a $+25^\circ$ (`CAL_VALIDAR_25`) y reposo de $600\text{ ms}$.
   4. Reposo de $2.5\text{ s}$ y búsqueda de torque opuesto (`CAL_B`).
   5. Retorno estricto a $0^\circ$ inicial (`CAL_RETORNO`) y reseteo de pose global.
5. **Dogma de Simetría Mecánica y Polaridad 4WD:**
   * Motores delanteros montados con cuerpos hacia atrás; traseros con cuerpos hacia adelante.
   * Compensación global de avance físico: `PWM_FORWARD_POLARITY = -1`. Salida lógica positiva produce traslación hacia $+Y$ físico.

---

## 6. Esquema Completo de Conexiones Físicas y Descargas Fritzing

El cableado físico del prototipo está documentado en detalle en [`docs_markdowns/diagrama_fritzing/ESQUEMA_CONEXIONES_ESP32S3.md`](docs_markdowns/diagrama_fritzing/ESQUEMA_CONEXIONES_ESP32S3.md) e implementado en el diseño esquemático oficial de Fritzing.

### 6.1. Tabla Maestra de Asignación de Pines (ESP32-S3-DevKitC-1, 44 pines)

| Pin ESP32-S3 | Constante Firmware | Función Lógica | Destino Físico en Hardware |
| :--- | :--- | :--- | :--- |
| **GPIO 1** | `PIN_SENSOR_5V` | Monitor de alimentación 5V | Conversor TXS0108E pin **A8** (`B8` conectado a +5V) |
| **GPIO 4** | `PIN_BL_FWD` | Motor Trasero Izquierdo (Avance) | DRV8833 Izquierdo pin **IN2** |
| **GPIO 5** | `PIN_BL_REV` | Motor Trasero Izquierdo (Reversa) | DRV8833 Izquierdo pin **IN1** |
| **GPIO 6** | `PIN_FL_REV` | Motor Delantero Izquierdo (Reversa) | DRV8833 Izquierdo pin **IN4** |
| **GPIO 7** | `PIN_FL_FWD` | Motor Delantero Izquierdo (Avance) | DRV8833 Izquierdo pin **IN3** |
| **GPIO 8** | `PIN_I2C_SDA` | Bus I²C SDA (400 kHz) | GY-521 (MPU6050) pin **SDA** |
| **GPIO 9** | `PIN_I2C_SCL` | Bus I²C SCL (400 kHz) | GY-521 (MPU6050) pin **SCL** |
| **GPIO 10** | `PIN_ENC_FR` | Encoder Delantero Derecho *(Blanco)* | Conversor TXS0108E pin **A6** |
| **GPIO 11** | `PIN_ENC_FL` | Encoder Delantero Izquierdo *(Verde)* | Conversor TXS0108E pin **A5** |
| **GPIO 12** | `PIN_ENC_BL` | Encoder Trasero Izquierdo *(Negro)* | Conversor TXS0108E pin **A2** |
| **GPIO 13** | `PIN_ENC_BR` | Encoder Trasero Derecho *(Rojo)* | Conversor TXS0108E pin **A1** |
| **GPIO 15** | `PIN_BR_REV` | Motor Trasero Derecho (Reversa) | DRV8833 Derecho pin **IN1** |
| **GPIO 16** | `PIN_BR_FWD` | Motor Trasero Derecho (Avance) | DRV8833 Derecho pin **IN2** |
| **GPIO 17** | `PIN_FR_REV` | Motor Delantero Derecho (Reversa) | DRV8833 Derecho pin **IN3** |
| **GPIO 18** | `PIN_FR_FWD` | Motor Delantero Derecho (Avance) | DRV8833 Derecho pin **IN4** |
| **3V3** | — | Alimentación lógica 3.3V | GY-521 `VCC` + TXS0108E `VCCA` y `OE` |
| **5V / 5VIN** | — | Entrada 5V digital | Salida +5V del PowerBank |
| **GND** | — | Masa común de referencia | Todos los pines `GND` interconectados |

### 6.2. Mapeo de Odometría con Conversor de Nivel (TXS0108E de 8 Canales)
Los sensores ópticos de ranura LM393 se alimentan a 5V para garantizar una detección nítida en el comparador. El chip conversor traslada las señales al nivel seguro de 3.3V del ESP32-S3:

| Canal | Lado B (5V - Sensores LM393) | Lado A (3.3V - ESP32-S3) | Rueda Asignada |
| :---: | :--- | :--- | :--- |
| **Canal 1** | Pin `DO` de LM393 Trasero Derecho | Pin `A1` $\rightarrow$ **GPIO 13** | Trasera Derecha (BR) |
| **Canal 2** | Pin `DO` de LM393 Trasero Izquierdo | Pin `A2` $\rightarrow$ **GPIO 12** | Trasera Izquierda (BL) |
| **Canal 5** | Pin `DO` de LM393 Frontal Izquierdo | Pin `A5` $\rightarrow$ **GPIO 11** | Delantera Izquierda (FL) |
| **Canal 6** | Pin `DO` de LM393 Frontal Derecho | Pin `A6` $\rightarrow$ **GPIO 10** | Delantera Derecha (FR) |
| **Canal 8** | Conectado a Riel +5V de Sensores | Pin `A8` $\rightarrow$ **GPIO 1** | Testigo de Salud de Fuente 5V |

---

## 7. Descarga de Archivos Esquemáticos y Fritzing

Para inspeccionar o modificar el circuito en el software CAD **Fritzing** o en visores vectoriales, los archivos están disponibles en los siguientes enlaces directos:

### 7.1. Descarga del Archivo Nativo Fritzing (`.fzz`)
* 🌐 **[Descargar IntentDiagramRobotS3.fzz desde GitHub Pages (Descarga Directa)](https://rubenandresgome.github.io/carroTest3-For-Esp32-s3-wroom/diagrama_fritzing/IntentDiagramRobotS3.fzz)**
* 🐙 **[Ver IntentDiagramRobotS3.fzz en el Repositorio GitHub](https://github.com/RubenAndresGome/carroTest3-For-Esp32-s3-wroom/blob/main/docs_markdowns/diagrama_fritzing/IntentDiagramRobotS3.fzz)**
* 📁 **Ruta local en el repositorio clonado**: `docs_markdowns/diagrama_fritzing/IntentDiagramRobotS3.fzz` o `docs/diagrama_fritzing/IntentDiagramRobotS3.fzz`.

### 7.2. Planos Esquemáticos Complementarios (Impresión y Visualización)
* 📄 **[Plano Esquemático en PDF para Impresión](https://rubenandresgome.github.io/carroTest3-For-Esp32-s3-wroom/diagrama_fritzing/IntentDiagramRobotS3_esquematico.pdf)** (`IntentDiagramRobotS3_esquematico.pdf`)
* 📐 **[Esquemático Vectorial Escalable en SVG](https://rubenandresgome.github.io/carroTest3-For-Esp32-s3-wroom/diagrama_fritzing/IntentDiagramRobotS3_esquematico.svg)** (`IntentDiagramRobotS3_esquematico.svg`)
* 🖼️ **[Esquemático en Imagen de Alta Resolución PNG](https://rubenandresgome.github.io/carroTest3-For-Esp32-s3-wroom/diagrama_fritzing/IntentDiagramRobotS3_esquematico.png)** (`IntentDiagramRobotS3_esquematico.png`)
* 🔌 **[Netlist Eléctrico en formato XML](https://rubenandresgome.github.io/carroTest3-For-Esp32-s3-wroom/diagrama_fritzing/IntentDiagramRobotS3_netlist.xml)** (`IntentDiagramRobotS3_netlist.xml`)
* 📖 **[Documentación Técnica de Conexiones de Taller](docs_markdowns/diagrama_fritzing/ESQUEMA_CONEXIONES_ESP32S3.md)** (`ESQUEMA_CONEXIONES_ESP32S3.md`)

---

## 8. Verificación Cruzada y Protocolo de Aceptación del Evaluador

Para dar por satisfactoria la reproducción técnica, el evaluador puede seguir este checklist secuencial:

- [ ] **1. Clonación y Checkout**: Repositorio en rama `main` en su último commit (`git log -1`).
- [ ] **2. Creación de Secrets**: Archivo `include/Secrets.h` configurado.
- [ ] **3. Compilación Embebida**: `pio run -e esp32-s3-devkitc-1` termina con `[SUCCESS]` consumiendo $< 20\%$ de RAM y $< 50\%$ de Flash.
- [ ] **4. Pruebas Unitarias**: `pio test -e pruebas_control_ruta_nativas` reporta `74 succeeded in ~2s`.
- [ ] **5. Inspección de Circuito**: Verificación visual de conexiones coincidente con `ESQUEMA_CONEXIONES_ESP32S3.md` y el esquemático Fritzing.
- [ ] **6. Extracción Forense ADB**: Ejecución exitosa de `android_app/extraer_db_tablet.ps1` o `scripts/pull_telemetry.sh`.
- [ ] **7. Análisis Cuantitativo**: `python tools/analyze_telemetry.py` calcula los errores experimentales sobre `tmp_db/robot.sqlite3` con intervalos de confianza al 95%.

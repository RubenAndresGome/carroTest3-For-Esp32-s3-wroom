# Instrucciones del Repositorio para Claude Code (CLAUDE.md)

## Visión General del Proyecto

`carroTest3-For-Esp32-s3-wroom` es un sistema robótico móvil 4WD diferencial gobernado por un microcontrolador **ESP32-S3 WROOM** (DevKitC-1-N8), con lazos cerrados en tiempo real mediante giróscopo/acelerómetro **MPU6050** (I2C) y encoders ópticos en las 4 ruedas mediante **PCNT** por hardware.

El sistema utiliza una arquitectura dual:
- **Firmware C++ / Arduino / FreeRTOS (`src/`, `include/`)**: Control de tiempo real en Core 1 (100 Hz síncrono), límites eléctricos DRV8833, odometría, estimación ICR y parada segura (E-STOP). Servidor WebSocket en Core 0.
- **Backend Canónico Python (`desktop_app/`)**: Flask + Waitress. Único propietario de la conexión WebSocket en Windows. Gestiona misiones complejas (ortogonales, polares, regreso Ockham), almacenamiento histórico en SQLite y sirve el HMI web.
- **Aplicación Android (`android_app/`)**: Empaqueta el backend canónico de Python mediante **Chaquopy** y presenta el HMI en un `WebView` a pantalla completa en la tablet Samsung Galaxy Tab.
- **HMI Modular (`desktop_app/robot_app/hmi/`)**: Interfaz web modular desacoplada en tarjetas HTML (`cards/`), hojas CSS (`css/`) y módulos JavaScript (`js/`), ensamblados dinámicamente con CSP nonce.

---

## Comandos Esenciales de Compilación y Prueba

Todas las suites deben ejecutarse en Windows PowerShell desde la raíz del repositorio:

### 1. Compilación de Firmware ESP32-S3
```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e esp32-s3-devkitc-1
```

### 2. Pruebas Unitarias Nativas de Cinemática y Control (74 tests)
```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" test -e pruebas_control_ruta_nativas
```

### 3. Pruebas Unitarias de Backend Python (99 tests)
```powershell
$env:PYTHONPATH="desktop_app"; & desktop_app\.test-venv\Scripts\python.exe -m unittest discover desktop_app/tests
```

### 4. Pruebas de Frontend Vitest (11 tests de contrato responsive, ARIA y CSP)
```powershell
npm --prefix desktop_app/frontend test
```

### 5. Verificación de Sincronización del HMI
```powershell
& desktop_app\.test-venv\Scripts\python.exe desktop_app/robot_app/hmi/compilar_hmi.py --check
```

### 6. Inspección de Base de Datos SQLite (Sesiones extraídas por ADB)
```powershell
python consultar_db.py                       # Resumen general de tablas y sesiones
python consultar_db.py --commands 15        # Últimos 15 comandos con estado y payload
python consultar_db.py --events 20          # Últimos 20 eventos del sistema
python consultar_db.py --telemetry 10       # Muestras de telemetría (pose, yaw, PWM)
python consultar_db.py --search "calib"     # Búsqueda por patrón
```

### 7. Actualización del Grafo de Conocimiento (Memoria de Código)
```powershell
cmd /c "scripts\desarrollo\actualizar_memoria_codigo.bat"
```

---

## Reglas Críticas de Arquitectura y Seguridad

### 1. Modelo de Ejecución en ESP32-S3
- **Core 0**: Exclusivo para `Task_Web` (Wi-Fi, HTTP y WebSocket asíncrono).
- **Core 1**: Exclusivo para el súper-ciclo síncrono nativo de 100 Hz (`loop()`). En cada ciclo consume la misma muestra atómica de PCNT, MPU, pose, cinemática y modulación PWM, sin tareas ni colas intermedias.
- **Lectura de Encoders**: Se realiza exclusivamente mediante hardware **PCNT** del ESP32-S3. Queda prohibido usar interrupciones `attachInterrupt` para leer encoders.

### 2. Protección Eléctrica del Driver DRV8833
- **PWM Máximo Continuo en Avance**: 242/255 (~95%). Giros autónomos, calibración y pivote continuo conservan 247/255 (~97%) para superar fricción en superficies difíciles.
- **Ráfagas al 100% (255/1023)**: Autorizadas únicamente para transitorios acotados (`PWM_BURST_MAX_MS = 80 ms`) como kickstart, desenclave de engranajes TT o micro-pulsos. Agotada la ráfaga, repliega a nivel continuo y exige `PWM_BURST_COOLDOWN_MS` de enfriamiento.
- **Zona Muerta de Arranque**: Actúa como **piso dinámico** solo por debajo de `PWM_DEADBAND_UMBRAL_CRUCERO` (180/255); nunca se suma sobre crucero.
- **Tiempo Muerto Universal**: 250 ms en `Motores.cpp:aplicarVelocidades()` al invertir sentido de giro. Aplica a joystick manual, giros autónomos y calibración.
- **Watchdog de Stall Manual**: `MANUAL_STALL_TIMEOUT_MS = 2500 ms` en `Config.h`. Permite absorber el tiempo muerto de 250 ms y modulaciones suaves de joystick sin disparar falso positivo de bloqueo en avance/reversa.

### 3. Dogma Canónico de Calibración
La rutina `calibrate` es dogma inmutable con el MPU6050 como autoridad angular única:
1. Cuenta regresiva y reposo de 5.0 s para estabilizar el filtro del giróscopo MPU6050.
2. Búsqueda de torque inicial en Polaridad Positiva (`CAL_A`): rampa de 140 a 247/255 hasta confirmar `ticksOk` bilateralmente y `fabsf(gyro_z) >= 0.12 rad/s` sostenido 100 ms. Si el giro inicial resulta negativo, invierte `candidatoCal` con pausa de 750 ms hasta validar polaridad positiva `gyro_z > 0`.
3. Validación de giro a +25° (`CAL_VALIDAR_25`): pivote puro hasta alcanzar `yawInicio + 25°` (±2.5°) con reposo de asentamiento de 600 ms.
4. Reposo de 2.5 s (`CAL_PAUSA`) y búsqueda de torque en Polaridad Opuesta (`CAL_B`): valida movimiento bilateral con signo opuesto y confirma `candidatoGiroPos != candidatoGiroNeg`.
5. Reposo de 2.5 s (`CAL_PAUSA_RETORNO`) y retorno estricto (`CAL_RETORNO`): pivote puro hacia el `yawInicioCalDeg` original, reseteando odometría (X=0, Y=0) y yaw de IMU al estabilizarse.

### 4. Dogma de Simetría Mecánica 4WD y Mapeo Físico
- Los reductores TT delanteros (FL/FR) y traseros (BL/BR) están montados en pares opuestos invertidos $180^\circ$. La polaridad lógica compensa esta simetría para que `FWD = 1` produzca traslación positiva pura hacia $+Y$.
- **Compensación Global**: `PWM_FORWARD_POLARITY = -1` (frente físico concuerda con plano coordenado $+Y$ del HMI).
- **Mapeo de Pines Físicos**:
  - Motores: FL (FWD=GPIO7, REV=GPIO6), BL (FWD=GPIO4, REV=GPIO5), FR (FWD=GPIO18, REV=GPIO17), BR (FWD=GPIO16, REV=GPIO15).
  - Encoders (TXS0108E): FL (GPIO11, verde), FR (GPIO10, blanco), BL (GPIO12, negro), BR (GPIO13, rojo).
  - IMU MPU6050: SDA=GPIO8, SCL=GPIO9.

### 5. Misiones Ockham (Retorno a Origen)
- Desanda waypoints en orden inverso ($W_N \to \dots \to W_0$) mediante avance frontal (`drive_mode: 'forward'`) para máxima tracción 4WD y control por giróscopo.
- Incluye simulación visual previa en HMI (`#route-ockham-confirm-box`).
- Etapa de reposo (settling) de 500 ms tras cada tramo para evaluar exactitud (errores longitudinal, lateral y angular) y registrarla en SQLite.
- Realigning ortogonal con micro-giros si el desvío angular supera 1.5° o el error lateral supera 2.5 cm.

---

## Estructura del HMI Modular (`desktop_app/robot_app/hmi/`)

El HMI no es monolítico; reside en una estructura modular limpia:
- **`index.html`** / **`template.html`**: Esqueleto ligero (24-25 líneas) con metadatos de viewport accesibles y marcadores de montaje.
- **`cards/`**: Tarjetas HTML independientes:
  - `header.html`: Cabecera, estados de conexión y botón de E-STOP.
  - `tab_conexion.html`: Conexión WebSocket, gestión de superficies y sesiones.
  - `tab_mapa.html`: Visualizador de trayectoria con Chart.js.
  - `tab_testing.html`: Control técnico de actuadores y monitoreo de pines.
  - `tab_touch.html`: Joystick táctil y reproductor de rutas manuales grabadas.
  - `tab_rutas.html`: Programador de rutas (ortogonal, polar, vectorial, Ockham).
  - `footer_panels.html`: FSM Flow, visor de logs técnicos y telemetría en vivo.
- **`css/`**: Hojas de estilo por dominio (`theme.css`, `layout.css`, `components.css`, `touch.css`, `responsive.css`).
- **`js/`**: Módulos JavaScript (`config.js`, `ws_control.js`, `calibration.js`, `telemetry.js`, `route_manager.js`, `tab_manager.js`, `touch.js`, `main.js`).
- **`web.py`**: Sirve rutas `/css/<filename>` y `/js/<filename>`, y en `/` ensambla dinámicamente en memoria con Content Security Policy (`nonce-{nonce}`) y `app-token`.

---

## Convenciones de Desarrollo y Código

1. **Idioma**:
   - Documentación, mensajes de commit, scripts y cadenas de la interfaz de usuario se redactan en **español**.
   - Módulos internos de Python y APIs de backend usan nomenclatura idiomática en **inglés** (`domain`, `services`, `gateway`).
2. **Archivos Excluidos**:
   - Nunca editar ni confirmar en git: `.pio/`, `desktop_app/build/`, `desktop_app/dist/`, `desktop_app/.test-venv/`, `node_modules/`, `tmp_db/`, ni `include/Secrets.h`.
3. **Commits**:
   - Seguir el estándar Conventional Commits (`feat`, `fix`, `docs`, `refactor`, `test`, `chore`) con viñetas explicativas del diagnóstico, cambios implementados y pruebas superadas.

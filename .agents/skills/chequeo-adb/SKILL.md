---
name: chequeo-adb
description: Extrae y analiza la base de datos SQLite y telemetría de la tablet Android vía ADB para el robot ESP32. Actívala siempre que el usuario pida "checar ADB", "revisar ADB", "analizar sesión de la tablet", o investigar fallos ocurridos durante pruebas físicas en el robot.
---

# Chequeo y Diagnóstico vía ADB

Esta skill define el procedimiento estandarizado para extraer e inspeccionar la telemetría, eventos y comandos almacenados en la tablet Android del robot ESP32 mediante ADB.

## 1. Extracción de la Base de Datos

Ejecutar el script canónico de extracción desde la raíz del repositorio:
```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File android_app\extraer_db_tablet.ps1
```
Este script extrae `robot.sqlite3` desde `/sdcard/Android/data/com.carrotest3.robot/files/` (o la ruta de almacenamiento de la app) hacia la carpeta local `tmp_db/robot.sqlite3`.

Si ADB no detecta el dispositivo:
1. Verificar conectividad: `adb devices`
2. Si está desconectado o no autorizado, notificar al usuario para reconectar el cable USB o autorizar depuración USB.

## 2. Inspección Rápida con la CLI Canónica

Usar `consultar_db.py` con el entorno Python de pruebas:
```powershell
& 'desktop_app\.test-venv\Scripts\python.exe' consultar_db.py
```
Opciones útiles:
- `--commands 20`: Últimos 20 comandos con su payload y estado.
- `--events 30`: Últimos 30 eventos con severidad y detalle.
- `--telemetry 25`: Muestras recientes de odometría, orientación e índices PWM.
- `--search "<texto>"`: Filtrar registros por término clave (ej. `calib`, `fail`, `guard`, `mismatch`, `stall`).

## 3. Análisis Forense Detallado de Sesión

Para inspeccionar una sesión específica (ej. la última sesión registrada o un aborto):
1. Obtener el `session_id` más reciente de la tabla `sessions`.
2. Analizar la secuencia temporal de telemetría alrededor del fallo:
   - `ticks` por rueda (FL, FR, BL, BR) y deltas por ciclo.
   - `gyro_z` filtrado y `yaw` integrado del MPU6050.
   - Estado de la máquina de estados (`faseComando`, `progreso`).
   - Motivo exacto del guard en `diagnosticoCal` o evento `FAILED`.
3. Distinguir causas raíz:
   - `cal_yaw_sign_mismatch`: Dirección física de giro opuesta a la lógica esperada.
   - `cal_rotation_not_confirmed`: Exceso de ticks de rueda sin que el MPU alcance velocidad o ángulo mínimo.
   - `cal_pivot_unbalanced`: Disparidad excesiva entre tren izquierdo y derecho.
   - `cal_origin_drift`: Desplazamiento lineal del centro de masa superior a 1.0 cm.
   - `cal_stall_left` / `cal_stall_right`: Falta de respuesta en saturación de PWM.

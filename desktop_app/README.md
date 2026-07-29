# App local Robot ESP32-S3 (`desktop_app/`)

La aplicación local backend/HMI separa la interfaz de usuario y la persistencia de datos del lazo de tiempo real del firmware. El navegador web utiliza exclusivamente HTTP REST y Server-Sent Events (SSE) contra `127.0.0.1:8080`; Python mantiene la única conexión WebSocket activa hacia `ws://192.168.4.1/ws` y almacena sesiones, historial y eventos en SQLite.

---

## Arquitectura de Propiedad y Componentes

- **`desktop_app/`** es la fuente canónica del HMI local.
- El backend de Windows es el único propietario del WebSocket hacia el robot cuando se opera desde la PC.
- [`android_app/`](../android_app/) empaqueta exactamente esta misma base y asume la titularidad única de la conexión cuando se opera desde la tablet.
- No se debe operar con ambos controladores conectados al robot de forma simultánea.

```text
desktop_app/robot_app/
├── domain.py         Modelos de dominio, validaciones e inmutabilidad (RobotCommand, RobotState, MissionPlan)
├── gateway.py        RobotGateway: cliente WebSocket monoprobador en hilo dedicado con reconexión y cola prioritaria
├── services.py       RobotFacade, MissionService, EventHub y TelemetryRecorder
├── database.py       Gestor de persistencia SQLite (sessions, commands, events, telemetry)
├── web.py            Rutas Flask REST API y endpoints SSE / Socket.IO
└── app_factory.py    Ensamblaje y factoría del servidor Flask/Waitress
```

---

## Especificación de Objetos del Backend Python

| Objeto / Clase | Módulo | Responsabilidad |
| :--- | :--- | :--- |
| `RobotCommand` | [`domain.py`](robot_app/domain.py) | Objeto de valor inmutable con validación estricta de payloads y generación de envoltura JSON V2. |
| `TelemetrySnapshot` | [`domain.py`](robot_app/domain.py) | Snapshot inmutable de telemetría reportada por el ESP32 (pose, yaw, pulsos, PWM, salud MPU). |
| `RobotGateway` | [`gateway.py`](robot_app/gateway.py) | Administrador de conexión WebSocket cliente con PriorityQueue (`_Outgoing`) y backoff exponencial. |
| `RobotFacade` | [`services.py`](robot_app/services.py) | Fachada principal de casos de uso (conexiones, envío de comandos, inicio de misiones y consulta de estado). |
| `EventHub` | [`services.py`](robot_app/services.py) | Broker pub-sub thread-safe para transmisión de eventos en tiempo real a clientes Web. |
| `TelemetryRecorder` | [`services.py`](robot_app/services.py) | Worker en hilo secundario para persistencia asíncrona de telemetría en SQLite sin latencia HTTP. |
| `Database` | [`database.py`](robot_app/database.py) | Gestión de tablas SQLite (`tmp_db/robot.sqlite3` o `tablet_robot.sqlite3`). |

Para la especificación UML y detallada de atributos, consulte:
- [Diagrama del Sistema General UML](../DIAGRAMA_SISTEMA_GENERAL.md)
- [Especificación de Objetos del Sistema](../docs/especificacion_objetos_sistema.md#1-especificación-de-objetos-del-servidor-python-desktop_app--android_app)

---

## Funcionamiento General

1. El usuario arranca el HMI local en Windows mediante `INICIAR_ROBOT.bat` o ejecutable.
2. El servidor Flask / Waitress se inicializa en `127.0.0.1:8080`.
3. `RobotGateway` establece el socket hacia `ws://192.168.4.1/ws` en la red Wi-Fi `ROBOT_S3_LOCAL`.
4. La HMI valida el estado del robot y bloquea misiones hasta completar la calibración obligatoria con ruedas elevadas (+25° yaw, reposo 2.5 s, retorno a 0°).
5. Las misiones de navegación se descomponen en tramos ortogonales de hasta 2 metros (`split_segment_mm` / `decompose_to_orthogonal_steps`).

---

## Instalación y Ejecución en Windows

### 1. Instalación Online
```powershell
powershell -ExecutionPolicy Bypass -File scripts\install_online.ps1
```

### 2. Ejecución Local
Conectarse a la red Wi-Fi del robot (`ROBOT_S3_LOCAL`) y ejecutar:
```text
scripts\run.bat
```

### 3. Generación del Ejecutable Autónomo (`dist\RobotS3.exe`)
```powershell
powershell -ExecutionPolicy Bypass -File scripts\build_exe.ps1
```
El ejecutable binario `dist\RobotS3.exe` empaqueta Flask, Waitress, el cliente WebSocket y las migraciones SQLite en un solo ejecutable sin requerir Python instalado en el equipo final.

---

## Desarrollo del Frontend (Opcional Vite / Node)

Node, Vite y TypeScript están preparados en `.tools/` (excluido de Git) para desarrollo de interfaz moderna.

```powershell
# Preparar entorno frontend
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\frontend\preparar_frontend.ps1

# Servidor de desarrollo con Hot Reload (127.0.0.1:5173)
scripts\frontend\desarrollar_frontend.bat
```

# Robot S3 HMI para Android (`android_app/`)

Aplicación Android autónoma para controlar el ESP32 sin necesidad de una PC. Empaqueta el backend Python canónico de [`desktop_app/`](../desktop_app/) mediante Chaquopy y muestra la interfaz HMI dentro de una `WebView` nativa.

---

## Arquitectura de Ejecución en Android

```text
Galaxy Tab / APK
├── WebView (Android UI) → HTTP REST / SSE → http://127.0.0.1:8080
├── Servidor Python Interno (Flask + Waitress 3.0.2 vía Chaquopy 17)
│   ├── RobotFacade & MissionService
│   ├── TelemetryRecorder & EventHub
│   └── RobotGateway (Único Propietario WebSocket Activo)
├── SQLite Privado Android (/data/data/com.robots3.hmi/databases/tablet_robot.sqlite3)
└── RobotGateway → ws://192.168.4.1/ws → Firmware ESP32-S3
```

---

## Principio de Propiedad Única del WebSocket

- La fuente canónica de la lógica de backend vive en [`desktop_app/robot_app/`](../desktop_app/robot_app/).
- Antes de cada compilación, Gradle ejecuta la tarea `syncPythonBackend` que sincroniza el código fuente hacia `app/build/generated/python/`.
- El APK de Android asume la **propiedad única del WebSocket** cuando se opera desde la tablet Galaxy Tab.
- **Regla Estricta**: No se deben tener abiertos simultáneamente el controlador de Windows y el de Android hacia el mismo robot.

Para conocer la especificación detallada de objetos Python y diagramas UML:
- [Diagrama de Sistema General UML](../DIAGRAMA_SISTEMA_GENERAL.md)
- [Especificación de Objetos del Sistema](../docs/especificacion_objetos_sistema.md#1-especificación-de-objetos-del-servidor-python-desktop_app--android_app)

---

## Requisitos y Compatibilidad

- **SO Android**: Android 7.0 / API 24 o posterior.
- **Arquitectura Target**: `arm64-v8a` (Optimizado para Samsung Galaxy Tab).
- **Entorno Python**: Python 3.13 empaquetado con Chaquopy 17.
- **Tamaño APK Debug**: ~20.6 MiB.

---

## Compilación e Instalación

### Requisitos de Construcción
- Android Studio con SDK 36 y JBR 17/21.
- Python 3.13 en la PC de compilación.

### Compilar APK desde PowerShell
```powershell
cd android_app
.\build_apk.ps1
```
El archivo instalable se genera en `android_app/app/build/outputs/apk/debug/app-debug.apk`.

### Instalar en Tablet mediante USB ADB
```powershell
cd android_app
.\install_tablet.ps1
```

---

## Operación en Tablet Samsung

1. Conectar la tablet a la red Wi-Fi `ROBOT_S3_LOCAL`.
2. Abrir la aplicación **Robot S3 HMI**.
3. Confirmar la dirección IP `192.168.4.1` y pulsar **Conectar**.
4. Realizar la calibración obligatoria de torque con ruedas elevadas (+25° yaw, reposo 2.5 s, retorno a 0°) antes de iniciar misiones de navegación.

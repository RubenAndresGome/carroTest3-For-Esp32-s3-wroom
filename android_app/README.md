# Robot S3 HMI para Android

Aplicación Android autónoma para controlar el ESP32 sin una PC. Empaqueta el
backend Python actual mediante Chaquopy y muestra el mismo HMI dentro de una
WebView.

## Arquitectura

```text
Galaxy Tab / APK
├── WebView → http://127.0.0.1:8080
├── Waitress + Flask
├── RobotService y máquina de misiones
├── SQLite privado de Android
└── RobotGateway → ws://192.168.4.1/ws → ESP32
```

La fuente canónica del backend/HMI sigue siendo [desktop_app/](../desktop_app/).
El APK de Android empaqueta esa misma base y asume la titularidad única del
WebSocket del robot cuando se opera desde la tablet. No conviene abrir la ruta
Windows y la ruta Android sobre el mismo robot a la vez.

## Funcionamiento

1. La tablet se conecta al Wi-Fi local `ROBOT_S3_LOCAL`.
2. La WebView abre el backend local en `127.0.0.1:8080`.
3. El servicio Python interno conecta el robot con `ws://192.168.4.1/ws`.
4. La aplicación gestiona misión, historial, estado y seguridad sin depender del
   hardware de la PC.

El servidor Flask escucha solamente en loopback. Otros equipos de la red no
pueden abrirlo y el APK conserva un único propietario del WebSocket del robot.

`desktop_app/robot_app/` continúa siendo la fuente canónica. Antes de cada
compilación, la tarea `syncPythonBackend` copia el backend, HMI y migraciones al
directorio generado `app/build/generated/python/`; nunca deben editarse allí.

## Compatibilidad y tamaño

- Android 7.0/API 24 o posterior.
- APK final: `arm64-v8a`, apropiado para tablets Samsung modernas.
- Python 3.13, Flask 3.1.3, Waitress 3.0.2 y Chaquopy 17.
- Tamaño debug medido el 2026-07-22: aproximadamente 20.6 MiB.
- La instalación ocupa más espacio por las bibliotecas Python extraídas y la
  base SQLite, pero sigue siendo pequeña para una Galaxy Tab.

## Compilar

Requisitos de construcción:

- Android Studio con SDK 36.
- JDK 17 o 21; el script usa el JBR de Android Studio si está disponible.
- Python 3.13 en la computadora de compilación.
- Internet la primera vez para descargar Chaquopy y las ruedas Android.

Desde PowerShell:

```powershell
cd android_app
.\build_apk.ps1
```

El APK instalable queda en:

```text
android_app/app/build/outputs/apk/debug/app-debug.apk
```

## Instalar por USB

1. En la tablet activa Opciones de desarrollador y Depuración USB.
2. Conecta la tablet y acepta la huella RSA.
3. Ejecuta:

```powershell
cd android_app
.\install_tablet.ps1
```

También se puede copiar `app-debug.apk` a la tablet y permitir temporalmente la
instalación de aplicaciones desconocidas para el gestor de archivos utilizado.

## Operación

1. En Android selecciona manualmente el Wi-Fi `ROBOT_S3_LOCAL`.
2. Acepta permanecer conectado aunque la red no tenga Internet.
3. Abre **Robot S3 HMI**.
4. Conserva `192.168.4.1` y pulsa **Conectar**.
5. Ejecuta la autoevaluación y recentra la pose igual que en Windows.

El servicio en primer plano mantiene CPU y Wi-Fi activos cuando la pantalla se
apaga. Samsung puede aplicar optimización adicional: conviene configurar Robot
S3 HMI como aplicación **Sin restricciones** en Batería. La notificación
permanente permite detener el backend explícitamente. Si se detiene durante una
maniobra, el watchdog de comunicaciones del ESP32 debe frenar el robot.

La base se guarda en el directorio privado de la aplicación. Desinstalar el APK
elimina ese historial. Los CSV/JSON exportados quedan en la carpeta de Descargas
privada de Robot S3 HMI y la app muestra su ruta completa.

## Validación realizada

- Compilación ARM64 correcta.
- Dependencias Python Android resueltas correctamente.
- Arranque real en emulador Android 36/x86-64.
- Creación de `robot.sqlite3` y ejecución de migraciones.
- Waitress disponible en `127.0.0.1:8080`.
- HMI y recursos locales renderizados en WebView.

La conexión física Wi-Fi/WebSocket y las maniobras deben validarse en la Galaxy
Tab conectada a `ROBOT_S3_LOCAL`; el emulador no sustituye esa prueba.

# App local Robot ESP32-S3

La app separa la interfaz y la persistencia del firmware. El navegador sólo usa
HTTP y Server-Sent Events contra `127.0.0.1`; Python mantiene la única conexión
WebSocket con `ws://192.168.4.1/ws` y guarda sesiones opcionales en SQLite.
La interfaz activa vive en `robot_app/hmi/`; no depende del mockup archivado.

## Arquitectura de propiedad

- [desktop_app/](.) es la fuente canónica del HMI local.
- El controlador de Windows debe ser el único propietario del WebSocket hacia el
  robot cuando se opera desde la PC.
- [android_app/](../android_app/) empaqueta exactamente la misma base y, al
  operar desde la tablet, asume la titularidad única de esa conexión.
- No conviene abrir ambos controladores a la vez sobre el mismo robot.

## Funcionamiento

1. El usuario arranca el HMI local desde Windows.
2. El backend Flask y Waitress quedan en `127.0.0.1:8080`.
3. La interfaz entrega comandos y misión al robot.
4. El firmware del ESP32-S3 responde con telemetría, pose y estado de seguridad.
5. Python registra sesiones, historial y validaciones en SQLite sin intervenir en
   el lazo temporal del microcontrolador.

## Windows

1. Con Internet una sola vez: `powershell -ExecutionPolicy Bypass -File scripts\install_online.ps1`.
2. Para preparar el paquete sin Internet: ejecuta `scripts\download_wheels.ps1` y conserva `vendor\wheels`.
3. En otro equipo: `scripts\install_offline.bat`.
4. Conecta Windows al Wi-Fi del carrito y ejecuta `scripts\run.bat`.

## Ejecutable Windows sin Internet

El panel puede empaquetarse como `dist\RobotS3.exe`. El ejecutable incluye
Flask, Waitress, el cliente WebSocket, la interfaz y las migraciones SQLite;
no necesita una instalación de Python en la computadora donde se ejecuta.

1. En una PC con Internet ejecuta `scripts\download_wheels.ps1` una vez.
2. Construye normalmente con `scripts\build_exe.ps1` o, sin Internet, con
   `scripts\build_exe_offline.bat`.
3. Copia solamente `dist\RobotS3.exe` a la HP Victus y ejecútalo.

El proceso usa un entorno separado `.build-venv` y no altera `.venv`. El panel
escucha solo en `127.0.0.1:8080`; la comunicación con el robot permanece en la
red local `ws://192.168.4.1/ws` y no requiere salida a Internet.

La base se guarda en el directorio de datos de usuario de `RobotS3`, no dentro
del repositorio. Cada conexión abre una sesión y registra telemetría. Después
de cada encendido, la HMI bloquea las rutas hasta completar, con ruedas elevadas
y corriente limitada, la calibración física de +25° y retorno a 0°. Python
divide rutas en pasos `robot-s3-steps-v2` de hasta 2 m y envía uno a la vez.

## Frontend futuro con Node

Node, Vite y TypeScript están preparados como herramientas de desarrollo, pero
no reemplazan Flask ni se ejecutan para manejar el robot. Node se instala de
forma portátil en `.tools/`, carpeta excluida de Git.

Preparación inicial:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\frontend\preparar_frontend.ps1
```

Desarrollo con recarga automática:

```text
scripts\frontend\desarrollar_frontend.bat
```

Validación completa:

```text
scripts\frontend\compilar_frontend.bat
```

El servidor de Vite usa `127.0.0.1:5173` y reenvía `/api` hacia Flask en
`127.0.0.1:8080`. El resultado compilado se genera en
`robot_app/static/vite-dist/`, pero la IU Flask actual seguirá activa hasta que
se apruebe explícitamente la migración.

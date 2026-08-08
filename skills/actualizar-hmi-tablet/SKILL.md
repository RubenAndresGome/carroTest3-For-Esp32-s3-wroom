---
name: actualizar-hmi-tablet
description: Compila e instala de forma repetible el HMI Android Robot S3 en una tablet USB autorizada. Usar cuando cambie desktop_app/robot_app (incluido el HMI), se necesite generar el APK debug, actualizar la tablet o comprobar que el paquete mx.ik.robots3 quedó instalado; no usar para operar simultáneamente el controlador Windows y la tablet contra el robot.
---

# Actualizar HMI de tablet

Usar esta skill para empaquetar la fuente canónica de `desktop_app/robot_app/` en el APK Android e instalarlo por ADB. La tarea Gradle `syncPythonBackend` incorpora el backend y el HMI antes de compilar.

## Procedimiento

1. Mantener la tablet conectada por USB con depuración autorizada. No iniciar el controlador Windows contra el ESP32 mientras se vaya a usar la tablet.
2. Ejecutar `scripts/actualizar_hmi_tablet.ps1` desde PowerShell. El script localiza un JDK 17+ y Android SDK, compila `assembleDebug`, exige una sola tablet autorizada e instala el APK con `adb install -r`.
3. Para sólo generar el APK, usar `-SoloCompilar`. Para más de una tablet, indicar `-Serial <serie-adb>`.
4. Confirmar que la salida termina con la ruta `package:` de `mx.ik.robots3`. El script no abre la aplicación ni inicia una conexión al robot.

## Seguridad y alcance

- Editar el HMI en `desktop_app/robot_app/hmi/`; no en los directorios `android_app/app/build/` generados.
- Conservar los datos privados de la aplicación al instalar: el script emplea `adb install -r`, sin desinstalar ni borrar datos.
- No afirmar la aceptación física por una instalación correcta. La calibración, la prueba de corriente y las pruebas de ruta siguen requiriendo el protocolo físico.

## Recurso

- `scripts/actualizar_hmi_tablet.ps1`: actualización reproducible con comprobaciones de JDK, SDK, ADB, dispositivo, APK e instalación.

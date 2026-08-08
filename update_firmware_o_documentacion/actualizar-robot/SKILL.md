---
name: actualizar-robot
description: Actualiza de forma repetible la HMI Android Robot S3 o el firmware ESP32-S3 usando los scripts Python incluidos. Usar cuando cambie desktop_app/robot_app, se requiera compilar e instalar la APK por ADB, se cambie src/ o include/ del firmware, o se necesite una carga segura con PlatformIO y puerto serie explícito.
---

# Actualizar Robot

Usar los dos scripts Python de esta carpeta para convertir la actualización de la HMI y del firmware en operaciones repetibles, verificables y con las precauciones físicas del robot.

## Elegir la actualización

- Si cambió `desktop_app/robot_app/` o el HMI, ejecutar `scripts/actualizar_hmi_tablet.py`. Compila la APK, exige una tablet ADB autorizada e instala sin borrar los datos de la aplicación.
- Si cambió `src/` o `include/`, ejecutar `scripts/actualizar_firmware_esp32.py --port COMx --confirmar-vmot-apagado`. Primero compila y luego carga al ESP32-S3.
- Para sólo validar compilación, añadir `--solo-compilar`. No se instala ni se carga hardware.

Ejecutar desde la raíz del repositorio con Python 3:

```powershell
python update_firmware_o_documentacion\actualizar-robot\scripts\actualizar_hmi_tablet.py
python update_firmware_o_documentacion\actualizar-robot\scripts\actualizar_firmware_esp32.py --port COM5 --confirmar-vmot-apagado
```

## Flujo de HMI

1. Conectar una sola tablet por USB y autorizar la depuración ADB.
2. Cerrar el controlador Windows antes de utilizar la tablet como controlador del robot; ambos no pueden poseer el WebSocket a la vez.
3. Ejecutar el script de HMI. Gradle sincroniza la fuente canónica antes de crear `app-debug.apk`.
4. Confirmar que informa `HMI instalada` y abrir la aplicación manualmente.

Opciones útiles: `--solo-compilar`, `--sin-compilar` y `--serial <serie-adb>`.

## Flujo de firmware

1. Desconectar VMOT o apagar físicamente los motores; dejar el ESP32 alimentado sólo por USB durante la carga.
2. Conectar un cable USB de datos. En el lanzador gráfico usar **Actualizar lista** y elegir el COM que muestra la descripción de Windows (por ejemplo, `USB-Enhanced-SERIAL CH343 (COM5)`); en CLI usar `--listar-puertos`. Cerrar monitores serie que lo ocupen.
3. Ejecutar el script con un puerto explícito y `--confirmar-vmot-apagado`. Esta confirmación es obligatoria para cargar.
4. Si aparece `No serial data received`, mantener **BOOT**, pulsar **RST** una vez, soltar **RST** y después **BOOT**; volver a intentar. Verificar también cable de datos y que el puerto elegido sea el del CH343/ESP32-S3.
5. Tras una carga correcta, mantener VMOT apagado hasta que el ESP32 esté estable, PWM esté en cero y se haya cumplido la prueba de corriente física.

No usar una compilación ni una carga correcta como evidencia de seguridad, calibración o precisión física.

## Recursos

- `scripts/actualizar_hmi_tablet.py`: crea e instala la APK de la HMI por ADB.
- `scripts/actualizar_firmware_esp32.py`: compila y carga firmware por PlatformIO, con precondición explícita de VMOT apagado.

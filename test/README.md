# Pruebas de firmware

`test_control_ruta` valida cálculos sin acceso a motores: signo de la
corrección lateral, PI acotado/anti-windup, criterio de endpoint e intentos de
recuperación. Compilarlo sin cargar ni ejecutar en el robot:

```powershell
C:\Users\IK\.platformio\penv\Scripts\platformio.exe test -e pruebas_control_ruta --without-uploading --without-testing
```

Para ejecutarlo se requiere un ESP32-S3 conectado; el binario de prueba no
inicializa ni energiza motores. Las pruebas manuales que reemplazan
temporalmente el firmware viven en `src/` y deben tener un nombre
`test_<función>.cpp`; solamente una puede permanecer activa.

`test_control_ruta_nativa` ejecuta las comprobaciones de ruta y los predicados
de seguridad (IMU, fusión robusta de encoder, stall y E-STOP) en el host, sin USB,
Arduino ni PWM:

```powershell
C:\Users\IK\.platformio\penv\Scripts\platformio.exe test -e pruebas_control_ruta_nativas
```

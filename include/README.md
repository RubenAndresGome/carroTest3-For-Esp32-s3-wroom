# Cabeceras y Configuración del Firmware (`include/`)

Este directorio contiene las definiciones de tipos, constantes de configuración, estructuras de datos y declaraciones de interfaz C++ para el firmware modular del ESP32-S3.

---

## Archivos de Cabecera

| Cabecera | Descripción y Contenido |
| :--- | :--- |
| [`Config.h`](Config.h) | Definición central de constantes del sistema: dimensiones físicas, constantes PD, puertos GPIO, límite de avance 242/255, tiempos muertos (250 ms) y umbrales de seguridad. |
| [`Estado.h`](Estado.h) | Enumeración `EstadoSistema` (`DESARMADO`, `CALIBRANDO`, `LISTO`, `EJECUTANDO`, `PAUSADO`, `FALLO`) y variables de progreso. |
| [`Comandos.h`](Comandos.h) | Structs y enumeraciones para la cola de comandos entrantes de red (`ComandoRed`, `TipoComando`). |
| [`Eventos.h`](Eventos.h) | Structs y enumeraciones para la cola de eventos salientes hacia el cliente (`EventoRed`, `TipoEventoRed`). |
| [`Motores.h`](Motores.h) | Declaración del controlador del puente H DRV8833 y protecciones de hardware. |
| [`Sensores.h`](Sensores.h) | Declaración de `SensorSnapshot` y funciones de lectura síncrona PCNT/MPU. |
| [`PoseEstimator.h`](PoseEstimator.h) | Clase `PoseEstimator` para el tracking de posición global $(x, y, \theta)$. |
| [`Cinematica.h`](Cinematica.h) | Métodos de control cinemático para avance recto y giro pivote continuo. |
| [`Seguridad.h`](Seguridad.h) | Clase `WatchdogSeguridadClass` para prevención de atascamiento y E-STOP. |
| [`Red.h`](Red.h) | Interfaz del servidor WebSocket y manejadores de Wi-Fi AP. |
| [`DiagnosticoRTOS.h`](DiagnosticoRTOS.h) | Funciones de monitoreo de pilas FreeRTOS y métricas de latencia. |
| [`Secrets.h`](Secrets.h) | Configuración local de SSID y contraseña Wi-Fi; debe crearse desde [`Secrets.example.h`](Secrets.example.h) y nunca confirmarse. |

### Secrets y compilación

`src/Estado.cpp` exige que exista `include/Secrets.h`; si falta, la
compilación termina deliberadamente. También valida en tiempo de compilación
los límites de `WiFi.softAP()` del ESP32: SSID de 1 a 32 bytes y contraseña
WPA de 8 a 63 bytes, sin terminadores NUL embebidos. La plantilla contiene
valores vacíos para impedir que se use accidentalmente sin configuración.

La validación modular aislada (`scripts/firmware/validar_firmware_modular.ps1`)
excluye el `Secrets.h` local y genera un archivo sintético sólo dentro de
`.pio/modular-validation/include/`; no copia ni expone credenciales locales.

---

## Parámetros Críticos en `Config.h`

```cpp
#define WHEEL_DIAMETER_CM       6.60f
#define PWM_SAFE_HARD_LIMIT    242   // Límite de avance rectilíneo (~95%)
#define PWM_TURN_MAX_LIMIT     247   // Límite seguro giros y calibración (97%)
#define PWM_DIRECTION_PAUSE_MS 250   // Interlock al invertir sentido
#define CONTROL_LOOP_PERIOD_US 10000 // Súper-ciclo 100 Hz (10 ms)
#define TOLERANCIA_GIRO_DEG    3.5f  // Tolerancia angular de giro
#define TURN_PULSE_ON_MS       50    // Ráfaga ON de exactitud (<5°)
#define TURN_PULSE_OFF_MS      100   // Pausa OFF de exactitud (<5°)
```

---

## Documentación General

Consulte [`docs/especificacion_objetos_sistema.md`](../docs/especificacion_objetos_sistema.md) para la especificación completa de los objetos.

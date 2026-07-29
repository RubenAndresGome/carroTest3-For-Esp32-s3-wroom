# Cabeceras y Configuración del Firmware (`include/`)

Este directorio contiene las definiciones de tipos, constantes de configuración, estructuras de datos y declaraciones de interfaz C++ para el firmware modular del ESP32-S3.

---

## Archivos de Cabecera

| Cabecera | Descripción y Contenido |
| :--- | :--- |
| [`Config.h`](Config.h) | Definición central de constantes del sistema: dimensiones físicas, constantes PD, puertos GPIO, límites PWM (230/255), tiempos muertos (250 ms) y umbrales de seguridad. |
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
| [`Secrets.h`](Secrets.h) | Configuración local de SSID y contraseña Wi-Fi (creado desde [`Secrets.example.h`](Secrets.example.h)). |

---

## Parámetros Críticos en `Config.h`

```cpp
#define WHEEL_DIAMETER_CM       6.50f
#define ENCODER_PPR            210
#define PWM_MAX_GLOBAL         230   // Límite seguro DRV8833 (90%)
#define PWM_DIRECTION_PAUSE_MS 250   // Interlock al invertir sentido
#define CONTROL_LOOP_PERIOD_US 10000 // Súper-ciclo 100 Hz (10 ms)
#define TOLERANCIA_GIRO_DEG    3.5f  // Tolerancia angular de giro
```

---

## Documentación General

Consulte [`docs/especificacion_objetos_sistema.md`](../docs/especificacion_objetos_sistema.md) para la especificación completa de los objetos.

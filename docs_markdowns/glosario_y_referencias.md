# Glosario técnico y referencias

## Conceptos desde cero

| Término | Explicación aplicada al Robot S3 |
|---|---|
| PWM | Modulación por ancho de pulso. El firmware conmuta una salida y cambia su ciclo útil para regular la energía media enviada al DRV8833. Un valor mayor no equivale automáticamente a una velocidad lineal conocida. |
| DRV8833 | Doble puente H que convierte las señales lógicas del ESP32 en corriente bidireccional para los motores. Sus límites térmicos y de corriente no se sustituyen con software. |
| PCNT | Periférico contador de pulsos del ESP32. Cuenta flancos de encoder en hardware y reduce la carga/jitter que introducirían interrupciones GPIO manuales. |
| Encoder | Sensor que produce pulsos al girar una rueda o eje. Los ticks confirman movimiento por lado, pero necesitan calibración para convertirse a distancia. |
| IMU | Unidad de medición inercial. El MPU6050 aporta acelerómetro y giroscopio; el sistema integra `gyro_z` para estimar cambios de yaw. |
| Yaw | Orientación horizontal alrededor del eje vertical. En este proyecto 0° apunta a Y+, 90° a X+, 180° a Y− y 270° a X−. |
| Pose | Estado geométrico estimado `(x, y, yaw)`. Es una estimación interna, no una medición absoluta del suelo. |
| Watchdog | Temporizador o condición de vigilancia que detiene el movimiento cuando falta progreso, telemetría o ejecución periódica esperada. |
| Interlock | Tiempo obligatorio con PWM cero antes de invertir la polaridad de un lado. Reduce conmutaciones bruscas en el puente H. |
| E-STOP | Parada de emergencia enclavada y prioritaria. Detiene PWM localmente y requiere una acción consciente para salir del estado. |
| WebSocket | Canal bidireccional persistente usado entre Python y el ESP32 para saludo, comandos, eventos y telemetría. |
| REST | Convención HTTP de petición/respuesta usada por la HMI para solicitar comandos y consultar estado. |
| SSE | Eventos enviados por el servidor sobre HTTP hacia la HMI; aquí transportan cambios sin que el navegador sondee continuamente. |
| Sesión | Identidad corta compartida por Python y ESP32 durante un proceso controlador. No debe confundirse con el ID entero de la sesión SQLite. |
| `seq` | Entero de orden/idempotencia del protocolo. Permite reconocer un comando ya completado dentro de una sesión, con las limitaciones auditadas. |
| Idempotencia | Propiedad de repetir una petición sin repetir su efecto físico. Requiere que ambas partes conserven la misma identidad y estado terminal. |
| SQLite WAL | Modo de bitácora anticipada: confirma cambios en un archivo WAL antes del checkpoint al archivo principal. Favorece lectura concurrente, pero exige conservar juntos DB/WAL/SHM durante una copia activa. |
| Ciclomática (CC) | Estimación del número de caminos de decisión de una función. El catálogo usa una heurística léxica; sirve para priorizar revisión, no sustituye análisis semántico. |
| Call graph | Grafo donde una flecha A→B significa que el cuerpo de A contiene una llamada detectada a B. Callbacks y reflexión pueden no aparecer. |
| Core 0/Core 1 | Núcleos del ESP32-S3. En esta arquitectura, Core 0 atiende red y Core 1 ejecuta el súper-ciclo de control de 100 Hz. |

## Convenciones visuales

- Flecha continua: llamada síncrona detectada.
- Flecha discontinua: asincronía, evento o cola.
- Verde: riesgo estático bajo; ámbar: medio; rojo: alto.
- “Sin llamada interna detectada” no significa automáticamente código muerto:
  puede ser callback, ruta HTTP, entrada de framework o función pública.

## Referencias primarias y normativas

- Espressif, [Pulse Counter (PCNT)](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/pcnt.html).
- Espressif, [FreeRTOS SMP en ESP32](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-guides/freertos-smp.html).
- IETF, [RFC 6455 — The WebSocket Protocol](https://www.rfc-editor.org/info/rfc6455/).
- SQLite, [Write-Ahead Logging](https://sqlite.org/wal.html).
- Texas Instruments, [DRV8833 Dual H-Bridge Motor Driver](https://www.ti.com/product/DRV8833).
- TDK InvenSense, [MPU-6000/MPU-6050 Register Map](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map.pdf).
- Android Developers, [Security checklist: WebView](https://developer.android.com/privacy-and-security/security-tips#WebView).
- Android Developers, [Unsafe file inclusion in WebView](https://developer.android.com/privacy-and-security/risks/webview-unsafe-file-inclusion).
- PlatformIO, [Core CLI quick start](https://docs.platformio.org/en/latest/core/quickstart.html).
- PlantUML, [sitio y referencia](https://plantuml.com/) y [licencia](https://plantuml.com/faq).
- Mermaid, [documentación oficial](https://mermaid.js.org/intro/).

Las referencias describen componentes o estándares. Las constantes efectivas
del Robot S3 siempre se verifican en el commit auditado; una guía genérica no
reemplaza `include/Config.h` ni las pruebas físicas.


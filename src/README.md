# Firmware Modular ESP32-S3 (`src/`)

Este directorio contiene el código fuente en C++ del firmware activo del robot autónomo diferencial en ESP32-S3.

---

## Arquitectura de Ejecución Dual-Core FreeRTOS

El firmware divide sus funciones estrictamente entre los dos núcleos del ESP32-S3:

1. **Core 0 - `Task_Web` (Stack 8192 bytes, Prioridad 1)**:
   - Administra la pila Wi-Fi (`ROBOT_S3_LOCAL`), el servidor HTTP y WebSockets (`AsyncWebSocket`).
   - Parsea paquetes de entrada en formato JSON `robot-s3-steps-v2`.
   - Encola comandos en `colaComandos` y envía eventos desde `colaEventosRed`.
   - Publica muestras de telemetría a 10 Hz (cada 100 ms).

2. **Core 1 - `loop()` Súper-Ciclo Nactivo a 100 Hz (10 ms)**:
   - Ejecuta un ciclo síncrono estricto sin tareas intermedias ni colas internas.
   - **Flujo en cada ciclo de 10 ms**:
     1. `procesarComandos()`: Recibe comandos de `colaComandos`.
     2. `leerSensoresSincrono()`: Obtiene snapshot inmutable de los 4 encoders hardware PCNT y la velocidad angular del giroscopio MPU6050.
     3. `PoseGlobal.actualizarOdometria()` & `actualizarOrientacion()`: Calcula posición global $(x, y, \theta)$.
     4. `WatchdogSeguridad.auditarSalud()`: Audita atascos de motores, desconexión MPU6050 y batería.
     5. `controlarMovimiento()`: Aplica lazo de control cinemático (Giro Pivote Continuo o Avance PD Recto).
     6. `aplicarVelocidades()`: Salida PWM hacia el puente H DRV8833 con protecciones eléctricas.

---

## Módulos Principales

| Archivo | Responsabilidad Principal |
| :--- | :--- |
| [`main.cpp`](main.cpp) | Inicialización del sistema, arranque de FreeRTOS, pinning de tareas y súper-ciclo de 100 Hz. |
| [`Cinematica.cpp`](Cinematica.cpp) | Controladores de lazo cerrado (giro pivote único continuo y avance con corrección PD de rumbo). |
| [`Motores.cpp`](Motores.cpp) | Driver de bajo nivel para DRV8833, clamp PWM (max 230/255) y interlock de 250 ms al invertir sentido. |
| [`Sensores.cpp`](Sensores.cpp) | Lectura síncrona de encoders hardware PCNT e integración I2C con MPU6050. |
| [`PoseEstimator.cpp`](PoseEstimator.cpp) | Odometría diferencial $(X, Y)$ y recienteado dinámico de yaw en reposo. |
| [`Seguridad.cpp`](Seguridad.cpp) | Watchdogs de hardware, atascamiento (stall) y corte de emergencia (E-STOP). |
| [`Red.cpp`](Red.cpp) | Manejo de comunicaciones Wi-Fi / WebSocket y protocolo JSON V2. |
| [`DiagnosticoRTOS.cpp`](DiagnosticoRTOS.cpp) | Monitoreo de memoria libre de stack, jitter y tiempos de ejecución. |

---

## Especificaciones de Seguridad Eléctrica (DRV8833)

- **Tope PWM Global**: Max 230/255 (~90%) en todas las maniobras.
- **Tiempo Muerto al Invertir Giro**: Pause obligatorio de 250 ms en `Motores.cpp`.
- **Rampa Suave de Arrancada**: Incremento paulatino de torque (+2/255 cada 20 ms) en giros.
- **Búsqueda de Torque de Fricción**: Rampa de 140 a 230 en pasos de 5 cada 250 ms.
- **Tolerancia de Orientación en Giro**: Banda muerta de ±3.5° con parada inmediata y corrección sin vuelta inventada.

---

## Documentación Técnica Completa

Para conocer los esquemas UML y la especificación detallada de clases y estructuras, consulta:
- [Diagrama de Sistema General UML](../DIAGRAMA_SISTEMA_GENERAL.md)
- [Especificación de Objetos del Sistema](../docs/especificacion_objetos_sistema.md#2-especificación-de-objetos-del-sistema-autónomo-firmware-esp32-s3)

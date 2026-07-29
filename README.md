# Robot autónomo ESP32-S3 con HMI local

Este repositorio agrupa la pila completa del carro diferencial:

- Firmware en ESP32-S3 en [src/](src/) e [include/](include/)
- Backend + HMI canónico en [desktop_app/](desktop_app/)
- APK Android que empaqueta el mismo backend en [android_app/](android_app/)
- Documentación técnica y protocolos en [docs/](docs/)
- Ensayos físicos y prototipos históricos en [archive/](archive/)

## Arquitectura del proyecto

La base del diseño es la siguiente:

1. El firmware del ESP32-S3 mantiene el lazo de control en tiempo real.
   Lee encoders, IMU, odometría, estado de seguridad, cinemática y PWM; además
   protege el robot con watchdog, E-STOP y límites eléctricos.
2. [desktop_app/](desktop_app/) es la fuente canónica del HMI local. Python
   mantiene la misión, la interfaz, el historial y la validación de entrada.
3. [android_app/](android_app/) empaqueta el mismo backend/HMI mediante
   Chaquopy y WebView para una tablet Samsung. Cuando se usa la tablet, esa
   aplicación asume el propietario único del WebSocket del robot.
4. No se debe operar el robot con el controlador de Windows y el de Android al
   mismo tiempo; se usa una sola ruta activa hacia el ESP32.

## Árbol del repositorio

```text
.
├── src/                       Firmware modular activo del ESP32-S3
├── include/                   Headers, estados, seguridad y configuración
├── desktop_app/               HMI canónico, backend Flask, frontend y scripts
├── android_app/               APK Chaquopy/WebView del mismo backend canónico
├── docs/                      Arquitectura, protocolo JSON V1 y hardware
├── scripts/                   Herramientas de compilación y validación
├── archive/                   Ensayos aprobados y prototipos legacy
├── lib/                       Bibliotecas de terceros del firmware
├── platformio.ini             Configuración de PlatformIO para ESP32-S3
├── INICIAR_ROBOT.bat          Entrada principal en Windows
└── README.md                  Documentación principal del proyecto
```

## Funcionamiento general

### 1. Arranque del robot

- El firmware se compila con PlatformIO y se carga al ESP32-S3.
- El backend Python de [desktop_app/](desktop_app/) se abre en el equipo local
  y conecta con el robot por el Wi-Fi de la red `ROBOT_S3_LOCAL`.
- La HMI realiza la calibración y la validación del estado físico antes de
  permitir el movimiento.

### 2. Flujo de control

- El usuario controla la misión desde la interfaz gráfica.
- El HMI genera pasos y giros atómicos y los envía mediante `robot-s3-steps-v3`.
- El ESP32-S3 procesa sensores, odometría, seguridad y cinemática en su bucle
  de control y responde con telemetry para seguimiento y diagnóstico.

### 3. Puesta en marcha en tablet

- [android_app/](android_app/) reutiliza el mismo backend y HMI.
- La tablet ejecuta el mismo flujo, pero la aplicación Android conserva el
  único propietario del WebSocket hacia el robot.

## Inicio rápido

En Windows, ejecuta `INICIAR_ROBOT.bat`. La primera ejecución prepara el
entorno Python y, en ejecuciones posteriores, abre el panel principal. También
puedes construir el ejecutable autónomo en [desktop_app/](desktop_app/).

Para la Galaxy Tab, consulta [android_app/README.md](android_app/README.md).

## Documentación adicional

- [DIAGRAMA_SISTEMA_GENERAL.md](DIAGRAMA_SISTEMA_GENERAL.md) para el esquema de funcionamiento y diagramas UML del sistema general (Componentes, Despliegue, Secuencia y Estados)
- [docs/especificacion_objetos_sistema.md](docs/especificacion_objetos_sistema.md) para la especificación detallada de clases, objetos y APIs de la versión actual
- [CONTRIBUTING.md](CONTRIBUTING.md) para convenciones y validaciones
- [docs/protocolo_json_steps_v2_hmi_esp32.md](docs/protocolo_json_steps_v2_hmi_esp32.md) para el canal HMI–robot
- [desktop_app/README.md](desktop_app/README.md) para la instalación local
- [android_app/README.md](android_app/README.md) para la versión tablet
- [docs/README.md](docs/README.md) para el panorama de arquitectura y hardware

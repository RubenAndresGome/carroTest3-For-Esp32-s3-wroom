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
├── docs/                      Arquitectura, protocolo JSON v3 y manuales
├── evidencia/                 Videos normalizados, hashes y fotogramas tutoriales
├── documentacionCompleta/     Portal documental estático listo para abrir
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
  y conecta con el robot por el Wi-Fi configurado como `<SSID_ROBOT>`.
- La HMI realiza la calibración y la validación del estado físico antes de
  permitir el movimiento.

### 2. Flujo de control

- El usuario controla la misión desde la interfaz gráfica.
- El HMI genera pasos y giros atómicos y los envía mediante `robot-s3-steps-v3`.
- El ESP32-S3 procesa sensores, odometría, seguridad y cinemática en su bucle
  de control y responde con telemetry para seguimiento y diagnóstico.

### Memoria interna de torque

El ESP32 conserva en SPIFFS las diez calibraciones de torque válidas más
recientes. En cada encendido sigue siendo obligatorio calibrar, pero la búsqueda
sube suavemente desde PWM cero hasta una base calculada con ese historial y
continúa en pasos de 5/255 hasta confirmar movimiento. El archivo interno usa
PWM de 8 bits como aproximación operativa; no representa torque mecánico en N·m.

En una placa nueva, prepara una vez la partición SPIFFS. Desconecta físicamente
VMOT y alimenta el ESP32 por un USB estable; una protoboard o fuente intermitente
no debe participar en la alimentación durante la carga:

```powershell
.\scripts\firmware\preparar_spiffs.ps1 -ConfirmarVmotDesconectado
```

El script valida `partitions.csv`, la cabecera del CSV y el tamaño de la imagen,
pero no ejecuta `erase` ni modifica NVS. Si se corta la alimentación, estabiliza
la conexión y repite el mismo comando. Después del reinicio la telemetría debe
mostrar `torque_history.mounted=true`, `loaded=true` y estado `loaded` o `empty`.
`mount_failed` indica que falta repetir este despliegue inicial.

El contrato `robot-s3-steps-v3` admite frames JSON de hasta 7168 bytes en ambos
extremos. Si el WebSocket se pierde durante una calibración, el súper-ciclo de
control detiene los motores y termina el intento como `cal_connection_lost`;
la reconexión no reanuda automáticamente el movimiento anterior.

Las cargas normales posteriores de firmware no borran el historial. La
telemetría `torque_history` publica cantidad, promedios, bases y estado de la
persistencia; `manual_phase` distingue el inicio y la sesión Touch activa.

La calibración rutinaria se realiza con el robot apoyado e inmóvil durante el
arranque de la MPU; después usa exclusivamente pivote sobre su centro. Busca
torque en dos polaridades, exige signos MPU opuestos, reposa 2.5 s y vuelve al
yaw inicial, sin avance lineal ni objetivo angular fijo. El MPU es la autoridad
angular; un PCNT con 2 ticks detecta torque, pero el pivote sólo se valida con
evidencia de al menos una fuente por lado y relación lateral entre 0.5 y 2.0.
Los encoders silenciosos se informan, pero una fase aislada no los excluye
globalmente. Esta evidencia reduce la traslación probable, pero no garantiza un
pivote físico porque los PCNT actuales no miden dirección. Si el
MPU confirma giro y los cuatro PCNT siguen en cero durante 500 ms, frena con
`cal_encoders_all_zero_while_turning`. La prueba eléctrica inicial con ruedas
elevadas sigue siendo un requisito único antes de operar en suelo.

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

- [Recuperación de endpoint y reversa](docs/rutina_recuperacion_endpoint_y_reversa.md):
  criterio de recuperación acotada, reversa sin pivote innecesario y trazabilidad.
- [Skill Flujo Carro ESP32](.agents/skills/carro-esp32-workflow/SKILL.md):
  skill de trabajo, diagnóstico, invariantes de seguridad y validación.
- [Router MCP de Subagentes IA](docs/mcp_agent_router.md):
  servidor MCP local para proveedores GPT, DeepSeek y OpenRouter.
- [Documentación Interna Aditiva](intern_markdowns/estado_actual_sistema_2026.md):
  síntesis técnica 2026, subagentes/rutas IA y guía del robot (excluida de Git).

- [docs/manual_usuario.md](docs/manual_usuario.md) para operación,
  recuperación de desconexiones y cierre seguro
- [docs/auditoria_estado_actual.md](docs/auditoria_estado_actual.md) para el
  dictamen verificable del código y las pruebas físicas disponibles
- [docs/portal/README.md](docs/portal/README.md) para abrir el portal
  HTML/Tailwind con diagramas y catálogo navegable
- [documentacionCompleta/README.md](documentacionCompleta/README.md) para abrir
  con doble clic la versión estática generada y versionada
- [docs/uml/README.md](docs/uml/README.md) para los UML generados de todas las
  funciones activas por carpeta
- [evidencia/README.md](evidencia/README.md) para videos, hashes y fotogramas
- [DIAGRAMA_SISTEMA_GENERAL.md](DIAGRAMA_SISTEMA_GENERAL.md) para el esquema de funcionamiento y diagramas UML del sistema general (Componentes, Despliegue, Secuencia y Estados)
- [docs/especificacion_objetos_sistema.md](docs/especificacion_objetos_sistema.md) para la especificación detallada de clases, objetos y APIs de la versión actual
- [CONTRIBUTING.md](CONTRIBUTING.md) para convenciones y validaciones
- [docs/protocolo_json_steps_v3_hmi_esp32.md](docs/protocolo_json_steps_v3_hmi_esp32.md) para el canal HMI–robot activo
- [desktop_app/README.md](desktop_app/README.md) para la instalación local
- [android_app/README.md](android_app/README.md) para la versión tablet
- [docs/README.md](docs/README.md) para el panorama de arquitectura y hardware

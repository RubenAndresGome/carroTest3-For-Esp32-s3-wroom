# Documentación Técnica del Sistema

Este directorio reúne la documentación técnica vigente, esquemas de funcionamiento y especificaciones del proyecto.

---

## Documentos Principales de Arquitectura

- 📘 [**Diagrama de Sistema General y Funcionamiento UML**](../DIAGRAMA_SISTEMA_GENERAL.md): Diagramas UML de componentes, despliegue, secuencia y máquina de estados del firmware y servidor.
- 📐 [**Especificación de Objetos y Módulos del Sistema**](especificacion_objetos_sistema.md): Detalle técnico de clases, objetos, structs, enumeraciones y APIs en la versión actual.
- 📡 [**Protocolo de Pasos V3 entre HMI y ESP32**](protocolo_json_steps_v3_hmi_esp32.md): Pasos y giros absolutos atómicos mediante `robot-s3-steps-v3`.
- 📚 [**Protocolo V2 histórico**](protocolo_json_steps_v2_hmi_esp32.md): Contrato anterior, conservado como referencia.
- ⚙️ [**Arquitectura de Control Cinemático y Rumbo**](arquitectura_control_cinematica.md): Desacoplamiento de giro/avance y lazo PD de rumbo.
- 🧪 [**Validación Física del Sistema Final**](validacion_sistema_final.md): Protocolos de pruebas de corriente y marcha en suelo.

---

## Documentación por Componente / Carpeta

- 🔌 [**Firmware Modular ESP32-S3 (`src/README.md`)**](../src/README.md): Tareas FreeRTOS, Core 0 / Core 1, súper-ciclo a 100 Hz y periféricos hardware.
- 📑 [**Cabeceras y Configuración (`include/README.md`)**](../include/README.md): Definición de constantes, structs de comandos, eventos y `Config.h`.
- 💻 [**Servidor Backend / HMI Local (`desktop_app/README.md`)**](../desktop_app/README.md): Servidor Python Flask, fachadas, WebSocket Gateway y persistencia SQLite.
- 📱 [**Aplicación Android Tablet (`android_app/README.md`)**](../android_app/README.md): Empaquetado Chaquopy/WebView y propiedad del WebSocket.
- 🛠️ [**Manual de Ensamble Físico y Hardware**](hardware/manual_ensamble_fisico.md): Diagramas de cableado y montaje físico.

---

## Archivos Históricos

Los documentos de iteraciones anteriores están conservados en [`archive/legacy`](../archive/legacy/README.md) y no describen necesariamente la versión activa.

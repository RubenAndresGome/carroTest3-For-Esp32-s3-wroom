# Documentación Técnica del Sistema

Este directorio reúne la documentación técnica vigente, esquemas de funcionamiento y especificaciones del proyecto.

## Punto de entrada recomendado

- [**Portal documental interactivo**](portal/README.md): instalación local del manual HTML/Tailwind con Mermaid, Cytoscape y Chart.js.
- [**Manual de uso y recuperación**](manual_usuario.md): operación completa, actualización Android, calibración, rutas, Ockham, desconexiones y cierre.
- 🔎 [**Auditoría del estado actual**](auditoria_estado_actual.md): dictamen, hallazgos trazables y matriz de reconexión.
- [**Informe de cobertura de la entrega**](informe_entrega_documentacion.md): matriz requisito–evidencia y resultados de todas las puertas ejecutadas.
-  [**UML de todas las funciones por carpeta**](uml/README.md): inventario regenerable de las fuentes activas.
- 🎬 [**Evidencia audiovisual**](../evidencia/README.md): videos normalizados, hashes, fotogramas y lectura tutorial.
- 🔌 [**Dogma físico de cableado**](../evidencia/README.md#evidencia-física-de-cableado): fotografía versionada, hash y transcripción de terminales, GPIO y colores.
- 🛠️ [**Plan de corrección de regresiones del Milestone**](plan_correccion_regresiones_milestone.md): recuperación priorizada de parada, calibración, pivote, PID y HMI.

---

## Documentos Principales de Arquitectura

- [**Diagrama de Sistema General y Funcionamiento UML**](../DIAGRAMA_SISTEMA_GENERAL.md): Diagramas UML de componentes, despliegue, secuencia y máquina de estados del firmware y servidor.
- [**Especificación de Objetos y Módulos del Sistema**](especificacion_objetos_sistema.md): Detalle técnico de clases, objetos, structs, enumeraciones y APIs en la versión actual.
- [**Protocolo de Pasos V3 entre HMI y ESP32**](protocolo_json_steps_v3_hmi_esp32.md): Pasos y giros absolutos atómicos mediante `robot-s3-steps-v3`.
- [**Recuperación de endpoint y reversa**](rutina_recuperacion_endpoint_y_reversa.md): decisiones acotadas al final de un paso, reversa sin pivote innecesario y telemetría para su auditoría.
- [**Estado actual de mejoras**](estado_actual_mejoras.md): alcance vigente de rutas, telemetría y publicación en GitHub Pages.
- [**Protocolo V2 histórico**](protocolo_json_steps_v2_hmi_esp32.md): Contrato anterior, conservado como referencia.
- [**Arquitectura de Control Cinemático y Rumbo**](arquitectura_control_cinematica.md): Desacoplamiento de giro/avance y lazo PD de rumbo.
-  [**Validación Física del Sistema Final**](validacion_sistema_final.md): Protocolos de pruebas de corriente y marcha en suelo.

---

## Documentación por Componente / Carpeta

- [**Firmware Modular ESP32-S3 (`src/README.md`)**](../src/README.md): Tareas FreeRTOS, Core 0 / Core 1, súper-ciclo a 100 Hz y periféricos hardware.
- [**Cabeceras y Configuración (`include/README.md`)**](../include/README.md): Definición de constantes, structs de comandos, eventos y `Config.h`.
- [**Servidor Backend / HMI Local (`desktop_app/README.md`)**](../desktop_app/README.md): Servidor Python Flask, fachadas, WebSocket Gateway y persistencia SQLite.
- [**Aplicación Android Tablet (`android_app/README.md`)**](../android_app/README.md): Empaquetado Chaquopy/WebView y propiedad del WebSocket.
- [**Manual de Ensamble Físico y Hardware**](hardware/manual_ensamble_fisico.md): Diagramas de cableado y montaje físico.
- [**Router MCP de Subagentes IA (`docs/mcp_agent_router.md`)**](../docs/mcp_agent_router.md): Integración de `ask_gpt`, `ask_deepseek` y `ask_openrouter`.
- [**Skill Flujo Carro ESP32 (`.agents/skills/carro-esp32-workflow/SKILL.md`)**](../.agents/skills/carro-esp32-workflow/SKILL.md): Flujo de trabajo, diagnóstico, invariantes y validación.
-  [**Documentación Interna Aditiva (`intern_markdowns/`)**](../intern_markdowns/estado_actual_sistema_2026.md): Estado actual 2026, subagentes/rutas IA y workflow del robot (excluida de Git).

---

## Archivos Históricos

Los documentos de iteraciones anteriores están conservados en [`archive/legacy`](../archive/legacy/README.md) y no describen necesariamente la versión activa.

## Regeneración y validación

```powershell
python scripts/documentacion/generar_catalogo.py
python scripts/documentacion/renderizar_uml.py
python scripts/documentacion/generar_resumen_sqlite.py
python scripts/documentacion/validar_enlaces.py
Push-Location docs/portal
pnpm install --frozen-lockfile
pnpm run check
pnpm run build
Pop-Location
```

El inventario excluye `archive/`, pruebas, dependencias, `.pio`, `build` y `dist`. La salida versionada se genera en `documentacionCompleta/site/`; se regenera desde `docs/portal/` y no se edita manualmente.

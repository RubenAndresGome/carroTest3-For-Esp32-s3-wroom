# Informe de cobertura de la entrega documental

Fecha de verificación: 2026-08-01. Corte de código auditado: `7a69eac` más los
artefactos documentales presentes en el árbol de trabajo.

Este informe diferencia dos afirmaciones:

- **Entrega documental completa:** los artefactos solicitados existen y pasan
  verificadores reproducibles.
- **Robot físicamente aceptado:** no demostrado; continúan abiertos hallazgos
  de resiliencia, encoders y seguridad eléctrica.

## Matriz de requisitos

| Requisito | Evidencia autoritativa | Resultado |
|---|---|---|
| Auditar el sistema actual | [`auditoria_estado_actual.md`](auditoria_estado_actual.md), código activo y extracto SQLite | Cubierto: 13 hallazgos, fortalezas, matriz de reconexión y criterios pendientes. |
| UML de funciones por carpeta | [`catalogo_funciones.json`](catalogo_funciones.json), [`uml/`](uml/README.md) y generador | Cubierto: 54 archivos, 460 funciones/prototipos, 26 tipos y 8 grafos funcionales. |
| Diagramas ultra completos | [`DIAGRAMA_SISTEMA_GENERAL.md`](../DIAGRAMA_SISTEMA_GENERAL.md) y [`uml/vistas_especializadas.md`](uml/vistas_especializadas.md) | Cubierto: 20 vistas del atlas y 5 vistas especializadas de protocolo, memoria, sensores y PWM. |
| Carpeta nueva `evidencia` | [`evidencia/README.md`](../evidencia/README.md) | Cubierto: README, videos, 48 fotogramas, 2 hojas de contacto, 2 miniaturas animadas y registro de incidente. |
| Dos videos solicitados | [`evidencia/videos/`](../evidencia/videos/) | Cubierto: derivados normalizados con hash propio y trazabilidad SHA-256 a las fuentes de `Downloads`. |
| Herramientas abiertas para grafos y documentación | [`portal/package.json`](portal/package.json) | Cubierto: Mermaid, Cytoscape.js, Chart.js, Tailwind CSS, Lucide y Vite con versiones fijadas. |
| Sistema HTML/CSS/Tailwind | [`portal/`](portal/README.md) y [`documentacionCompleta/`](../documentacionCompleta/README.md) | Cubierto: portal estático responsive, navegable, imprimible, sin CDN y con apertura por doble clic. |
| Manual de uso | [`manual_usuario.md`](manual_usuario.md) y sección interactiva del portal | Cubierto: preparación, Windows, Android/ADB, calibración, ruta, Ockham, recuperación, cierre y aceptación. |
| Imágenes del video como tutorial | Manual, portal y [`evidencia/README.md`](../evidencia/README.md) | Cubierto: tiempos, imágenes, descripciones y límites de interpretación. |
| 156 respuestas de alcance | [`decisiones_de_alcance.md`](decisiones_de_alcance.md) y `RespuestasPlan.docx` externo | Cubierto: las respuestas explícitas gobiernan inventario, UML, portal, evidencia, manual, privacidad y validación; las cuatro ambigüedades se resolvieron conservadoramente. |

## Verificaciones ejecutadas

| Puerta | Resultado observado |
|---|---|
| Firmware principal PlatformIO | Éxito; RAM 15.1 %, flash 41.8 %. |
| Firmware modular en staging | Éxito; RAM 15.1 %, flash 41.7 %. |
| Unitarias Python | 35/35 aprobadas. |
| TypeScript + Vitest | `tsc` aprobado; 2/2 pruebas; build Vite aprobado. |
| HMI canónica | JavaScript válido. |
| APK Android debug | Build aprobado; 21,846,801 bytes; SHA-256 `D2A3974B12961A1A22931D91935FC66BE496FCE2B78BCE71007646A652CB54F6`. |
| Enlaces Markdown | 673 destinos locales válidos en 35 documentos. |
| Mermaid | 46 diagramas analizados sintácticamente. |
| PlantUML | 9 fuentes y 18 exportaciones SVG/PNG. |
| Portal | Dependencias reproducibles, verificador aprobado y build Vite aprobado. |
| Portal visual | Escritorio 1440×900, tablet 1024×768 y móvil 390×844 sin desbordamiento horizontal; navegación UML espera el render antes de posicionarse. |
| CI documental | Workflow manual/PR regenera catálogo y UML, valida enlaces, construye el portal y falla ante diferencias versionadas. |
| Videos | 2/2 presentes y con SHA-256 esperado. |
| Higiene Git | `git diff --check` aprobado; `node_modules` y salidas ajenas al portal final ignorados. |

## Cobertura del inventario

| Carpeta lógica | Funciones/prototipos |
|---|---:|
| `firmware/src` | 106 |
| `firmware/include` | 55 |
| `backend/python` | 118 |
| `hmi/canonica` | 74 |
| `frontend/typescript` | 3 |
| `android/kotlin` | 28 |
| `android/python` | 4 |
| `herramientas/scripts` | 72 |

El inventario excluye deliberadamente `archive/`, pruebas, dependencias,
archivos `.disabled`, `.pio`, `build`, `dist` y `node_modules`. Los prototipos de
cabecera se conservan porque forman parte del contrato activo. Un nodo sin
aristas no se declara automáticamente “código muerto”: callbacks, rutas y
entradas de framework pueden invocarse fuera del análisis léxico.

## Limitaciones que permanecen abiertas

1. La reconexión se detiene después de cinco intentos.
2. Sesión, calibración, pose y `last_seq` son volátiles ante reboot del ESP32.
3. La secuencia vuelve a 1 por misión y el UUID Python no viaja al firmware.
4. Un evento terminal puede perderse si la cola de eventos está llena.
5. Constantes reales y especificación de seguridad no coinciden.
6. La calibración observada terminó en `cal_stall_left`.
7. No existe medición de corriente suficiente para aceptar operación en suelo.
8. Los runtimes SQLite observados preceden las correcciones oficiales del
   fallo WAL-reset de 2026; deben actualizarse antes de tratar WAL como evidencia
   robusta bajo escritura concurrente.

Estas limitaciones son resultados de la auditoría, no defectos de la entrega
documental. La documentación las muestra explícitamente para impedir que una
compilación verde se interprete como validación física del robot.

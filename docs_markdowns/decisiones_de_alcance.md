# Decisiones de alcance de la documentación

Fuente de decisión: respuestas del propietario recibidas el 1 de agosto de
2026 en `RespuestasPlan.docx`. El documento original se conserva fuera del
repositorio; este archivo registra su interpretación operativa y permite
auditar qué se implementó.

## Criterio rector

- El corte describe `7a69eac` y los artefactos documentales añadidos; una
  propuesta futura se marca como recomendación, no como función existente.
- La auditoría es descriptiva: registra defectos y continúa. No modifica la
  lógica del robot.
- El público es académico, técnico y operativo. El idioma es español, con una
  vista básica y otra avanzada.
- Se documentan configuración, APK, compilación, scripts, SQLite, pruebas,
  `Mision.cpp`, el descomponedor HTML independiente y la existencia histórica
  de `.disabled`/`archive/legacy`.

## Inventario y UML

- Cada función activa, privada o auxiliar tiene entrada trazable; los elementos
  generados por frameworks se explican como categoría general.
- El catálogo incluye llamadas entrantes/salientes, complejidad ciclomática
  estimada, posibles funciones sin llamada interna, propietario, plataforma,
  riesgo, estado compartido e interacción síncrona/asíncrona/evento/cola.
- Los diagramas se agrupan por subsistema, conservan fuente Mermaid y PlantUML
  y se exportan a SVG y PNG.
- Se excluyen dependencias, `.pio`, `build`, `dist`, entornos, pruebas e
  históricos del conteo activo. Se documentan aparte para no confundirlos con
  el ejecutable actual.

## Portal

- El portal final vive en `documentacionCompleta/site/`, es local, oscuro,
  independiente de Flask y Android, y usa dependencias empaquetadas sin CDN.
- `index.html` es la entrada general; existen rutas estáticas para auditoría,
  UML, manual, datos y evidencia. Debe poder abrirse por doble clic.
- Incluye búsqueda completa, índice alfabético y filtros por plataforma,
  carpeta, riesgo y estado; exporta catálogo CSV/JSON, imprime y permite zoom,
  arrastre y pantalla completa en grafos.
- Mermaid, Cytoscape.js, Chart.js, Lucide, Tailwind CSS, Vite y PlantUML son
  herramientas de código abierto usadas de forma reproducible.
- Se mantiene sólo local por ahora. No se integra en la WebView ni se publica
  en alojamiento externo.

## Evidencia audiovisual

- Los dos videos se conservan en el repositorio en versiones comprimidas,
  nombres normalizados y seguimiento Git LFS. Las copias originales no se
  duplican dentro del repositorio.
- Se publican hashes SHA-256, reproducción completa, capítulos, miniaturas
  animadas y 24 fotogramas por video. No se agregan audio transcrito,
  subtítulos, superposiciones ni imágenes especiales de errores.
- El primer video es evidencia integrada de servidor y robot. El segundo se
  etiqueta según el nombre suministrado; no se afirma una correspondencia
  métrica que el video por sí solo no pueda demostrar.
- El retorno Ockham aparece en el primer video; el segundo se corta antes de
  completar la secuencia indicada. Los videos demuestran el comportamiento
  observable del estado actual, no seguridad eléctrica formal.
- El propietario autorizó publicación futura, pero esta entrega permanece
  local. SSID, IP, tokens e identificadores del dispositivo se ocultan en la
  documentación publicable.

## Manual, datos y seguridad

- El manual cubre ensamble, instalación de PlatformIO, carga de firmware,
  compilación/instalación APK por ADB, Windows, Android, Wi-Fi, token local,
  sesión/`seq`, desconexión/reinicio, reanudación, calibración, encoders,
  SQLite, cierre seguro y forzado, y listas antes/después de prueba.
- La SQLite reciente se usa como evidencia resumida. No se publica la base
  cruda ni registros con identificadores. Se agregan gráficas documentales de
  sesiones, comandos, eventos, yaw, pose y PWM.
- Se documentan privacidad, modelo de amenazas, controles implementados y
  pendientes, además del procedimiento de exportación y borrado.
- No se crea política nueva de retención ni una hoja independiente de
  emergencia porque fueron rechazadas explícitamente.

## Validación y entrega

- Se validan enlaces, Mermaid, cobertura del catálogo, firmware, firmware
  modular, Python, frontend, HMI y APK.
- La generación es bajo demanda y sus artefactos se versionan. No existía CI;
  se añade una puerta documental reutilizable para integración futura.
- La entrega contiene matriz requisito–evidencia y revisión visual en
  escritorio, tablet y móvil.

## Respuestas ambiguas resueltas conservadoramente

- La pregunta 94 no contiene respuesta visible: el nombre del segundo video se
  conserva como etiqueta, no como prueba métrica.
- “Sin metadatos” se interpreta como no publicar EXIF/autor/dispositivo/fecha;
  duración y marcas de tiempo necesarias para los capítulos sí se conservan.
- La respuesta sobre unidad montada/ensamble no selecciona una de las dos
  opciones; el manual cubre ambos casos.
- Los identificadores de sesión se consideran datos internos: se enmascaran en
  material publicable junto con IP, SSID, token y dispositivo.


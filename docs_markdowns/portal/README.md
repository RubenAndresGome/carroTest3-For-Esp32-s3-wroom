# Portal documental Robot S3

Portal local y reproducible para consultar la auditoría, los diagramas Mermaid,
el grafo interactivo Cytoscape, el catálogo de funciones, el manual operativo y
la evidencia audiovisual.

## Tecnologías

- Vite para construcción estática.
- Tailwind CSS para el sistema visual.
- Mermaid para UML, secuencias, estados y modelo de datos.
- Cytoscape.js para el grafo interactivo de subsistemas.
- Chart.js para métricas de cobertura.
- Lucide para iconografía.

Todas son dependencias abiertas declaradas con versión exacta en
[`package.json`](package.json). El portal no necesita servicios externos durante
la operación y no envía telemetría ni evidencia fuera del equipo.

## Preparación y ejecución

Desde esta carpeta:

```powershell
pnpm install --frozen-lockfile
pnpm run check
pnpm run dev
```

La dirección de desarrollo se muestra en la terminal, normalmente
`http://127.0.0.1:5173`.

Para generar la versión estática:

```powershell
pnpm run build
```

La salida se crea en `documentacionCompleta/site/`. Es regenerable y versionada;
no se edita manualmente. Para abrirla sin iniciar el entorno de desarrollo,
usa [`../../documentacionCompleta/ABRIR_PORTAL.bat`](../../documentacionCompleta/ABRIR_PORTAL.bat).

## Regenerar el inventario

Antes de validar el portal, desde la raíz del repositorio:

```powershell
python scripts/documentacion/generar_catalogo.py
```

El comando vuelve a producir `docs/catalogo_funciones.json` y los diagramas
UML por carpeta bajo `docs/uml/`. Requiere Universal Ctags disponible en PATH.

Las exportaciones SVG/PNG se reconstruyen con:

```powershell
python scripts/documentacion/renderizar_uml.py
```

## Qué comprueba `pnpm run check`

- Coherencia entre estadísticas y entradas del catálogo.
- Existencia de cada archivo fuente inventariado.
- Exclusión de histórico, pruebas, dependencias y salidas generadas.
- Presencia de las secciones, manuales y diagramas principales.
- Parseo Mermaid de los diagramas del portal, el atlas, las vistas
  especializadas y los ocho catálogos por carpeta.
- Existencia y SHA-256 de ambos videos normalizados.
- Existencia y descripción de los fotogramas tutoriales.
- Presencia de los documentos canónicos de auditoría, manual, UML y evidencia.

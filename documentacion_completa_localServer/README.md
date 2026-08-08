# Portal documental completo

Doble clic en [`ABRIR_PORTAL.bat`](ABRIR_PORTAL.bat). El script inicia un
servidor de archivos estáticos ligado sólo a `127.0.0.1:8765` y abre la entrada
general [`site/index.html`](site/index.html). No inicia Flask, no controla el
robot, no requiere Internet y no se integra en Android.

## Rutas locales

- [`site/index.html`](site/index.html): dashboard completo.
- [`site/auditoria.html`](site/auditoria.html): auditoría y hallazgos.
- [`site/uml.html`](site/uml.html): UML, Mermaid, PlantUML y grafo funcional.
- [`site/manual.html`](site/manual.html): procedimientos operativos.
- [`site/datos.html`](site/datos.html): gráficas SQLite seudonimizadas.
- [`site/evidencia.html`](site/evidencia.html): videos y 48 fotogramas.

La salida `site/` es un artefacto generado y versionado. La fuente vive en
`docs/portal/`; no se edita el JavaScript compilado a mano.

## Capturas de aceptación visual

- [Escritorio 1440×900](capturas/portal_escritorio_1440x900.png)
- [Tablet 1024×768](capturas/portal_tablet_1024x768.png)
- [Móvil 390×844](capturas/portal_movil_390x844.png)

Las tres capturas corresponden al mismo portal responsive y no presentan
desbordamiento horizontal en la resolución indicada.

## Regenerar

```powershell
$env:Path = 'C:\Users\IK\.cache\codex-runtimes\codex-primary-runtime\dependencies\node\bin;' + $env:Path
Push-Location docs\portal
pnpm install --frozen-lockfile
pnpm run check
pnpm run build
Pop-Location
```

Para detener el servidor documental, cerrar la ventana minimizada titulada
`Robot S3 - portal documental`. La HMI del robot y este portal son procesos
independientes.

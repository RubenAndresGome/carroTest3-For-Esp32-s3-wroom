# Scripts del repositorio

Automatizaciones de desarrollo que afectan al proyecto completo:

- `firmware/validar_firmware_modular.ps1`: recompila el firmware modular activo
  en un área temporal.
- `firmware/validar_test_historico.ps1`: compila un ensayo deshabilitado de
  `archive/firmware_tests/` sin copiarlo a `src/`.
- `desarrollo/actualizar_memoria_codigo.bat`: actualiza la memoria de código
  cuando la herramienta correspondiente está instalada.
- `documentacion/generar_catalogo.py`: usa Universal Ctags y análisis estático
  conservador para regenerar `docs/catalogo_funciones.json` y los UML Mermaid
  y PlantUML de funciones por carpeta bajo `docs/uml/`; también produce CSV y
  el descomponedor HTML independiente.
- `documentacion/renderizar_uml.py`: descarga PlantUML con versión y SHA-256
  fijados y genera SVG/PNG versionados.
- `documentacion/generar_resumen_sqlite.py`: deriva estadísticas documentales
  seudonimizadas desde la extracción SQLite local, sin publicar la base cruda.
- `documentacion/validar_enlaces.py`: verifica que todos los enlaces e imágenes
  locales de la documentación activa apunten a destinos existentes.

Los scripts exclusivos del HMI y del empaquetado para Windows viven en
[`desktop_app/scripts`](../desktop_app/scripts/README.md).

Para validar el sistema documental después de regenerar el catálogo:

```powershell
python scripts/documentacion/validar_enlaces.py
python scripts/documentacion/renderizar_uml.py
Push-Location docs/portal
pnpm run check
pnpm run build
Pop-Location
```

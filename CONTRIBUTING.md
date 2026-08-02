# Convenciones y validación

## Idioma y nombres

- Español: documentación, interfaz, scripts, mensajes y firmware nuevo.
- Inglés idiomático: módulos internos de Python (`domain`, `services`,
  `gateway`) y nombres impuestos por Flask, TypeScript o PyInstaller.
- Carpetas y scripts nuevos: minúsculas con `snake_case`.
- Clases: `PascalCase`; funciones y variables: convención natural del lenguaje.
- Un archivo debe representar una responsabilidad principal y su nombre debe
  describirla. No usar sufijos ambiguos como `nuevo`, `final`, `copia` o `sopa`
  fuera de `archive/legacy/`.

## Límites de la estructura

- Código operativo: `src/`, `include/`, `desktop_app/` y `android_app/`.
- Documentación vigente: `docs/`.
- Herramientas repetibles: `scripts/<subsistema>/`.
- Referencias no operativas: `archive/legacy/`.
- Datos locales y bases crudas nunca se confirman en Git. Los artefactos
  documentales reproducibles de `docs/uml/exportados/` y
  `documentacionCompleta/site/` sí se versionan como excepción explícita.

## Comprobaciones antes de integrar

```powershell
C:\Users\IK\.platformio\penv\Scripts\platformio.exe run
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\firmware\validar_firmware_modular.ps1
Push-Location desktop_app
& '.\.test-venv\Scripts\python.exe' -m unittest discover -s tests -v
Pop-Location
desktop_app\scripts\frontend\compilar_frontend.bat
scripts\desarrollo\validar_hmi.bat
android_app\build_apk.ps1
python scripts\documentacion\generar_catalogo.py
python scripts\documentacion\renderizar_uml.py
python scripts\documentacion\generar_resumen_sqlite.py
python scripts\documentacion\validar_enlaces.py
Push-Location docs\portal
pnpm install --frozen-lockfile
pnpm run check
pnpm run build
Pop-Location
```

Los ensayos `archive/firmware_tests/*.cpp.disabled` se compilan únicamente con
`scripts/firmware/validar_test_historico.ps1`; nunca se copian a `src/`.

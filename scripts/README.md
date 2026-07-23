# Scripts del repositorio

Automatizaciones de desarrollo que afectan al proyecto completo:

- `firmware/validar_firmware_modular.ps1`: recompila el firmware modular activo
  en un área temporal.
- `firmware/validar_test_historico.ps1`: compila un ensayo deshabilitado de
  `archive/firmware_tests/` sin copiarlo a `src/`.
- `desarrollo/actualizar_memoria_codigo.bat`: actualiza la memoria de código
  cuando la herramienta correspondiente está instalada.

Los scripts exclusivos del HMI y del empaquetado para Windows viven en
[`desktop_app/scripts`](../desktop_app/scripts/README.md).

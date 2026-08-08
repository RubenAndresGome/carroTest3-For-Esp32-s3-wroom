# Guía de auditorías futuras por commit

Objetivo: revisar sólo lo que cambió desde un corte conocido sin perder la
trazabilidad del sistema completo ni volver a consumir contexto en carpetas
intactas.

## 1. Fijar los dos cortes

```powershell
$base = '7a69eac'
$objetivo = 'HEAD'
git status --short
git diff --name-status "$base..$objetivo"
git diff --stat "$base..$objetivo"
```

No se debe auditar un árbol sucio como si fuera un commit. Si hay cambios sin
confirmar, registrarlos como “árbol de trabajo” y conservar su diff por separado.

## 2. Reducir el alcance sin ocultar dependencias

```powershell
git diff --name-only "$base..$objetivo" |
  Select-String '^(src|include|desktop_app|android_app|scripts|docs)/'
git diff --unified=20 "$base..$objetivo" -- src include desktop_app android_app scripts
```

Por cada archivo cambiado, revisar también sus llamadas entrantes y salientes
en `docs/catalogo_funciones.json`. Si cambia protocolo, estado compartido,
seguridad, sesión o persistencia, ampliar la revisión a ambos lados de la
frontera Python↔ESP32 aunque sólo figure un archivo en el diff.

## 3. Regenerar inventario y diagramas

```powershell
$python = 'C:\Users\IK\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $python scripts\documentacion\generar_catalogo.py
& $python scripts\documentacion\renderizar_uml.py
& $python scripts\documentacion\validar_enlaces.py
```

Comparar el catálogo generado contra el commit anterior:

```powershell
git diff -- docs/catalogo_funciones.json docs/catalogo_funciones.csv docs/uml
```

Una función nueva sin propietario, riesgo, estado, complejidad y enlaces de
entrada/salida debe fallar la puerta documental.

## 4. Clasificar cada hallazgo

Usar la escala y el registro de
[`hallazgos_y_riesgos.md`](hallazgos_y_riesgos.md). Cada hallazgo debe contener:

1. corte y archivo/línea;
2. condición que lo activa;
3. efecto físico o lógico;
4. nota 0–10 y estado de aceptación;
5. recomendación verificable y esfuerzo;
6. prueba que permitiría cerrarlo.

No heredar “aceptado” de otro commit cuando cambien constantes, hardware,
controlador, protocolo o condiciones de ensayo.

## 5. Ejecutar puertas proporcionales

- Firmware o cabeceras: PlatformIO principal y staging modular.
- Python/API/SQLite: todas las unitarias Python.
- HMI/TypeScript: validador HMI, `tsc`, Vitest y build.
- Android: APK si cambia Kotlin, `mobile_entry.py`, HMI o backend empaquetado.
- Documentación/portal: catálogo, enlaces, Mermaid/PlantUML, build y revisión
  visual en escritorio/tablet/móvil.
- Seguridad o movimiento: además, ensayo físico con ruedas elevadas y corriente
  limitada; una prueba de software nunca cierra esa puerta.

## 6. Conservar evidencia mínima

Registrar commit, firmware, APK, hashes de evidencia, comandos ejecutados y
resultado. No copiar bases SQLite crudas, tokens, SSID, IP ni identificadores de
dispositivo a documentación publicable.


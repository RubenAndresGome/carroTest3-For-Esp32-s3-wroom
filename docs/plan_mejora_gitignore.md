# Plan de mejora de `.gitignore`

Fecha de auditoría: 2026-08-02  
Rama auditada: `main`  
Repositorio remoto: `origin/main`

## Objetivo

Evitar que el repositorio publique secretos, bases de datos extraídas,
volcados de diagnóstico, artefactos de compilación o credenciales de firma,
manteniendo versionados los fuentes y los entregables documentales que el
proyecto declara como reproducibles.

## Resultado de la auditoría

| Área | Hallazgo | Decisión |
| --- | --- | --- |
| `include/Secrets.h` | Existe en el árbol local y contiene configuración de red. | Permanece local; ya está ignorado. Solo se versiona `include/Secrets.example.h`. |
| `tmp_db/` y `tablet_robot.sqlite3` | Bases SQLite extraídas del dispositivo, potencialmente con sesiones, payloads y telemetría. | No versionar. Se añade una regla explícita para `tmp_db/` y extensiones SQLite. |
| `logs_mensajes/` | Logs locales de gran tamaño. | No versionar; la carpeta ya estaba ignorada. |
| `android_app/crash.txt`, `logcat.txt`, `logcat2.txt`, `logcat_full.txt` | Volcados de diagnóstico rastreados por Git; incluyen datos del dispositivo y del entorno de ejecución. | Se retiran del índice, se conservan localmente y se añaden reglas preventivas para futuros volcados. |
| `.pio/`, `compile_commands.json`, entornos virtuales, `build/`, `dist/`, `node_modules/` | Artefactos regenerables. | No versionar; ya están cubiertos por reglas existentes. |
| `*.apk`, `*.aab`, `*.jks`, `*.keystore` | Paquetes de Android y materiales de firma que pueden ser generados o contener credenciales. | Se ignoran globalmente. |
| `docs/uml/exportados/` y `documentacionCompleta/site/` | Salidas documentales reproducibles referenciadas por la documentación. | Se mantienen versionadas como excepción explícita del proyecto. |
| `evidencia/` | Evidencia audiovisual y fotogramas descritos en `evidencia/README.md`. | Se mantiene versionada; los videos usan Git LFS. |
| `docs/datos_sqlite_documentales.json` | Resumen agregado y seudonimizado generado por script, sin IDs, payloads, SSID, IP ni tokens. | Se mantiene versionado y se regenera con `scripts/documentacion/generar_resumen_sqlite.py`. |

## Cambios aplicados

- Se añadieron reglas explícitas para `tmp_db/`, SQLite/DB, volcados de
  diagnóstico y paquetes/material de firma Android.
- Se retiraron del índice los cuatro logs de `android_app/` sin eliminar las
  copias del árbol de trabajo.
- Se mantiene la regla de Git LFS para `evidencia/videos/*.mp4`.
- Se conserva la excepción documental indicada en `CONTRIBUTING.md` para los
  artefactos generados que forman parte de la entrega.

## Plan de mejora posterior

1. Ejecutar antes de cada commit `git status --short --ignored` y revisar
   cualquier archivo local nuevo que no esté cubierto por una regla.
2. Mantener los scripts documentales como fuente de verdad y regenerar sus
   salidas solo mediante los comandos de `CONTRIBUTING.md`.
3. Incorporar en CI una comprobación de secretos y de archivos grandes, por
   ejemplo con un escáner aprobado por el equipo y un límite coherente con Git
   LFS.
4. Revisar periódicamente `git ls-files` para detectar bases, logs, volcados,
   credenciales o artefactos binarios ya rastreados.
5. Si una credencial real aparece en el historial, revocarla/rotarla primero y
   tratar después la reescritura del historial y la coordinación con quienes
   hayan clonado el repositorio. Cambiar `.gitignore` no elimina datos de
   commits anteriores.

## Criterio de aceptación

El repositorio queda sin volcados de diagnóstico rastreados, las rutas locales
críticas producen una coincidencia de `git check-ignore`, el contenido
documental intencional sigue disponible y el commit resultante no incluye
`Secrets.h`, bases SQLite, logs ni artefactos de compilación.

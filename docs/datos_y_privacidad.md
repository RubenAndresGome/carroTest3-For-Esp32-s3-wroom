# Datos, exportación, borrado y privacidad

## Qué se conserva

SQLite registra sesiones, comandos, eventos, telemetría y ajustes. El portal
documental no incluye la base cruda: usa
[`datos_sqlite_documentales.json`](datos_sqlite_documentales.json), un resumen
seudonimizado sin IDs, tiempos absolutos, payloads, SSID, IP, tokens o
identificadores del robot/tablet.

## Inspección y exportación

Con una base extraída a `tmp_db/robot.sqlite3`:

```powershell
$python = 'desktop_app\.test-venv\Scripts\python.exe'
& $python consultar_db.py
& $python consultar_db.py --sessions 10
& $python consultar_db.py --commands 20
& $python consultar_db.py --events 50
& $python consultar_db.py --telemetry 20
```

La HMI ofrece exportación autenticada de una sesión:

- `GET /api/v1/sessions/<id>.json`
- `GET /api/v1/sessions/<id>/telemetry.csv`

Exportar antes de borrar. Los archivos exportados pueden contener identificadores
y payloads; no deben adjuntarse a reportes públicos sin revisión.

## Borrado

La ruta `POST /api/v1/sessions/cleanup` exige `X-App-Token` y un cuerpo con
`days`. Un valor `0` elimina todas las sesiones. La operación borra telemetría,
comandos y eventos asociados antes de la sesión y no tiene deshacer. Debe
ejecutarse sólo desde la HMI local, después de exportar la evidencia necesaria.

Esta entrega no impone una política de retención automática: fue excluida por
decisión del propietario.

## Modelo de amenazas básico

| Activo | Amenaza | Control actual | Brecha pendiente |
|---|---|---|---|
| Movimiento físico | Dos controladores envían órdenes | Regla de propietario único; un WebSocket operativo | No hay arbitraje criptográfico entre Windows y Android. |
| API local | Sitio externo intenta mutar estado | `X-App-Token`, `TRUSTED_HOSTS`, CSP y loopback | Proteger token en logs/capturas; revisar cada nueva ruta. |
| WebView Android | JavaScript no confiable llama al puente nativo | Carga local y puente limitado al cierre | `addJavascriptInterface` exige que sólo se cargue contenido de confianza. |
| SQLite | Corte abrupto o corrupción WAL | WAL, `synchronous=FULL`, transacciones | Actualizar SQLite por A-13 y serializar escritores/checkpoints. |
| Evidencia | Publicación de SSID/IP/token/IDs | Resumen seudonimizado y videos revisados | Repetir revisión antes de publicar fuera del equipo. |
| Firmware | Comando duplicado tras reconexión | Sesión corta y `seq` | Persistencia/`boot_id`/`command_id` incompletos. |

## Controles implementados

- Backend enlazado a loopback para la HMI.
- Token de aplicación para mutaciones.
- `TRUSTED_HOSTS` y política de contenido.
- Puente Android restringido a páginas locales y cierre seguro.
- SQLite con claves foráneas, timeout, WAL, `synchronous=FULL` y transacciones
  explícitas.
- Cuarentena de misión al reiniciar Python y registro de cierre forzado.

## Controles pendientes

- SQLite corregido para WAL-reset 2026.
- `boot_id` y secuencia/identidad persistente del lado ESP32.
- Reintento de transporte indefinido y arbitraje explícito de propietario.
- Automatización para detectar secretos o identificadores en evidencia antes de
  publicación.


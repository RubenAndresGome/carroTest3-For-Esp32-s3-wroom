# Incidente: reconexión, estado perdido y calibración fallida

Este registro conserva el resumen mínimo reproducible de la base SQLite local
`tmp_db/robot-live.sqlite3` consultada durante la auditoría. La base completa no
se confirma porque contiene telemetría local y está excluida por `.gitignore`.

## Identidad de la fuente

- Tamaño: 62,218,240 bytes.
- Última modificación: 2026-07-30 00:18:56 UTC.
- SHA-256: `18FDFF33187DE5C624B39FCA5EA4963BF99752F47D3FAAAD42990B9435E018A5`.
- Herramienta: `consultar_db.py` y consultas SQLite de sólo lectura.
- Todos los tiempos siguientes son UTC, tal como los almacena SQLite.

## Línea de tiempo

| UTC | Sesión SQLite | Hecho observado |
|---|---:|---|
| 00:12:11 | 88 | WebSocket conectado a `ws://192.168.4.1/ws`, firmware `robot-s3-v3`, protocolo `steps-v3`. |
| 00:12:17 | 88 | `calibrate`, `seq=1`, aceptado. |
| 00:12:44 | 88 | `completed`, `detail=cal_ok`; telemetría posterior `listo`, `cal=true`, `last_seq=1`. |
| 00:13:04 | 88 | `BACKOFF`, `Software caused connection abort`. |
| 00:13:08 | 89 | WebSocket reconectado. El token de controlador sigue siendo `e0bc5b1670f54726`. |
| 00:13:08 | 89 | Primera telemetría: `desarmado`, `cal=false`, `last_seq=0`, pose `(0,0)`, `reset_reason=power_on`. |
| 00:14:00 | — | `settings.active_mission=null`; ningún comando `step` fue registrado. |
| 00:15:12 | 89 | Segundo `calibrate`, `seq=2`, aceptado. |
| 00:15:23 | 89 | `fault`, `detail=cal_stall_left`; telemetría en fallo con PWM `0/0`. |
| 00:15:41 | 89 | Nueva caída de transporte seguida de `Network is unreachable`. |

## Interpretación respaldada

1. La reconexión conservó el texto de la sesión Python; no hubo rechazo por
   token distinto.
2. El ESP32 había perdido la memoria volátil de calibración, secuencia y pose.
   Esa transición no corresponde a un corte exclusivo de WebSocket.
3. La HMI podía mostrar transporte conectado, pero el backend debía rechazar
   una ruta porque el robot estaba `desarmado/cal=false`.
4. Ningún tramo de misión se encoló después de la primera calibración.
5. El segundo intento terminó por ausencia de progreso confirmado en el lado
   izquierdo; la parada a PWM cero quedó registrada.

## Límites

- Este extracto no demuestra la causa eléctrica del reinicio.
- `reset_reason=power_on` describe la razón reportada por el firmware, pero no
  identifica quién interrumpió físicamente la alimentación.
- La SQLite no sustituye una medición de corriente ni una inspección de PCNT.
- ADB no estaba conectado durante la revisión, por lo que no existe logcat vivo
  correlacionado con este extracto.

## Consecuencias para aceptación

- Distinguir `connected` de `ready` en la HMI.
- Añadir un `boot_id` del ESP32 al handshake y la telemetría.
- Mantener secuencias monótonas durante toda la sesión.
- No repetir un paso activo después de un reboot con avance parcial incierto.
- Reintentar transporte indefinidamente con backoff acotado.
- Validar ambos encoders del lado izquierdo antes de una nueva ruta física.

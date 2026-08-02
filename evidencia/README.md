# Evidencia audiovisual del Robot S3

Esta carpeta contiene dos demostraciones del comportamiento actual, no una
certificación de seguridad. Muestran interfaz y movimiento observable; no
demuestran por sí solas corriente, ticks de encoder, integridad SQLite ni
ausencia de reinicios.

## Videos versionados

| Evidencia | SHA-256 de la versión del repositorio |
|---|---|
| [`servidor_y_robot_ruta_ortogonal.mp4`](videos/servidor_y_robot_ruta_ortogonal.mp4) | `239F02EED4B207913C1A642247727B0A4E833B351D8C1DAEB0CF037DCC01626A` |
| [`ruta_ortogonal_y100_y50_x190_xmenos190.mp4`](videos/ruta_ortogonal_y100_y50_x190_xmenos190.mp4) | `D10C633E054D7566FC8933140226CBAEDFD8AE19D791A6E1347EB685F69A30D3` |

Los MP4 están normalizados, sin metadatos personales del dispositivo,
recomprimidos a H.264/AAC, 15 fps y la resolución vertical suministrada. Git
LFS los gestiona mediante `.gitattributes`. Las copias de `Downloads` no se
duplican en el repositorio.

Para trazabilidad, los archivos fuente externos tenían SHA-256
`64A6D1FF5640824B51B0A30742CC9F79A37EBB0F81F4CFD062C7C7395CAF92E3`
y `969514D1F85BCF7136F1E52ECE054F79A52C2C7CC3839B9AEF2CD1A0787A7508`.

## Lectura guiada: servidor y robot

![Miniatura animada](fotogramas/miniatura_server_y_robot.webp)

![Hoja de contacto con 24 muestras](fotogramas/contacto_server_y_robot.jpg)

Capítulos visuales aproximados:

| Marca | Contenido observable |
|---:|---|
| 00:00 | Arranque de la demostración y controlador/HMI. |
| 00:23 | Robot y área física de prueba. |
| 01:09 | Consulta de estado y preparación de la ruta. |
| 02:15 | Ejecución física y cambios de orientación. |
| 02:43 | Retorno Ockham indicado por el propietario y observación física. |
| 03:29 | Cierre visual de la demostración. |

La grabación integra servidor y robot. Su retorno se interpreta visualmente;
la distancia y el estado terminal sólo pueden confirmarse al correlacionar
telemetría, eventos y medición física.

| # | Tiempo | Fotograma | # | Tiempo | Fotograma |
|---:|---:|---|---:|---:|---|
| 01 | 00:04 | [muestra 01](fotogramas/server_y_robot/fotograma_01.jpg) | 13 | 01:56 | [muestra 13](fotogramas/server_y_robot/fotograma_13.jpg) |
| 02 | 00:13 | [muestra 02](fotogramas/server_y_robot/fotograma_02.jpg) | 14 | 02:05 | [muestra 14](fotogramas/server_y_robot/fotograma_14.jpg) |
| 03 | 00:23 | [muestra 03](fotogramas/server_y_robot/fotograma_03.jpg) | 15 | 02:15 | [muestra 15](fotogramas/server_y_robot/fotograma_15.jpg) |
| 04 | 00:32 | [muestra 04](fotogramas/server_y_robot/fotograma_04.jpg) | 16 | 02:24 | [muestra 16](fotogramas/server_y_robot/fotograma_16.jpg) |
| 05 | 00:41 | [muestra 05](fotogramas/server_y_robot/fotograma_05.jpg) | 17 | 02:33 | [muestra 17](fotogramas/server_y_robot/fotograma_17.jpg) |
| 06 | 00:51 | [muestra 06](fotogramas/server_y_robot/fotograma_06.jpg) | 18 | 02:43 | [muestra 18](fotogramas/server_y_robot/fotograma_18.jpg) |
| 07 | 01:00 | [muestra 07](fotogramas/server_y_robot/fotograma_07.jpg) | 19 | 02:52 | [muestra 19](fotogramas/server_y_robot/fotograma_19.jpg) |
| 08 | 01:09 | [muestra 08](fotogramas/server_y_robot/fotograma_08.jpg) | 20 | 03:01 | [muestra 20](fotogramas/server_y_robot/fotograma_20.jpg) |
| 09 | 01:19 | [muestra 09](fotogramas/server_y_robot/fotograma_09.jpg) | 21 | 03:11 | [muestra 21](fotogramas/server_y_robot/fotograma_21.jpg) |
| 10 | 01:28 | [muestra 10](fotogramas/server_y_robot/fotograma_10.jpg) | 22 | 03:20 | [muestra 22](fotogramas/server_y_robot/fotograma_22.jpg) |
| 11 | 01:37 | [muestra 11](fotogramas/server_y_robot/fotograma_11.jpg) | 23 | 03:29 | [muestra 23](fotogramas/server_y_robot/fotograma_23.jpg) |
| 12 | 01:47 | [muestra 12](fotogramas/server_y_robot/fotograma_12.jpg) | 24 | 03:39 | [muestra 24](fotogramas/server_y_robot/fotograma_24.jpg) |

## Lectura guiada: ruta ortogonal

![Miniatura animada](fotogramas/miniatura_ruta_ortogonal.webp)

![Hoja de contacto con 24 muestras](fotogramas/contacto_ruta_ortogonal.jpg)

El nombre suministrado indica `Y+100`, `Y+50`, `X+190`, `X-190`. Se conserva
como etiqueta nominal, no como prueba métrica. El video se corta antes de una
vuelta Ockham completa.

| Marca | Lectura visual aproximada |
|---:|---|
| 00:00 | Posición inicial y referencias del suelo. |
| 00:12 | Primer avance nominal. |
| 00:31 | Continuidad y transición entre tramos. |
| 00:46 | Giro/cambio de eje visible. |
| 01:08 | Tramo posterior y consulta de HMI. |
| 01:22 | Final observable; no equivale a retorno completo. |

| # | Tiempo | Fotograma | # | Tiempo | Fotograma |
|---:|---:|---|---:|---:|---|
| 01 | 00:01 | [muestra 01](fotogramas/ruta_ortogonal/fotograma_01.jpg) | 13 | 00:46 | [muestra 13](fotogramas/ruta_ortogonal/fotograma_13.jpg) |
| 02 | 00:05 | [muestra 02](fotogramas/ruta_ortogonal/fotograma_02.jpg) | 14 | 00:49 | [muestra 14](fotogramas/ruta_ortogonal/fotograma_14.jpg) |
| 03 | 00:09 | [muestra 03](fotogramas/ruta_ortogonal/fotograma_03.jpg) | 15 | 00:53 | [muestra 15](fotogramas/ruta_ortogonal/fotograma_15.jpg) |
| 04 | 00:12 | [muestra 04](fotogramas/ruta_ortogonal/fotograma_04.jpg) | 16 | 00:57 | [muestra 16](fotogramas/ruta_ortogonal/fotograma_16.jpg) |
| 05 | 00:16 | [muestra 05](fotogramas/ruta_ortogonal/fotograma_05.jpg) | 17 | 01:00 | [muestra 17](fotogramas/ruta_ortogonal/fotograma_17.jpg) |
| 06 | 00:20 | [muestra 06](fotogramas/ruta_ortogonal/fotograma_06.jpg) | 18 | 01:04 | [muestra 18](fotogramas/ruta_ortogonal/fotograma_18.jpg) |
| 07 | 00:23 | [muestra 07](fotogramas/ruta_ortogonal/fotograma_07.jpg) | 19 | 01:08 | [muestra 19](fotogramas/ruta_ortogonal/fotograma_19.jpg) |
| 08 | 00:27 | [muestra 08](fotogramas/ruta_ortogonal/fotograma_08.jpg) | 20 | 01:11 | [muestra 20](fotogramas/ruta_ortogonal/fotograma_20.jpg) |
| 09 | 00:31 | [muestra 09](fotogramas/ruta_ortogonal/fotograma_09.jpg) | 21 | 01:15 | [muestra 21](fotogramas/ruta_ortogonal/fotograma_21.jpg) |
| 10 | 00:35 | [muestra 10](fotogramas/ruta_ortogonal/fotograma_10.jpg) | 22 | 01:19 | [muestra 22](fotogramas/ruta_ortogonal/fotograma_22.jpg) |
| 11 | 00:38 | [muestra 11](fotogramas/ruta_ortogonal/fotograma_11.jpg) | 23 | 01:22 | [muestra 23](fotogramas/ruta_ortogonal/fotograma_23.jpg) |
| 12 | 00:42 | [muestra 12](fotogramas/ruta_ortogonal/fotograma_12.jpg) | 24 | 01:26 | [muestra 24](fotogramas/ruta_ortogonal/fotograma_24.jpg) |

## Procedencia y límites

- Los derivados se generaron con FFmpeg sin superposiciones, transcripción,
  subtítulos ni imágenes especiales de errores.
- No se detectó una necesidad declarada de difuminar rostros, domicilios o
  credenciales; aun así, debe repetirse la revisión antes de publicar fuera del
  entorno local.
- La posición HMI no sustituye medición del suelo; ausencia visual de movimiento
  no prueba PWM cero; una ruta visible no demuestra persistencia terminal.
- La seguridad eléctrica requiere el protocolo físico de
  [`docs/validacion_sistema_final.md`](../docs/validacion_sistema_final.md).

El incidente de reconexión/calibración se resume por separado en
[`incidentes/2026-07-30_reinicio_y_calibracion.md`](incidentes/2026-07-30_reinicio_y_calibracion.md).


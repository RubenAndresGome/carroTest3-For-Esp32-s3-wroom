# Arquitectura V2 histórica (deshabilitada)

Esta carpeta conserva una propuesta anterior de separación entre la computadora
y el ESP32-S3. Ningún archivo de este directorio participa en la compilación
actual. Las extensiones `.disabled` son intencionales.

## Firmware activo

El firmware de prueba vigente está fuera del archivo, en
`src/test_motores_inferiores.cpp`. Esta estructura histórica no debe copiarse
sobre el firmware actual.

## Responsabilidades futuras

La computadora ejecutara la interfaz, graficas, historial y planificacion de
rutas. El ESP32 conservara PCNT, MPU-6050, PWM, odometria, seguridad, maquina de
estados, Wi-Fi y el servidor WebSocket JSON.

## Pinout previsto

| Funcion | GPIO |
| --- | --- |
| MPU SDA | 1 |
| MPU SCL | 2 |
| Motor izquierdo inferior A/B | 4 / 5 |
| Motor izquierdo superior A/B | 6 / 7 |
| Encoder superior izquierdo, cable rojo | 10 |
| Encoder superior derecho, cable cafe | 11 |
| Encoder inferior izquierdo, cable negro | 12 |
| Encoder inferior derecho, cable blanco | 13 |
| Motor derecho inferior A/B | 15 / 16 |
| Motor derecho superior A/B | 17 / 18 |

## Parametros iniciales pendientes de calibracion

- Rueda: 66 mm de diametro.
- Disco: 20 ranuras.
- Conteo: unicamente flanco ascendente, 20 cuentas por vuelta.
- Circunferencia teorica: 207.345 mm.
- Resolucion teorica: 10.3673 mm por cuenta.
- El ancho efectivo entre ruedas debe medirse antes de habilitar pose o rutas.

## Activacion futura

La activacion requiere mover o renombrar deliberadamente los archivos, agregar
las dependencias a PlatformIO y completar las pruebas de seguridad. No se debe
activar un modulo parcialmente.

## Probar la app local

La IU puede visualizarse sin activar el firmware V2. En Windows, ejecuta:

```text
archive\legacy\architecture_v2\frontend\iniciar_ui.bat
```

El lanzador de compatibilidad abre la implementación de `desktop_app`. El
navegador se comunica solamente con Flask por HTTP/SSE. Python es el único
cliente de `ws://192.168.4.1/ws`, aplica reintentos con espera exponencial y
puede guardar sesiones en SQLite. Los HTML/JS `.disabled` de esta carpeta se
conservan únicamente como referencia y ya no se sirven.

Consulta `desktop_app/README.md` para instalar dependencias en línea o preparar
el paquete de ruedas (`vendor/wheels`) para una instalación posterior sin red.

El entorno Node/Vite reservado para la siguiente iteración está documentado en
`docs/desarrollo_frontend.md`. No reemplaza el HMI activo ni habilita código del
firmware.

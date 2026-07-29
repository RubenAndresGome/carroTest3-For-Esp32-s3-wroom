# Reglas del repositorio para agentes

## Fuentes canónicas

- `desktop_app/` contiene el backend/HMI canónico. En Windows es el único
  propietario del WebSocket hacia el ESP32; `android_app/` empaqueta esa misma
  fuente y asume la propiedad única cuando se opera desde la tablet. No ejecutar
  ambos controladores contra el robot al mismo tiempo.
- `src/` e `include/` contienen el firmware modular actual. Los ensayos físicos
  aprobados están deshabilitados en `archive/firmware_tests/` y se compilan
  únicamente mediante su script de staging.
- `archive/legacy/` es material histórico de solo referencia. No se copia hacia
  la implementación activa sin una revisión explícita.
- El mockup web antiguo se conserva íntegro en `archive/legacy/IUInWeb/`. No se
  elimina ni se vacía.

## Arquitectura y seguridad

- Mantener `Task_Web` en Core 0. Core 1 usa exclusivamente el `loop()` nativo
  como súper-ciclo síncrono de 100 Hz: PCNT/MPU, pose, seguridad, cinemática y
  PWM consumen la misma muestra, sin tareas ni colas intermedias.
- Los encoders se leen con PCNT; no usar `attachInterrupt` para reemplazarlo.
- El ESP32 conserva los lazos de tiempo real, límites, E-STOP y watchdog. Python
  conserva misión, interfaz, historial y validación de entrada.
- No incluir credenciales en el repositorio. Usar `include/Secrets.example.h`
  como plantilla local de `include/Secrets.h`.

### Protección eléctrica del DRV8833

#### Límites por firmware (todas las rutas de control)
- PWM máximo global: 230/255 (~90%) en avance rectilíneo y 247/255 (~97%) en giros autónomos, calibración y pivote continuo para vencer fricción en superficies difíciles.
- Tiempo muerto universal de 250 ms en `Motores.cpp:aplicarVelocidades()` al
  invertir sentido de giro. Aplica a joystick, giro autónomo y calibración.
- Los giros arrancan con rampa suave de 2/255 cada 20 ms desde cero. El watchdog
  de 2.5 s por lado se arma sólo después de alcanzar el torque calibrado; el
  avance conserva su corte de 450 ms.
- La calibración y confirmación de giro usan el promedio de ambos encoders por
  lado. El control recto usa los deltas filtrados, no el error acumulado.
- Búsqueda continua de torque como el ensayo aprobado, desde 140 hasta 247/255
  en pasos de 5 cada 250 ms; confirmación sostenida mediante gyro y ticks por
  lado. Antes de cambiar polaridad espera 750 ms. El watchdog individual
  acumulativo de 800 ms durante calibración se arma al alcanzar el PWM máximo;
  durante la rampa un PWM que aún no vence fricción no se clasifica como stall.
- El signo del giro sigue continuamente el error real. Al entrar en ±2° se
  apagan motores; un sobrepaso baja PWM a cero, respeta el interlock universal
  y corrige en sentido contrario sin pausa residual ni vuelta inventada.
- La calibración valida +25°, reposa 2.5 s y regresa al yaw 0° con una maniobra
  contraria independiente. Las rutas aceptan solamente tramos ortogonales con
  tolerancia geométrica de 1 mm y terminan tras su alineación cardinal final.
- Todos los giros autónomos usan un único pivot dinámico. `AUTO`, `PIVOT` y los nombres de arco heredados se resuelven al mismo controlador; la aproximación fina (<5°) utiliza micro-pulsos intermitentes de exactitud (`TURN_PULSE_ON_MS` / `TURN_PULSE_OFF_MS`) para evaluar la inercia e integración del IMU y evitar sobrepasos. El movimiento se valida con `fabsf(gyro_z)` y deltas de encoder. Mientras no se confirme movimiento en macro-giros, el torque escala en rampa adaptativa sin exceder 247/255 (~97%). Un atasco físico de lado, pérdida de IMU, E-STOP o protección eléctrica produce parada segura.
- Stacks: Web 8192 y súper-ciclo de control 8192 bytes. El firmware publica el
  mínimo libre medido y el motivo de reinicio; menos de 1024 bytes es fallo de
  aceptación aunque no haya ocurrido un reset.

#### Protección física requerida (no implementable por software)
- El DRV8833 original incorpora pull-down interno en sus entradas de control.
  Pull-downs externos son opcionales como defensa adicional para módulos clon.
- Mantener VMOT físicamente apagado durante boot y carga; habilitar motores
  únicamente después de confirmar ESP32 estable y PWM izquierdo/derecho en cero.
- Capacitor electrolítico ≥100 µF en VMOT–GND de cada módulo DRV8833.
- Capacitor cerámico 0.1 µF entre terminales de cada motor.
- Fusible o polyfuse de 1.5 A por driver o por motor.
- Los módulos DRV8833 típicos no exponen nSLEEP ni nFAULT.

#### Boot ROM (no controlable por software)
`setup_MotorPinsLow()` fuerza GPIO a LOW como primera operación de hardware
en `setup()`, pero no cubre el intervalo del boot ROM. El pull-down interno del
DRV8833 ayuda durante ese lapso; el corte físico de VMOT sigue siendo la defensa
operativa obligatoria durante carga y reinicio.

#### Prueba de corriente (obligatoria antes de operar en suelo)
1. Ruedas elevadas.
2. Fuente limitada a 0.5 A o batería con fusible de 1 A.
3. Medir corriente en serie con VMOT durante calibración, avance y giro.
4. Medir corriente de arranque y de rotor bloqueado por motor.
5. Solo después de verificar corrientes <1 A sostenidos, operar en suelo.
6. No se afirmará seguridad eléctrica sin esta prueba física.

## Convenciones

- Documentación, scripts y mensajes para el usuario se escriben en español.
- Firmware: nombres descriptivos en español cuando ya exista esa convención.
- Python: conservar nombres idiomáticos en inglés para módulos y APIs internas.
- Directorios nuevos en minúsculas y `snake_case`; usar nombres estándar del
  ecosistema cuando correspondan (`src`, `include`, `tests`, `README.md`).
- Antes de mover archivos, actualizar todas sus referencias y ejecutar las
  validaciones indicadas en `CONTRIBUTING.md`.

## Herramientas

- Si codebase-memory-mcp está disponible, usarlo antes de búsquedas amplias.
  Si no está disponible, usar `rg` y documentar el hallazgo sin bloquearse.
- Para inspeccionar la base de datos SQLite extraída por ADB (`tmp_db/robot.sqlite3`), usar la herramienta CLI canónica en la raíz:
  - `python consultar_db.py` (resumen general de tablas y sesiones)
  - `python consultar_db.py --commands 15` (últimos 15 comandos con su estado y payload)
  - `python consultar_db.py --events 20` (últimos 20 eventos del sistema)
  - `python consultar_db.py --telemetry 10` (muestras de pose, yaw y PWM)
  - `python consultar_db.py --search "calib"` (búsqueda rápida por patrón)
- No editar salidas generadas, entornos virtuales, `node_modules`, `.pio/`,
  `desktop_app/build/` ni `desktop_app/dist/`.

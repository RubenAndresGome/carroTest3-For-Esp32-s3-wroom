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
- PWM máximo de avance: 242/255 (~95%); giros autónomos, calibración y pivote continuo conservan 247/255 (~97%) para vencer fricción en superficies difíciles.
- Tiempo muerto universal de 250 ms en `Motores.cpp:aplicarVelocidades()` al
  invertir sentido de giro. Aplica a joystick, giro autónomo y calibración.
- Los giros arrancan con rampa suave de 2/255 cada 20 ms desde cero. El watchdog
  de 2.5 s por lado se arma sólo después de alcanzar el torque calibrado; el
  avance conserva su corte de 450 ms.
- **Dogma Canónico de Calibración**: La calibración usa el MPU como autoridad angular única.
  La rutina de calibración (`calibrate`) es dogma inmutable del sistema:
  1. Cuenta regresiva y reposo de 5.0 s para estabilizar el filtro del giróscopo MPU6050.
  2. Búsqueda de torque inicial en Polaridad Positiva (`CAL_A`): rampa de 140 a 247/255 hasta
     confirmar `ticksOk` bilateralmente y `fabsf(gyro_z) >= 0.12 rad/s` sostenido 100 ms. Si el
     giro inicial resulta negativo, invierte `candidatoCal` con pausa de 750 ms hasta validar
     polaridad positiva `gyro_z > 0`.
  3. Validación de giro a +25° (`CAL_VALIDAR_25`): pivote puro hasta alcanzar `yawInicio + 25°`
     (±2.5°) con reposo de asentamiento de 600 ms.
  4. Reposo de 2.5 s (`CAL_PAUSA`) y búsqueda de torque en Polaridad Opuesta (`CAL_B`): valida
     movimiento bilateral con signo opuesto y confirma que `candidatoGiroPos != candidatoGiroNeg`.
  5. Reposo de 2.5 s (`CAL_PAUSA_RETORNO`) y retorno estricto (`CAL_RETORNO`): pivote puro
     hacia el `yawInicioCalDeg` original, reseteando la odometría de `PoseGlobal` (X=0, Y=0) y
     el yaw de la IMU al estabilizarse.
- **Dogma de Simetría y Espejado Mecánico del Chasis 4WD**:
  - En la estructura física del chasis 4WD acrílico de doble plataforma, los reductores TT están montados en pares opuestos: los motores delanteros (FL/FR) tienen sus cuerpos orientados hacia atrás y los traseros (BL/BR) hacia adelante (enfrentados entre sí en el eje longitudinal). Asimismo, los pares izquierdo y derecho están reflejados respecto al eje sagital.
  - Al estar invertidos mecánicamente $180^\circ$, la polaridad eléctrica de las borneras DRV8833 y el mapeo de GPIO compensa esta simetría para que la señal lógica de avance (`FWD` = 1, cable naranja $+$, cable negro $-$) genere traslación positiva pura y uniforme hacia el vector $+Y$ en las cuatro ruedas.
  - El sistema de cinemática y calibración asume esta compensación de hardware como dogma inmutable.

```
                  FRENTE (+Y, Yaw = 0°)
          ┌───────────────────────────────────┐
          │   [Motor FL]          [Motor FR]  │
          │   (Cuerpo hacia       (Cuerpo hacia│
          │      atrás)              atrás)   │
          │                                   │
IZQUIERDA │        (ESP32-S3 / Baterías)      │ DERECHA (+X, Yaw = 90°)
(-X)      │         Pose (0,0) IMU            │
          │                                   │
          │   [Motor BL]          [Motor BR]  │
          │   (Cuerpo hacia       (Cuerpo hacia│
          │     adelante)           adelante) │
          └───────────────────────────────────┘
                  ATRÁS (-Y, Yaw = 180°)
```

- **Polaridad Harcodeada de Ejes y Motores**:
  - `PWM_FORWARD_POLARITY = 1`: Correspondiente al cableado físico actual en las borneras del DRV8833 (invertido físicamente por el operador), de modo que el avance y traslación concuerdan exactamente hacia +Y (frente) y los desplazamientos en +X concuerdan hacia la derecha de la interfaz de usuario.
  - El sentido de giro positivo del yaw (horario / dextrógiro) corresponde a la orientación
    cardinal canónica donde el ángulo avanza de +Y (0°) hacia +X (+90°).
  - **Mapeo Físico de Pines (Verificado por Hardware en Taller)**:
    - Motores: FL (FWD=GPIO7, REV=GPIO6), BL (FWD=GPIO4, REV=GPIO5), FR (FWD=GPIO18, REV=GPIO17), BR (FWD=GPIO16, REV=GPIO15).
    - Encoders (TXS0108E): FL (GPIO11, verde, B5->A5), FR (GPIO10, blanco, B6->A6), BL (GPIO12, negro, B2->A2), BR (GPIO13, rojo, B1->A1).
    - IMU MPU6050: SDA=GPIO8, SCL=GPIO9.
- Todos los giros autónomos usan un único pivot dinámico. `AUTO`, `PIVOT` y los nombres de arco heredados se resuelven al mismo controlador; la aproximación fina (<5°) utiliza micro-pulsos intermitentes de exactitud (`TURN_PULSE_ON_MS` / `TURN_PULSE_OFF_MS`) para evaluar la inercia e integración del IMU y evitar sobrepasos. El movimiento se valida con `fabsf(gyro_z)` y deltas de encoder. Mientras no se confirme movimiento en macro-giros, el torque escala en rampa adaptativa sin exceder 247/255 (~97%). Un atasco físico de lado, pérdida de IMU, E-STOP o protección eléctrica produce parada segura.
- Stacks: Web 8192 y súper-ciclo de control 8192 bytes. El firmware publica el
  mínimo libre medido y el motivo de reinicio; menos de 1024 bytes es fallo de
  aceptación aunque no haya ocurrido un reset.
- `robot-s3-steps-v3` acepta frames JSON de hasta 7168 bytes en firmware,
  backend y Android. Una pérdida del WebSocket durante calibración se solicita
  desde Core 0 y Core 1 frena con `cal_connection_lost`; nunca se continúa una
  calibración sin supervisión.

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

- Antes de una búsqueda amplia, consultar `codebase-memory-mcp` en este orden:
  `list_projects`, `index_status` y después `search_graph`, `query_graph` o
  `get_architecture`, según la pregunta. Usar `get_code_snippet` para recuperar
  solamente el contexto necesario.
- Si el proyecto no está indexado o `detect_changes` informa cambios, ejecutar
  `scripts\desarrollo\actualizar_memoria_codigo.bat` desde la raíz antes de
  continuar. El índice es una caché local: nunca se confirma en Git.
- Si `codebase-memory-mcp` no está instalado o falla, usar `rg` y documentar el
  hallazgo sin bloquearse. La memoria complementa `AGENTS.md`; no reemplaza las
  fuentes canónicas ni las reglas de seguridad de este archivo.
- Para inspeccionar la base de datos SQLite extraída por ADB (`tmp_db/robot.sqlite3`), usar la herramienta CLI canónica en la raíz:
  - `python consultar_db.py` (resumen general de tablas y sesiones)
  - `python consultar_db.py --commands 15` (últimos 15 comandos con su estado y payload)
  - `python consultar_db.py --events 20` (últimos 20 eventos del sistema)
  - `python consultar_db.py --telemetry 10` (muestras de pose, yaw y PWM)
  - `python consultar_db.py --search "calib"` (búsqueda rápida por patrón)
- No editar salidas generadas, entornos virtuales, `node_modules`, `.pio/`,
  `desktop_app/build/` ni `desktop_app/dist/`.

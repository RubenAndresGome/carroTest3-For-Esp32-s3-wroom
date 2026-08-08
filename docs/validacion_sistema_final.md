# Validación física del sistema final

## Preparación obligatoria

1. Crear el `include/Secrets.h` local desde `Secrets.example.h`; nunca
   confirmarlo en Git.
2. Compilar firmware, pruebas del backend, HMI, ejecutable Windows y APK.
3. Usar un solo controlador: Windows o tablet, nunca ambos simultáneamente.
4. Mantener VMOT apagado durante boot/carga y acceso físico al corte de energía.
5. Confirmar capacitor electrolítico de al menos 100 µF por DRV8833, cerámico
   de 0.1 µF por motor y fusible/polyfuse adecuado.

## Prueba de corriente con ruedas elevadas

1. Limitar la fuente a 0.5 A o usar batería con fusible de 1 A.
2. Medir en serie con VMOT durante calibración, avance y giro.
3. Medir corriente de arranque y rotor bloqueado por motor.
4. Confirmar PWM cero antes de calibrar, techo global 230/255 en avance, 247/255 en giros y al menos 250 ms
   a cero antes de toda inversión.
5. Ejecutar calibración: búsqueda 140–247/255, validación +25°, reposo 2.5 s y
   regreso independiente a yaw 0°.
6. Probar E-STOP durante una maniobra y confirmar enclavamiento y PWM cero.

No se afirmará seguridad eléctrica y no se operará en suelo hasta verificar
corrientes sostenidas menores de 1 A.

## Protocolo y reconexión

- Confirmar saludo `robot-s3-steps-v3`, sesión de 16 caracteres y `last_seq`.
- Enviar un `step` ortogonal y verificar `accepted`, `progress` y `completed`
  con el mismo entero `seq`.
- Reenviar un `seq` completado: debe regresar `already_done` sin movimiento.
- Desconectar el HMI durante un paso, dejarlo terminar y comprobar que Python
  reconcilia `last_seq` al volver.
- Reiniciar el ESP32 durante un paso sin terminal: Python debe bloquear la
  misión como `robot_restarted_mid_step`, nunca repetirla automáticamente.
- Comprobar que `manual`, `test_pwm`, `drive`, `turn` y `mission_upload` son
  rechazados por el backend y no llegan al robot.

## Movimiento y diagnóstico

- Programar 4500 mm: Python debe mostrar tres segmentos de hasta 2 m y mantener
  sólo uno activo.
- Rechazar tramos diagonales que cambien más de 1 mm en ambos ejes.
- Verificar 0° hacia +Y, 90° hacia +X, alineación precisa al rumbo del paso durante la ruta y alineación cardinal final a 0° al concluir la misión.
- Bloquear cada encoder para comprobar `stall_FL/FR/BL/BR`; en giro, el
  watchdog de 2.5 s sólo comienza tras alcanzar torque calibrado.
- Confirmar `reset_reason`, mínimos de stack Web/Control superiores a 1024
  bytes, período objetivo de 10 000 µs y cero deadlines perdidos durante el
  ensayo de aceptación.

## Aceptación de convergencia de rutas

1. Autorizar ADB y extraer una copia nueva de SQLite inmediatamente después de
   una prueba grabada; registrar juntos el nombre del video, hora de inicio y
   archivo de base para evitar comparar días distintos. Desde la raíz, usar
   `powershell -ExecutionPolicy Bypass -File android_app/extraer_db_tablet.ps1`;
   el script exige una única tablet autorizada y nunca sobrescribe una base
   previa. Si hay más de una, indicar explícitamente `-Serial <serie>`.
2. Antes de operar en suelo, verificar diámetro efectivo, giro libre y contacto
   de FL/FR/BL/BR, sentido de cada encoder, conectores y diferencia de ticks
   por lado. Conservar las mediciones de tres recorridos de 1 m y tres de 3 m.
   Si un canal se excluye, la telemetría debe indicar `encoder_health` y la
   fusión debe conservar al menos una fuente por lado; no aceptar la prueba si
   ambos encoders de un lado quedan excluidos.
3. La calibración provisional actual usa `ENCODER_ERROR_PORCENTAJE=0.15` por
   el sobreavance físico de 10 % en 1 m y 15--20 % en 2 m. El porcentaje es
   firmado y sigue `constante_efectiva = constante * (1 + error)`; repetir tres
   recorridos con la fusión estable antes de conservarlo o modificarlo. La
   misma constante debe aparecer en `encoder_fusion.distance_scale_factor` y
   en `motion.remaining_cm`; no ajustar una gráfica sin ajustar el PID.
4. Ejecutar tres recorridos de 3 m para cada rumbo cardinal: +X, −X, +Y y −Y.
   Medir físicamente el punto final y guardar vídeo y SQLite sincronizados.
4. En cada tramo de misión, confirmar en HMI que `target` muestra el objetivo,
   `drive_control` muestra P/I/D, `motion` muestra freno previsto/arrastre y
   `recovery` muestra toda decisión de endpoint; ruta planificada, trayectoria
   estimada y círculo de ±5 cm deben aparecer a la vez. Un error menor a 13 cm
   debe terminar como `step_ok_endpoint_soft`, sin pivote, y dejar calibración
   pendiente; una recuperación posterior debe publicar dirección y si evitó el
   pivote. Para un punto detrás
   del robot, `motion.effective_mode` debe indicar
   `reverse`, sin un pivote de 180° salvo que se envíe `turn_to` explícitamente.
5. Para aceptar: error lateral y euclidiano físicos ≤5 cm, yaw final ≤3°,
   ningún reset/stall falso/límite PWM excedido y ambos stacks por encima de
   1024 bytes. Usar `python consultar_db.py --segments 20` para contrastar
   inicio/fin, FL/FR/BL/BR, yaw máximo y error final de cada tramo.
6. Si la medición física y la telemetría difieren más de 3 cm, no ajustar la
   gráfica: recalibrar diámetro efectivo, offset/orientación IMU o investigar
   deslizamiento antes de repetir.
7. Ejecutar un retorno sobre el mismo eje con el objetivo detrás del robot. La
   telemetría debe indicar `effective_mode=reverse`, rampa activa durante los
   primeros 900 ms y el lado de freno de rumbo; no debe haber `giro_ini` de
   180° salvo que la misión contenga `turn_to`. Verificar que cada ejecución
   tiene un `command_run_id` distinto y usarlo al trazar la SQLite.

La compilación y las pruebas automatizadas no sustituyen estas mediciones ni
las maniobras físicas supervisadas.

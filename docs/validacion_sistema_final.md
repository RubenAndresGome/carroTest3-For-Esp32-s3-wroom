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
4. Confirmar PWM cero antes de calibrar, techo global 230/255 y al menos 250 ms
   a cero antes de toda inversión.
5. Ejecutar calibración: búsqueda 140–230/255, validación +25°, reposo 2.5 s y
   regreso independiente a yaw 0°.
6. Probar E-STOP durante una maniobra y confirmar enclavamiento y PWM cero.

No se afirmará seguridad eléctrica y no se operará en suelo hasta verificar
corrientes sostenidas menores de 1 A.

## Protocolo y reconexión

- Confirmar saludo `robot-s3-steps-v2`, sesión de 16 caracteres y `last_seq`.
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
- Verificar 0° hacia +Y, 90° hacia +X y alineación final a 0° en cada paso.
- Bloquear cada encoder para comprobar `stall_FL/FR/BL/BR`; en giro, el
  watchdog de 2.5 s sólo comienza tras alcanzar torque calibrado.
- Confirmar `reset_reason`, mínimos de stack Web/Control superiores a 1024
  bytes, período objetivo de 10 000 µs y cero deadlines perdidos durante el
  ensayo de aceptación.

La compilación y las pruebas automatizadas no sustituyen estas mediciones ni
las maniobras físicas supervisadas.

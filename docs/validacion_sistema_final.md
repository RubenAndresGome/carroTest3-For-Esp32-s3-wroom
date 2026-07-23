# Validación física del sistema final

## Preparación

1. Copiar `include/Secrets.example.h` como `include/Secrets.h` y establecer las
   credenciales locales del AP sin confirmarlas en Git.
2. Compilar y cargar el firmware activo con PlatformIO.
3. Conectar la computadora al AP del carrito y abrir `desktop_app`.
4. Mantener acceso físico al interruptor de alimentación durante toda prueba.
5. Con VMOT apagado, ejecutar **Autoevaluación segura** desde el HMI. Debe
   terminar en `PASSED` y no debe existir movimiento ni PWM físico.
6. Si falla, exportar los logs desde el HMI y no energizar VMOT.

## Ruedas elevadas

- Confirmar que el arranque muestra `UNCALIBRATED`, PWM cero y pestañas
  bloqueadas.
- Confirmar que el AP y los logs estén disponibles antes de ejecutar la
  autoevaluación.
- Comprobar que `drive`, `turn`, `move`, joystick y prueba PWM son rechazados.
- Después de `self_test: PASSED`, energizar VMOT y probar `test_pwm` corto a
  140 por motor y por sentido.
- Verificar ticks en los cuatro encoders, el límite de 230/255 y 250 ms de
  salida cero antes de cualquier inversión.
- Mantener el joystick con PWM constante y confirmar renovaciones cada 100 ms.
  Interrumpir el navegador: el ESP32 debe frenar en menos de 400 ms.
- Probar `stop`, prueba PWM limitada, pérdida del WebSocket y reconexión.

## Prueba en piso

- Con piso libre, ejecutar la calibración completa, incluida su validación de
  +25° y retorno a 0°. Pulsar E-STOP durante una repetición controlada para comprobar
  el enclavamiento y después completar calibración hasta `IDLE`.
- Ejecutar `turn` a +90°, -90°, +180° y -180°; verificar que no exista avance
  deliberado al terminar.
- En Testing, repetir ±25° con `auto`, `pivot`, `arc_left_active` y
  `arc_right_active`. Los nombres heredados de arco deben ejecutar el mismo
  `PIVOT_CONTINUOUS`; no debe aparecer ninguna pausa, pulso ni fallback.
- Confirmar en telemetría `turn_requested_mode`, `turn_mode`, los instantes de
  energía física por lado y cero transiciones de fallback. El programador Ockam
  continúa enviando `AUTO` para sus `move`.
- Confirmar `rtos.reset_reason`, arquitectura
  `web_core0_control_superloop_core1`, cero deadlines perdidos y mínimos libres
  superiores a 1024 bytes para Web y Control.
- Ejecutar `drive` a +500 mm y -500 mm y comparar distancia física y pose.
- Crear una ruta rectangular y verificar `TURN_CARDINAL`/`CONTINUOUS`,
  `DRIVE_STRAIGHT` y `FINAL_ALIGN` antes de cada `completed` correlacionado.
  No debe existir parada intermedia entre giro grueso y preciso.
- Intentar un tramo diagonal con cambios mayores de 1 mm en ambos ejes; Python
  debe rechazarlo indicando el índice del waypoint y no debe emitir `move`.
- Enviar un waypoint ortogonal de hasta 2000 mm y comprobar giro, avance y
  `completed` con el mismo UUID.
- Programar 4500 mm: Python debe mostrar tres segmentos y nunca enviar dos a la
  vez.
- Desconectar la computadora durante un waypoint. El carrito debe terminarlo y
  `desktop_app` debe reconciliar el resultado al volver.
- Desconectar o bloquear un encoder por vez para verificar `degraded_mode`;
  perder ambos encoders de un lado durante un giro bajo torque físico debe
  producir `fault` tras 2.5 s, sin apagar antes la rampa ni cambiar a arco.
- Provocar una desviación recuperable de rumbo y comprobar `RECOVERING`; una
  desviación mayor de 25° debe resolverse con correcciones parciales de hasta
  25°, no con un fallo inmediato.

## Criterio de aceptación

No se acepta la instalación si hay movimiento previo a calibración, si una
ruta avanza sin el UUID `completed` correcto, si el joystick supera su lease o
si un fallo deja PWM distinto de cero.

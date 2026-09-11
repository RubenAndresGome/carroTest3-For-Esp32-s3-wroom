# Plan de mejora del sistema ortogonal Milestone

> **Estado: supersedido para recuperación.** Este documento conserva las
> decisiones de la iteración original. La corrección de las regresiones
> observadas se rige por
> [`plan_correccion_regresiones_milestone.md`](plan_correccion_regresiones_milestone.md).

## Objetivo verificable

El robot debe interpretar `+Y` como frente físico, `+X` como derecha física y
el yaw positivo como giro horario. Debe calcular distancia mientras exista al
menos un encoder confiable en cada lado, excluir señales individuales ausentes
y reincorporarlas únicamente después de evidencia repetida.

## Decisiones de esta subrama

1. Mantener el PI/PID de rumbo, anti-windup, corrección lateral y recuperación
   de endpoint existentes en Milestone. Ya contienen las funciones útiles de
   la línea principal y no se reemplazan por el commit de inversión de Gemini.
2. Decidir el lado que reduce el PID mediante el signo geométrico del error de
   yaw. La polaridad eléctrica aprendida por la calibración no participa en esa
   decisión.
3. Conservar el mapeo GPIO verificado y resolver la dirección por rueda:
   `+PWM` lógico activa FL=6, BL=4, FR=17 y BR=15; no existe inversión global.
4. Durante calibración exigir MPU válida, al menos una fuente PCNT por lado y
   el guard `calibration_pivot_guard_v1`.
   Una fuente silenciosa no divide por dos la distancia de la fuente restante.
5. Declarar confiable después de calibración sólo un encoder observado en los
   dos sentidos del pivote. Un canal excluido puede reingresar en marcha tras
   las ventanas consecutivas configuradas.
6. Publicar si los cuatro PCNT iniciaron y si queda al menos uno por lado. Un
   fallo de inicialización individual degrada; perder un lado completo bloquea.
7. Limitar la animación SVG de motores a 15 FPS y suspender actualizaciones DOM
   cuando WebView no está visible.

## Validación automática

- Pruebas nativas: marco cardinal, lado frenado por PID en avance/reversa,
  cálculo con un encoder por lado, pérdida total de un lado, exclusión/reingreso
  y diagnóstico PCNT.
- Compilación PlatformIO del firmware modular.
- Pruebas Python de rutas ortogonales, gateway, persistencia y API.
- Verificación de que Android continúa empaquetando la HMI canónica de
  `desktop_app/`.

## Prueba física controlada pendiente

1. Mantener VMOT apagado durante carga y arranque.
2. Con ruedas elevadas y corriente limitada, confirmar cada rueda por separado:
   un PWM lógico positivo debe impulsar el frente marcado del chasis.
3. Girar el chasis manualmente en sentido horario con motores apagados: yaw debe
   aumentar hacia `+90°`. Si disminuye, corregir únicamente la frontera IMU.
4. Ejecutar calibración y comprobar `phase_a_response`, `phase_b_response`,
   `guard_reason` y la
   máscara final. Los sensores colocados a 45° deben aparecer primero como
   `recovering` y después `healthy`, no cambiar por un único pulso.
5. Probar 30 cm en `+Y`, retorno, 30 cm en `+X` y retorno; comparar regla física,
   pose persistida y video desde una referencia fija.

## Riesgos y límites

- Los dos conductores/canales físicos de un sensor no constituyen dos fuentes
  independientes para el firmware si terminan en un solo GPIO. El firmware
  actual usa un canal PCNT por rueda y cuenta ambos flancos de esa señal.
- Inicialización PCNT correcta sólo demuestra que el periférico se configuró;
  un cable abierto o un imán mal alineado se detecta por ausencia de pulsos en
  movimiento.
- Compilar y aprobar pruebas no demuestra polaridad física, corriente segura,
  tracción ni alineación del sensor. Eso requiere la prueba controlada anterior.

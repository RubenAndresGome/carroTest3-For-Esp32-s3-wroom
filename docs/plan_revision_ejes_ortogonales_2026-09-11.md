# Plan de corrección y análisis de ejes ortogonales

## Objetivo y alcance

Corregir las regresiones identificadas en el árbol de trabajo de
`codex/milestone-funcional-1.0-ejes-ortogonales`, contrastarlas con la extracción
ADB más reciente y entregar código, pruebas y evidencia revisables. No atribuir
autoría ni causalidad física sin evidencia. Conservar los cambios previos del
usuario. No confirmar commits, flashear, instalar APK ni mover motores.

## Responsabilidades

- GPT Sol, razonamiento medio: implementación, pruebas y documentación técnica
  afectada. Propietario de firmware, headers, pruebas y cambios necesarios de HMI.
- GPT Luna: extracción ADB, conservación de evidencia y reporte
  `analisis_adb_revision_ejes_2026-09-11.md`. No modifica código operativo.
- Coordinador: este plan, ledger, contraste de resultados y revisión final.

## 1. Establecer evidencia inicial

1. Verificar directorio, rama, HEAD y estado sucio; conservar diferencias previas.
2. Leer AGENTS.md del Milestone y las habilidades aplicables. El cableado de este
   checkout difiere del proyecto vecino: no copiar GPIO ni configuraciones de él.
3. Consultar herramientas de memoria disponibles; si no exponen inventario/estado
   o fallan, documentar el límite y utilizar fuentes y diff locales.
4. Registrar validaciones iniciales: firmware compiló y las 50 pruebas nativas
   existentes pasaron durante la revisión; esto no cubre la máquina completa.

## 2. Diagnóstico ADB independiente

1. Preservar copia previa de SQLite y extraer mediante el script canónico.
2. Registrar dispositivo, fecha de extracción, hash, sesión más reciente y
   firmware/protocolo identificable. No asumir que la tablet ejecuta este diff.
3. Consultar con consultar_db.py sesiones, comandos, eventos y telemetría.
4. Reconstruir cada comando relevante: inicio, fases, pose, error longitudinal y
   lateral, yaw/gyro, PWM por lado, ticks y confianza por rueda, progreso y causa
   terminal. Identificar la primera transición anómala y contexto anterior.
5. Separar observación, inferencia y datos ausentes. Contrastar las cuatro
   regresiones y comunicar pronto cualquier hallazgo que cambie la corrección.
6. Si falta conexión/autorización ADB, declarar el bloqueo exacto y marcar la
   evidencia local anterior como tal; continuar correcciones independientes.

## 3. Correcciones de control

### 3.1 Convergencia longitudinal por maniobra

- Conservar la detección de movimiento contrario durante el segmento normal.
- En recuperación de endpoint/reingreso usar progreso hacia el objetivo activo,
  con baseline propio y unidades coherentes, evitando exigir que disminuya el
  error firmado del segmento original cuando se regresa desde un sobrepaso.
- Cubrir avance, reversa, objetivo detrás, objetivo lateral y los cuatro rumbos.
- No desactivar el guard: alejarse de la meta debe seguir fallando tras 2 cm y
  400 ms, según constantes actuales; limpiar persistencia al cambiar maniobra.
- Reproducción inicial: destino (0,100), pose (0,120) -> (0,116): distancia
  20 -> 16 cm, error longitudinal -20 -> -16; guard anterior falla indebidamente.

### 3.2 Progreso conservado en realineación

- Conservar el desplazamiento antes de reiniciar baseline de encoders, o
  reconstruirlo desde la proyección del segmento después del asentamiento.
- Aplicar tanto al avance continuo como al pulsado, sin contar ticks del giro
  como traslación ni duplicar distancias ya acumuladas.
- Verificar distancia restante, transición a pulsos, progreso y ventanas de
  10 cm después de uno o varios cruces de eje.
- El cruce pequeño conserva su semántica: no consume intento de recuperación.

### 3.3 Presupuesto temporal y de intentos

- Mantener 21 intentos compartidos por segmento para recentrado/reingreso/endpoint.
- Establecer un reloj de recuperación distinto del reloj de fase: comienza una
  vez al iniciar el episodio y no se renueva por verify_retry ni endpoint retry.
- Cubrir pausa, giro inicial, avance, verificación y retorno de rumbo dentro de
  los mismos 30 s. Cerrar el episodio sólo al recuperar y reanudar la ruta o
  terminar/fallar/cancelar; un episodio posterior puede abrir un nuevo reloj.
- Comprobar vencimiento antes de que una transición limpie/renueve el reloj.
- Evitar usar millis()==0 como sinónimo ambiguo de inactivo; cubrir rollover.
- Mantener la realineación simple independiente del consumo de intentos sin
  permitir que borre un episodio de recuperación aún activo.

### 3.4 Calibración separada

- Restaurar CAL_GUARD_MAX_ATTEMPTS a 11 como valor independiente del presupuesto
  de ruta, y ajustar pruebas y documentación que lo cambiaron a 21.
- No alterar rampa, PWM, reposos, autoridad MPU, guard de signo, PCNT ni GPIO.
- Si ADB aporta evidencia de calibración, analizarla sin ampliar silenciosamente
  el alcance de estas correcciones ni aumentar límites físicos.

## 4. Contrato, geometría e integración

- Revisar que el coste de cada candidato incluya el rumbo final que realmente
  ejecutará el controlador, incluso cuando difieren dirección de recuperación
  y dirección planificada. Corregir una discrepancia sólo con caso reproducible.
- Mantener HMI y firmware coherentes si cambia cálculo, fase o telemetría.
- Verificar campos opcionales y tolerancia a telemetría antigua; no inventar
  datos ausentes en el análisis ADB.
- Mantener MPU como autoridad angular, FALLO enclavado, STOP/E-STOP, watchdog,
  límites de PWM, interlock de inversión y súper-ciclo síncrono de 100 Hz.
- Revisar cualquier hallazgo nuevo de Luna antes de incorporarlo: requiere
  evidencia, causa concreta, corrección acotada y prueba de regresión.

## 5. Validación y criterios de aceptación

- Pruebas nativas con lógica compartida por producción: sobrepaso y retorno
  válido, divergencia real, persistencia y reinicio de baseline, realineación
  continua/pulsada repetida, intento 21/22, tiempo total entre fases/reintentos,
  cierre de episodio y rollover. Evitar pruebas que sólo repitan constantes.
- Compilar firmware PlatformIO sin carga USB y ejecutar validación modular.
- Ejecutar Python existente y validaciones TypeScript/Vitest/HMI pertinentes
  para comprobar integración; registrar comando, resultado y alcance.
- Revisar diff final, git diff --check y que no cambien GPIO, motores, sensores,
  protecciones ni calibración fuera de la restauración explícita a 11 intentos.
- Si un check falla, corregir o declarar causa y límite concretos; no presentar
  como aprobado un check no ejecutado ni una sesión antigua como sesión nueva.
- Entregar análisis ADB y resumen de cambios con enlaces y pruebas. Separar
  software validado de comportamiento físico pendiente; no afirmar seguridad
  eléctrica ni éxito sobre suelo con pruebas matemáticas.

## Estado

Plan ejecutado y completado transversalmente:
1. **Extracción y Diagnóstico ADB (Luna)**: Tres extracciones canónicas realizadas
   (`robot_20260911_163141.sqlite3`, `robot_20260911_163639.sqlite3` y
   `robot_20260911_164322.sqlite3`, hash `994B687008809D7BBE5C7CFFC4BA620AA834398EAAEBDA04345D0AABCDA3342F`).
   Se analizaron sesiones #34 (encoders apagados confirmados por operador) y #35
   (avance de 100 cm exitoso `step_ok`, reversa 100 cm terminada en
   `endpoint_not_reached` por deriva en +X y cese de intentos).
2. **Correcciones de Control (Sol)**:
   - *3.1 Convergencia longitudinal*: Integrada la función `distanciaRestanteObjetivoActivo()`
     y `SeguimientoProgreso` con baseline relativo a la maniobra activa.
   - *3.2 Progreso en realineación*: El avance continuo y pulsado preservan la distancia
     medida acumulada a través de `ControlRuta::distanciaConBaseline()`, sin contar ticks
     de giro ni duplicar odometría.
   - *3.3 Presupuesto e intentos*: Presupuesto unificado de 21 intentos (`ROUTE_RECOVERY_MAX_ATTEMPTS`)
     con reloj continuo de 30 s (`EpisodioRecuperacion`) protegido contra rollover y
     sin renovación espuria en subfases.
   - *3.4 Calibración separada*: `CAL_GUARD_MAX_ATTEMPTS` restaurado a 11 de forma
     independiente del presupuesto de ruta, alineando pruebas y firmware.
3. **Contrato y Geometría (Sol / HMI)**:
   - Sobrecarga de `ControlRuta::evaluarCandidatoReingreso()` para derivar automáticamente
     `rumboRetornoCuerpoDeg` a partir de `direccionPlanificada`.
   - Corrección en espejo HMI (`desktop_app/robot_app/hmi/index.html:mirrorRecoveryGeometry`)
     para calcular `bodyLine` usando la dirección planificada del segmento en vez de la
     del candidato individual.
4. **Validaciones Automatizadas (Coordinador)**:
   - Pruebas C++ Unity nativas: **54/54 PASSED (100%)** en 1.38 s.
   - Pruebas Python backend: **64/64 OK (100%)** en 7.98 s.
   - Type-check TypeScript: **tsc --noEmit PASSED (100%)**.
   - Pruebas Vitest HMI / responsive: **11/11 PASSED (100%)** en 207 ms.
   - Compilación firmware PlatformIO ESP32-S3: **SUCCESS** (Flash 43.4%, RAM 18.6%).
   - Verificación de formato: **git diff --check limpio (0 errores)**.
   - Se mantiene el diff en el working tree sin confirmar commits ni mover hardware,
     conforme a la restricción operativa del usuario.

# Hallazgos, recomendaciones y riesgos aceptados

Esta matriz acompaña la [auditoría vigente](auditoria_estado_actual.md). La
calificación va de 0 a 10 en incrementos de 0.5 y mide consecuencia,
probabilidad y dificultad de detección, no el esfuerzo de corregir.

| Rango | Clasificación |
|---:|---|
| 0–2.0 | Bajo |
| 2.5–4.0 | Medio |
| 4.5–6.5 | Alto |
| 7.0–9.0 | Crítico |
| 9.5–10 | Crítico no aceptable |

## Registro priorizado

| ID | Nota | Clase | Estado | Esfuerzo orientativo | Recomendación verificable |
|---|---:|---|---|---:|---|
| A-01 | 8.0 | Crítico | No aceptado | 2–4 días | Reintento indefinido con backoff acotado, evento de red Android y prueba de corte mayor al quinto intento. |
| A-02 | 9.0 | Crítico | No aceptado | 5–10 días | Añadir `boot_id` y journal mínimo; probar reboot antes, durante y después de un paso. |
| A-03 | 8.5 | Crítico | No aceptado | 3–5 días | Mantener `seq` monotónico por sesión y transportar `command_id`; probar paquetes tardíos y duplicados. |
| A-04 | 8.5 | Crítico | No aceptado | 2–4 días | Reservar terminales en cola, contar pérdidas y reconciliar por telemetría. |
| A-05 | 7.0 | Crítico documental | Aceptación condicionada | 1–3 días | Convertir constantes aprobadas en fuente única y probar que documento y binario coincidan. |
| A-06 | 9.5 | Crítico no aceptable | No aceptado | 0.5–2 días físicos | Medir cada encoder, revisar PCNT/cableado y repetir calibración con corriente limitada. |
| A-07 | 4.0 | Medio | Aceptado temporal | 0.5–1 día | Mover `Mision.cpp` a referencia o marcarlo fuera del flujo en build/documentación. |
| A-08 | 5.5 | Alto | No aceptado | 1–2 días | Conservar una sesión lógica y registrar cortes como eventos de transporte. |
| A-09 | 5.5 | Alto | No aceptado | 3–5 días | Ejecutar validaciones puras en host o self-test con VMOT deshabilitado. |
| A-10 | 4.0 | Medio | Aceptado temporal | 10–20 días | Migrar gradualmente HMI a TypeScript empezando por cierre, sesión y misión. |
| A-11 | 3.0 | Medio | No aceptado | 2–3 días | Definir códigos de preparación y recuperación contextual en la HMI. |
| A-12 | 10.0 | Crítico no aceptable | No aceptado | 0.5–1 día físico | Ejecutar y registrar la prueba de corriente antes de operar en suelo. |
| A-13 | 8.0 | Crítico | No aceptado | 1–2 días | Actualizar SQLite a una versión corregida, serializar escritores y ejecutar estrés concurrente WAL. |

Las cifras son estimaciones de ingeniería para una persona familiarizada con
el repositorio; no incluyen compra de hardware ni repetición de ensayos físicos.

## Riesgos aceptados explícitamente

| Riesgo o decisión | Alcance de la aceptación | Condición de revisión |
|---|---|---|
| Stall de avance configurado en 6000 ms y recuperaciones | Aceptado empíricamente para priorizar completar misión. No equivale a aceptar PWM indefinido ni anula E-STOP/watchdogs. | Revisar cuando exista medición de corriente y una especificación única; hoy contradice el texto de 450 ms de `AGENTS.md`. |
| Límites PWM, interlock de 250 ms y tolerancias calibradas actuales | Se mantienen; esta auditoría no cambia firmware. | Reabrir sólo con prueba física comparativa y control de corriente. |
| `Mision.cpp` compilado pero no operativo | Aceptado temporalmente como deuda visible. Python continúa siendo propietario único de misión. | Retirar o integrar mediante decisión arquitectónica explícita. |
| HMI HTML/JS monolítica | Aceptada temporalmente porque es la interfaz canónica desplegada. | Migración incremental cuando las pruebas cubran los flujos críticos. |

## Riesgos que no pueden aceptarse sólo por software

- Encoders en cero durante movimiento observado.
- Operación en suelo sin corriente de arranque, sostenida y rotor bloqueado.
- Repetición automática de una distancia parcial desconocida tras reboot.
- Afirmar parada física cuando no llegó `stop_ok`.
- Usar SQLite 3.43.1/3.50.4 como única garantía de recuperación sin mitigar el
  fallo WAL-reset publicado en 2026.

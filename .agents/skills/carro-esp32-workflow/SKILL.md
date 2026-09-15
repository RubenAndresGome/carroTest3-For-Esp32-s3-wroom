---
name: carro-esp32-workflow
description: Diagnostica, audita, planifica o implementa cambios en carroTest3-For-Esp32-s3-wroom correlacionando firmware ESP32, protocolo WebSocket/JSON, HMI Python/Android, SQLite/ADB y pruebas. Úsala para fallos del robot, telemetría, encoders, MPU, PWM, giros, rutas, integración HMI-firmware o revisiones transversales; no para redacción académica, CV ni tareas genéricas ajenas al robot.
---

# Flujo de trabajo del carro ESP32

Trabaja desde evidencia observable hasta una conclusión verificable. Lee primero el `AGENTS.md` vigente del repositorio: contiene las fuentes canónicas y los límites de seguridad que esta skill no duplica.

## Seleccionar el modo pedido

- **Explicar o resumir:** inspecciona y responde; no cambies archivos.
- **Diagnosticar o auditar:** identifica causa, impacto y evidencia; no implementes salvo petición expresa.
- **Generar plan:** entrega decisiones cerradas, archivos/componentes afectados, pruebas y riesgos; no implementes.
- **Implementar o corregir:** realiza el cambio mínimo coherente de extremo a extremo y valídalo.
- **Analizar una prueba física:** trata logs, SQLite extraída, video y relato como evidencia; no afirmes seguridad física sin las mediciones exigidas por el repositorio.

Si el usuario mezcla modos, la acción más explícita y reciente manda. No conviertas “analiza” en autorización para cambiar firmware.

## Empezar con contexto barato

1. Revisa `git status --short` y conserva cambios ajenos.
2. Usa primero el grafo de código en el orden exigido por `AGENTS.md`. Si falta, está desactualizado o falla, actualízalo con el script canónico cuando proceda; usa `rg` como respaldo y registra esa limitación.
3. Ubica el dueño canónico antes de editar:
   - firmware activo: `src/` e `include/`;
   - backend/HMI canónico: `desktop_app/`;
   - empaquetado Android de la misma fuente: `android_app/`;
   - ensayos: `archive/firmware_tests/`, sólo mediante staging;
   - legado: `archive/legacy/`, sólo referencia.
4. Lee únicamente las funciones, contratos y pruebas que participan en la ruta afectada. Traza llamadas en ambas direcciones cuando un síntoma cruce firmware, gateway, servicio y UI.

Para fallos observados, lee [references/diagnostico.md](references/diagnostico.md). Para implementar o cerrar una auditoría, lee [references/validacion.md](references/validacion.md).

## Mantener un modelo de extremo a extremo

Reconstruye la ruta relevante sin asumir que el síntoma señala la causa:

`entrada HMI → validación Python → comando/cola/gateway → JSON/WebSocket → estado firmware → control 100 Hz → motores/sensores → telemetría → persistencia/UI`

En cada frontera verifica nombres de comandos, unidades, rangos, estados, identificadores, tiempos, errores y compatibilidad. Un cambio de contrato exige revisar productor, consumidor, telemetría, pruebas y documentación relacionada.

Antes de tocar control de movimiento, anota qué muestra comparten PCNT, MPU, pose, seguridad, cinemática y PWM. No introduzcas tareas, colas o fuentes de tiempo que rompan el súper-ciclo síncrono de Core 1.

## Razonar desde invariantes

Trata como restricciones duras las protecciones definidas en `AGENTS.md`: propietario único del WebSocket, E-STOP, watchdogs, PCNT, límites PWM por maniobra, rampa, tiempo muerto de inversión, validación por IMU y encoders, stacks y procedimiento eléctrico.

Cuando una protección cause un falso positivo, corrige su clasificación o evidencia sin eliminar la protección completa. Distingue siempre:

- ausencia real de movimiento;
- sensor individual degradado con estimación todavía válida;
- pérdida total de estimación de un lado;
- dato ausente, viejo o no persistido;
- límite de PWM alcanzado frente a torque insuficiente durante una rampa permitida.

No atribuyas un fallo a “falta de PWM” sólo porque el robot se detuvo. Prueba la secuencia temporal y el motivo terminal.

## Implementar de forma transversal

Haz el cambio más pequeño que mantenga consistente el sistema entero. Reutiliza controladores, estrategias y formatos existentes. No copies código de `archive/legacy/` ni conviertas un ensayo físico en firmware activo sin revisión explícita.

Al cambiar firmware o protocolo, considera en la misma pasada:

- constantes y tipos públicos en `include/`;
- lógica activa en `src/`;
- capacidades/self-test y telemetría anunciada;
- serialización, validación y estados del backend;
- interfaz y empaquetado Android si consumen el contrato;
- pruebas nativas y Python;
- documentación técnica que declare valores concretos.

No edites artefactos generados, entornos, `.pio/`, `desktop_app/build/`, `desktop_app/dist/` ni `node_modules/`.

## Cerrar con evidencia

Valida en proporción al alcance y separa con claridad:

- **comprobado automáticamente:** pruebas, compilación, análisis estático, simulación o dry-run;
- **inferido:** comportamiento esperado a partir del código y registros;
- **pendiente físicamente:** carga, corriente, tracción, inercia, cableado o comportamiento sobre suelo.

El informe final debe comenzar con el resultado o la causa, citar archivos y líneas relevantes, enumerar validaciones ejecutadas y declarar explícitamente lo no probado. Si hubo una prueba ADB, incluye sesión/comando, estado terminal, progreso, PWM, encoders/IMU y la primera transición anómala cuando estén disponibles.

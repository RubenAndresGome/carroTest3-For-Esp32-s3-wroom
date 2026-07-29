# Roadmap de Mejoras y Respuesta Casuística de Auditoría

Este documento recopila la evaluación casuística y detallada de los cuestionamientos formulados sobre la arquitectura del robot (Firmware ESP32-S3, HMI Web y Backend Python), fundamentando las decisiones de diseño físico y registrando el mapa de ruta (Roadmap) de evoluciones futuras.

---

## 1. Respuesta Casuística a los Cuestionamientos de Auditoría

### Hallazgos Críticos

#### 1. Protección de Atasco en Avance (`DRIVE_STALL_MS = 6000 ms` vs 450 ms teóricos)
- **Origen y Racional de Diseño**: En los ensayos físicos en suelo real (superficies de madera/alfombra) con motorreductoras N20/TT y controladores DRV8833, un corte rígido a 450 ms provocaba **falsos positivos de atascamiento** durante la inercia de arranque o ante pequeñas irregularidades del terreno.
- **Protección Real**: `Config.h:62` utiliza `DRIVE_STALL_MS = 6000 ms`. Para evitar daños térmicos o picos de corriente, `Motores.cpp:aplicarVelocidades()` impone un tiempo muerto universal de 250 ms (`PWM_DIRECTION_PAUSE_MS`) al cambiar sentido.
- **Candidato a Roadmap**: Reducir el número máximo de reintentos de 20 a 3–5 en `Cinematica.cpp:575` para limitar el tiempo total acumulado ante un atasco mecánico severo.

#### 2. Clasificación Degradada de Encoders en Reevaluación (`Cinematica.cpp:518, 471`)
- **Origen**: Al llamar `iniciarAvance(true)` tras una pausa por anomalía, el firmware invocaba `resetConfEncoders()`, re-habilitando los 4 encoders bajo la premisa de probar si el outlier fue un pulso perdido aislado.
- **Dictamen**: Esto provocaba que el estado degradado durara pocos ciclos.
- **Candidato a Roadmap**: Modificar `iniciarAvance()` para que preserve la máscara de degradación `encoderConfiable[4]` durante reevaluaciones dentro del mismo tramo.

#### 3. Preservación de Distancia Recorrida en Pausa por Encoder (`Cinematica.cpp:572`)
- **Origen**: Al entrar a reevaluación por encoder defectuoso, se reiniciaban los ticks base. Si la distancia parcial `distMedida` no se acumulaba explícitamente en `distAcumuladaCm`, el robot podía reiniciar la distancia objetivo restante desde cero.
- **Candidato a Roadmap**: Sumar `distMedida` a `distAcumuladaCm` en el instante previo a restablecer los ticks base en `Cinematica.cpp`.

#### 4. Persistencia de Estado de Misión Bloqueada en Backend (`services.py:395, 215`)
- **Origen**: En el backend Python (`desktop_app`), `_block_mission()` actualizaba la bandera `blocked` únicamente en la estructura en memoria. Al reiniciar el proceso Python, el estado guardado en SQLite no registraba el bloqueo, pudiendo reinterpretar la misión como pendiente.
- **Candidato a Roadmap**: Persistir la columna `blocked` y `error_reason` en la tabla `active_mission` de SQLite al invocar `_block_mission()`.

---

### Hallazgos Altos

#### 5. Recuperación de Mapa `seq -> command_id` tras Reinicio del Backend (`services.py:118, 590`)
- **Origen**: Al reiniciar la aplicación de escritorio mientras el ESP32 continúa su ejecución en hardware, el backend recupera `active_seq` pero no la tabla hash del ID del comando emitido previa desconexión.
- **Candidato a Roadmap**: Persistir el mapeo `seq -> command_id` en SQLite para garantizar la asociación del evento `completed` al reconectar.

#### 6. Acumulación de Error Geométrico Absoluto (`services.py:367`)
- **Origen**: El protocolo modular `robot-s3-steps-v2` desacopla la planificación de ruta de la odometría de a bordo; cada tramo calcula su inicio desde el objetivo teórico anterior.
- **Candidato a Roadmap**: Implementar lazo de compensación de pose integrada $(X_{\text{real}}, Y_{\text{real}})$ en Python para reajustar la distancia requerida del siguiente segmento ortogonal.

#### 7. Alineación Cardinal Final a Yaw 0° (`Cinematica.cpp:744`)
- **Origen y Racional**: Los tramos ortogonales individuales concluyen manteniendo el rumbo del segmento ($\theta = 0^\circ, 90^\circ, 180^\circ, 270^\circ$) para permitir encadenamientos fluidos de pasos sin detener el robot ni girar innecesariamente a $0^\circ$ entre segmentos.
- **Estado Actual**: La alineación final a $0.0^\circ$ se reserva para el cierre completo de la misión (`mission_completed_zero_aligned` en `Mision.cpp`).

#### 8. Bug en Reorientación Final HMI (`index.html:2218`)
- **Diagnóstico Confirmado**: `index.html` leía `orientacion` y la comparaba contra `3.5` como si fueran grados (mientras la variable telemetría se procesaba en radianes), e invocaba el envío de micro-desplazamientos de 0.5–1 cm.
- **Candidato a Roadmap**: Sustituir la lógica de la HMI por la emisión de un comando exclusivo de rotación `heading: 0.0` sin vector de desplazamiento ($d = 0$).

#### 9. Desbordamiento Silencioso en Cola de Eventos (`Eventos.cpp:16`)
- **Racional de Tiempo Real**: `encolarEvento()` actualiza la variable global de estado e intenta encolar en `xQueueSend()`. Si la cola de `Task_Web` en Core 0 se llena por latencia del cliente Web, Core 1 ignora el desbordamiento para **evitar bloquear el súper-ciclo síncrono a 100 Hz**. La fase real del robot se continúa emitiendo a través del canal de telemetría periódica.
- **Candidato a Roadmap**: Activar el polling de reconciliación en la capa Python mediante la telemetría periódica.

#### 10. Concurrencia en Buffer de WebSocket (`Red.cpp:23`)
- **Racional de Memoria**: `enviarJSON()` utiliza un buffer global `msgBuf` para evitar asignaciones dinámicas `malloc` en RTOS que puedan fragmentar la memoria Heap del ESP32-S3.
- **Candidato a Roadmap**: Adicionar un semáforo mutex (`SemaphoreHandle_t`) alrededor de `msgBuf` en `Red.cpp`.

#### 11. Recuperación de Punto de Acceso WiFi (`Red.cpp:215, 229`)
- **Racional**: En operación normal, si la red STA no responde tras 30 intentos, el ESP32 conmuta a modo Access Point (`ssid_AP`). El servidor WebSocket se inicia sobre la IP estática `192.168.4.1`.
- **Candidato a Roadmap**: Actualizar el flag `servidorIniciado` ante reconexiones tardías del AP.

#### 12. Caso Límite en Tests Unitarios (`Motores.cpp:133`)
- **Origen**: `validarInterlockMotores()` fue un test de staging. Al incrementar el límite de potencia de giros a `247/255` (~97%), la aserción de prueba unitaria legacy requería actualizar su constante esperada de 230 a 247.

---

### Divergencias de Seguridad y Parámetros Empíricos

- **Stall de Giro (4000 ms vs 2500 ms) y Tolerancia (±3.5° vs ±2°)**:
  - *Justificación Técnica*: En pruebas físicas sobre suelo real, una tolerancia de $\pm 2^\circ$ sin micro-pulsos provocaba oscilaciones continuas de interlock (overshoot). Los parámetros de `Config.h` (`TURN_STALL_MS = 4000ms`, `TOLERANCIA_GIRO_DEG = 3.5°`, `CALIBRATION_PWM_STEP = 13/255`) fueron **calibrados físicamente** para evitar oscilaciones por inercia y garantizar la rotación sobre superficies con alta fricción estática.
- **Respaldado por Pruebas de Campo**: Operación verificada físicamente en suelo con el vehículo funcional.

---

## 2. Propuesta Arquitectónica: Ruta Ockham Inversa $f^{-1}$

Actualmente, el regreso al origen en la Ruta Ockham calcula una trayectoria ortogonal directa basada en las coordenadas finales $(X, Y)$. Se establece en el roadmap la alternativa basada en la inversión de la función de trayectoria ejecutada:

$$\text{Ruta Directa: } f(\text{pasos}) = \{p_1, p_2, p_3, \dots, p_n\}$$

$$\text{Ruta Ockham Inversa: } f^{-1}(\text{pasos}) = \{p_n^{-1}, p_{n-1}^{-1}, \dots, p_2^{-1}, p_1^{-1}\}$$

### Mecanismo de Ejecución:
1. **Pila de Pasos (`Step Stack`)**: Cada paso ejecutado exitosamente se almacena en una pila con su distancia $d_i$ y su rumbo $\theta_i$.
2. **Desplazamiento Inverso**: Al solicitar "Regresar al Inicio (Ockham)", el sistema desapila los pasos en orden inverso ($p_n \to p_1$).
3. **Inversión Vectorial**:
   - Para cada paso $p_i = (\theta_i, d_i)$, se ejecuta la maniobra especular:
     $$\theta_{\text{retorno}} = (\theta_i + 180^\circ) \bmod 360^\circ$$
     $$\text{distancia} = d_i$$
4. **Resultado**: El robot desanda exactamente el mismo camino físico por el que avanzó, sortea obstáculos previamente librados y concluye en el origen $(0,0)$ orientado exactamente a $0.0^\circ$ (Norte cardinal).

---

## 3. Matriz de Tareas Candidatas para el Roadmap

| ID | Componente | Descripción de la Mejora Candidata | Prioridad |
|---|---|---|---|
| **RM-01** | HMI Web | Corregir cálculo de `currentYaw` en `index.html` y eliminar envío de micro-desplazamientos de 0.5–1 cm. | Alta |
| **RM-02** | Firmware | Preservar `encoderConfiable[4]` durante reevaluaciones dentro del mismo tramo en `Cinematica.cpp`. | Alta |
| **RM-03** | Firmware | Acumular `distMedida` en `distAcumuladaCm` antes de reiniciar ticks base por falla de encoder en `Cinematica.cpp`. | Alta |
| **RM-04** | Backend | Persistir estado `blocked` y mapa `seq -> command_id` en SQLite local para tolerancia a reinicios de app. | Media |
| **RM-05** | Firmware | Adicionar mutex (`SemaphoreHandle_t`) alrededor de `msgBuf` en `Red.cpp`. | Media |
| **RM-06** | HMI / Backend | Implementar motor de Ruta Ockham Inversa $f^{-1}$ basada en pila de pasos invertidos. | Media |
| **RM-07** | Backend | Lazo de compensación de pose absoluta integrated $(X_{\text{real}}, Y_{\text{real}})$ entre segmentos ortogonales. | Baja |

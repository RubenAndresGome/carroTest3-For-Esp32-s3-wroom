# Estado actual de mejoras

## Alcance vigente

La versión activa fortalece rutas ortogonales, corrección de rumbo en reversa y
trazabilidad posterior. La recuperación permite continuar ante un residuo
pequeño, pero no oculta una desviación que requiere calibración física.

- El endpoint completa normalmente, permite cierre suave bajo 13 cm, realiza
  como máximo dos correcciones amplias y después termina con fallo seguro.
- Si el objetivo queda detrás, `auto` selecciona reversa sin pivot de 180°;
  conserva el interlock y una rampa de 900 ms.
- La telemetría publica decisión de recuperación, lado de freno, rampa y
  `command_run_id` para separar ejecuciones incluso si cambia `seq`.
- La HMI compacta el programador de ruta y conserva el modo de eje único para
  puntos rectangulares.
- **Navegación Robusta y Watchdogs Dinámicos**: Se resolvió el estancamiento (stall) al rotar ajustando los watchdogs por fase de movimiento y una rampa de torque adaptativo sin sobrepasar la seguridad eléctrica (97% de PWM).
- **Precisión de Alineación**: Los giros emplean ahora micropulsos para aproximación fina y logran tolerancia estricta (< 1.0°) en calibración.
- **Corrección de Polaridad MPU**: Se estandarizó la polaridad del Yaw del sensor (antihorario positivo) coordinado con el sistema de coordenadas.
- **Validación Exitosa de Rutas Ortogonales (Sesión #41 / Commit 92ab106)**: Se resolvió el deadlock en `giro_fin` liberando el latch de tolerancia al asentar estacionario. El robot completó 8 comandos consecutivos con `step_ok` gobernando rumbos cardinales con desviación < 1.0°.
- **Calibración Empírica de Marcas y Asimetría Dinámica**: Se identificó sobreavance residual sobre marcas de cinta y una tracción superior en el tren izquierdo que el lazo actual compensa suavemente. Se formalizó la estrategia de balance dinámico de PWM hasta un 30% proporcional para evitar oscilaciones bang-bang.

## Publicación en GitHub Pages

Las fuentes documentales viven en `docs_markdowns/`; `docs/` es el artefacto
generado. El portal incorpora Mermaid en su bundle y Vite copia al artefacto
los videos, fotogramas e incidentes desde `evidencia/`. Por ello los materiales
gráficos usan rutas relativas válidas también bajo `/<repositorio>/`.

La mejora no sustituye los recorridos físicos, medición de yaw/corriente ni la
validación de SQLite descrita en [validacion_sistema_final.md](validacion_sistema_final.md).

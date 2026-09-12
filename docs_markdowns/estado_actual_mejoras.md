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
- **Calibración de Marcas y Compensación Dinámica de Asimetría (Commit 63b99c8)**: Recorte proporcional de hasta 30% en PWM para el tren con exceso de tracción, autoridad lateral ampliada a 8.0° y escala de odometría calibrada a 0.920 para alinear las marcas físicas en suelo.
- **Geometría de Chasis y Supresión de Giro Parásito en Reversa (Sesión #42)**: Se analizó el desfase de 15 cm correspondiente a la semilongitud del chasis (30x20 cm) respecto al centro de giro entre ruedas (0,0). Se verificó que el hardware PCNT no pierde pulsos a alta velocidad (100 Hz con 0 plazos perdidos). Se suprimió el `giro_fin` parásito en reversa mediante `ControlRuta::rumboFinalParaPaso`, impidiendo que el chasis pivotee en destino y eliminando el arrastre lateral sobre concreto.

## Publicación en GitHub Pages

Las fuentes documentales viven en `docs_markdowns/`; `docs/` es el artefacto
generado. El portal incorpora Mermaid en su bundle y Vite copia al artefacto
los videos, fotogramas e incidentes desde `evidencia/`. Por ello los materiales
gráficos usan rutas relativas válidas también bajo `/<repositorio>/`.

La mejora no sustituye los recorridos físicos, medición de yaw/corriente ni la
validación de SQLite descrita en [validacion_sistema_final.md](validacion_sistema_final.md).

# Módulo histórico de misión completa

Este directorio conserva `Mision.cpp` y `Mision.h` únicamente como referencia
histórica. El módulo implementaba persistencia y ejecución de una misión
completa dentro del firmware, pero ya no forma parte del firmware activo.

La arquitectura vigente asigna la propiedad exclusiva de la misión a Python;
el ESP32 conserva únicamente la ejecución de pasos atómicos y sus protecciones
de tiempo real. Estos archivos no se compilan desde `src/` ni deben copiarse al
árbol activo sin una revisión arquitectónica explícita y validación completa.

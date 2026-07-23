# Archivo histórico de pruebas del firmware

Esta carpeta conserva los ensayos físicos aprobados que sirvieron para validar
por separado sensores, motores, encoders y control cerrado. Todos terminan en
`.cpp.disabled`, por lo que PlatformIO no los incorpora al firmware operativo.

El firmware real vive exclusivamente en `src/*.cpp`. No se debe copiar ni
renombrar un ensayo dentro de `src/` para ejecutarlo.

## Catálogo

| Archivo | Propósito | Movimiento físico |
|---|---|---|
| `test_mpu6050_angulo_unicamente_aprobado.cpp.disabled` | MPU6050, calibración de bias y yaw | No |
| `test_encoders_funcion_only_aprobado.cpp.disabled` | Lectura independiente de los cuatro LM393 con PCNT | No acciona motores |
| `test_motores_funcion_fisica_verificada.cpp.disabled` | Polaridad y accionamiento de los cuatro motores | Sí |
| `test_encoders_funcion_fisica_aprobado.cpp.disabled` | Encoders durante desplazamiento físico controlado | Sí |
| `test_lazo_cerrado_mpu_encoders_aprobado.cpp.disabled` | MPU, cuatro encoders, motores, giros, distancia y fallos | Sí, hasta 3 m |

## Compilación aislada

Desde la raíz del repositorio:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\firmware\validar_test_historico.ps1 `
  -TestName test_mpu6050_angulo_unicamente_aprobado
```

El script copia únicamente el ensayo elegido a `.pio/historical-tests/` y lo
compila allí como `main.cpp`. No modifica `src/` ni desactiva el sistema final.

Para otro ensayo, usar uno de los nombres de la tabla sin `.cpp.disabled`.

## Reglas de seguridad

- Comenzar siempre con las ruedas elevadas y E-STOP accesible.
- Los ensayos de motores, encoders físicos y lazo cerrado requieren baterías,
  cableado y polaridades revisados.
- La prueba de lazo cerrado necesita una zona libre de al menos 3 m.
- No ejecutar dos ensayos ni el firmware operativo simultáneamente.
- No editar estos archivos para cambiar el producto final; cualquier corrección
  vigente debe implementarse y probarse en los módulos de `src/`.

## Procedencia

Los archivos se archivaron después de aprobar las pruebas individuales. Se
conservan como evidencia técnica y herramientas de diagnóstico, mientras que
la integración HMI–ESP32 utiliza el firmware modular activo.

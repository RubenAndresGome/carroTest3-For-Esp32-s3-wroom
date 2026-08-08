# Entorno frontend para futuras iteraciones

## Decisión arquitectónica

Node se usa para compilar, validar y probar la interfaz. No debe convertirse en
un segundo backend ni abrir otra conexión WebSocket con el robot.

```text
ESP32-S3 <--- WebSocket ---> Python/Flask <--- HTTP/SSE ---> Navegador
                              ^
                              |
                     salida compilada por Vite
```

Python sigue siendo responsable de reconexión, validación de comandos,
persistencia SQLite, sesiones y seguridad de la API local. El frontend sólo
presenta estado y solicita acciones por `/api/v1`.

## Versiones y reproducción

- Node se fija mediante `.node-version` y `.nvmrc`.
- La distribución portátil se guarda en `.tools/` y no entra en Git.
- Las dependencias exactas se guardan en `package.json` y `package-lock.json`.
- `node_modules/` y la salida `vite-dist/` son regenerables y están ignoradas.

No uses `npm install` global. Ejecuta
`desktop_app/scripts/frontend/preparar_frontend.ps1`, que utiliza
el Node local del proyecto.

## Flujo de trabajo

1. Inicia Flask con `INICIAR_ROBOT.bat`.
2. Inicia Vite con `desktop_app/scripts/frontend/desarrollar_frontend.bat`.
3. Abre `http://127.0.0.1:5173` durante el desarrollo.
4. Antes de integrar cambios, ejecuta
   `desktop_app/scripts/frontend/compilar_frontend.bat`.

El comando de validación ejecuta TypeScript estricto, pruebas Vitest y el build
de producción. Vite reenvía las llamadas `/api` a Flask en el puerto 8080.

## Regla para la futura migración

La carpeta actual `robot_app/hmi/` es la interfaz estable. No se debe cambiar
Flask para servir `robot_app/static/vite-dist/` hasta que la nueva IU tenga
paridad funcional:

- conexión y reconexión visibles;
- paro de emergencia accesible;
- comandos deshabilitados sin conexión;
- telemetría y cuatro encoders;
- configuración de IP;
- sesiones SQLite y exportación CSV;
- pruebas de contrato contra `/api/v1`.

Después de demostrar esa paridad, Flask podrá servir el build de Vite sin que
Node permanezca ejecutándose en el sistema final.

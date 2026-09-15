---
name: git-commits
description: Guía y directivas estandarizadas para preparar, validar, generar mensajes de commit semánticos y realizar push en el repositorio carroTest3-For-Esp32-s3-wroom. Úsala siempre que el usuario pida "hacer commit", "generar commit", "hacer push", "commitear cambios" o documentar un hito de trabajo.
---

# Procedimiento Estandarizado de Git: Commits y Push

Esta skill define el protocolo obligatorio para preparar, validar, redactar mensajes descriptivos y realizar `commit` y `push` en el repositorio `carroTest3-For-Esp32-s3-wroom`.

---

## 1. Verificación Previa y Limpieza (Pre-flight Checklist)

Antes de preparar cualquier commit, verificar el estado del árbol de trabajo:
```powershell
git status -u
git diff
```

### Reglas de Exclusión Estricta
Bajo ninguna circunstancia se deben confirmar:
- Salidas de compilación o binarios generados en local (`.pio/`, `desktop_app/build/`, `desktop_app/dist/`).
- Credenciales o archivos de entorno local (`include/Secrets.h`, `.env`).
- Archivos temporales o extracciones de bases de datos (`tmp_db/`, bases sqlite de prueba, `*.log`).
- Entornos virtuales o dependencias externas (`.test-venv/`, `.venv/`, `node_modules/`).

---

## 2. Validación de Compilación y Pruebas Unitarias

Antes de confirmar cualquier cambio en firmware o backend, ejecutar obligatoriamente las tres suites de pruebas:

1. **Pruebas Nativas PlatformIO**:
   ```powershell
   & "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" test -e pruebas_control_ruta_nativas
   ```
2. **Compilación de Firmware ESP32-S3**:
   ```powershell
   & "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e esp32-s3-devkitc-1
   ```
3. **Pruebas de Backend Python (Desktop App / Gateway)**:
   ```powershell
   $env:PYTHONPATH='desktop_app'; & "C:\Users\IK\AppData\Local\hermes\bin\uv.exe" run --with-requirements desktop_app/requirements.txt --python 3.11 python -m unittest discover desktop_app/tests
   ```

Si alguna prueba falla o la compilación arroja errores, resolver la falla antes de proceder al commit.

---

## 3. Estructura y Redacción de Commits Semánticos

Seguir el estándar de **Conventional Commits**:
```text
<tipo>(<alcance>): <descripción corta y clara en imperativo/presente>

- <Punto clave 1: diagnóstico, puntuación de rendimiento o causa física>
- <Punto clave 2: cambios específicos implementados por módulo>
- <Punto clave 3: ajustes en parámetros de configuración o versión de firmware>
- <Punto clave 4: posibles soluciones o próximos pasos para iteraciones futuras>
```

### Tipos Permitidos
- `fix`: Corrección de errores de firmware, cinemática, lazo de control o backend.
- `feat`: Nueva funcionalidad de usuario, comando o modo de operación.
- `refactor`: Reestructuración de código sin alterar el comportamiento observable.
- `test`: Nuevas pruebas unitarias o ajustes en tests existentes.
- `docs`: Documentación técnica, manuales o diagramas.
- `chore`: Mantenimiento de configuración, dependencias o tooling.

### Alcances Sugeridos (Scopes)
`control`, `cinematica`, `motores`, `odometria`, `mpu`, `hmi`, `gateway`, `red`, `seguridad`, `calibracion`.

---

## 4. Staging y Ejecución del Commit

1. **Staging Selectivo**:
   Agregar únicamente los archivos explícitos que corresponden al cambio (evitar `git add .` indiscriminado):
   ```powershell
   git add include/Config.h include/ControlRuta.h src/Cinematica.cpp
   ```

2. **Crear Commit con Mensaje Multilínea**:
   ```powershell
   git commit -m "fix(control): descripción concisa

   - Detalle de la modificación 1.
   - Detalle de la modificación 2.
   - Evaluación de exactitud o comportamiento observado."
   ```

3. **Verificar el Resultado**:
   ```powershell
   git log -n 1
   ```

---

## 5. Publicación al Repositorio Remoto (Push)

1. Verificar la rama activa y el upstream:
   ```powershell
   git status
   ```
2. Ejecutar `push` hacia la rama remota:
   ```powershell
   git push origin main
   ```
3. Confirmar que la salida indique `main -> main` sin rechazos ni conflictos.

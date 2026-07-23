@echo off
setlocal
set "APP_ROOT=%~dp0.."
if not exist "%APP_ROOT%\.venv\Scripts\python.exe" (
  echo.
  echo Falta instalar el entorno local de Python.
  echo Ejecuta INICIAR_ROBOT.bat desde la carpeta principal.
  echo.
  pause
  exit /b 1
)
"%APP_ROOT%\.venv\Scripts\python.exe" "%APP_ROOT%\app.py" %*
if errorlevel 1 (
  echo.
  echo La app termino con un error. Revisa el mensaje anterior.
  pause
)

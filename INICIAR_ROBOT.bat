@echo off
setlocal
cd /d "%~dp0"

if not exist "desktop_app\.venv\Scripts\python.exe" (
  echo ============================================================
  echo  Primera instalacion del panel Robot ESP32-S3
  echo ============================================================
  echo Se creara un entorno Python dentro de desktop_app\.venv.
  echo Esta primera instalacion necesita Internet.
  echo.
  powershell -NoProfile -ExecutionPolicy Bypass -File "desktop_app\scripts\install_online.ps1"
  if errorlevel 1 (
    echo.
    echo ERROR: no se pudieron instalar las dependencias.
    echo Comprueba Internet y que Python 3.11 este instalado.
    pause
    exit /b 1
  )
)

call "desktop_app\scripts\run.bat"

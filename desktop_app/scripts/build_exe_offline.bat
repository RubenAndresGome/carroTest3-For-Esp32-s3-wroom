@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build_exe.ps1" -Offline
if errorlevel 1 (
  echo.
  echo No se pudo construir RobotS3.exe con el paquete offline.
  pause
  exit /b 1
)
echo.
echo RobotS3.exe esta en desktop_app\dist\
pause

@echo off
setlocal
for %%I in ("%~dp0..\..") do set "REPO_ROOT=%%~fI"
for /f "usebackq delims=" %%V in ("%REPO_ROOT%\.node-version") do set "NODE_VERSION=%%V"
set "NODE=%REPO_ROOT%\.tools\node-v%NODE_VERSION%-win-x64\node.exe"
if not exist "%NODE%" (
  echo Falta Node local. Ejecuta desktop_app\scripts\frontend\preparar_frontend.ps1.
  exit /b 1
)
cd /d "%REPO_ROOT%"
"%NODE%" scripts\desarrollo\validar_hmi.js

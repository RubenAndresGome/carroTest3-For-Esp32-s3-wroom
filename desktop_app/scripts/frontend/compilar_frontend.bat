@echo off
setlocal
for %%I in ("%~dp0..\..\..") do set "REPO_ROOT=%%~fI"
for /f "usebackq delims=" %%V in ("%REPO_ROOT%\.node-version") do set "NODE_VERSION=%%V"
set "NPM=%REPO_ROOT%\.tools\node-v%NODE_VERSION%-win-x64\npm.cmd"
set "PATH=%REPO_ROOT%\.tools\node-v%NODE_VERSION%-win-x64;%PATH%"
if not exist "%NPM%" (
  echo Falta Node local. Ejecuta desktop_app\scripts\frontend\preparar_frontend.ps1.
  pause
  exit /b 1
)
cd /d "%REPO_ROOT%\desktop_app\frontend"
call "%NPM%" run check
if errorlevel 1 exit /b 1
call "%NPM%" run test
if errorlevel 1 exit /b 1
call "%NPM%" run build

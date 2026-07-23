@echo off
setlocal
set "APP_ROOT=%~dp0.."
py -3.11 -m venv "%APP_ROOT%\.venv"
if errorlevel 1 exit /b 1
"%APP_ROOT%\.venv\Scripts\python.exe" -m pip install --no-index --find-links "%APP_ROOT%\vendor\wheels\windows-py311" -r "%APP_ROOT%\requirements.lock"
if errorlevel 1 exit /b 1
echo Instalacion offline terminada. Ejecuta scripts\run.bat

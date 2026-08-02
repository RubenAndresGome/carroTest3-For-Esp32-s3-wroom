@echo off
setlocal
cd /d "%~dp0\.."
set "DOC_PORT=8765"
set "DOC_URL=http://127.0.0.1:%DOC_PORT%/documentacionCompleta/site/index.html"

if exist "desktop_app\.venv\Scripts\python.exe" (
  set "DOC_PYTHON=desktop_app\.venv\Scripts\python.exe"
) else (
  where py >nul 2>nul
  if errorlevel 1 (
    echo ERROR: se necesita Python para servir el portal estatico local.
    echo Instala Python 3 o ejecuta primero INICIAR_ROBOT.bat.
    pause
    exit /b 1
  )
  set "DOC_PYTHON=py -3"
)

start "Robot S3 - portal documental" /min %DOC_PYTHON% -m http.server %DOC_PORT% --bind 127.0.0.1 --directory "%CD%"
timeout /t 1 /nobreak >nul
start "" "%DOC_URL%"


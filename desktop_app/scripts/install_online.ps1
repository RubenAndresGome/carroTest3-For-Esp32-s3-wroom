$ErrorActionPreference = "Stop"
$AppRoot = Split-Path -Parent $PSScriptRoot
$VenvPath = Join-Path $AppRoot ".venv"
$VenvPython = Join-Path $VenvPath "Scripts\python.exe"

# PlatformIO ya incluye Python 3.11 en este equipo. Se prefiere porque `py.exe`
# puede existir aunque no tenga ningún runtime registrado.
$PlatformIoPython = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\python.exe"
$PythonExe = $null
$PythonArgs = @()

if (Test-Path $PlatformIoPython) {
    $PythonExe = $PlatformIoPython
    $PythonArgs = @("-B")
} elseif (Get-Command py -ErrorAction SilentlyContinue) {
    & py -3.11 -c "import sys; raise SystemExit(0 if sys.version_info[:2] == (3, 11) else 1)" *> $null
    if ($LASTEXITCODE -eq 0) {
        $PythonExe = "py"
        $PythonArgs = @("-3.11")
    }
} elseif (Get-Command python -ErrorAction SilentlyContinue) {
    & python -c "import sys; raise SystemExit(0 if sys.version_info >= (3, 11) else 1)" *> $null
    if ($LASTEXITCODE -eq 0) {
        $PythonExe = "python"
    }
}

if (-not $PythonExe) {
    throw "No se encontró un Python compatible. Instala Python 3.11 o PlatformIO."
}

Write-Host "Usando Python: $PythonExe"
& $PythonExe @PythonArgs -m venv --clear $VenvPath
if ($LASTEXITCODE -ne 0 -or -not (Test-Path $VenvPython)) {
    throw "Python no pudo crear desktop_app\.venv."
}

& $VenvPython -m pip install --upgrade pip
if ($LASTEXITCODE -ne 0) {
    throw "No se pudo actualizar pip. Comprueba la conexión a Internet."
}

& $VenvPython -m pip install -r (Join-Path $AppRoot "requirements.lock")
if ($LASTEXITCODE -ne 0) {
    throw "No se pudieron instalar las dependencias de requirements.lock."
}

Write-Host "Instalación terminada. Iniciando el panel..."

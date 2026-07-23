param(
    [switch]$Offline
)

$ErrorActionPreference = "Stop"
$AppRoot = Split-Path -Parent $PSScriptRoot
$BuildVenv = Join-Path $AppRoot ".build-venv"
$BuildPython = Join-Path $BuildVenv "Scripts\python.exe"
$WheelDir = Join-Path $AppRoot "vendor\wheels\windows-py311"
$PlatformIoPython = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\python.exe"

if (Test-Path $PlatformIoPython) {
    $BootstrapPython = $PlatformIoPython
} elseif (Get-Command py -ErrorAction SilentlyContinue) {
    $BootstrapPython = "py"
} else {
    throw "No se encontro Python 3.11 para construir RobotS3.exe."
}

if (-not (Test-Path $BuildPython)) {
    if ($BootstrapPython -eq "py") {
        & py -3.11 -m venv $BuildVenv
    } else {
        & $BootstrapPython -m venv $BuildVenv
    }
    if ($LASTEXITCODE -ne 0) { throw "No se pudo crear .build-venv." }
}

$PipArgs = @("-m", "pip", "install", "--disable-pip-version-check")
if ($Offline) {
    if (-not (Test-Path $WheelDir)) { throw "No existe el paquete de wheels offline: $WheelDir" }
    $PipArgs += @("--no-index", "--find-links", $WheelDir)
}
$PipArgs += @("-r", (Join-Path $AppRoot "requirements.lock"), "-r", (Join-Path $AppRoot "requirements-build.lock"))
& $BuildPython @PipArgs
if ($LASTEXITCODE -ne 0) { throw "No se pudieron instalar las dependencias de construccion." }

Push-Location $AppRoot
try {
    & $BuildPython -m PyInstaller --noconfirm --clean "RobotS3.spec"
    if ($LASTEXITCODE -ne 0) { throw "PyInstaller no pudo construir RobotS3.exe." }
} finally {
    Pop-Location
}

$Exe = Join-Path $AppRoot "dist\RobotS3.exe"
if (-not (Test-Path $Exe)) { throw "La construccion termino sin producir $Exe" }
Write-Host "Ejecutable listo: $Exe"

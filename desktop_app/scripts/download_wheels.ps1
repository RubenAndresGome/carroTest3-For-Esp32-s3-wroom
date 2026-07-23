$ErrorActionPreference = "Stop"
$AppRoot = Split-Path -Parent $PSScriptRoot
$WheelDir = "$AppRoot\vendor\wheels\windows-py311"
$PlatformIoPython = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\python.exe"
New-Item -ItemType Directory -Force -Path $WheelDir | Out-Null
if (Test-Path $PlatformIoPython) {
    & $PlatformIoPython -m pip download --only-binary=:all: --dest $WheelDir `
        -r "$AppRoot\requirements.lock" -r "$AppRoot\requirements-build.lock"
} elseif (Get-Command py -ErrorAction SilentlyContinue) {
    & py -3.11 -m pip download --only-binary=:all: --dest $WheelDir `
        -r "$AppRoot\requirements.lock" -r "$AppRoot\requirements-build.lock"
} else {
    throw "No se encontro Python 3.11 para descargar el paquete offline."
}
if ($LASTEXITCODE -ne 0) { throw "No se pudo completar el paquete de wheels offline." }
Write-Host "Paquete offline guardado en $WheelDir"

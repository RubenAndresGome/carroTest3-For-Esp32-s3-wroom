$ErrorActionPreference = "Stop"
$Root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\.."))
$Stage = Join-Path $Root ".pio\modular-validation"
$StageSrc = Join-Path $Stage "src"
$StageInclude = Join-Path $Stage "include"
$PlatformIo = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\platformio.exe"

if (-not (Test-Path $PlatformIo)) { throw "No se encontro PlatformIO: $PlatformIo" }
if (Test-Path $Stage) { Remove-Item -LiteralPath $Stage -Recurse -Force }
New-Item -ItemType Directory -Force -Path $StageSrc, $StageInclude | Out-Null
Copy-Item -Force (Join-Path $Root "platformio.ini") (Join-Path $Stage "platformio.ini")
Copy-Item -Force (Join-Path $Root "partitions.csv") (Join-Path $Stage "partitions.csv")
Copy-Item -Force (Join-Path $Root "include\*.h") $StageInclude
if (-not (Test-Path (Join-Path $StageInclude "Secrets.h"))) {
    Copy-Item -Force (Join-Path $Root "include\Secrets.example.h") (Join-Path $StageInclude "Secrets.h")
}

$Modules = @(
    "main.cpp", "Estado.cpp", "Eventos.cpp", "Motores.cpp", "Sensores.cpp",
    "Seguridad.cpp", "PoseEstimator.cpp", "Cinematica.cpp", "Red.cpp",
    "DiagnosticoRTOS.cpp"
)
foreach ($Module in $Modules) {
    $Source = Join-Path $Root ("src\" + $Module)
    if (-not (Test-Path $Source)) { throw "Falta el modulo activo: $Source" }
    Copy-Item -Force $Source (Join-Path $StageSrc $Module)
}

& $PlatformIo run --project-dir $Stage
if ($LASTEXITCODE -ne 0) { throw "El firmware modular activo no compila." }
Write-Host "Firmware modular activo validado correctamente."

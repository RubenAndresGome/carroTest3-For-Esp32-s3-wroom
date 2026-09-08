$ErrorActionPreference = "Stop"
$Root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\.."))
$Stage = Join-Path $Root ".pio\modular-validation"
$StageSrc = Join-Path $Stage "src"
$StageInclude = Join-Path $Stage "include"
$PlatformIoPython = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\python.exe"

if (-not (Test-Path $PlatformIoPython)) { throw "No se encontro Python de PlatformIO: $PlatformIoPython" }
if (Test-Path $Stage) { Remove-Item -LiteralPath $Stage -Recurse -Force }
New-Item -ItemType Directory -Force -Path $StageSrc, $StageInclude | Out-Null
Copy-Item -Force (Join-Path $Root "platformio.ini") (Join-Path $Stage "platformio.ini")
Copy-Item -Force (Join-Path $Root "partitions.csv") (Join-Path $Stage "partitions.csv")
Get-ChildItem -LiteralPath (Join-Path $Root "include") -Filter "*.h" -File |
    Where-Object { $_.Name -ne "Secrets.h" } |
    ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $StageInclude -Force }

# El staging siempre usa credenciales sintéticas y nunca lee ni copia el
# include/Secrets.h local. El archivo vive sólo dentro de .pio.
$ValidationSecrets = @'
#pragma once
constexpr char WIFI_AP_SSID[] = "ROBOT_S3_VALIDATION";
constexpr char WIFI_AP_PASSWORD[] = "validation-only";
'@
Set-Content -LiteralPath (Join-Path $StageInclude "Secrets.h") `
    -Value $ValidationSecrets -Encoding ascii

$Modules = @(
    "main.cpp", "Estado.cpp", "Eventos.cpp", "Motores.cpp", "Sensores.cpp",
    "Seguridad.cpp", "PoseEstimator.cpp", "Cinematica.cpp", "Red.cpp",
    "DiagnosticoRTOS.cpp", "MemoriaTorque.cpp"
)
foreach ($Module in $Modules) {
    $Source = Join-Path $Root ("src\" + $Module)
    if (-not (Test-Path $Source)) { throw "Falta el modulo activo: $Source" }
    Copy-Item -Force $Source (Join-Path $StageSrc $Module)
}

& $PlatformIoPython -m platformio run --project-dir $Stage
if ($LASTEXITCODE -ne 0) { throw "El firmware modular activo no compila." }
Write-Host "Firmware modular activo validado correctamente."

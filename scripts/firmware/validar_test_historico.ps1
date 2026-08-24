param(
    [Parameter(Mandatory = $true)]
    [ValidateSet(
        "test_mpu6050_angulo_unicamente_aprobado",
        "test_motores_funcion_fisica_verificada",
        "test_encoders_funcion_only_aprobado",
        "test_encoders_funcion_fisica_aprobado",
        "test_lazo_cerrado_mpu_encoders_aprobado"
    )]
    [string]$TestName
)

$ErrorActionPreference = "Stop"
$Root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\.."))
$Archive = Join-Path $Root "archive\firmware_tests"
$Source = Join-Path $Archive ($TestName + ".cpp.disabled")
$Stage = Join-Path $Root (".pio\historical-tests\" + $TestName)
$StageSrc = Join-Path $Stage "src"
$StageInclude = Join-Path $Stage "include"
$PlatformIo = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\platformio.exe"

if (-not (Test-Path $Source)) { throw "No existe el ensayo historico: $Source" }
if (-not (Test-Path $PlatformIo)) { throw "No se encontro PlatformIO: $PlatformIo" }

New-Item -ItemType Directory -Force -Path $StageSrc, $StageInclude | Out-Null
Copy-Item -Force (Join-Path $Root "platformio.ini") (Join-Path $Stage "platformio.ini")
Copy-Item -Force (Join-Path $Root "partitions.csv") (Join-Path $Stage "partitions.csv")
Get-ChildItem -LiteralPath (Join-Path $Root "include") -Filter "*.h" -File |
    Where-Object { $_.Name -ne "Secrets.h" } |
    ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $StageInclude -Force }

# Los ensayos archivados nunca reciben el Secrets.h local. La configuración
# sintética existe únicamente dentro del staging de .pio.
$ValidationSecrets = @'
#pragma once
constexpr char WIFI_AP_SSID[] = "ROBOT_S3_VALIDATION";
constexpr char WIFI_AP_PASSWORD[] = "validation-only";
'@
Set-Content -LiteralPath (Join-Path $StageInclude "Secrets.h") `
    -Value $ValidationSecrets -Encoding ascii
Copy-Item -Force $Source (Join-Path $StageSrc "main.cpp")

& $PlatformIo run --project-dir $Stage
if ($LASTEXITCODE -ne 0) { throw "El ensayo historico no compila: $TestName" }
Write-Host "Ensayo historico validado sin activar archivos en src: $TestName"

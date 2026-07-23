$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$androidSdk = if ($env:ANDROID_HOME) { $env:ANDROID_HOME } else { Join-Path $env:LOCALAPPDATA "Android\Sdk" }
$adb = Join-Path $androidSdk "platform-tools\adb.exe"
$apk = Join-Path $projectRoot "app\build\outputs\apk\debug\app-debug.apk"

if (-not (Test-Path $adb)) { throw "No se encontró adb dentro del Android SDK." }
if (-not (Test-Path $apk)) { throw "Primero ejecuta build_apk.ps1." }

$devices = @(& $adb devices | Select-Object -Skip 1 | Where-Object { $_ -match "\tdevice$" })
if ($devices.Count -ne 1) {
    throw "Conecta exactamente una tablet con Depuración USB autorizada. Detectadas: $($devices.Count)."
}

& $adb install -r $apk
if ($LASTEXITCODE -ne 0) { throw "adb no pudo instalar el APK." }
Write-Host "Robot S3 HMI instalado. Conecta Android a ROBOT_S3_LOCAL y abre la app." -ForegroundColor Green

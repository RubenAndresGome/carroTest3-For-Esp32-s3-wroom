[CmdletBinding()]
param(
    [string]$Serial,
    [switch]$SoloCompilar
)

$ErrorActionPreference = "Stop"

function Obtener-JavaHome {
    $candidatos = @()
    if ($env:JAVA_HOME) { $candidatos += $env:JAVA_HOME }
    $candidatos += "C:\Program Files\Android\Android Studio\jbr"
    $adoptium = Join-Path $env:ProgramFiles "Eclipse Adoptium"
    if (Test-Path $adoptium) {
        $candidatos += Get-ChildItem -LiteralPath $adoptium -Directory |
            Sort-Object Name -Descending |
            Select-Object -ExpandProperty FullName
    }

    foreach ($candidato in $candidatos | Select-Object -Unique) {
        $java = Join-Path $candidato "bin\java.exe"
        if (-not (Test-Path $java)) { continue }
        $inicio = [System.Diagnostics.ProcessStartInfo]::new()
        $inicio.FileName = $java
        $inicio.Arguments = "-version"
        $inicio.RedirectStandardError = $true
        $inicio.UseShellExecute = $false
        $proceso = [System.Diagnostics.Process]::Start($inicio)
        $version = $proceso.StandardError.ReadToEnd()
        $proceso.WaitForExit()
        if ($proceso.ExitCode -eq 0 -and $version -match 'version "(\d+)') {
            if ([int]$Matches[1] -ge 17) { return $candidato }
        }
    }
    throw "Se requiere un JDK 17 o posterior. Instala Android Studio/JDK 17+ o ajusta JAVA_HOME."
}

$raiz = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$androidApp = Join-Path $raiz "android_app"
$gradle = Join-Path $androidApp "gradlew.bat"
$apk = Join-Path $androidApp "app\build\outputs\apk\debug\app-debug.apk"
if (-not (Test-Path $gradle)) { throw "No se encontró android_app/gradlew.bat desde $PSScriptRoot." }

$sdkCandidatos = @($env:ANDROID_HOME, $env:ANDROID_SDK_ROOT, (Join-Path $env:LOCALAPPDATA "Android\Sdk")) |
    Where-Object { $_ } | Select-Object -Unique
$androidSdk = $sdkCandidatos | Where-Object { Test-Path (Join-Path $_ "platform-tools\adb.exe") } | Select-Object -First 1
if (-not $androidSdk) { throw "No se encontró Android SDK con platform-tools/adb.exe. Configura ANDROID_HOME o instala Android Studio." }

$env:JAVA_HOME = Obtener-JavaHome
$env:ANDROID_HOME = $androidSdk
$env:ANDROID_SDK_ROOT = $androidSdk
$adb = Join-Path $androidSdk "platform-tools\adb.exe"

Push-Location $androidApp
try {
    & $gradle --no-daemon assembleDebug --console=plain
    if ($LASTEXITCODE -ne 0) { throw "Gradle terminó con código $LASTEXITCODE." }
} finally {
    Pop-Location
}
if (-not (Test-Path $apk)) { throw "Gradle terminó sin generar $apk." }
$tamanoMiB = [math]::Round((Get-Item -LiteralPath $apk).Length / 1MB, 2)
$hash = (Get-FileHash -LiteralPath $apk -Algorithm SHA256).Hash
Write-Host "APK listo: $apk ($tamanoMiB MiB, SHA-256 $hash)" -ForegroundColor Green
if ($SoloCompilar) { exit 0 }

$dispositivos = @(& $adb devices | Select-Object -Skip 1 | ForEach-Object {
    if ($_ -match '^(\S+)\s+device$') { $Matches[1] }
})
if ($Serial) {
    if ($dispositivos -notcontains $Serial) { throw "La tablet '$Serial' no está autorizada por ADB." }
} elseif ($dispositivos.Count -eq 1) {
    $Serial = $dispositivos[0]
} else {
    throw "Conecta exactamente una tablet autorizada o indica -Serial. Detectadas: $($dispositivos.Count)."
}

& $adb -s $Serial install -r $apk
if ($LASTEXITCODE -ne 0) { throw "ADB no pudo instalar el APK." }
$paquete = & $adb -s $Serial shell pm path mx.ik.robots3
if ($LASTEXITCODE -ne 0 -or -not $paquete) { throw "No se pudo comprobar mx.ik.robots3 tras la instalación." }
Write-Host "HMI instalado en ${Serial}: $paquete" -ForegroundColor Green

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$androidSdk = if ($env:ANDROID_HOME) { $env:ANDROID_HOME } else { Join-Path $env:LOCALAPPDATA "Android\Sdk" }
$studioJava = "C:\Program Files\Android\Android Studio\jbr"

if (-not (Test-Path (Join-Path $androidSdk "platform-tools\adb.exe"))) {
    throw "No se encontró Android SDK. Instala Android Studio y el SDK 36."
}
if (-not $env:JAVA_HOME -and (Test-Path $studioJava)) {
    $env:JAVA_HOME = $studioJava
}
$env:ANDROID_HOME = $androidSdk

Push-Location $projectRoot
try {
    & ".\gradlew.bat" assembleDebug
    if ($LASTEXITCODE -ne 0) { throw "Gradle terminó con código $LASTEXITCODE" }
    $apk = Resolve-Path ".\app\build\outputs\apk\debug\app-debug.apk"
    $sizeMiB = [math]::Round((Get-Item $apk).Length / 1MB, 2)
    Write-Host "APK listo: $apk ($sizeMiB MiB)" -ForegroundColor Green
} finally {
    Pop-Location
}

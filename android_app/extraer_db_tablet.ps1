[CmdletBinding()]
param(
    [string]$Serial,
    [string]$Destino
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repositoryRoot = Split-Path -Parent $projectRoot
$androidSdk = if ($env:ANDROID_HOME) { $env:ANDROID_HOME } else { Join-Path $env:LOCALAPPDATA "Android\Sdk" }
$adb = Join-Path $androidSdk "platform-tools\adb.exe"
$packageName = "mx.ik.robots3"
$internalDb = "files/robot_s3/robot.sqlite3"

if (-not (Test-Path -LiteralPath $adb)) {
    throw "No se encontró adb dentro del Android SDK: $adb"
}

if (-not $Serial) {
    $devices = @(& $adb devices | Select-Object -Skip 1 | Where-Object { $_ -match "`tdevice$" })
    if ($devices.Count -ne 1) {
        throw "Conecta exactamente una tablet con Depuración USB autorizada o indica -Serial. Detectadas: $($devices.Count)."
    }
    $Serial = ($devices[0] -split "`t")[0]
}
if ($Serial -notmatch "^[A-Za-z0-9._:-]+$") {
    throw "El serial ADB contiene caracteres no permitidos."
}

if (-not $Destino) {
    $evidenceDirectory = Join-Path $repositoryRoot "tmp_db"
    New-Item -ItemType Directory -Force -Path $evidenceDirectory | Out-Null
    $Destino = Join-Path $evidenceDirectory ("robot_{0}.sqlite3" -f (Get-Date -Format "yyyyMMdd_HHmmss"))
}

$destinationDirectory = Split-Path -Parent $Destino
if (-not $destinationDirectory) {
    $destinationDirectory = (Get-Location).Path
    $Destino = Join-Path $destinationDirectory $Destino
}
New-Item -ItemType Directory -Force -Path $destinationDirectory | Out-Null
if (Test-Path -LiteralPath $Destino) {
    throw "El destino ya existe y no se sobrescribirá: $Destino"
}

$startInfo = New-Object System.Diagnostics.ProcessStartInfo
$startInfo.FileName = $adb
$startInfo.Arguments = "-s $Serial exec-out run-as $packageName cat $internalDb"
$startInfo.UseShellExecute = $false
$startInfo.CreateNoWindow = $true
$startInfo.RedirectStandardOutput = $true
$startInfo.RedirectStandardError = $true
$process = New-Object System.Diagnostics.Process
$process.StartInfo = $startInfo

try {
    [void]$process.Start()
    $stream = [System.IO.File]::Open($Destino, [System.IO.FileMode]::CreateNew,
                                     [System.IO.FileAccess]::Write, [System.IO.FileShare]::None)
    try {
        $process.StandardOutput.BaseStream.CopyTo($stream)
    } finally {
        $stream.Dispose()
    }
    $standardError = $process.StandardError.ReadToEnd()
    $process.WaitForExit()
    if ($process.ExitCode -ne 0 -or (Get-Item -LiteralPath $Destino).Length -eq 0) {
        throw "No se pudo leer la SQLite interna de $packageName. $standardError"
    }
} catch {
    if (Test-Path -LiteralPath $Destino) {
        Remove-Item -LiteralPath $Destino -Force
    }
    throw
} finally {
    $process.Dispose()
}

Write-Host "SQLite extraída: $Destino" -ForegroundColor Green
Write-Host "Analiza los tramos con:" -ForegroundColor Cyan
Write-Host "  python consultar_db.py --db `"$Destino`" --segments 20"

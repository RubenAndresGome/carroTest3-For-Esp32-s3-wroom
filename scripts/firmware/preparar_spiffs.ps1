[CmdletBinding()]
param(
    [string]$Puerto,
    [switch]$ConfirmarVmotDesconectado,
    [switch]$SoloValidar
)

$ErrorActionPreference = "Stop"

if (-not $SoloValidar -and -not $ConfirmarVmotDesconectado) {
    throw "Desconecta fisicamente VMOT y repite con -ConfirmarVmotDesconectado."
}

$raiz = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$particiones = Join-Path $raiz "partitions.csv"
$csvInicial = Join-Path $raiz "data\torque_history.csv"
$cabeceraEsperada = "version,sequence,pwm_positive_left_8bit,pwm_negative_left_8bit,positive_polarity,negative_polarity,pwm_positive_right_8bit,pwm_negative_right_8bit"

if (-not (Test-Path -LiteralPath $particiones)) {
    throw "No existe la tabla de particiones: $particiones"
}
if (-not (Test-Path -LiteralPath $csvInicial)) {
    throw "No existe el CSV inicial: $csvInicial"
}
$primeraLinea = Get-Content -LiteralPath $csvInicial -TotalCount 1
if ($primeraLinea.Trim() -ne $cabeceraEsperada) {
    throw "La cabecera de torque_history.csv no coincide con el formato del firmware."
}

$filas = Get-Content -LiteralPath $particiones |
    Where-Object { $_.Trim() -and -not $_.Trim().StartsWith("#") } |
    ConvertFrom-Csv -Header Name,Type,SubType,Offset,Size,Flags
$spiffs = @($filas | Where-Object {
    $_.Name.Trim() -eq "spiffs" -and $_.Type.Trim() -eq "data" -and $_.SubType.Trim() -eq "spiffs"
})
if ($spiffs.Count -ne 1) {
    throw "partitions.csv debe contener exactamente una particion data/spiffs."
}

function Convertir-NumeroParticion([string]$Valor) {
    $limpio = $Valor.Trim()
    if ($limpio -match '^0[xX][0-9a-fA-F]+$') {
        return [Convert]::ToInt64($limpio.Substring(2), 16)
    }
    return [Convert]::ToInt64($limpio, 10)
}

$tamanoSpiffs = Convertir-NumeroParticion $spiffs[0].Size
if ($tamanoSpiffs -le 0) {
    throw "La particion SPIFFS tiene un tamano invalido."
}

$pythonPioPreferido = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\python.exe"
if (Test-Path -LiteralPath $pythonPioPreferido) {
    $ejecutablePio = $pythonPioPreferido
    $prefijoPio = @("-m", "platformio")
} else {
    $comandoPio = Get-Command platformio -ErrorAction SilentlyContinue
    if (-not $comandoPio) { $comandoPio = Get-Command pio -ErrorAction SilentlyContinue }
    if (-not $comandoPio) { throw "No se encontro PlatformIO." }
    $ejecutablePio = $comandoPio.Source
    $prefijoPio = @()
}

Push-Location $raiz
try {
    & $ejecutablePio @prefijoPio run -e esp32-s3-devkitc-1 -t buildfs
    if ($LASTEXITCODE -ne 0) { throw "Fallo la construccion de SPIFFS." }

    $imagen = Join-Path $raiz ".pio\build\esp32-s3-devkitc-1\spiffs.bin"
    if (-not (Test-Path -LiteralPath $imagen)) { throw "PlatformIO no genero spiffs.bin." }
    $tamanoImagen = (Get-Item -LiteralPath $imagen).Length
    if ($tamanoImagen -gt $tamanoSpiffs) {
        throw "spiffs.bin ($tamanoImagen bytes) excede la particion ($tamanoSpiffs bytes)."
    }

    if ($SoloValidar) {
        Write-Host "SPIFFS validado: imagen de $tamanoImagen bytes para particion de $tamanoSpiffs bytes." -ForegroundColor Green
        return
    }

    $argumentos = @("run", "-e", "esp32-s3-devkitc-1", "-t", "uploadfs")
    if ($Puerto) { $argumentos += @("--upload-port", $Puerto) }
    & $ejecutablePio @prefijoPio @argumentos
    if ($LASTEXITCODE -ne 0) { throw "Fallo la carga inicial de SPIFFS; puede repetirse sin borrar NVS." }
} finally {
    Pop-Location
}

Write-Host "SPIFFS cargado sin erase. Reinicia y confirma torque_history.mounted=true y loaded=true." -ForegroundColor Green

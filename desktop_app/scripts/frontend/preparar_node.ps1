$ErrorActionPreference = "Stop"

$RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\..\.."))
$NodeVersion = (Get-Content (Join-Path $RepoRoot ".node-version") -Raw).Trim()
$ArchiveName = "node-v$NodeVersion-win-x64"
$ToolsRoot = Join-Path $RepoRoot ".tools"
$NodeRoot = Join-Path $ToolsRoot $ArchiveName
$NodeExe = Join-Path $NodeRoot "node.exe"
$ArchivePath = Join-Path $ToolsRoot "$ArchiveName.zip"
$DownloadUrl = "https://nodejs.org/dist/v$NodeVersion/$ArchiveName.zip"

New-Item -ItemType Directory -Force -Path $ToolsRoot | Out-Null

if (-not (Test-Path $NodeExe)) {
    Write-Host "Descargando Node.js $NodeVersion LTS portátil..."
    Invoke-WebRequest -Uri $DownloadUrl -OutFile $ArchivePath
    Expand-Archive -Path $ArchivePath -DestinationPath $ToolsRoot -Force
    Remove-Item -LiteralPath $ArchivePath -Force
}

if (-not (Test-Path $NodeExe)) {
    throw "No se encontró node.exe después de extraer el paquete."
}

Write-Host "Node preparado: $(& $NodeExe --version)"
Write-Host "Ubicación local: $NodeRoot"

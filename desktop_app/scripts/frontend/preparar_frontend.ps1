$ErrorActionPreference = "Stop"

$RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\..\.."))
$NodeVersion = (Get-Content (Join-Path $RepoRoot ".node-version") -Raw).Trim()
$NodeRoot = Join-Path $RepoRoot ".tools\node-v$NodeVersion-win-x64"
$NpmCmd = Join-Path $NodeRoot "npm.cmd"
$FrontendRoot = Join-Path $RepoRoot "desktop_app\frontend"

if (-not (Test-Path $NpmCmd)) {
    & (Join-Path $PSScriptRoot "preparar_node.ps1")
}
if (-not (Test-Path $NpmCmd)) {
    throw "Node local no está disponible."
}

Push-Location $FrontendRoot
try {
    $PreviousPath = $env:PATH
    $env:PATH = "$NodeRoot;$env:PATH"
    & $NpmCmd install --save-dev --save-exact vite typescript vitest '@types/node@24'
    if ($LASTEXITCODE -ne 0) { throw "npm install terminó con error." }
    & $NpmCmd run check
    if ($LASTEXITCODE -ne 0) { throw "La validación del frontend falló." }
    & $NpmCmd run build
    if ($LASTEXITCODE -ne 0) { throw "El build del frontend falló." }
} finally {
    $env:PATH = $PreviousPath
    Pop-Location
}

Write-Host "Entorno frontend preparado y validado."

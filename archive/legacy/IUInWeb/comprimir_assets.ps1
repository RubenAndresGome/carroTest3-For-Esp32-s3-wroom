$ErrorActionPreference = "Stop"
$SourceDir = $PSScriptRoot
$DataDir = Join-Path $SourceDir "data"
New-Item -ItemType Directory -Force -Path $DataDir | Out-Null

# Herramienta historica del mockup ESP32. No alimenta al HMI Python actual.
$files = Get-ChildItem (Join-Path $SourceDir "*") -Include "*.html","*.js","*.css"
foreach ($file in $files) {
    $dest = Join-Path $DataDir "$($file.Name).gz"
    # Usar .NET GzipStream para comprimir
    $input = [System.IO.File]::ReadAllBytes($file.FullName)
    $ms = New-Object System.IO.MemoryStream
    $gz = New-Object System.IO.Compression.GZipStream($ms, [System.IO.Compression.CompressionLevel]::Optimal)
    $gz.Write($input, 0, $input.Length)
    $gz.Close()
    $compressedBytes = $ms.ToArray()
    [System.IO.File]::WriteAllBytes($dest, $compressedBytes)
    Write-Host "Comprimido: $($file.Name) -> $($file.Name).gz ($($input.Length) -> $($compressedBytes.Length) bytes)"
}

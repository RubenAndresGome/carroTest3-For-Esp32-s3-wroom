# Script de automatización de Microsoft Word COM para actualizar campos, TOC y Tabla de Figuras
$ErrorActionPreference = "Stop"

$docPath = "D:\ITESS\TICS\VERANO 2026\TEST_RESIDENCIAS\AnteProyecto\documentos_corregidos\informe\informe_Ruben_Andres_Gomez_Molina_alfa_0_6_2.docx"
Write-Host "Abriendo Microsoft Word para actualizar campos..."
$word = New-Object -ComObject Word.Application
$word.Visible = $false
$word.DisplayAlerts = 0

try {
    $doc = $word.Documents.Open($docPath)
    Write-Host "Documento abierto. Páginas totales iniciales:" $doc.ComputeStatistics(2)
    
    # 1. Actualizar todos los campos del cuerpo (PAGEREF de figuras, etc.)
    Write-Host "Actualizando campos de documento (Fields.Update())..."
    $doc.Fields.Update()
    
    # 2. Actualizar Tabla de Contenidos (TOC)
    if ($doc.TablesOfContents.Count -ge 1) {
        Write-Host "Actualizando Tabla de Contenidos (TablesOfContents(1).Update())..."
        $doc.TablesOfContents.Item(1).Update()
    }
    
    # 3. Actualizar Tablas de Ilustraciones si existen
    if ($doc.TablesOfFigures.Count -ge 1) {
        Write-Host "Actualizando Tabla de Ilustraciones (TablesOfFigures(1).Update())..."
        $doc.TablesOfFigures.Item(1).Update()
    }
    
    # 4. Guardar cambios
    Write-Host "Guardando documento actualizado..."
    $doc.Save()
    Write-Host "Páginas totales finales:" $doc.ComputeStatistics(2)
    
    # 5. Exportar PDF actualizado
    $pdfPath = [System.IO.Path]::ChangeExtension($docPath, ".pdf")
    Write-Host "Exportando PDF a: $pdfPath..."
    $doc.ExportAsFixedFormat($pdfPath, 17) # 17 = wdExportFormatPDF
    Write-Host "PDF generado exitosamente."

    $doc.Close([ref]$true)
    Write-Host "Proceso completado exitosamente."
}
catch {
    Write-Error ("Error actualizando Word: " + $_.Exception.Message)
    if ($doc -ne $null) {
        $doc.Close([ref]$false)
    }
}
finally {
    $word.Quit()
}

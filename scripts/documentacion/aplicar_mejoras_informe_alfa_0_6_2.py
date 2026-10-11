"""
Script integral para aplicar las mejoras académicas y correcciones formales al Informe de Residencia Profesional:
1. Rediseño del Abstract en inglés (humilde, riguroso, forense, corrección del 79.7% en step commands, límites físicos y valor del baseline reproducible).
2. Reescribir el Resumen en español (elegante, coherente, eliminando listas crudas de carpetas y adoptando perspectiva de ingeniería en TICs).
3. Renumeración secuencial estricta de figuras (1 a 37) en cuerpo, tablas y collage de taller, erradicando saltos de página en el índice.
4. Generación de marcadores OpenXML (_TocFig01 a _TocFig37) y reconstrucción limpia de la Tabla de Figuras en el front matter.
5. Limpieza del Anexo D (eliminación de '[' sobrante) y estandarización del Anexo E de Gantt.
6. Erradicación de la 'versión Alfa 0.01' en recomendaciones por 'Firmware ESP32-S3 Milestone Funcional 1.0 (v1.0.0-rc2)'.
7. Actualización de versión en la portada (. Alfa 0.6.2).
"""

import os
import sys
from lxml import etree as ET
import docx

W_NS = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
W14_NS = "http://schemas.microsoft.com/office/word/2010/wordml"

SRC_PATH = r"D:\ITESS\TICS\VERANO 2026\TEST_RESIDENCIAS\AnteProyecto\documentos_corregidos\informe\informe_Ruben_Andres_Gomez_Molina_alfa_0_6_1.docx"
DST_PATH = r"D:\ITESS\TICS\VERANO 2026\TEST_RESIDENCIAS\AnteProyecto\documentos_corregidos\informe\informe_Ruben_Andres_Gomez_Molina_alfa_0_6_2.docx"

def qn(tag):
    return f"{{{W_NS}}}{tag}"

def set_p_text_and_bookmark(target, text, bm_id, bm_name):
    """Limpia el contenido textual del párrafo y coloca el nuevo texto con un bookmark."""
    p_elem = target if isinstance(target, ET._Element) else target._element
    
    # Eliminar runs, hyperlinks y bookmarks previos
    for child in list(p_elem):
        tag = child.tag.split('}')[-1]
        if tag in ['r', 'hyperlink', 'bookmarkStart', 'bookmarkEnd']:
            p_elem.remove(child)
            
    # Insertar bookmarkStart
    bm_start = ET.Element(qn('bookmarkStart'))
    bm_start.set(qn('id'), str(bm_id))
    bm_start.set(qn('name'), bm_name)
    p_elem.append(bm_start)
    
    # Insertar run con texto
    r = ET.Element(qn('r'))
    t = ET.SubElement(r, qn('t'))
    t.text = text
    p_elem.append(r)
    
    # Insertar bookmarkEnd
    bm_end = ET.Element(qn('bookmarkEnd'))
    bm_end.set(qn('id'), str(bm_id))
    p_elem.append(bm_end)

def crear_entry_tabla_figuras(titulo, bm_name):
    """Genera el párrafo XML para una fila en la Tabla de Ilustraciones con PAGEREF."""
    p = ET.Element(qn('p'))
    p_pr = ET.SubElement(p, qn('pPr'))
    p_style = ET.SubElement(p_pr, qn('pStyle'))
    p_style.set(qn('val'), 'Tabladeilustraciones')
    tabs = ET.SubElement(p_pr, qn('tabs'))
    tab = ET.SubElement(tabs, qn('tab'))
    tab.set(qn('val'), 'right')
    tab.set(qn('leader'), 'dot')
    tab.set(qn('pos'), '10337')
    spacing = ET.SubElement(p_pr, qn('spacing'))
    spacing.set(qn('before'), '40')
    spacing.set(qn('after'), '40')
    
    hlink = ET.SubElement(p, qn('hyperlink'))
    hlink.set(qn('anchor'), bm_name)
    hlink.set(qn('history'), '1')
    
    # Run 1: Título de la figura
    r1 = ET.SubElement(hlink, qn('r'))
    r1_pr = ET.SubElement(r1, qn('rPr'))
    r1_style = ET.SubElement(r1_pr, qn('rStyle'))
    r1_style.set(qn('val'), 'Referenciasutil')
    t1 = ET.SubElement(r1, qn('t'))
    t1.text = titulo
    
    # Run 2: Tabulador con puntos
    r2 = ET.SubElement(hlink, qn('r'))
    r2_pr = ET.SubElement(r2, qn('rPr'))
    r2_style = ET.SubElement(r2_pr, qn('rStyle'))
    r2_style.set(qn('val'), 'Referenciasutil')
    ET.SubElement(r2, qn('tab'))
    
    # Run 3: Field Begin
    r3 = ET.SubElement(hlink, qn('r'))
    r3_pr = ET.SubElement(r3, qn('rPr'))
    r3_style = ET.SubElement(r3_pr, qn('rStyle'))
    r3_style.set(qn('val'), 'Referenciasutil')
    fld_begin = ET.SubElement(r3, qn('fldChar'))
    fld_begin.set(qn('fldCharType'), 'begin')
    
    # Run 4: InstrText PAGEREF
    r4 = ET.SubElement(hlink, qn('r'))
    r4_pr = ET.SubElement(r4, qn('rPr'))
    r4_style = ET.SubElement(r4_pr, qn('rStyle'))
    r4_style.set(qn('val'), 'Referenciasutil')
    instr = ET.SubElement(r4, qn('instrText'))
    instr.set('{http://www.w3.org/XML/1998/namespace}space', 'preserve')
    instr.text = f" PAGEREF {bm_name} \\h "
    
    # Run 5: Field Separate
    r5 = ET.SubElement(hlink, qn('r'))
    r5_pr = ET.SubElement(r5, qn('rPr'))
    r5_style = ET.SubElement(r5_pr, qn('rStyle'))
    r5_style.set(qn('val'), 'Referenciasutil')
    fld_sep = ET.SubElement(r5, qn('fldChar'))
    fld_sep.set(qn('fldCharType'), 'separate')
    
    # Run 6: Placeholder número de página
    r6 = ET.SubElement(hlink, qn('r'))
    r6_pr = ET.SubElement(r6, qn('rPr'))
    r6_style = ET.SubElement(r6_pr, qn('rStyle'))
    r6_style.set(qn('val'), 'Referenciasutil')
    ET.SubElement(r6_pr, qn('noProof'))
    t6 = ET.SubElement(r6, qn('t'))
    t6.text = "1"
    
    # Run 7: Field End
    r7 = ET.SubElement(hlink, qn('r'))
    r7_pr = ET.SubElement(r7, qn('rPr'))
    r7_style = ET.SubElement(r7_pr, qn('rStyle'))
    r7_style.set(qn('val'), 'Referenciasutil')
    fld_end = ET.SubElement(r7, qn('fldChar'))
    fld_end.set(qn('fldCharType'), 'end')
    
    return p

def main():
    print(f"Cargando documento base: {SRC_PATH}")
    doc = docx.Document(SRC_PATH)

    # 1. ACTUALIZAR PORTADA (Textbox con . Alfa 0.6.1 -> . Alfa 0.6.2)
    print("1. Actualizando versión en portada...")
    for t_elem in doc._element.iter(qn('t')):
        if t_elem.text and ". Alfa 0.6.1" in t_elem.text:
            t_elem.text = t_elem.text.replace(". Alfa 0.6.1", ". Alfa 0.6.2")
            print("   -> Versión de portada actualizada a '. Alfa 0.6.2'")

    # 2. RESUMEN EN ESPAÑOL (P17 a P23)
    print("2. Refactorizando Resumen en español...")
    nuevo_resumen_p1 = (
        "Este proyecto de residencia profesional documenta el diseño embebido, instrumentación y "
        "evaluación experimental de un prototipo robótico móvil 4WD de bajo costo desarrollado como banco de "
        "pruebas para la carrera de Ingeniería en Tecnologías de la Información y Comunicaciones (TICS). "
        "El sistema integra un microcontrolador ESP32-S3, encoders ópticos ranurados LM393 leídos mediante "
        "contadores de pulsos por hardware (PCNT) y una unidad de medición inercial MPU6050. La arquitectura de "
        "software desacopla tareas mediante un esquema asimétrico: el Core 1 ejecuta en bucle nativo un súper-ciclo "
        "síncrono y determinista a 100 Hz para filtrado de señales, control de rumbo en lazo cerrado y mitigación de "
        "atascos, mientras que el Core 0 atiende de forma no bloqueante la pila de red Wi-Fi, el servidor WebSocket y "
        "el protocolo transaccional en formato JSON (robot-s3-steps-v3). La supervisión de misión se implementó "
        "mediante una arquitectura distribuida tolerante a fallos, configurable de forma exclusiva en una aplicación de "
        "escritorio Windows o empaquetada para movilidad en una tablet Android mediante Chaquopy. La telemetría forense "
        "es registrada muestra a muestra a 100 Hz en una base de datos SQLite en modo Write-Ahead Logging (WAL) y "
        "extraída mediante scripts automatizados por ADB."
    )
    nuevo_resumen_p2 = (
        "Las evaluaciones dinámicas en pista sobre baldosa cerámica confirmaron la eficacia de la estabilización angular "
        "a 100 Hz, alcanzando un error medio de -0.04° ± 0.89° (con 93.5% de llegadas dentro de una tolerancia estricta de ±1.5°). "
        "Lejos de postular un sistema infalible, la experimentación física expuso de forma transparente las limitaciones del hardware: "
        "una tasa de éxito en comandos autónomos de paso (step) del 79.7% (94 de 118 tramos completados sin disparo de protecciones físicas; "
        "83.2% de éxito global en 268 comandos de sistema) y un subavance longitudinal sistemático de -8.63 ± 8.53 cm atribuible a la "
        "compresión viscoelástica de neumáticos de 66 mm y al deslizamiento durante el patinaje (skid-steering) sin calibración dinámica de "
        "su Centro Instantáneo de Rotación (ICR). La principal contribución de este trabajo radica en entregar una línea base ciberfísica "
        "abierta, instrumentada y reproducible (respaldada por 13,930 registros en SQLite, 74/74 pruebas unitarias aprobadas y un atlas "
        "técnico interactivo en GitHub Pages) que aísla cuantitativamente las fortalezas del control frente a las fronteras mecánicas del prototipo."
    )

    doc.paragraphs[17].text = nuevo_resumen_p1
    doc.paragraphs[18].text = nuevo_resumen_p2
    for p_idx in [19, 20, 21, 22, 23]:
        doc.paragraphs[p_idx].text = ""

    # 3. ABSTRACT EN INGLÉS (P47)
    print("3. Refactorizando Abstract en inglés...")
    nuevo_abstract = (
        "This professional residency project presents the embedded design, instrumentation, and experimental validation "
        "of a low-cost 4WD mobile robot prototype developed under academic and laboratory resource constraints. Centered on "
        "an ESP32-S3 microcontroller, dual slotted optical encoders (LM393 with hardware pulse counting via PCNT), and an "
        "MPU6050 6-DOF inertial measurement unit, the platform implements an asymmetric dual-core architecture: a deterministic "
        "100 Hz synchronous super-cycle in FreeRTOS on Core 1 dedicated to sensor filtering, heading PID control, and dynamic pivot "
        "maneuvers, decoupled from asynchronous WebSocket communications and JSON telemetry (protocol robot-s3-steps-v3) managed "
        "on Core 0. Forensics state vectors are persisted in real time into an SQLite database operating under Write-Ahead Logging (WAL) "
        "and audited post-run via ADB synchronization. Physical track testing across 94 discrete route steps confirmed effective heading "
        "stabilization, achieving a mean angular error of -0.04° ± 0.89° (with 93.5% of endpoints contained within a ±1.5° tolerance). "
        "Rather than claiming ideal autonomy, systematic metric evaluation revealed clear hardware boundaries: a step-command execution "
        "success rate of 79.7% (94 of 118 route segments completed without triggering stall watchdogs; 83.2% global protocol success across "
        "268 total system commands) and a systematic longitudinal slip error of -8.63 ± 8.53 cm attributable to uncalibrated Instantaneous "
        "Center of Rotation (ICR) dynamics and tire viscoelastic compliance on smooth tile surfaces. The primary contribution of this work "
        "lies in delivering an open, fully instrumented, and reproducible cyber-physical baseline (backed by 13,930 forensic database records, "
        "74/74 unit tests, and a live web technical atlas) that quantitatively isolates control strengths from mechanical friction limits, "
        "establishing an empirical foundation for future state-estimation research."
    )
    doc.paragraphs[47].text = nuevo_abstract

    # 4. RECOMENDACIONES (P1050)
    print("4. Estandarizando versión en Recomendaciones...")
    p1050 = doc.paragraphs[1050]
    if "versión Alfa 0.01" in p1050.text:
        p1050.text = p1050.text.replace(
            "versión Alfa 0.01",
            "Firmware ESP32-S3 Milestone Funcional 1.0, v1.0.0-rc2"
        )
        print("   -> Párrafo 1050 actualizado con éxito.")

    # 5. ANEXO D Y ANEXO GANTT (P1390 y P1397)
    print("5. Limpiando títulos de Anexos...")
    doc.paragraphs[1390].text = 'ANEXO D. PORTAL WEB “ROBOT S3 · ATLAS TÉCNICO Y MANUAL”'
    doc.paragraphs[1397].text = 'ANEXO E. DIAGRAMA DE GANTT Y CRONOGRAMA DE INGENIERÍA'
    if len(doc.paragraphs) > 1398:
        doc.paragraphs[1398].text = (
            "El cronograma detallado de 11 semanas (Semanas 1 a 11, del 15 de junio al 11 de septiembre de 2026), "
            "el desglose de 32 actividades ingenieriles bajo metodología híbrida (Cascada, Scrum y ciclo PDCA para "
            "contingencias de hardware), y la matriz de hitos de integración se encuentran documentados de forma interactiva "
            "en el archivo docs/cronograma_gantt_metodologia.html y en el libro de cálculo docs/cronograma_gantt_metodologia.xlsx."
        )

    # 6. RENUMERACIÓN SECUENCIAL DE FIGURAS EN EL CUERPO (37 FIGURAS)
    print("6. Renumerando las 37 figuras secuenciales en el cuerpo...")
    body = doc._body._body

    figuras_def = [
        (1, 225, 'p', "Figura 1. Instituto Tecnológico Superior de Salvatierra (Campus ITESS)"),
        (2, 241, 'p', "Figura 2. Diagrama conceptual de bloques del sistema en arquitectura general y contexto operativo"),
        (3, 383, 'p', "Figura 3. Diagrama simplificado de bloques del sistema y arquitectura general"),
        (4, 388, 'p', "Figura 4. Extracto de sistema y dogma de calibración inercial en GitHub Pages"),
        (5, 511, 'tbl_p', "Figura 5. Ensamble estructural del robot móvil 4WD sobre chasis de doble plataforma"),
        (6, 542, 'p', "Figura 6. Repositorio canónico de código fuente en GitHub (carroTest3-For-Esp32-s3-wroom)"),
        (7, 709, 'p', "Figura 7. Mapa conceptual de conexión del ESP32-S3 con la Etapa de Potencia motriz"),
        (8, 711, 'p', "Figura 8. Mapa conceptual de conexión digital de encoders ópticos mediante conversor TXS0108E"),
        (9, 713, 'p', "Figura 9. Mapa conceptual de conexionado de baterías Li-Ion 18650 y sensor MPU6050"),
        (10, 716, 'p', "Figura 10. Esquema de conexiones de la Zona de Potencia (módulos DRV8833 y motores DC)"),
        (11, 718, 'p', "Figura 11. Esquema de conexiones de la Zona de Control Digital (ESP32-S3, encoders y MPU6050)"),
        (12, 738, 'p', "Figura 12. Diagrama de Gantt y cronología gráfica de la metodología de desarrollo"),
        (13, 748, 'p', "Figura 13. Habilidad y script ejecutable para sincronización de firmware ESP32 y tablet Android vía ADB"),
        (14, 751, 'p', "Figura 14. Atlas técnico del robot y manual de referencia viva en GitHub Pages"),
        # Collage T9 (9 celdas)
        (15, (851, 0, 0), 'tbl_cell', "Figura 15. Ensamble de chasis y montaje de reductores TT amarillos a la plataforma inferior"),
        (16, (851, 0, 1), 'tbl_cell', "Figura 16. Primera iteración de la zona de potencia DC (eliminación de LEDs por interferencias)"),
        (17, (851, 0, 2), 'tbl_cell', "Figura 17. Montaje e integración del microcontrolador ESP32-S3 DevKitC-1 sobre el chasis"),
        (18, (851, 1, 0), 'tbl_cell', "Figura 18. Ensayos preliminares en prototipo no funcional y diagnóstico de hardware"),
        (19, (851, 1, 1), 'tbl_cell', "Figura 19. Prototipado en protoboard para conexión del sensor inercial MPU6050 y detección de falsos contactos"),
        (20, (851, 1, 2), 'tbl_cell', "Figura 20. Cableado inicial con falta de canon y documentación previa a la estandarización"),
        (21, (851, 2, 0), 'tbl_cell', "Figura 21. Montaje y alineación de los cuatro módulos de encoder óptico LM393"),
        (22, (851, 2, 1), 'tbl_cell', "Figura 22. Disposición de herramientas de taller y puntos de control en distintas superficies de ensayo"),
        (23, (851, 2, 2), 'tbl_cell', "Figura 23. Pista de pruebas métricas sobre azulejo cerámico con baliza de 2 metros"),
        # Resultados
        (24, 885, 'p', "Figura 24. Registro audiovisual de validación de mejoras en control PID y funciones motrices"),
        (25, 888, 'p', "Figura 25. Interfaz Gráfica de Usuario (HMI) en funcionamiento durante el control de misión"),
        (26, 891, 'p', "Figura 26. Registro preliminar de telemetría inercial y odométrica a 100 Hz"),
        (27, 895, 'tbl_p', "Figura 27. Diagrama esquemático completo de conexiones del sistema (versión de ingeniería)"),
        (28, 1067, 'p', "Figura 28. Prototipo móvil durante la ejecución de pruebas dinámicas en pista de suelo"),
        # Anexos
        (29, 1373, 'p', "Figura 29. Carta de Aceptación oficial de Residencia Profesional (Anexo A)"),
        (30, 1378, 'tbl_p', "Figura 30. Carta de Liberación oficial de Residencia Profesional (Anexo B)"),
        (31, 1383, 'tbl_p', "Figura 31. Esquema de conexiones del robot diseñado en Fritzing (Anexo C)"),
        (32, 1385, 'p', "Figura 32. Evidencias de la evolución física de prototipos y montajes mecánicos"),
        (33, 1389, 'p', "Figura 33. Evidencia de telemetría forense sincronizada a 100 Hz en base de datos SQLite"),
        (34, 1399, 'p', "Figura 34. Complicación resuelta en PlatformIO para carga de firmware a microcontrolador ESP32-S3"),
        (35, 1402, 'p', "Figura 35. Uso eficiente de herramientas de costo optimizado (DeepSeek) como ejecutor de planes"),
        (36, 1406, 'p', "Figura 36. Entorno de desarrollo Visual Studio Code utilizado en etapas tempranas"),
        (37, 1407, 'p', "Figura 37. Estrategias de control de versiones y trazabilidad de firmware para ESP32-S3"),
    ]

    base_bm_id = 9500
    for num, loc, tipo, titulo in figuras_def:
        bm_id = base_bm_id + num
        bm_name = f"_TocFig{num:02d}"
        
        target_p = None
        if tipo == 'p':
            target_p = body[loc]
        elif tipo == 'tbl_p':
            tbl_elem = body[loc]
            p_nodes = [e for e in tbl_elem.iter(qn('p'))]
            if p_nodes:
                target_p = p_nodes[-1]
        elif tipo == 'tbl_cell':
            tbl_loc, r_idx, c_idx = loc
            tbl_elem = body[tbl_loc]
            rows = [e for e in tbl_elem.iter(qn('tr'))]
            if r_idx < len(rows):
                cells = [e for e in rows[r_idx].iter(qn('tc'))]
                if c_idx < len(cells):
                    p_nodes = [e for e in cells[c_idx].iter(qn('p'))]
                    if p_nodes:
                        target_p = p_nodes[-1]
                        
        if target_p is not None:
            set_p_text_and_bookmark(target_p, titulo, bm_id, bm_name)

    # 7. RECONSTRUIR TABLA DE FIGURAS EN EL FRONT MATTER
    print("7. Reconstruyendo Tabla de Figuras en el front matter...")
    cat_idx = None
    ecuaciones_idx = None
    for i, child in enumerate(body[:120]):
        txt = ''.join(child.itertext()).strip().lower()
        if cat_idx is None and ('catálogo de ilustraciones' in txt or 'catalogo de ilustraciones' in txt):
            cat_idx = i
        elif cat_idx is not None and ('índice de ecuaciones' in txt or 'indice de ecuaciones' in txt):
            ecuaciones_idx = i
            break
            
    print(f"   Catálogo encontrado en Elem {cat_idx}, Ecuaciones en Elem {ecuaciones_idx}")
    if cat_idx is not None and ecuaciones_idx is not None:
        elems_to_remove = [body[k] for k in range(cat_idx + 1, ecuaciones_idx)]
        for elem in elems_to_remove:
            body.remove(elem)
            
        insert_point = cat_idx + 1
        for num, loc, tipo, titulo in figuras_def:
            bm_name = f"_TocFig{num:02d}"
            p_elem = crear_entry_tabla_figuras(titulo, bm_name)
            body.insert(insert_point, p_elem)
            insert_point += 1
        print(f"   Insertadas {len(figuras_def)} entradas limpias de figuras.")

    # 8. GUARDAR DOCUMENTO REVISADO
    print(f"8. Guardando documento modificado en: {DST_PATH}")
    doc.save(DST_PATH)
    print("Documento guardado con éxito.")

if __name__ == "__main__":
    main()

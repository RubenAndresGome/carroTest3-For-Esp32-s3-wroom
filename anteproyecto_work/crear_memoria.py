from pathlib import Path
from docx import Document
from docx.enum.section import WD_SECTION_START
from docx.enum.style import WD_STYLE_TYPE
from docx.enum.table import WD_ALIGN_VERTICAL
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
from docx.shared import Cm, Inches, Pt, RGBColor


OUT = Path("Memoria_Verano_Robot_Movil_ESP32_S3_2026_cronograma_corregido.docx")
BLUE = "2E74B5"
DARK_BLUE = "1F4D78"
LIGHT_BLUE = "F4F6F9"
GRAY = "666666"
INK = "1F1F1F"
CONTENT_WIDTH_DXA = 9360


def set_font(run, name="Calibri", size=11, color=INK, bold=None, italic=None):
    run.font.name = name
    run._element.rPr.rFonts.set(qn("w:ascii"), name)
    run._element.rPr.rFonts.set(qn("w:hAnsi"), name)
    run.font.size = Pt(size)
    run.font.color.rgb = RGBColor.from_string(color)
    if bold is not None:
        run.bold = bold
    if italic is not None:
        run.italic = italic


def shade(cell, fill):
    tc_pr = cell._tc.get_or_add_tcPr()
    shd = tc_pr.find(qn("w:shd"))
    if shd is None:
        shd = OxmlElement("w:shd")
        tc_pr.append(shd)
    shd.set(qn("w:fill"), fill)


def set_cell_margins(cell, top=80, start=120, bottom=80, end=120):
    tc_pr = cell._tc.get_or_add_tcPr()
    tc_mar = tc_pr.first_child_found_in("w:tcMar")
    if tc_mar is None:
        tc_mar = OxmlElement("w:tcMar")
        tc_pr.append(tc_mar)
    for side, value in (("top", top), ("start", start), ("bottom", bottom), ("end", end)):
        node = tc_mar.find(qn(f"w:{side}"))
        if node is None:
            node = OxmlElement(f"w:{side}")
            tc_mar.append(node)
        node.set(qn("w:w"), str(value))
        node.set(qn("w:type"), "dxa")


def set_table_geometry(table, widths, indent=120):
    table.autofit = False
    table_pr = table._tbl.tblPr
    tbl_w = table_pr.find(qn("w:tblW"))
    if tbl_w is None:
        tbl_w = OxmlElement("w:tblW")
        table_pr.append(tbl_w)
    tbl_w.set(qn("w:w"), str(sum(widths)))
    tbl_w.set(qn("w:type"), "dxa")
    tbl_ind = table_pr.find(qn("w:tblInd"))
    if tbl_ind is None:
        tbl_ind = OxmlElement("w:tblInd")
        table_pr.append(tbl_ind)
    tbl_ind.set(qn("w:w"), str(indent))
    tbl_ind.set(qn("w:type"), "dxa")
    grid = table._tbl.tblGrid
    for grid_col, width in zip(grid.gridCol_lst, widths):
        grid_col.set(qn("w:w"), str(width))
    for row in table.rows:
        for cell, width in zip(row.cells, widths):
            tc_pr = cell._tc.get_or_add_tcPr()
            tc_w = tc_pr.find(qn("w:tcW"))
            if tc_w is None:
                tc_w = OxmlElement("w:tcW")
                tc_pr.append(tc_w)
            tc_w.set(qn("w:w"), str(width))
            tc_w.set(qn("w:type"), "dxa")
            set_cell_margins(cell)
            cell.vertical_alignment = WD_ALIGN_VERTICAL.CENTER


def set_repeat_table_header(row):
    tr_pr = row._tr.get_or_add_trPr()
    header = tr_pr.find(qn("w:tblHeader"))
    if header is None:
        header = OxmlElement("w:tblHeader")
        tr_pr.append(header)
    header.set(qn("w:val"), "true")


def border_bottom(paragraph, color=BLUE, size="8", space="4"):
    p_pr = paragraph._p.get_or_add_pPr()
    p_bdr = p_pr.find(qn("w:pBdr"))
    if p_bdr is None:
        p_bdr = OxmlElement("w:pBdr")
        p_pr.append(p_bdr)
    bottom = OxmlElement("w:bottom")
    bottom.set(qn("w:val"), "single")
    bottom.set(qn("w:sz"), size)
    bottom.set(qn("w:space"), space)
    bottom.set(qn("w:color"), color)
    p_bdr.append(bottom)


def add_page_number(paragraph):
    run = paragraph.add_run()
    fld_char1 = OxmlElement("w:fldChar")
    fld_char1.set(qn("w:fldCharType"), "begin")
    instr_text = OxmlElement("w:instrText")
    instr_text.set(qn("xml:space"), "preserve")
    instr_text.text = "PAGE"
    fld_char2 = OxmlElement("w:fldChar")
    fld_char2.set(qn("w:fldCharType"), "end")
    run._r.append(fld_char1)
    run._r.append(instr_text)
    run._r.append(fld_char2)
    set_font(run, size=9, color=GRAY)


def add_para(doc, text="", style="Normal", align=None, before=None, after=None, keep=False):
    p = doc.add_paragraph(style=style)
    if align is not None:
        p.alignment = align
    if before is not None:
        p.paragraph_format.space_before = Pt(before)
    if after is not None:
        p.paragraph_format.space_after = Pt(after)
    if keep:
        p.paragraph_format.keep_with_next = True
    if text:
        r = p.add_run(text)
        set_font(r)
    return p


def add_bullet(doc, text):
    p = doc.add_paragraph(style="List Bullet")
    p.paragraph_format.space_after = Pt(4)
    r = p.add_run(text)
    set_font(r)
    return p


def add_numbered(doc, text):
    p = doc.add_paragraph(style="List Number")
    p.paragraph_format.space_after = Pt(4)
    r = p.add_run(text)
    set_font(r)
    return p


def add_table(doc, headers, rows, widths, font_size=9.5):
    table = doc.add_table(rows=1, cols=len(headers))
    table.style = "Table Grid"
    hdr = table.rows[0].cells
    for cell, text in zip(hdr, headers):
        cell.text = ""
        p = cell.paragraphs[0]
        p.paragraph_format.space_after = Pt(0)
        r = p.add_run(text)
        set_font(r, size=font_size, color=INK, bold=True)
        shade(cell, LIGHT_BLUE)
    set_repeat_table_header(table.rows[0])
    for row in rows:
        cells = table.add_row().cells
        for cell, text in zip(cells, row):
            cell.text = ""
            p = cell.paragraphs[0]
            p.paragraph_format.space_after = Pt(0)
            r = p.add_run(text)
            set_font(r, size=font_size)
    set_table_geometry(table, widths)
    return table


def configure_document(doc):
    section = doc.sections[0]
    section.top_margin = Inches(1)
    section.bottom_margin = Inches(1)
    section.left_margin = Inches(1)
    section.right_margin = Inches(1)
    section.header_distance = Inches(0.492)
    section.footer_distance = Inches(0.492)

    normal = doc.styles["Normal"]
    normal.font.name = "Calibri"
    normal._element.rPr.rFonts.set(qn("w:ascii"), "Calibri")
    normal._element.rPr.rFonts.set(qn("w:hAnsi"), "Calibri")
    normal.font.size = Pt(11)
    normal.font.color.rgb = RGBColor.from_string(INK)
    normal.paragraph_format.alignment = WD_ALIGN_PARAGRAPH.JUSTIFY
    normal.paragraph_format.space_before = Pt(0)
    normal.paragraph_format.space_after = Pt(8)
    normal.paragraph_format.line_spacing = 1.333

    for name, size, color, before, after in (
        ("Heading 1", 16, BLUE, 18, 10),
        ("Heading 2", 13, BLUE, 12, 6),
        ("Heading 3", 12, DARK_BLUE, 8, 4),
    ):
        style = doc.styles[name]
        style.font.name = "Calibri"
        style._element.rPr.rFonts.set(qn("w:ascii"), "Calibri")
        style._element.rPr.rFonts.set(qn("w:hAnsi"), "Calibri")
        style.font.size = Pt(size)
        style.font.bold = True
        style.font.color.rgb = RGBColor.from_string(color)
        style.paragraph_format.space_before = Pt(before)
        style.paragraph_format.space_after = Pt(after)
        style.paragraph_format.keep_with_next = True

    for list_name in ("List Bullet", "List Number"):
        style = doc.styles[list_name]
        style.font.name = "Calibri"
        style._element.rPr.rFonts.set(qn("w:ascii"), "Calibri")
        style._element.rPr.rFonts.set(qn("w:hAnsi"), "Calibri")
        style.font.size = Pt(11)
        style.paragraph_format.space_after = Pt(4)
        style.paragraph_format.line_spacing = 1.208

    header = section.header.paragraphs[0]
    header.alignment = WD_ALIGN_PARAGRAPH.RIGHT
    header.text = ""
    r = header.add_run("Memoria de verano | Robot móvil ESP32-S3")
    set_font(r, size=9, color=GRAY)
    border_bottom(header, color="D7DBE2", size="4", space="3")
    footer = section.footer.paragraphs[0]
    footer.alignment = WD_ALIGN_PARAGRAPH.CENTER
    footer.text = ""
    r = footer.add_run("Página ")
    set_font(r, size=9, color=GRAY)
    add_page_number(footer)


def add_cover(doc):
    for _ in range(4):
        add_para(doc, "", after=0)
    p = add_para(doc, "INSTITUTO TECNOLÓGICO SUPERIOR DE SALVATIERRA", align=WD_ALIGN_PARAGRAPH.CENTER, after=12)
    set_font(p.runs[0], size=13, color=DARK_BLUE, bold=True)
    p = add_para(doc, "Memoria de Verano de Investigación 2026", align=WD_ALIGN_PARAGRAPH.CENTER, after=32)
    set_font(p.runs[0], size=14, color=GRAY, italic=True)
    p = add_para(doc, "Desarrollo y validación funcional de un robot móvil diferencial con ESP32-S3, MPU6050 y encoders", align=WD_ALIGN_PARAGRAPH.CENTER, after=16)
    set_font(p.runs[0], size=24, color=DARK_BLUE, bold=True)
    p.paragraph_format.line_spacing = 1.05
    p = add_para(doc, "Anteproyecto y memoria técnica", align=WD_ALIGN_PARAGRAPH.CENTER, after=42)
    set_font(p.runs[0], size=13, color=GRAY)
    rows = [
        ("Estudiante", "Rubén Andrés Gómez Molina"),
        ("Programa", "Tecnologías de la Información y Comunicaciones"),
        ("Asesor(a)", "[Por completar con el nombre oficial]"),
        ("Periodo", "15 de junio al 24 de julio de 2026"),
        ("Lugar", "Salvatierra, Guanajuato"),
    ]
    table = doc.add_table(rows=0, cols=2)
    table.style = "Table Grid"
    for label, value in rows:
        cells = table.add_row().cells
        cells[0].text = ""
        cells[1].text = ""
        p1 = cells[0].paragraphs[0]
        p2 = cells[1].paragraphs[0]
        p1.paragraph_format.space_after = Pt(0)
        p2.paragraph_format.space_after = Pt(0)
        r1 = p1.add_run(label)
        r2 = p2.add_run(value)
        set_font(r1, size=10.5, color=DARK_BLUE, bold=True)
        set_font(r2, size=10.5)
        shade(cells[0], LIGHT_BLUE)
    set_repeat_table_header(table.rows[0])
    set_table_geometry(table, [2550, 6810])
    add_para(doc, "", after=0)
    p = add_para(doc, "Agosto de 2026", align=WD_ALIGN_PARAGRAPH.CENTER, after=0)
    set_font(p.runs[0], size=11, color=GRAY)
    doc.add_page_break()


def add_contents(doc):
    add_para(doc, "Contenido", style="Heading 1")
    entries = [
        "Resumen", "Introducción", "1. Protocolo del proyecto", "2. Marco teórico",
        "3. Metodología y desarrollo", "4. Resultados y discusión", "5. Conclusiones y trabajo futuro",
        "Referencias", "Anexo A. Checklist de aceptación",
    ]
    for entry in entries:
        add_bullet(doc, entry)
    p = add_para(doc, "Nota: los títulos están estructurados con estilos de encabezado para que Word pueda actualizar la tabla de contenido automática si la institución la solicita.", after=0)
    p.runs[0].italic = True
    p.runs[0].font.color.rgb = RGBColor.from_string(GRAY)
    doc.add_page_break()


def add_body(doc):
    add_para(doc, "Resumen", style="Heading 1")
    add_para(doc, "Esta memoria documenta el desarrollo de un prototipo de robot móvil diferencial basado en un ESP32-S3, una unidad de medición inercial MPU6050, cuatro encoders y controladores de motor DRV8833. El propósito fue transformar un vehículo de control abierto en una plataforma modular y observable, capaz de adquirir señales de movimiento, estimar orientación y desplazamiento, aplicar control PWM con protecciones y comunicar telemetría a una interfaz local. La arquitectura separa el control físico de tiempo real, que permanece en el ESP32-S3, de la misión, la interfaz, el historial y la validación de entrada, que se administran con un backend Python y una HMI local. El trabajo se condujo mediante prototipado iterativo: verificación eléctrica y de cableado, pruebas unitarias de sensores y actuadores, integración de comunicación y validación funcional supervisada. Se reportan resultados de integración y limitaciones con prudencia: las evidencias disponibles muestran funcionamiento del prototipo, pero no sustentan todavía una afirmación de precisión metrológica ni de seguridad eléctrica para operación en suelo. El documento propone un protocolo de aceptación repetible y líneas de mejora para convertir el prototipo en una plataforma experimental cuantitativamente evaluable.")
    p = add_para(doc, "Palabras clave: robótica móvil, ESP32-S3, odometría, MPU6050, encoders, control con retroalimentación, WebSocket.", after=12)
    p.runs[0].bold = True

    add_para(doc, "Introducción", style="Heading 1")
    add_para(doc, "Un robot diferencial produce su trayectoria a partir de la relación entre las velocidades de sus lados izquierdo y derecho. Por ello, una orden temporal de avance o giro no es suficiente para asegurar el resultado físico: las diferencias mecánicas, la inercia, el deslizamiento, el ruido eléctrico y las variaciones de alimentación modifican el movimiento real. La odometría y la orientación estimada permiten observar ese movimiento y cerrar el ciclo de control (Siegwart et al., 2011).")
    add_para(doc, "El proyecto integra un ESP32-S3, un MPU6050, cuatro encoders y controladores DRV8833 para adquirir señales, estimar pose, modular la potencia de los motores y reportar telemetría. La HMI y el backend gestionan la misión y la bitácora; el firmware conserva las decisiones de seguridad y control físico. Esta división evita que operaciones de interfaz o red sustituyan los límites locales requeridos por un sistema embebido.")
    add_para(doc, "La memoria se formula como anteproyecto y reporte técnico del periodo de verano. Distingue con claridad lo implementado, lo observado en pruebas y lo que permanece como recomendación. El criterio es central: una evidencia de movimiento no equivale por sí sola a una validación de exactitud, resiliencia ante reinicios o seguridad eléctrica.")

    add_para(doc, "1. Protocolo del proyecto", style="Heading 1")
    add_para(doc, "1.1 Planteamiento del problema", style="Heading 2")
    add_para(doc, "El prototipo inicial de vehículo móvil dependía de comandos de potencia o de duración. Sin retroalimentación, no podía comprobar si había girado el ángulo solicitado, avanzado la distancia deseada o permanecido dentro de condiciones seguras. El problema de investigación consistió en diseñar e integrar una arquitectura de control y observación que relacionara referencias de movimiento con mediciones físicas y estados de seguridad.")
    add_para(doc, "1.2 Objetivo general", style="Heading 2")
    p = add_para(doc, "Implementar y validar funcionalmente un sistema de control para un robot móvil diferencial que ejecute referencias de avance, reversa y giro mediante un ESP32-S3, utilizando un MPU6050 y encoders para estimar orientación, desplazamiento y error de movimiento, con telemetría y mecanismos de parada segura.")
    p.runs[0].bold = True
    add_para(doc, "1.3 Objetivos específicos", style="Heading 2")
    for item in [
        "Configurar y calibrar el MPU6050 mediante I2C para obtener la velocidad angular del eje Z y estimar el yaw relativo.",
        "Leer los cuatro encoders mediante el periférico PCNT, agrupar sus conteos por lado y estimar desplazamiento y velocidad.",
        "Implementar control PWM de los motores, corrección de rumbo, interlock de inversión, E-STOP y watchdogs locales.",
        "Integrar el canal de telemetría y comandos con una HMI y backend Python, manteniendo una única propiedad del WebSocket hacia el robot.",
        "Definir pruebas de aceptación y criterios que separen la integración funcional de la validación cuantitativa y eléctrica.",
    ]:
        add_numbered(doc, item)
    add_para(doc, "1.4 Justificación", style="Heading 2")
    add_para(doc, "El proyecto articula sistemas embebidos, electrónica de potencia, control, robótica móvil y desarrollo de software en un sistema físico observable. El uso de encoders y una IMU no elimina por sí mismo los errores, pero proporciona evidencia para detectar desviaciones, estimar el estado y activar condiciones de paro. El enfoque es congruente con la teoría de control con retroalimentación: medir la salida permite compararla con una referencia y corregir el comportamiento del sistema (Franklin et al., 2019).")
    add_para(doc, "La contribución académica consiste en una arquitectura modular y documentada que permite aislar y evaluar los subsistemas de sensado, estimación, motores, comunicación e interfaz. Su contribución práctica es una base para ensayos reproducibles, siempre que las afirmaciones de desempeño se sostengan con medición física repetida.")
    add_para(doc, "1.5 Alcance, exclusiones y limitaciones", style="Heading 2")
    add_para(doc, "El alcance incluye un prototipo diferencial con avance, reversa, giros, lectura de IMU y encoders, PWM, comunicación WebSocket/JSON, telemetría, historial y una HMI local. En operación normal, el firmware conserva los lazos físicos de tiempo real, los límites, el E-STOP y los watchdogs; Python conserva la misión, la validación de entrada y el historial.")
    add_bullet(doc, "Se incluyen rutas ortogonales descompuestas en pasos atómicos, siempre que la geometría y el estado físico hayan sido validados.")
    add_bullet(doc, "No se afirma navegación autónoma general, precisión lineal o angular garantizada, ni seguridad eléctrica certificada.")
    add_bullet(doc, "No se opera Windows y Android como controladores simultáneos; cada sesión utiliza un único propietario del WebSocket.")
    add_bullet(doc, "La aceptación en suelo depende de pruebas físicas de corriente, fusibles, desacoplo, encoders y medición externa; no puede sustituirse con una compilación correcta.")
    add_para(doc, "1.6 Lugar y periodo", style="Heading 2")
    add_para(doc, "El trabajo se desarrolló en el marco del Verano de Investigación 2026, del 15 de junio al 24 de julio de 2026, con actividades de integración, documentación y validación de prototipo en Salvatierra, Guanajuato.")
    add_para(doc, "1.7 Cronograma ampliado de actividades", style="Heading 2")
    add_para(doc, "El siguiente cronograma se ajusta al calendario institucional agosto 2026-enero 2027 mostrado. Las actividades del estudiante inician el 25 de septiembre de 2026; las fechas previas de inicio de labores corresponden al personal docente y quedan fuera del alcance de este proyecto. Se consideran la evaluación sumativa del 14 al 18 de diciembre y el periodo vacacional a partir del 21 de diciembre. El proyecto cierra sus actividades programadas el 18 de diciembre. Cada bloque contempla evidencia documental, revisión de riesgos y una ventana explícita para cambios de alcance o solicitudes de escalabilidad. Las solicitudes nuevas se registrarán, analizarán y priorizarán antes de incorporarse; no se integrarán directamente al firmware de control sin pruebas y aceptación técnica.")
    rows = [
        ("1", "25 sep.-2 oct.", "Inicio de actividades del estudiante: revisión de la memoria de verano, inventario de componentes, actualización de requisitos, riesgos y casos de uso.", "Acta de inicio, inventario y matriz de riesgos."),
        ("2", "5-9 oct.", "Diseño detallado de arquitectura, diagrama eléctrico, mockups de HMI y definición de interfaces entre firmware, backend y base de datos.", "Requisitos priorizados, arquitectura versionada y plan de integración."),
        ("3", "13-23 oct.", "Preparación de PlatformIO, compilación base, revisión de pinout, continuidad, tierra común y desacoplo. El 12 de octubre queda fuera de actividades.", "Bitácora de puesta en marcha y lista de verificación eléctrica."),
        ("4", "26 oct.-6 nov.", "Pruebas aisladas de MPU6050 y PCNT; calibración de offsets, sentidos de encoder y diagnóstico de ruido o pérdida de pulsos. Se excluye el 2 de noviembre.", "Registro de calibración y reporte de sensado."),
        ("5", "9-13 nov.", "Integración de motores: PWM, rampas, inversión con interlock, E-STOP, watchdogs y pruebas con ruedas elevadas.", "Matriz de pruebas de actuadores y eventos de seguridad."),
        ("6", "17-27 nov.", "Integración de HMI, backend, WebSocket, JSON y bitácora SQLite; validación de comandos, telemetría y propiedad única del controlador. Se respeta el día no laborable 16 de noviembre.", "HMI funcional, esquema de logs y protocolo validado."),
        ("7", "30 nov.-4 dic.", "Revisión intermedia, análisis de incidencias, refactorización acotada y actualización de diagramas, manuales, repositorio y deuda técnica.", "Informe de avance, cambios documentados y plan de ajustes."),
        ("8", "7-11 dic.", "Pruebas de control de rumbo, avance, reversa, giros y rutas ortogonales; correlación de telemetría, video y medición física.", "Matriz de resultados funcionales y evidencia sincronizada."),
        ("9", "14-18 dic.", "Periodo de evaluación sumativa: análisis de escalabilidad, registro y priorización de peticiones, cierre técnico, anexos, manuales, presentación y entrega.", "Backlog de escalabilidad, paquete final de entrega y plan de continuidad."),
    ]
    add_table(doc, ["Semana", "Fechas", "Actividades", "Entregable"], rows, [900, 1400, 4600, 2460], font_size=8.8)
    p = add_para(doc, "Nota de calendario: del 21 de diciembre de 2026 al reinicio institucional de enero de 2027 se considera periodo vacacional; no se programan tareas de desarrollo ni pruebas físicas. Fuente: elaboración propia con base en el calendario institucional proporcionado.", after=12)
    p.runs[0].italic = True
    p.runs[0].font.size = Pt(9)
    p.runs[0].font.color.rgb = RGBColor.from_string(GRAY)

    add_para(doc, "2. Marco teórico", style="Heading 1")
    add_para(doc, "2.1 Robótica móvil diferencial y odometría", style="Heading 2")
    add_para(doc, "La cinemática diferencial relaciona la trayectoria con la velocidad de las ruedas. Si ambos lados se desplazan a velocidad semejante, el chasis avanza aproximadamente recto; con velocidades distintas describe una curva; y en sentidos opuestos puede pivotar. La odometría usa los giros de rueda para estimar distancia, pero acumula errores por tolerancias mecánicas, deslizamiento y pérdida de pulsos (Siegwart et al., 2011).")
    add_para(doc, "Para un encoder incremental, una aproximación de distancia por rueda puede expresarse como d = N*pi*D/PPR, donde N es el número de pulsos, D el diámetro efectivo y PPR los pulsos por revolución. El término 'efectivo' es importante: debe obtenerse por calibración física y no sólo del valor nominal de la rueda.")
    add_para(doc, "2.2 MPU6050 y estimación angular", style="Heading 2")
    add_para(doc, "El MPU6050 integra acelerómetro y giroscopio triaxiales con interfaz I2C. Para observar el giro horizontal se utiliza la velocidad angular del eje Z, se descuenta un offset obtenido en reposo y se integra la velocidad con un periodo de muestreo estable. Las especificaciones y el mapa de registros del dispositivo sustentan la configuración de rangos, filtros y comunicaciones (TDK InvenSense, 2013a, 2013b).")
    add_para(doc, "La integración de la velocidad angular puede representarse como theta_k = theta_(k-1) + omega_k*delta_t. Esta estimación es útil en maniobras breves, pero su error crece con el sesgo y el ruido; por ello el proyecto la trata como una fuente de retroalimentación relativa que debe contrastarse con encoders y mediciones externas.")
    add_para(doc, "2.3 ESP32-S3, PCNT y PWM", style="Heading 2")
    add_para(doc, "El ESP32-S3 dispone de periféricos para contar pulsos y generar PWM. PCNT cuenta flancos por hardware y permite filtrar pulsos breves, lo que reduce el trabajo que tendría que realizarse por interrupciones de propósito general (Espressif Systems, s. f.-a). LEDC permite configurar frecuencia y resolución de PWM para controlar el ciclo de trabajo de las señales de potencia (Espressif Systems, s. f.-b).")
    add_para(doc, "En el proyecto, Core 0 se reserva para la tarea de red y Core 1 ejecuta un superciclo síncrono de control de 100 Hz. En cada ciclo se adquieren PCNT e IMU, se actualiza la pose, se aplica seguridad y cinemática y finalmente se actualiza PWM. Esta organización evita colas intermedias que romperían la coherencia temporal de la muestra física.")
    add_para(doc, "2.4 Motor DC, puente H y seguridad", style="Heading 2")
    add_para(doc, "El DRV8833 es un controlador de dos puentes H para motores DC que admite control PWM y requiere considerar corriente, tensión, disipación y cableado apropiado (Texas Instruments, 2015). Por ello, la estrategia del proyecto limita PWM, inserta un periodo de cero al invertir sentido y conserva E-STOP y watchdogs en el microcontrolador. Estas medidas de firmware complementan, pero no sustituyen, fusible, capacitores de desacoplo, tierra común, corte físico de VMOT y medición de corriente.")
    add_para(doc, "2.5 Comunicación y separación de responsabilidades", style="Heading 2")
    add_para(doc, "WebSocket permite un canal bidireccional persistente para comandos, estados y telemetría (Fette & Melnikov, 2011). JSON define una representación intercambiable de estructuras de datos (Bray, 2017). El backend se implementa con Python/Flask para conservar la misión, la HMI, el historial y la validación; el firmware no delega las decisiones de paro, límites o watchdog al nivel de interfaz. Flask y PlatformIO aportan, respectivamente, el marco de aplicación y la organización reproducible del proyecto (Pallets Projects, s. f.; PlatformIO, s. f.).")

    add_para(doc, "3. Metodología y desarrollo", style="Heading 1")
    add_para(doc, "3.1 Tipo de trabajo", style="Heading 2")
    add_para(doc, "Se realizó un desarrollo tecnológico aplicado con validación funcional exploratoria. La metodología combinó investigación documental, prototipado incremental, observación de telemetría, revisión de código y pruebas físicas supervisadas. No se diseñó una campaña estadística suficiente para inferir exactitud poblacional; por eso los resultados se describen como funcionales y de integración.")
    add_para(doc, "3.2 Arquitectura implementada", style="Heading 2")
    rows = [
        ("Sensado", "MPU6050 por I2C y cuatro encoders por PCNT.", "Medición de yaw relativo, pulsos, distancia y velocidad por lado."),
        ("Control embebido", "ESP32-S3, Core 1, superciclo de 100 Hz.", "Pose, seguridad, cinemática, PWM y watchdogs."),
        ("Red", "ESP32-S3, Core 0.", "WebSocket, JSON, telemetría y estados."),
        ("Misión e HMI", "Backend Python/Flask y HMI local.", "Pasos atómicos, validación de entrada, historial y visualización."),
        ("Empaquetado móvil", "Aplicación Android con el mismo backend/HMI.", "Control alterno; propietario único cuando se usa la tablet."),
    ]
    add_table(doc, ["Capa", "Tecnologías", "Responsabilidad"], rows, [1900, 3000, 4460], font_size=9)
    p = add_para(doc, "Fuente: elaboración propia a partir de la arquitectura técnica vigente del proyecto.", after=12)
    p.runs[0].italic = True
    p.runs[0].font.size = Pt(9)
    p.runs[0].font.color.rgb = RGBColor.from_string(GRAY)
    add_para(doc, "3.3 Secuencia de desarrollo", style="Heading 2")
    for item in [
        "Verificación de alimentación, tierra común, pinout, continuidad y sentido de motores antes de ejecutar movimientos.",
        "Configuración del MPU6050 y calibración estática para obtener offset de giro; filtrado e integración de yaw en el ciclo de control.",
        "Lectura de encoders con PCNT y fusión por lado para estimar distancia y velocidad, conservando diagnóstico por canal.",
        "Pruebas aisladas de PWM, avance, reversa, giro, parada, inversión y recuperación de estados de seguridad.",
        "Integración de telemetría, comandos estructurados, HMI, SQLite e historial para observar cada maniobra.",
        "Revisión de resultados, documentación de fallas, reversión de cambios inseguros y definición de criterios de aceptación.",
    ]:
        add_numbered(doc, item)
    add_para(doc, "3.4 Criterios de aceptación y seguridad", style="Heading 2")
    add_para(doc, "La operación responsable requiere verificar que VMOT permanezca apagado durante carga y arranque, que los PWM estén en cero antes de habilitar motores y que exista acceso a corte de energía. En pruebas con ruedas elevadas se debe usar fuente limitada o fusible, medir corriente durante calibración, avance y giro, y comprobar E-STOP. Ninguna evidencia de software sustituye esta comprobación física.")
    rows = [
        ("Inicialización", "Sensores disponibles tras calibración; PWM en cero antes de movimiento.", "Telemetría, lectura de estado y observación física."),
        ("Encoders", "Conteo coherente por FL/FR/BL/BR y al menos una fuente válida por lado.", "Telemetría y bloqueo controlado por canal."),
        ("Giro", "Error angular converge sin exceder límites; parada en tolerancia.", "Yaw, gyro Z, PWM, eventos y medición externa."),
        ("Ruta", "Tramos ortogonales; error físico lateral y euclidiano dentro de objetivo experimental.", "Regla, video y SQLite sincronizados."),
        ("Seguridad", "E-STOP, watchdog, inversión con interlock y límites de PWM activos.", "Eventos, PWM cero y prueba supervisada."),
        ("Eléctrica", "Corriente sostenida menor a 1 A antes de uso en suelo.", "Fuente limitada y medición en serie de VMOT."),
    ]
    add_table(doc, ["Área", "Criterio", "Evidencia requerida"], rows, [1650, 4620, 3090], font_size=8.8)
    p = add_para(doc, "Nota: los umbrales finales de ruta deben validarse con una campaña repetida y no se infieren de una sola demostración.", after=12)
    p.runs[0].italic = True
    p.runs[0].font.size = Pt(9)
    p.runs[0].font.color.rgb = RGBColor.from_string(GRAY)

    add_para(doc, "4. Resultados y discusión", style="Heading 1")
    add_para(doc, "4.1 Resultados de integración", style="Heading 2")
    add_para(doc, "El resultado principal es una arquitectura integrada que permite observar los sensores, ejecutar órdenes de movimiento, actualizar PWM y comunicar estados hacia la HMI. Se documentó el funcionamiento de la inicialización del MPU6050, conteo independiente de encoders, avance y reversa con rampa y límites, giros con retroalimentación y canal de telemetría. También se añadieron interlocks, E-STOP y watchdogs para llevar la decisión de seguridad al lado del firmware.")
    rows = [
        ("MPU6050", "Lecturas disponibles después de calibración y cálculo de yaw relativo.", "Funcional para integración."),
        ("Encoders", "Pulsos por canales PCNT y diagnóstico por lado.", "Funcional; requiere campaña de precisión."),
        ("Motores", "PWM, rampa, inversión protegida y parada.", "Funcional con supervisión."),
        ("Giro", "Disminución de error angular y parada por tolerancia.", "Funcional; requiere medición externa repetida."),
        ("HMI-ESP32", "Comandos, estados y telemetría JSON por WebSocket.", "Funcional; único propietario por sesión."),
        ("Protecciones", "E-STOP, sensores inválidos, falta de progreso y límites.", "Implementadas; validar físicamente."),
    ]
    add_table(doc, ["Prueba", "Evidencia observada", "Interpretación"], rows, [1700, 4900, 2760], font_size=8.8)
    p = add_para(doc, "Fuente: síntesis de las pruebas funcionales y de la documentación técnica del proyecto.", after=12)
    p.runs[0].italic = True
    p.runs[0].font.size = Pt(9)
    p.runs[0].font.color.rgb = RGBColor.from_string(GRAY)
    add_para(doc, "4.2 Discusión", style="Heading 2")
    add_para(doc, "La elección de PCNT y LEDC asigna al hardware tareas repetitivas de conteo y modulación de pulsos, en lugar de trasladarlas a una rutina general de interrupciones. Esta decisión es coherente con las capacidades documentadas para el ESP32-S3 (Espressif Systems, s. f.-a, s. f.-b). De manera semejante, los límites, el interlock de inversión y la medición de corriente responden a las exigencias prácticas de un puente H y de motores DC, no sólo a una preferencia de software (Texas Instruments, 2015).")
    add_para(doc, "La evidencia también revela límites relevantes. Un encoder que no registra movimiento, un conector intermitente o el ruido eléctrico pueden simular un fallo de algoritmo. Asimismo, una reconexión de red o un reinicio del ESP32 puede dejar una distancia parcial desconocida; por seguridad, esa situación exige bloqueo y decisión humana, no repetición automática. La documentación vigente identifica mejoras pendientes en reconexión, identidad de comandos, eventos terminales y consistencia entre especificación y firmware compilado.")
    add_para(doc, "No se presentan promedios, desviaciones estándar ni intervalos de confianza porque no se dispone de repeticiones homogéneas suficientes. En consecuencia, el documento no declara una precisión de navegación. La siguiente etapa debe medir, como mínimo, repeticiones controladas por distancia, ángulo y superficie, con comparación externa y correlación entre video, telemetría y bitácora.")

    add_para(doc, "5. Conclusiones y trabajo futuro", style="Heading 1")
    add_para(doc, "Se implementó una base funcional para controlar y observar un robot móvil diferencial con ESP32-S3, MPU6050, encoders, PWM, telemetría y una HMI local. La arquitectura mantiene el control físico y la seguridad en el firmware, mientras delega la misión, el historial y la interfaz al backend. Esta separación facilita el diagnóstico y evita que la capa visual sea la autoridad de seguridad del robot.")
    add_para(doc, "Los objetivos de integración se alcanzaron a nivel de prototipo: se configuró el sensado inercial, se incorporó conteo de encoders con PCNT, se controlaron motores con PWM y protecciones, y se conectó la telemetría con la HMI. El resultado no debe interpretarse como una certificación de seguridad eléctrica ni de precisión metrológica. Esas afirmaciones requieren mediciones físicas repetidas, control de corriente y trazabilidad sincronizada.")
    add_para(doc, "Como continuidad, se proponen las siguientes acciones:", style="Heading 2")
    for item in [
        "Completar el protocolo de corriente con ruedas elevadas, fusible, desacoplo y medición de arranque, sostenida y rotor bloqueado.",
        "Ejecutar al menos 20 repeticiones por distancia y ángulo, reportar error absoluto y variabilidad y comparar contra medición externa.",
        "Calibrar diámetro efectivo y escala de encoders sobre recorridos controlados; conservar la evidencia junto con video y SQLite.",
        "Fortalecer la recuperación ante reinicio mediante boot_id, identidad de comando persistente y manejo seguro de eventos terminales.",
        "Convertir las constantes aprobadas de control y seguridad en una especificación única verificable frente al firmware compilado.",
    ]:
        add_bullet(doc, item)

    add_para(doc, "Referencias", style="Heading 1")
    refs = [
        "Bray, T. (2017). The JavaScript Object Notation (JSON) Data Interchange Format (RFC 8259). RFC Editor. https://www.rfc-editor.org/rfc/rfc8259",
        "Espressif Systems. (s. f.-a). LED control (LEDC) - ESP32-S3. ESP-IDF Programming Guide. https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/ledc.html",
        "Espressif Systems. (s. f.-b). Pulse counter (PCNT) - ESP32-S3. ESP-IDF Programming Guide. https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/pcnt.html",
        "Fette, I., & Melnikov, A. (2011). The WebSocket protocol (RFC 6455). RFC Editor. https://www.rfc-editor.org/rfc/rfc6455",
        "Franklin, G. F., Powell, J. D., & Emami-Naeini, A. (2019). Feedback control of dynamic systems (8th ed.). Pearson.",
        "Pallets Projects. (s. f.). Flask documentation. https://flask.palletsprojects.com/",
        "PlatformIO. (s. f.). PlatformIO Core documentation. https://docs.platformio.org/en/latest/core/index.html",
        "Siegwart, R., Nourbakhsh, I. R., & Scaramuzza, D. (2011). Introduction to autonomous mobile robots (2nd ed.). MIT Press.",
        "TDK InvenSense. (2013a). MPU-6000/MPU-6050 product specification (Rev. 3.4). https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet.pdf",
        "TDK InvenSense. (2013b). MPU-6000/MPU-6050 register map and descriptions (Rev. 4.2). https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map.pdf",
        "Texas Instruments. (2015). DRV8833 dual H-bridge motor driver datasheet (Rev. E). https://www.ti.com/lit/ds/symlink/drv8833.pdf",
    ]
    for ref in refs:
        p = add_para(doc, ref, after=6)
        p.paragraph_format.left_indent = Inches(0.5)
        p.paragraph_format.first_line_indent = Inches(-0.5)
        p.paragraph_format.line_spacing = 1.15
        p.alignment = WD_ALIGN_PARAGRAPH.LEFT

    doc.add_page_break()
    add_para(doc, "Anexo A. Checklist de aceptación", style="Heading 1")
    add_para(doc, "Lista para registrar una prueba supervisada. El checklist no reemplaza el juicio del responsable de laboratorio ni las medidas de protección física.")
    checks = [
        ("Preparación", "Secrets.h local y no versionado; un único controlador activo; acceso a corte de VMOT."),
        ("Protección física", "Capacitor >=100 uF por driver, 0.1 uF por motor, fusible o polyfuse y tierra común."),
        ("Energía", "Fuente limitada o batería con fusible; corriente medida durante calibración, avance, giro y rotor bloqueado."),
        ("Sensores", "IMU calibrada; FL/FR/BL/BR reportan encoders coherentes; sentido de cada canal verificado."),
        ("Control", "PWM cero antes de arranque; interlock de inversión; E-STOP y watchdog comprobados."),
        ("Ruta", "Tramos ortogonales; objetivo, trayectoria y telemetría visibles; medición física registrada."),
        ("Evidencia", "Video, hora, comando y SQLite corresponden a la misma prueba; resultados anotados."),
        ("Cierre", "Parada confirmada, VMOT desenergizado y bitácora resguardada."),
    ]
    rows = [("☐", category, detail) for category, detail in checks]
    add_table(doc, ["", "Área", "Verificación"], rows, [450, 1900, 7010], font_size=9.3)


def main():
    doc = Document()
    configure_document(doc)
    add_cover(doc)
    add_contents(doc)
    add_body(doc)
    doc.core_properties.title = "Memoria de Verano de Investigación: Robot móvil ESP32-S3"
    doc.core_properties.subject = "Anteproyecto y memoria técnica"
    doc.core_properties.author = "Rubén Andrés Gómez Molina"
    doc.core_properties.comments = "Documento generado a partir de la documentación técnica del proyecto y fuentes APA 7."
    doc.save(OUT)
    print(OUT.resolve())


if __name__ == "__main__":
    main()

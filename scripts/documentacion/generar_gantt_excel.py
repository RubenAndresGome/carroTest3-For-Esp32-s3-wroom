#!/usr/bin/env python3
"""
generar_gantt_excel.py
Genera un libro de Excel (.xlsx) elegante, en estilo claro y profesional (sin emojis),
con el Diagrama de Gantt y la Cronología de Desarrollo basada en el capítulo
'Metodología de desarrollo y planificación' del informe de residencias profesionales.

Periodo ajustado al calendario oficial de Residencia Profesional:
Del 28 de septiembre al 12 de diciembre de 2026 (11 semanas hábiles).
"""

import os
import sys
from datetime import date
import openpyxl
from openpyxl.styles import Font, PatternFill, Alignment, Border, Side
from openpyxl.utils import get_column_letter

def build_gantt_workbook(output_paths):
    wb = openpyxl.Workbook()
    default_sheet = wb.active

    # -------------------------------------------------------------------------
    # Estilos y Paleta de Colores (Estilo Claro Elegante)
    # -------------------------------------------------------------------------
    font_title = Font(name="Segoe UI", size=15, bold=True, color="1E3A8A")
    font_subtitle = Font(name="Segoe UI", size=10, italic=True, color="4B5563")
    font_section = Font(name="Segoe UI", size=11, bold=True, color="1E3A8A")
    font_header = Font(name="Segoe UI", size=10, bold=True, color="FFFFFF")
    font_data = Font(name="Segoe UI", size=9, color="1F2937")
    font_data_bold = Font(name="Segoe UI", size=9, bold=True, color="1F2937")
    font_phase = Font(name="Segoe UI", size=9.5, bold=True, color="1E3A8A")
    font_kpi_num = Font(name="Segoe UI", size=18, bold=True, color="1E3A8A")
    font_kpi_lbl = Font(name="Segoe UI", size=8.5, color="4B5563")

    fill_header = PatternFill(start_color="1E3A8A", end_color="1E3A8A", fill_type="solid")
    fill_header_sub = PatternFill(start_color="2563EB", end_color="2563EB", fill_type="solid")
    fill_phase = PatternFill(start_color="EEF2F6", end_color="EEF2F6", fill_type="solid")
    fill_zebra_even = PatternFill(start_color="FFFFFF", end_color="FFFFFF", fill_type="solid")
    fill_zebra_odd = PatternFill(start_color="F8FAFC", end_color="F8FAFC", fill_type="solid")
    fill_kpi = PatternFill(start_color="F1F5F9", end_color="F1F5F9", fill_type="solid")

    # Colores de barras Gantt por fase
    fill_bar_req = PatternFill(start_color="3B82F6", end_color="3B82F6", fill_type="solid")      # Azul
    fill_bar_arch = PatternFill(start_color="6366F1", end_color="6366F1", fill_type="solid")     # Índigo
    fill_bar_proto = PatternFill(start_color="0EA5E9", end_color="0EA5E9", fill_type="solid")    # Cyan
    fill_bar_elec = PatternFill(start_color="14B8A6", end_color="14B8A6", fill_type="solid")     # Teal
    fill_bar_hmi = PatternFill(start_color="8B5CF6", end_color="8B5CF6", fill_type="solid")      # Púrpura
    fill_bar_integ = PatternFill(start_color="10B981", end_color="10B981", fill_type="solid")    # Esmeralda
    fill_bar_eval = PatternFill(start_color="64748B", end_color="64748B", fill_type="solid")     # Slate
    fill_bar_4wd = PatternFill(start_color="F59E0B", end_color="F59E0B", fill_type="solid")      # Ámbar
    fill_bar_conting = PatternFill(start_color="EF4444", end_color="EF4444", fill_type="solid")  # Coral/Rojo suave
    fill_bar_test = PatternFill(start_color="059669", end_color="059669", fill_type="solid")     # Verde oscuro
    fill_bar_opt = PatternFill(start_color="2563EB", end_color="2563EB", fill_type="solid")      # Azul real

    border_thin_gray = Side(style="thin", color="E2E8F0")
    border_cell = Border(left=border_thin_gray, right=border_thin_gray, top=border_thin_gray, bottom=border_thin_gray)

    align_center = Alignment(horizontal="center", vertical="center", wrap_text=True)
    align_left = Alignment(horizontal="left", vertical="center", wrap_text=True)
    align_right = Alignment(horizontal="right", vertical="center")

    # =========================================================================
    # HOJA 1: RESUMEN METODOLOGICO Y MARCO DE INGENIERIA
    # =========================================================================
    ws1 = wb.create_sheet(title="Resumen Metodologico")
    ws1.views.sheetView[0].showGridLines = True

    # Encabezado institucional
    ws1["B2"] = "INSTITUTO TECNOLOGICO SUPERIOR DE SALVATIERRA"
    ws1["B2"].font = Font(name="Segoe UI", size=11, bold=True, color="6B7280")
    ws1["B3"] = "INFORME TECNICO DE RESIDENCIA PROFESIONAL - ROBOT S3 4WD"
    ws1["B3"].font = font_title
    ws1["B4"] = "Capitulo: Metodologia de desarrollo y planificacion (Modelo Hibrido Waterfall + Scrum)"
    ws1["B4"].font = font_subtitle

    # Tarjetas resumen (KPIs)
    kpis = [
        ("Duracion Total", "11 Semanas", "28 Sep - 12 Dic 2026"),
        ("Marco Metodologico", "Hibrido", "Waterfall + Sprints Scrum"),
        ("Sprints Ejecutados", "11 Sprints", "Iteraciones semanales"),
        ("Perifericos Criticos", "PCNT / I2C / PWM", "Hardware determinista Core 1"),
        ("Tasa de Pruebas Unitarias", "100%", "74 de 74 tests aprobados")
    ]
    for idx, (title, val, sub) in enumerate(kpis):
        c_start = 2 + idx * 2
        ws1.merge_cells(start_row=6, start_column=c_start, end_row=6, end_column=c_start+1)
        ws1.merge_cells(start_row=7, start_column=c_start, end_row=7, end_column=c_start+1)
        ws1.merge_cells(start_row=8, start_column=c_start, end_row=8, end_column=c_start+1)
        cell_t = ws1.cell(row=6, column=c_start, value=title)
        cell_v = ws1.cell(row=7, column=c_start, value=val)
        cell_s = ws1.cell(row=8, column=c_start, value=sub)
        cell_t.font = font_kpi_lbl
        cell_v.font = font_kpi_num
        cell_s.font = Font(name="Segoe UI", size=8, color="9CA3AF")
        cell_t.alignment = align_center
        cell_v.alignment = align_center
        cell_s.alignment = align_center
        for r in range(6, 9):
            for c in range(c_start, c_start+2):
                ws1.cell(row=r, column=c).fill = fill_kpi
                ws1.cell(row=r, column=c).border = border_cell

    # Seccion 1: Principios Metodologicos
    ws1["B10"] = "1. Fundamentacion del Modelo Hibrido de Ingenieria"
    ws1["B10"].font = font_section

    principios = [
        ("Planificacion Macro (Modelo Waterfall):", "Definicion secuencial de etapas mayores: Requisitos -> Arquitectura y Hardware -> Firmware Base -> Comunicaciones -> Integracion -> Escalado 4WD -> Pruebas Finales."),
        ("Ejecucion Operativa (Enfoque Agil / Scrum):", "Ciclos iterativos en sprints semanales de prototipado rapido con enfoque prueba-error-correccion sobre hardware real, aislando pruebas en ramas y bancos locales."),
        ("Validacion Empirica Continua:", "Contrastacion fisica con instrumental basico (cinta metrica, marcas ortogonales en piso, verificacion visual y telemetria forense en SQLite a 100 Hz)."),
        ("Gestion de Contingencias de Hardware:", "Ante fallas destructivas de componentes (dano termico en MPU6050 y DRV8833), se implementaron ramas de aislamiento, rediseño de protecciones (interlock 250 ms) y adquisicion de refacciones.")
    ]
    for idx, (p_title, p_desc) in enumerate(principios):
        row = 12 + idx
        ws1.cell(row=row, column=2, value=p_title).font = font_data_bold
        ws1.merge_cells(start_row=row, start_column=3, end_row=row, end_column=11)
        c = ws1.cell(row=row, column=3, value=p_desc)
        c.font = font_data
        c.alignment = align_left

    # Seccion 2: Desglose por Fases
    ws1["B17"] = "2. Estructura de Fases del Proyecto"
    ws1["B17"].font = font_section

    headers_fases = ["Fase", "Periodo", "Enfoque Principal", "Entregable Mayor", "Estado"]
    for col_idx, h in enumerate(headers_fases):
        c = ws1.cell(row=19, column=2 + col_idx * 2 if col_idx > 1 else 2 + col_idx, value=h)
        c.font = font_header
        c.fill = fill_header
        c.alignment = align_center
        c.border = border_cell

    fases_data = [
        ("Fase I: Prototipo Base Inicial (2WD)", "28 Sep - 06 Nov 2026", "Acondicionamiento electrico, firmware modular ESP32-S3 y HMI", "Prototipo funcional base y memoria tecnica", "Completado"),
        ("Fase II: Evaluacion y Transicion", "09 Nov - 13 Nov 2026", "Deteccion de limites mecanicos e inestabilidad del chasis 2 ruedas", "Dictamen de escalado a plataforma 4WD", "Completado"),
        ("Fase III: Escalado a Chasis 4WD", "16 Nov - 20 Nov 2026", "Planificacion express, adquisicion y circuito de 15 GPIOs para 4WD", "Diseño esquematico y mecanico 4WD", "Completado"),
        ("Fase IV: Contingencias y Robustez", "23 Nov - 04 Dic 2026", "Atencion a quema de MPU/driver, interlock de 250 ms y PCNT 4 canales", "Firmware tolerante a fallos y app Android", "Completado"),
        ("Fase V: Optimizacion y Cierre", "07 Dic - 12 Dic 2026", "Protocolo JSON v3 (-40% RAM), telemetria 100 Hz y portal GitHub Pages", "Atlas Tecnico y producto operativo final", "Completado")
    ]
    for r_idx, f_row in enumerate(fases_data):
        curr_row = 20 + r_idx
        fill_curr = fill_zebra_odd if r_idx % 2 == 1 else fill_zebra_even
        for c_idx, val in enumerate(f_row):
            c = ws1.cell(row=curr_row, column=2 + c_idx * 2 if c_idx > 1 else 2 + c_idx, value=val)
            c.font = font_data_bold if c_idx == 0 else font_data
            c.fill = fill_curr
            c.border = border_cell
            c.alignment = align_center if c_idx in [1, 4] else align_left

    # =========================================================================
    # HOJA 2: DIAGRAMA DE GANTT DETALLADO
    # =========================================================================
    ws2 = wb.create_sheet(title="Diagrama de Gantt")
    ws2.views.sheetView[0].showGridLines = True

    # Encabezado
    ws2["A2"] = "CRONOGRAMA DE TRABAJO Y DIAGRAMA DE GANTT DETALLADO"
    ws2["A2"].font = font_title
    ws2["A3"] = "Planificacion Macro e Iteraciones Operativas Semanales (Semanas 1 a 11 · 28 Sep - 12 Dic 2026)"
    ws2["A3"].font = font_subtitle

    # Columnas de tabla
    headers_gantt = [
        "ID", "Fase Metodologica", "Actividad / Sprint Detallado", "Inicio", "Fin", "Dias", "Responsable", "Entregable Tecnico", "Estado"
    ]
    weeks_headers = [
        ("S01", "28 Sep-02 Oct"),
        ("S02", "05-09 Oct"),
        ("S03", "12-16 Oct"),
        ("S04", "19-23 Oct"),
        ("S05", "26-30 Oct"),
        ("S06", "02-06 Nov"),
        ("S07", "09-13 Nov"),
        ("S08", "16-20 Nov"),
        ("S09", "23-27 Nov"),
        ("S10", "30 Nov-04 Dic"),
        ("S11", "07-12 Dic")
    ]

    row_h1 = 5
    row_h2 = 6

    # Encabezados de datos
    for col_idx, h_text in enumerate(headers_gantt, start=1):
        ws2.merge_cells(start_row=row_h1, start_column=col_idx, end_row=row_h2, end_column=col_idx)
        cell = ws2.cell(row=row_h1, column=col_idx, value=h_text)
        cell.font = font_header
        cell.fill = fill_header
        cell.alignment = align_center
        cell.border = border_cell

    # Encabezados de Semanas de Gantt
    start_gantt_col = len(headers_gantt) + 1
    for w_idx, (w_code, w_dates) in enumerate(weeks_headers):
        c_num = start_gantt_col + w_idx
        c1 = ws2.cell(row=row_h1, column=c_num, value=w_code)
        c2 = ws2.cell(row=row_h2, column=c_num, value=w_dates)
        c1.font = font_header
        c1.fill = fill_header_sub
        c1.alignment = align_center
        c1.border = border_cell
        c2.font = Font(name="Segoe UI", size=7.5, bold=True, color="FFFFFF")
        c2.fill = fill_header_sub
        c2.alignment = align_center
        c2.border = border_cell

    # 32 Actividades detalladas
    actividades = [
        # SEMANA 1
        ("ACT-01", "Fase I: Requisitos", "S01: Investigacion documental y analisis de estado del arte en robots diferenciales", "28/09/2026", "30/09/2026", 3, "Residente", "Matriz comparativa de hardware y algoritmos", "Completado", 0, 0, fill_bar_req),
        ("ACT-02", "Fase I: Requisitos", "S01: Definicion formal de requisitos funcionales, metricos y restricciones fisicas", "30/09/2026", "01/10/2026", 2, "Residente", "Documento de especificacion de requerimientos", "Completado", 0, 0, fill_bar_req),
        ("ACT-03", "Fase I: Requisitos", "S01: Cotizacion, adquisicion de componentes iniciales y planteamiento de riesgos", "01/10/2026", "02/10/2026", 2, "Residente", "BOM inicial y matriz preliminar de riesgos", "Completado", 0, 0, fill_bar_req),

        # SEMANA 2
        ("ACT-04", "Fase I: Diseno", "S02: Diseno esquematico preliminar y arquitectura de interconexion de modulos", "05/10/2026", "06/10/2026", 2, "Residente", "Diagrama de bloques y conexiones en borrador", "Completado", 1, 1, fill_bar_arch),
        ("ACT-05", "Fase I: Diseno", "S02: Analisis de compatibilidad de niveles logicos (3.3V ESP32 vs 5V perifericos)", "07/10/2026", "08/10/2026", 2, "Residente", "Especificacion de conversion de nivel de tension", "Completado", 1, 1, fill_bar_arch),
        ("ACT-06", "Fase I: Diseno", "S02: Creacion de mockups de interfaz grafica (HMI) y arquitectura de servidor Python", "08/10/2026", "09/10/2026", 2, "Residente", "Prototipos visuales de HMI y contratos API", "Completado", 1, 1, fill_bar_arch),

        # SEMANA 3
        ("ACT-07", "Fase I: Prototipado 2WD", "S03: Pruebas de encendido del ESP32-S3 y verificacion de GPIOs disponibles", "12/10/2026", "13/10/2026", 2, "Residente", "Tabla de mapeo de pines verificada", "Completado", 2, 2, fill_bar_proto),
        ("ACT-08", "Fase I: Prototipado 2WD", "S03: Validacion de perifericos basicos y puente H DRV8833 en vacio", "13/10/2026", "14/10/2026", 2, "Residente", "Protocolo de control de giro de motores DC", "Completado", 2, 2, fill_bar_proto),
        ("ACT-09", "Fase I: Prototipado 2WD", "S03: Primer producto minimo viable (MVP) de firmware modular", "15/10/2026", "16/10/2026", 2, "Residente", "Codigo base con teleoperacion basica", "Completado", 2, 2, fill_bar_proto),

        # SEMANA 4
        ("ACT-10", "Fase I: Electronica", "S04: Reorganizacion de circuito en protoboard compacta y topologia de tierra comun", "19/10/2026", "20/10/2026", 2, "Residente", "Esquema fisico libre de lazos de tierra (GND)", "Completado", 3, 3, fill_bar_elec),
        ("ACT-11", "Fase I: Electronica", "S04: Integracion del conversor bidireccional TXS0108E para encoders LM393", "21/10/2026", "22/10/2026", 2, "Residente", "Señales digitales adaptadas a 3.3V estables", "Completado", 3, 3, fill_bar_elec),
        ("ACT-12", "Fase I: Electronica", "S04: Migracion formal del entorno Arduino IDE a PlatformIO", "22/10/2026", "23/10/2026", 2, "Residente", "Estructura modular src/ e include/ versionada", "Completado", 3, 3, fill_bar_elec),

        # SEMANA 5
        ("ACT-13", "Fase I: Software y HMI", "S05: Desarrollo del backend Python (Flask) y servidor de sockets bidireccional", "26/10/2026", "27/10/2026", 2, "Residente", "Servidor local con soporte WebSocket JSON", "Completado", 4, 4, fill_bar_hmi),
        ("ACT-14", "Fase I: Software y HMI", "S05: Implementacion de la interfaz grafica web preliminar (HMI de monitoreo)", "27/10/2026", "28/10/2026", 2, "Residente", "Panel de control con telemetria en vivo", "Completado", 4, 4, fill_bar_hmi),
        ("ACT-15", "Fase I: Software y HMI", "S05: Mecanismo de persistencia local en base de datos SQLite y diagnostico", "29/10/2026", "30/10/2026", 2, "Residente", "Esquema relacional de logs y telemetria", "Completado", 4, 4, fill_bar_hmi),

        # SEMANA 6
        ("ACT-16", "Fase I: Sensores y Control", "S06: Adopcion del periferico PCNT por hardware para encoders (descarte de ISR)", "02/11/2026", "03/11/2026", 2, "Residente", "Conteo sin perdida por interrupciones", "Completado", 5, 5, fill_bar_integ),
        ("ACT-17", "Fase I: Sensores y Control", "S06: Calibracion de offsets del sensor inercial MPU6050 y lazo I2C", "03/11/2026", "04/11/2026", 2, "Residente", "Filtro complementario de rumbo calibrado", "Completado", 5, 5, fill_bar_integ),
        ("ACT-18", "Fase I: Sensores y Control", "S06: Implementacion del super-ciclo sincrono determinista a 100 Hz en Core 1", "05/11/2026", "06/11/2026", 2, "Residente", "Super-ciclo 100 Hz y memoria de verano", "Completado", 5, 5, fill_bar_integ),

        # SEMANA 7
        ("ACT-19", "Fase II: Evaluacion 2WD", "S07: Ensayos de traccion y cabeceo sobre chasis 2WD en suelo de azulejo", "09/11/2026", "10/11/2026", 2, "Residente", "Matriz de errores ometricos en 2WD", "Completado", 6, 6, fill_bar_eval),
        ("ACT-20", "Fase II: Evaluacion 2WD", "S07: Identificacion de limites mecanicos y patinaje excesivo en rueda loca", "11/11/2026", "12/11/2026", 2, "Residente", "Registro de cabeceo y deriva inaceptable", "Completado", 6, 6, fill_bar_eval),
        ("ACT-21", "Fase II: Evaluacion 2WD", "S07: Dictamen de ingenieria y decision estrategica de escalado a plataforma 4WD", "12/11/2026", "13/11/2026", 2, "Residente", "Documento de justificacion de cambio de chasis", "Completado", 6, 6, fill_bar_eval),

        # SEMANA 8
        ("ACT-22", "Fase III: Escalado 4WD", "S08: Planificacion express del prototipo de traccion en cuatro ruedas (4WD)", "16/11/2026", "17/11/2026", 2, "Residente", "Diagrama de distribucion de pesos y torque", "Completado", 7, 7, fill_bar_4wd),
        ("ACT-23", "Fase III: Escalado 4WD", "S08: Adquisicion de chasis acrilico de doble plataforma y 4 reductores TT", "17/11/2026", "18/11/2026", 2, "Residente", "BOM actualizado y recepcion de hardware", "Completado", 7, 7, fill_bar_4wd),
        ("ACT-24", "Fase III: Escalado 4WD", "S08: Rediseño del cableado y asignacion de 15 GPIOs en ESP32-S3 para 4WD", "19/11/2026", "20/11/2026", 2, "Residente", "Esquema electrico de 15 pines documentado", "Completado", 7, 7, fill_bar_4wd),

        # SEMANA 9
        ("ACT-25", "Fase IV: Contingencias", "S09: Montaje mecanico de 4 motores y conexion a doble puente H DRV8833", "23/11/2026", "24/11/2026", 2, "Residente", "Estructura 4WD armada y cableada", "Completado", 8, 8, fill_bar_conting),
        ("ACT-25B", "Fase IV: Contingencias", "S09: Contingencia critica: Dano termico en DRV8833/MPU, refacciones e interlock 250 ms", "25/11/2026", "27/11/2026", 3, "Residente", "Tiempo muerto universal 250 ms y rampas suaves", "Completado", 8, 8, fill_bar_conting),

        # SEMANA 10
        ("ACT-26", "Fase IV: Pruebas 4WD", "S10: Pruebas progresivas de encoders (1 encoder -> 4 encoders PCNT)", "30/11/2026", "01/12/2026", 2, "Residente", "Filtro de glitches 1023 ciclos verificado", "Completado", 9, 9, fill_bar_test),
        ("ACT-27", "Fase IV: Pruebas 4WD", "S10: Ensayos de traslacion recta y resolucion de desbordamiento de memoria", "01/12/2026", "02/12/2026", 2, "Residente", "Memoria optimizada y avance rectilineo", "Completado", 9, 9, fill_bar_test),
        ("ACT-28", "Fase IV: Pruebas 4WD", "S10: Desacoplo de arquitectura: Servidor multiplataforma (Tablet Android)", "03/12/2026", "04/12/2026", 2, "Residente", "App Android (Chaquopy) y scripts ADB", "Completado", 9, 9, fill_bar_test),

        # SEMANA 11
        ("ACT-29", "Fase V: Optimizacion", "S11: Refactorizacion de protocolo: evolucion de JSON v1 a JSON v2 y v3", "07/12/2026", "08/12/2026", 2, "Residente", "Ahorro del 40% en memoria por compactacion", "Completado", 10, 10, fill_bar_opt),
        ("ACT-30", "Fase V: Optimizacion", "S11: Telemetria a 100 Hz, lazo PD de rumbo y purga manual de registros BD", "08/12/2026", "09/12/2026", 2, "Residente", "13,930 muestras de telemetria forense", "Completado", 10, 10, fill_bar_opt),
        ("ACT-31", "Fase V: Optimizacion", "S11: Creacion de portal documental local y en GitHub Pages (Atlas Tecnico)", "10/12/2026", "11/12/2026", 2, "Residente", "Portal web con diagramas y catalogo", "Completado", 10, 10, fill_bar_opt),
        ("ACT-32", "Fase V: Optimizacion", "S11: Validacion final, suite de 74 tests unitarios y elaboracion de informe", "11/12/2026", "12/12/2026", 2, "Residente", "Informe tecnico final y entrega formal", "Completado", 10, 10, fill_bar_opt)
    ]

    current_row = 7
    current_phase = ""

    for act in actividades:
        (act_id, fase, act_desc, d_ini, d_fin, dur_dias, resp, entregable, estado, w_start, w_end, f_bar) = act

        if fase != current_phase:
            current_phase = fase
            ws2.merge_cells(start_row=current_row, start_column=1, end_row=current_row, end_column=len(headers_gantt) + len(weeks_headers))
            p_cell = ws2.cell(row=current_row, column=1, value=fase.upper())
            p_cell.font = font_phase
            p_cell.fill = fill_phase
            p_cell.alignment = Alignment(horizontal="left", vertical="center", indent=1)
            for c in range(1, len(headers_gantt) + len(weeks_headers) + 1):
                ws2.cell(row=current_row, column=c).border = border_cell
            current_row += 1

        is_even = (current_row % 2 == 0)
        row_fill = fill_zebra_even if is_even else fill_zebra_odd

        row_data = [act_id, fase, act_desc, d_ini, d_fin, dur_dias, resp, entregable, estado]
        for c_idx, val in enumerate(row_data, start=1):
            cell = ws2.cell(row=current_row, column=c_idx, value=val)
            cell.font = font_data_bold if c_idx in [1, 9] else font_data
            cell.fill = row_fill
            cell.border = border_cell
            if c_idx in [1, 4, 5, 6, 7, 9]:
                cell.alignment = align_center
            else:
                cell.alignment = align_left

        if estado == "Completado":
            ws2.cell(row=current_row, column=9).fill = PatternFill(start_color="DCFCE7", end_color="DCFCE7", fill_type="solid")
            ws2.cell(row=current_row, column=9).font = Font(name="Segoe UI", size=8.5, bold=True, color="166534")

        for w_idx in range(len(weeks_headers)):
            col_target = start_gantt_col + w_idx
            bar_cell = ws2.cell(row=current_row, column=col_target)
            bar_cell.border = border_cell
            if w_start <= w_idx <= w_end:
                bar_cell.fill = f_bar
                bar_cell.value = ""
            else:
                bar_cell.fill = row_fill

        current_row += 1

    # =========================================================================
    # HOJA 3: CRONOLOGIA DE HITOS Y CONTINGENCIAS
    # =========================================================================
    ws3 = wb.create_sheet(title="Hitos y Contingencias")
    ws3.views.sheetView[0].showGridLines = True

    ws3["B2"] = "CRONOLOGIA DE HITOS TECNICOS Y CONTINGENCIAS DE INGENIERIA"
    ws3["B2"].font = font_title
    ws3["B3"] = "Registro de Lecciones Aprendidas, Decisiones de Diseño y Mitigaciones en Hardware y Software"
    ws3["B3"].font = font_subtitle

    # Tabla de Contingencias Criticas
    ws3["B5"] = "1. Registro de Contingencias Criticas y Resolucion de Hardware"
    ws3["B5"].font = font_section

    headers_conting = ["ID Evento", "Fecha / Sprint", "Componente Afectado", "Modo de Falla Observado", "Causa Raiz Identificada", "Accion Correctiva y Mitigacion en Firmware", "Resultado Verificado"]
    for idx, h in enumerate(headers_conting):
        c = ws3.cell(row=7, column=2 + idx, value=h)
        c.font = font_header
        c.fill = fill_header
        c.alignment = align_center
        c.border = border_cell

    contingencias_data = [
        ("EVT-01", "Semana 4 (20 Oct)", "Señales de Encoders LM393", "Pulsos espurios y lectura erratica por incompatibilidad de tension", "El ESP32 opera a 3.3V y los LM393 operan a 5V con niveles ruidosos", "Implementacion de conversor bidireccional TXS0108E y topologia de tierra en estrella", "Lecturas estables sin riesgo de sobretension en GPIOs"),
        ("EVT-02", "Semana 7 (12 Nov)", "Plataforma Mecanica 2WD", "Cabeceo excesivo, patinaje en rueda loca y falta de espacio para hardware", "Chasis 2WD insuficiente para albergar baterias, doble etapa de potencia y telemetria", "Decision de escalado a chasis 4WD con traccion independiente y doble plataforma acrilica", "Mayor estabilidad mecanica y espacio para sensores"),
        ("EVT-03", "Semana 9 (25 Nov)", "Driver DRV8833 y MPU6050", "Dano termico permanente durante pruebas de cambio brusco de direccion", "Picos inductivos (Back-EMF) y ausencia de tiempo muerto en conmutacion de puente H", "Adquisicion de refacciones en Celaya e implementacion de interlock de 250 ms con rampas suaves", "Proteccion permanente de transistores y MPU sin fallas posteriores"),
        ("EVT-04", "Semana 10 (02 Dic)", "Memoria RAM del ESP32-S3", "Desbordamiento de memoria (overflow) y bloqueos esporadicos de la pila", "Acumulacion de tareas concurrentes y mensajes JSON voluminosos en un solo hilo", "Distribucion estricta en 2 nucleos (Core 0 Web / Core 1 Control) y migracion a Tablet Android", "Sincronismo determinista a 100 Hz garantizado sin bloqueos"),
        ("EVT-05", "Semana 11 (08 Dic)", "Persistencia SQLite y Red", "Saturacion de ancho de banda y acumulacion rapida de megabytes de logs", "Envio continuo de frames repetidos en protocolo JSON inicial (v1/v2)", "Refactorizacion a protocolo JSON v3 con compactacion diferencial (-40% uso de memoria)", "Base de datos compacta y telemetria fluida a 100 Hz")
    ]
    for r_idx, c_row in enumerate(contingencias_data):
        curr_row = 8 + r_idx
        c_fill = fill_zebra_odd if r_idx % 2 == 1 else fill_zebra_even
        for c_idx, val in enumerate(c_row):
            c = ws3.cell(row=curr_row, column=2 + c_idx, value=val)
            c.font = font_data_bold if c_idx == 0 else font_data
            c.fill = c_fill
            c.border = border_cell
            c.alignment = align_center if c_idx in [0, 1] else align_left

    # Tabla de Hitos Mayores
    ws3["B15"] = "2. Cronologia de Hitos de Validacion (Milestones)"
    ws3["B15"].font = font_section

    headers_hitos = ["ID Hito", "Fecha / Sprint", "Entregable Mayor", "Criterio de Aceptacion Tecnico", "Impacto en el Proyecto"]
    for idx, h in enumerate(headers_hitos):
        c = ws3.cell(row=17, column=2 + idx, value=h)
        c.font = font_header
        c.fill = fill_header_sub
        c.alignment = align_center
        c.border = border_cell

    hitos_data = [
        ("M-01", "Semana 1 (02 Oct)", "Aprobacion de Requisitos y BOM", "Firma de objetivos de investigacion y presupuesto base", "Fundamentacion teorica y viabilidad operativa"),
        ("M-02", "Semana 3 (16 Oct)", "MVP Operativo 2WD en Vacio", "Motores respondiendo a consignas de direccion basicas", "Primera traslacion verificada en banco"),
        ("M-03", "Semana 4 (23 Oct)", "Migracion Exitosa a PlatformIO", "Compilacion limpia modular y GND en estrella funcional", "Fin de reseteos por ruido electrico"),
        ("M-04", "Semana 6 (06 Nov)", "Lazo Determinista a 100 Hz (Core 1)", "PCNT y MPU6050 integrados en súper-ciclo sin retardo", "Base de odometria e integracion inercial"),
        ("M-05", "Semana 7 (13 Nov)", "Dictamen de Escalado a 4WD", "Aprobacion de transicion a chasis de doble plataforma", "Superacion de limitaciones de traccion 2WD"),
        ("M-06", "Semana 8 (20 Nov)", "Ensamble Fisico 4WD Operativo", "Chasis montado con 4 motores y 15 GPIOs cableados", "Nueva plataforma con mayor capacidad fisica"),
        ("M-07", "Semana 10 (04 Dic)", "Desacoplo Dual: ESP32 + Tablet Android", "HMI y SQLite corriendo en Android via Chaquopy", "Soberania de mision y telemetria en tiempo real"),
        ("M-08", "Semana 11 (12 Dic)", "Atlas Tecnico, Suite de Tests y Cierre", "74/74 tests unitarios nativos aprobados y entrega formal", "Proyecto cerrado y reproducible al 100%")
    ]
    for r_idx, h_row in enumerate(hitos_data):
        curr_row = 18 + r_idx
        c_fill = fill_zebra_odd if r_idx % 2 == 1 else fill_zebra_even
        for c_idx, val in enumerate(h_row):
            c = ws3.cell(row=curr_row, column=2 + c_idx, value=val)
            c.font = font_data_bold if c_idx == 0 else font_data
            c.fill = c_fill
            c.border = border_cell
            c.alignment = align_center if c_idx in [0, 1] else align_left

    # Ajuste de ancho de columnas
    col_widths_ws2 = {
        1: 10, 2: 24, 3: 45, 4: 13, 5: 13, 6: 8, 7: 14, 8: 34, 9: 14,
        10: 12, 11: 12, 12: 12, 13: 12, 14: 12, 15: 12, 16: 12, 17: 12, 18: 12, 19: 12, 20: 12
    }
    for col_i, w in col_widths_ws2.items():
        ws2.column_dimensions[get_column_letter(col_i)].width = w

    col_widths_ws3 = {
        2: 12, 3: 18, 4: 26, 5: 36, 6: 38, 7: 42, 8: 36
    }
    for col_i, w in col_widths_ws3.items():
        ws3.column_dimensions[get_column_letter(col_i)].width = w

    col_widths_ws1 = {
        2: 24, 3: 20, 4: 18, 5: 20, 6: 18, 7: 20, 8: 18, 9: 20, 10: 18, 11: 20
    }
    for col_i, w in col_widths_ws1.items():
        ws1.column_dimensions[get_column_letter(col_i)].width = w

    # Remover hoja por defecto inicial si existe
    if default_sheet in wb.worksheets and len(wb.worksheets) > 1:
        wb.remove(default_sheet)

    for p in output_paths:
        os.makedirs(os.path.dirname(p), exist_ok=True)
        wb.save(p)
        print(f"Excel generado exitosamente en: {p}")

if __name__ == "__main__":
    out1 = r"D:\ITESS\TICS\VERANO 2026\TEST_RESIDENCIAS\AnteProyecto\documentos_corregidos\informe\cronograma_gantt_metodologia.xlsx"
    out2 = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "docs", "cronograma_gantt_metodologia.xlsx"))
    build_gantt_workbook([out1, out2])

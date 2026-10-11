#!/usr/bin/env python3
"""
generar_gantt_html.py
Genera una página HTML elegante, en estilo claro y profesional (sin emojis),
con el Diagrama de Gantt y la Cronología Gráfica basada en el capítulo
'Metodología de desarrollo y planificación' del informe de residencias profesionales.

Periodo ajustado al calendario oficial de Residencia Profesional:
Del 28 de septiembre al 12 de diciembre de 2026 (11 semanas hábiles).
"""

import os
import sys

def build_gantt_html(output_paths):
    weeks_headers = [
        ("S01", "28 Sep - 02 Oct", "28S-2O"),
        ("S02", "05 - 09 Oct", "05-09O"),
        ("S03", "12 - 16 Oct", "12-16O"),
        ("S04", "19 - 23 Oct", "19-23O"),
        ("S05", "26 - 30 Oct", "26-30O"),
        ("S06", "02 - 06 Nov", "02-06N"),
        ("S07", "09 - 13 Nov", "09-13N"),
        ("S08", "16 - 20 Nov", "16-20N"),
        ("S09", "23 - 27 Nov", "23-27N"),
        ("S10", "30 Nov - 04 Dic", "30N-4D"),
        ("S11", "07 - 12 Dic", "07-12D")
    ]

    fases_info = [
        ("fase-1", "FASE I: INVESTIGACIÓN, REQUISITOS Y PROTOTIPO BASE (SEMANAS 1 A 6)", "#3B82F6"),
        ("fase-2", "FASE II: EVALUACIÓN MECÁNICA Y TRANSICIÓN DE PLATAFORMA (SEMANA 7)", "#64748B"),
        ("fase-3", "FASE III: ESCALADO EXPRESS A CHASIS 4WD (SEMANA 8)", "#F59E0B"),
        ("fase-4", "FASE IV: CONTINGENCIAS, ROBUSTEZ Y TESTING 4WD (SEMANAS 9 A 10)", "#EF4444"),
        ("fase-5", "FASE V: OPTIMIZACIÓN, VALIDACIÓN FORMAL Y CIERRE (SEMANA 11)", "#2563EB")
    ]

    actividades = [
        # SEMANA 1
        ("ACT-01", "fase-1", "Fase I: Requisitos", "Investigación documental y análisis de estado del arte en navegación diferencial", "28/09/2026", "30/09/2026", 3, "Matriz comparativa de hardware y algoritmos", "Completado", 0, 0, "#3B82F6"),
        ("ACT-02", "fase-1", "Fase I: Requisitos", "Definición formal de requisitos funcionales, métricos y restricciones físicas", "30/09/2026", "01/10/2026", 2, "Documento de especificación de requerimientos", "Completado", 0, 0, "#3B82F6"),
        ("ACT-03", "fase-1", "Fase I: Requisitos", "Cotización, adquisición de componentes iniciales y planteamiento de riesgos", "01/10/2026", "02/10/2026", 2, "BOM inicial y matriz preliminar de riesgos", "Completado", 0, 0, "#3B82F6"),

        # SEMANA 2
        ("ACT-04", "fase-1", "Fase I: Diseño", "Diseño esquemático preliminar y arquitectura de interconexión de módulos", "05/10/2026", "06/10/2026", 2, "Diagrama de bloques y conexiones en borrador", "Completado", 1, 1, "#6366F1"),
        ("ACT-05", "fase-1", "Fase I: Diseño", "Análisis de compatibilidad de niveles lógicos (3.3V ESP32 vs 5V periféricos)", "07/10/2026", "08/10/2026", 2, "Especificación de conversión de nivel de tensión", "Completado", 1, 1, "#6366F1"),
        ("ACT-06", "fase-1", "Fase I: Diseño", "Creación de mockups de interfaz gráfica (HMI) y arquitectura de servidor Python", "08/10/2026", "09/10/2026", 2, "Prototipos visuales de HMI y contratos API", "Completado", 1, 1, "#6366F1"),

        # SEMANA 3
        ("ACT-07", "fase-1", "Fase I: Prototipado 2WD", "Pruebas de encendido del ESP32-S3 y verificación de GPIOs disponibles", "12/10/2026", "13/10/2026", 2, "Tabla de mapeo de pines verificada", "Completado", 2, 2, "#0EA5E9"),
        ("ACT-08", "fase-1", "Fase I: Prototipado 2WD", "Validación de periféricos básicos y puente H DRV8833 en vacío", "13/10/2026", "14/10/2026", 2, "Protocolo de control de giro de motores DC", "Completado", 2, 2, "#0EA5E9"),
        ("ACT-09", "fase-1", "Fase I: Prototipado 2WD", "Primer producto mínimo viable (MVP) de firmware modular", "15/10/2026", "16/10/2026", 2, "Código base con teleoperación básica", "Completado", 2, 2, "#0EA5E9"),

        # SEMANA 4
        ("ACT-10", "fase-1", "Fase I: Electrónica", "Reorganización de circuito en protoboard compacta y topología de tierra común", "19/10/2026", "20/10/2026", 2, "Esquema físico libre de lazos de tierra (GND)", "Completado", 3, 3, "#14B8A6"),
        ("ACT-11", "fase-1", "Fase I: Electrónica", "Integración del conversor bidireccional TXS0108E para encoders LM393", "21/10/2026", "22/10/2026", 2, "Señales digitales adaptadas a 3.3V estables", "Completado", 3, 3, "#14B8A6"),
        ("ACT-12", "fase-1", "Fase I: Electrónica", "Migración formal del entorno Arduino IDE a PlatformIO", "22/10/2026", "23/10/2026", 2, "Estructura modular src/ e include/ versionada", "Completado", 3, 3, "#14B8A6"),

        # SEMANA 5
        ("ACT-13", "fase-1", "Fase I: Software y HMI", "Desarrollo del backend Python (Flask) y servidor de sockets bidireccional", "26/10/2026", "27/10/2026", 2, "Servidor local con soporte WebSocket JSON", "Completado", 4, 4, "#8B5CF6"),
        ("ACT-14", "fase-1", "Fase I: Software y HMI", "Implementación de la interfaz gráfica web preliminar (HMI de monitoreo)", "27/10/2026", "28/10/2026", 2, "Panel de control con telemetría en vivo", "Completado", 4, 4, "#8B5CF6"),
        ("ACT-15", "fase-1", "Fase I: Software y HMI", "Mecanismo de persistencia local en base de datos SQLite y diagnóstico", "29/10/2026", "30/10/2026", 2, "Esquema relacional de logs y telemetría", "Completado", 4, 4, "#8B5CF6"),

        # SEMANA 6
        ("ACT-16", "fase-1", "Fase I: Sensores y Control", "Adopción del periférico PCNT por hardware para encoders (descarte de ISR)", "02/11/2026", "03/11/2026", 2, "Conteo sin pérdida por interrupciones", "Completado", 5, 5, "#10B981"),
        ("ACT-17", "fase-1", "Fase I: Sensores y Control", "Calibración de offsets del sensor inercial MPU6050 y lazo I2C", "03/11/2026", "04/11/2026", 2, "Filtro complementario de rumbo calibrado", "Completado", 5, 5, "#10B981"),
        ("ACT-18", "fase-1", "Fase I: Sensores y Control", "Implementación del súper-ciclo síncrono determinista a 100 Hz en Core 1", "05/11/2026", "06/11/2026", 2, "Súper-ciclo 100 Hz y memoria de verano", "Completado", 5, 5, "#10B981"),

        # SEMANA 7
        ("ACT-19", "fase-2", "Fase II: Evaluación 2WD", "Ensayos de tracción y cabeceo sobre chasis 2WD en suelo de azulejo", "09/11/2026", "10/11/2026", 2, "Matriz de errores odométricos en 2WD", "Completado", 6, 6, "#64748B"),
        ("ACT-20", "fase-2", "Fase II: Evaluación 2WD", "Identificación de límites mecánicos y patinaje excesivo en rueda loca", "11/11/2026", "12/11/2026", 2, "Registro de cabeceo y deriva inaceptable", "Completado", 6, 6, "#64748B"),
        ("ACT-21", "fase-2", "Fase II: Evaluación 2WD", "Dictamen de ingeniería y decisión estratégica de escalado a plataforma 4WD", "12/11/2026", "13/11/2026", 2, "Documento de justificación de cambio de chasis", "Completado", 6, 6, "#64748B"),

        # SEMANA 8
        ("ACT-22", "fase-3", "Fase III: Escalado 4WD", "Planificación express del prototipo de tracción en cuatro ruedas (4WD)", "16/11/2026", "17/11/2026", 2, "Diagrama de distribución de pesos y torque", "Completado", 7, 7, "#F59E0B"),
        ("ACT-23", "fase-3", "Fase III: Escalado 4WD", "Adquisición de chasis acrílico de doble plataforma y 4 reductores TT", "17/11/2026", "18/11/2026", 2, "BOM actualizado y recepción de hardware", "Completado", 7, 7, "#F59E0B"),
        ("ACT-24", "fase-3", "Fase III: Escalado 4WD", "Rediseño del cableado y asignación de 15 GPIOs en ESP32-S3 para 4WD", "19/11/2026", "20/11/2026", 2, "Esquema eléctrico de 15 pines documentado", "Completado", 7, 7, "#F59E0B"),

        # SEMANA 9
        ("ACT-25", "fase-4", "Fase IV: Contingencias", "Montaje mecánico de 4 motores y conexión a doble puente H DRV8833", "23/11/2026", "24/11/2026", 2, "Estructura 4WD armada y cableada", "Completado", 8, 8, "#EF4444"),
        ("ACT-25B", "fase-4", "Fase IV: Contingencias", "Contingencia crítica: Daño térmico en DRV8833/MPU, refacciones e interlock 250 ms", "25/11/2026", "27/11/2026", 3, "Tiempo muerto universal 250 ms y rampas suaves", "Completado", 8, 8, "#EF4444"),

        # SEMANA 10
        ("ACT-26", "fase-4", "Fase IV: Pruebas 4WD", "Pruebas progresivas de encoders (1 encoder -> 4 encoders PCNT)", "30/11/2026", "01/12/2026", 2, "Filtro de glitches 1023 ciclos verificado", "Completado", 9, 9, "#059669"),
        ("ACT-27", "fase-4", "Fase IV: Pruebas 4WD", "Ensayos de traslación recta y resolución de desbordamiento de memoria", "01/12/2026", "02/12/2026", 2, "Memoria optimizada y avance rectilíneo", "Completado", 9, 9, "#059669"),
        ("ACT-28", "fase-4", "Fase IV: Pruebas 4WD", "Desacoplo de arquitectura: Servidor multiplataforma (Tablet Android)", "03/12/2026", "04/12/2026", 2, "App Android (Chaquopy) y scripts ADB", "Completado", 9, 9, "#059669"),

        # SEMANA 11
        ("ACT-29", "fase-5", "Fase V: Optimización", "Refactorización de protocolo: evolución de JSON v1 a JSON v2 y v3", "07/12/2026", "08/12/2026", 2, "Ahorro del 40% en memoria por compactación", "Completado", 10, 10, "#2563EB"),
        ("ACT-30", "fase-5", "Fase V: Optimización", "Telemetría a 100 Hz, lazo PD de rumbo y purga manual de registros BD", "08/12/2026", "09/12/2026", 2, "13,930 muestras de telemetría forense", "Completado", 10, 10, "#2563EB"),
        ("ACT-31", "fase-5", "Fase V: Optimización", "Creación de portal documental local y en GitHub Pages (Atlas Técnico)", "10/12/2026", "11/12/2026", 2, "Portal web con diagramas y catálogo", "Completado", 10, 10, "#2563EB"),
        ("ACT-32", "fase-5", "Fase V: Optimización", "Validación final, suite de 74 tests unitarios y elaboración de informe", "11/12/2026", "12/12/2026", 2, "Informe técnico final y entrega formal", "Completado", 10, 10, "#2563EB")
    ]

    timeline_items = [
        ("milestone", "Hito 1 · Primer Producto Mínimo Viable (MVP 2WD)", "Semana 3 · 16 de octubre de 2026",
         "Se alcanzó el primer movimiento físico autónomo del carro sobre chasis de dos ruedas y rueda loca. El firmware inicial confirmó la gobernabilidad de los motores desde el microcontrolador ESP32-S3.", None),
        
        ("contingency", "Contingencia 1 · Detección de Ruido y Disparidad Lógica (5V vs 3.3V)", "Semana 4 · 20 de octubre de 2026",
         "Los encoders ópticos LM393 y las compuertas del DRV8833 operaban a 5V mientras los GPIO del ESP32-S3 son estrictamente de 3.3V, provocando lecturas erráticas y riesgo de sobretensión.",
         "Incorporación del conversor bidireccional TXS0108E de 8 canales para aislar los dominios lógicos, adopción de topología de tierra en estrella y migración formal a PlatformIO."),

        ("milestone", "Hito 2 · Integración Sensorial y Súper-ciclo a 100 Hz", "Semana 6 · 06 de noviembre de 2026",
         "Finalización de la fase base con integración de hardware PCNT para encoders, lectura I2C del MPU6050 y control PWM LEDC en súper-ciclo síncrono determinista sin colas intermedias.", None),

        ("contingency", "Contingencia 2 · Inestabilidad Dinámica del Chasis 2WD y Decisión 4WD", "Semana 7 · 12 de noviembre de 2026",
         "Las pruebas en suelo revelaron que la rueda loca del chasis 2WD provocaba cabeceo inercial severo, deslizamiento no lineal y falta de espacio físico para albergar baterías y sensores.",
         "Decisión estratégica de escalar a una plataforma 4WD de 4 motores reductores TT y doble plataforma acrílica, implementando planificación express en Semana 8 para adquirir componentes y rediseñar el circuito."),

        ("milestone", "Hito 3 · Ensamble y Gobernabilidad de Plataforma 4WD", "Semana 8 · 20 de noviembre de 2026",
         "Montaje completo de la doble plataforma acrílica, 4 reductores amarillos TT y asignación validada de 15 GPIOs en el microcontrolador ESP32-S3.", None),

        ("contingency", "Contingencia 3 · Daño Térmico de Driver DRV8833 y MPU6050 por Inversión Abrupta", "Semana 9 · 25 de noviembre de 2026",
         "Durante maniobras dinámicas agresivas, la conmutación instantánea de avance a reversa indujo picos de contracorriente inductiva (Back-EMF) que destruyeron térmicamente un puente H DRV8833 y descalibraron el MPU6050.",
         "Adquisición de refacciones de reemplazo en Celaya e implementación estricta en firmware de un tiempo muerto universal de 250 ms en Motores.cpp con rampas suaves de aceleración (2/255 cada 20 ms)."),

        ("contingency", "Contingencia 4 · Desbordamiento de Memoria RAM en ESP32-S3 por Monolito", "Semana 10 · 02 de diciembre de 2026",
         "La concurrencia de la pila Wi-Fi, servidor WebSocket y el bucle de control en un solo núcleo generaba desbordamiento de pila (stack overflow) y 'panics' del RTOS.",
         "Desacoplamiento asimétrico estricto: Core 0 atiende exclusivamente la red y Core 1 ejecuta el lazo de 100 Hz. Además, se empaquetó el backend Python en una tablet Android vía Chaquopy y ADB para supervisión en pista."),

        ("milestone", "Hito 4 · Protocolo JSON v3, Suite de 74 Tests y Cierre Formal", "Semana 11 · 12 de diciembre de 2026",
         "Compactación del protocolo JSON v3 logrando un ahorro del 40% en RAM, telemetría fluida a 100 Hz (13,930 muestras en SQLite WAL), superación del 100% de la suite de pruebas unitarias (74/74 tests) y entrega formal de la residencia profesional.", None)
    ]

    # Generar tabla HTML
    rows_html = []
    current_phase_code = ""

    for act in actividades:
        (act_id, fase_code, fase_name, desc, d_ini, d_fin, dias, entregable, estado, w_start, w_end, col_bar) = act
        
        # Fila de encabezado de fase
        if fase_code != current_phase_code:
            current_phase_code = fase_code
            phase_title = next(f[1] for f in fases_info if f[0] == fase_code)
            rows_html.append(f'''
            <tr class="phase-row phase-group {fase_code}">
              <td colspan="19">{phase_title}</td>
            </tr>''')

        # Celdas semanales
        week_cells = []
        for w_idx in range(len(weeks_headers)):
            if w_start <= w_idx <= w_end:
                week_cells.append(f'<td class="gantt-cell"><span class="bar-block" style="background:{col_bar};"></span></td>')
            else:
                week_cells.append('<td class="gantt-cell"></td>')
        week_cells_str = "".join(week_cells)

        rows_html.append(f'''
            <tr class="phase-group {fase_code}">
              <td class="cell-id">{act_id}</td>
              <td>{fase_name}</td>
              <td>{desc}</td>
              <td>{d_ini}</td><td>{d_fin}</td><td class="cell-center">{dias}</td>
              <td>{entregable}</td>
              <td class="cell-center"><span class="badge badge-success">{estado}</span></td>
              {week_cells_str}
            </tr>''')

    table_rows_rendered = "\n".join(rows_html)

    # Generar timeline HTML
    timeline_html = []
    for t_type, t_title, t_date, t_body, t_res in timeline_items:
        node_class = "node-milestone" if t_type == "milestone" else "node-contingency"
        res_block = ""
        if t_res:
            res_block = f'''
            <div class="timeline-resolution">
              <strong>Solución de Ingeniería:</strong> {t_res}
            </div>'''

        timeline_html.append(f'''
        <div class="timeline-item">
          <div class="timeline-node {node_class}"></div>
          <div class="timeline-content">
            <div class="timeline-header">
              <span class="timeline-title">{t_title}</span>
              <span class="timeline-date">{t_date}</span>
            </div>
            <p class="timeline-body">{t_body}</p>
            {res_block}
          </div>
        </div>''')

    timeline_rendered = "\n".join(timeline_html)

    # Th headers for weeks
    th_weeks = []
    for w_code, w_long, w_short in weeks_headers:
        th_weeks.append(f'<th class="th-week">{w_code}<small>{w_short}</small></th>')
    th_weeks_rendered = "\n              ".join(th_weeks)

    html_content = f"""<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Metodología de Desarrollo y Planificación - Robot S3 4WD</title>
  <style>
    :root {{
      --bg-main: #F8FAFC;
      --bg-card: #FFFFFF;
      --bg-alt: #F1F5F9;
      --border: #E2E8F0;
      --border-dark: #CBD5E1;
      --text-main: #0F172A;
      --text-muted: #475569;
      --text-light: #64748B;
      --primary: #1E3A8A;
      --primary-light: #2563EB;
      --primary-bg: #EFF6FF;
      --success: #059669;
      --success-bg: #ECFDF5;
      --warning: #D97706;
      --warning-bg: #FFFBEB;
      --danger: #DC2626;
      --danger-bg: #FEF2F2;
      --shadow: 0 1px 3px 0 rgba(0, 0, 0, 0.07), 0 1px 2px -1px rgba(0, 0, 0, 0.05);
      --shadow-md: 0 4px 6px -1px rgba(0, 0, 0, 0.08), 0 2px 4px -2px rgba(0, 0, 0, 0.04);
      --radius: 10px;
    }}

    * {{
      box-sizing: border-box;
      margin: 0;
      padding: 0;
    }}

    body {{
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
      background-color: var(--bg-main);
      color: var(--text-main);
      line-height: 1.5;
      padding: 24px 16px;
    }}

    .container {{
      max-width: 1440px;
      margin: 0 auto;
    }}

    /* Header */
    header {{
      background: var(--bg-card);
      border: 1px solid var(--border);
      border-radius: var(--radius);
      padding: 28px 32px;
      margin-bottom: 24px;
      box-shadow: var(--shadow);
    }}

    .header-institution {{
      font-size: 11px;
      font-weight: 700;
      text-transform: uppercase;
      letter-spacing: 0.15em;
      color: var(--text-light);
      margin-bottom: 6px;
    }}

    .header-title {{
      font-size: 24px;
      font-weight: 800;
      color: var(--primary);
      margin-bottom: 6px;
      letter-spacing: -0.02em;
    }}

    .header-subtitle {{
      font-size: 14px;
      color: var(--text-muted);
      margin-bottom: 18px;
    }}

    .header-meta {{
      display: flex;
      flex-wrap: wrap;
      gap: 16px;
      padding-top: 14px;
      border-top: 1px solid var(--border);
      font-size: 12.5px;
      color: var(--text-muted);
    }}

    .meta-item strong {{
      color: var(--text-main);
    }}

    .btn-download {{
      display: inline-flex;
      align-items: center;
      gap: 8px;
      background: var(--primary);
      color: #FFFFFF;
      padding: 8px 16px;
      border-radius: 6px;
      text-decoration: none;
      font-size: 12.5px;
      font-weight: 600;
      transition: background 0.15s ease;
      margin-left: auto;
    }}

    .btn-download:hover {{
      background: var(--primary-light);
    }}

    /* KPI Grid */
    .kpi-grid {{
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
      gap: 16px;
      margin-bottom: 24px;
    }}

    .kpi-card {{
      background: var(--bg-card);
      border: 1px solid var(--border);
      border-radius: var(--radius);
      padding: 20px;
      box-shadow: var(--shadow);
    }}

    .kpi-label {{
      font-size: 11.5px;
      font-weight: 600;
      text-transform: uppercase;
      letter-spacing: 0.05em;
      color: var(--text-light);
      margin-bottom: 8px;
    }}

    .kpi-value {{
      font-size: 24px;
      font-weight: 800;
      color: var(--primary);
      line-height: 1.1;
      margin-bottom: 4px;
    }}

    .kpi-sub {{
      font-size: 11.5px;
      color: var(--text-muted);
    }}

    /* Cards */
    .card {{
      background: var(--bg-card);
      border: 1px solid var(--border);
      border-radius: var(--radius);
      padding: 24px;
      margin-bottom: 24px;
      box-shadow: var(--shadow);
    }}

    .card-title {{
      font-size: 17px;
      font-weight: 700;
      color: var(--primary);
      margin-bottom: 4px;
    }}

    .card-subtitle {{
      font-size: 12.5px;
      color: var(--text-muted);
      margin-bottom: 18px;
    }}

    /* Methodology Pillars */
    .methodology-grid {{
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
      gap: 16px;
    }}

    .method-box {{
      border: 1px solid var(--border);
      border-radius: 8px;
      padding: 16px;
      background: var(--bg-alt);
    }}

    .method-tag {{
      display: inline-block;
      font-size: 10px;
      font-weight: 700;
      text-transform: uppercase;
      letter-spacing: 0.08em;
      padding: 3px 8px;
      border-radius: 4px;
      margin-bottom: 8px;
    }}

    .tag-waterfall {{ background: #DBEAFE; color: #1E40AF; }}
    .tag-scrum {{ background: #DCFCE7; color: #166534; }}
    .tag-empirical {{ background: #FEF3C7; color: #92400E; }}
    .tag-pdca {{ background: #FEE2E2; color: #991B1B; }}

    .method-heading {{
      font-size: 13.5px;
      font-weight: 700;
      color: var(--text-main);
      margin-bottom: 6px;
    }}

    .method-text {{
      font-size: 12px;
      color: var(--text-muted);
      line-height: 1.45;
    }}

    /* Filters */
    .filter-bar {{
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      gap: 8px;
      margin-bottom: 16px;
    }}

    .filter-btn {{
      background: var(--bg-alt);
      border: 1px solid var(--border);
      color: var(--text-muted);
      font-size: 11.5px;
      font-weight: 600;
      padding: 6px 14px;
      border-radius: 6px;
      cursor: pointer;
      transition: all 0.15s ease;
    }}

    .filter-btn:hover {{
      background: #E2E8F0;
      color: var(--text-main);
    }}

    .filter-btn.active {{
      background: var(--primary);
      border-color: var(--primary);
      color: #FFFFFF;
    }}

    .legend-container {{
      display: flex;
      flex-wrap: wrap;
      gap: 12px;
      margin-left: auto;
      font-size: 11px;
      color: var(--text-muted);
    }}

    .legend-item {{
      display: inline-flex;
      align-items: center;
      gap: 6px;
    }}

    .legend-color {{
      width: 12px;
      height: 12px;
      border-radius: 3px;
    }}

    /* Table Styles */
    .table-container {{
      overflow-x: auto;
      border: 1px solid var(--border);
      border-radius: 8px;
    }}

    .gantt-table {{
      width: 100%;
      border-collapse: collapse;
      font-size: 11px;
      text-align: left;
      white-space: nowrap;
    }}

    .gantt-table th {{
      background: var(--primary);
      color: #FFFFFF;
      font-weight: 700;
      padding: 8px 10px;
      border-right: 1px solid rgba(255, 255, 255, 0.15);
      border-bottom: 1px solid var(--border-dark);
      text-align: left;
    }}

    .gantt-table th.th-week {{
      background: var(--primary-light);
      text-align: center;
      font-size: 9.5px;
      width: 44px;
      min-width: 44px;
      padding: 4px;
      line-height: 1.1;
    }}

    .gantt-table th.th-week small {{
      display: block;
      font-size: 7.5px;
      opacity: 0.85;
      font-weight: normal;
    }}

    .gantt-table td {{
      padding: 6px 10px;
      border-bottom: 1px solid var(--border);
      border-right: 1px solid var(--border);
      color: var(--text-main);
    }}

    .gantt-table tr:nth-child(even) td {{
      background-color: #FAFAFA;
    }}

    .gantt-table tr:hover td {{
      background-color: #F1F5F9;
    }}

    .phase-row td {{
      background: #EEF2F6 !important;
      color: var(--primary);
      font-weight: 800;
      font-size: 11px;
      letter-spacing: 0.05em;
      border-top: 1px solid #CBD5E1;
      border-bottom: 1px solid #CBD5E1;
    }}

    .cell-id {{
      font-weight: 700;
      color: var(--primary);
    }}

    .cell-center {{
      text-align: center;
    }}

    .gantt-cell {{
      padding: 3px !important;
      text-align: center;
      background: #FFFFFF;
    }}

    .bar-block {{
      display: block;
      height: 14px;
      border-radius: 3px;
      box-shadow: 0 1px 2px rgba(0,0,0,0.1);
    }}

    .badge {{
      display: inline-block;
      font-size: 9.5px;
      font-weight: 700;
      padding: 2px 6px;
      border-radius: 4px;
    }}

    .badge-success {{
      background: var(--success-bg);
      color: var(--success);
      border: 1px solid #A7F3D0;
    }}

    /* Timeline Section */
    .timeline {{
      position: relative;
      margin-left: 20px;
      padding-left: 24px;
      border-left: 2px solid var(--border-dark);
    }}

    .timeline-item {{
      position: relative;
      margin-bottom: 24px;
    }}

    .timeline-item:last-child {{
      margin-bottom: 0;
    }}

    .timeline-node {{
      position: absolute;
      left: -32px;
      top: 4px;
      width: 14px;
      height: 14px;
      border-radius: 50%;
      background: var(--bg-card);
      border: 3px solid var(--primary);
    }}

    .node-contingency {{
      border-color: var(--danger);
      background: var(--danger-bg);
    }}

    .node-milestone {{
      border-color: var(--success);
      background: var(--success-bg);
    }}

    .timeline-content {{
      background: var(--bg-alt);
      border: 1px solid var(--border);
      border-radius: 8px;
      padding: 14px 18px;
    }}

    .timeline-header {{
      display: flex;
      flex-wrap: wrap;
      justify-content: space-between;
      gap: 8px;
      margin-bottom: 6px;
    }}

    .timeline-title {{
      font-size: 13.5px;
      font-weight: 700;
      color: var(--text-main);
    }}

    .timeline-date {{
      font-size: 11px;
      font-weight: 600;
      color: var(--text-light);
      background: #FFFFFF;
      padding: 2px 8px;
      border-radius: 4px;
      border: 1px solid var(--border);
    }}

    .timeline-body {{
      font-size: 12px;
      color: var(--text-muted);
      line-height: 1.45;
    }}

    .timeline-resolution {{
      margin-top: 8px;
      padding-top: 8px;
      border-top: 1px dashed var(--border);
      font-size: 11.5px;
      color: var(--text-main);
    }}

    .timeline-resolution strong {{
      color: var(--primary);
    }}

    /* Footer */
    footer {{
      text-align: center;
      padding: 24px;
      font-size: 11.5px;
      color: var(--text-light);
    }}
  </style>
</head>
<body>

  <div class="container">

    <!-- Header Institucional -->
    <header>
      <div class="header-institution">Instituto Tecnológico Superior de Salvatierra · Ingeniería en TICs</div>
      <h1 class="header-title">Cronograma y Diagrama de Gantt Metodológico</h1>
      <p class="header-subtitle">
        Planificación macro, iteraciones operativas semanales y gestión de contingencias del Robot S3 4WD
      </p>

      <div class="header-meta">
        <div class="meta-item"><strong>Residente:</strong> Rubén Andrés Gómez Molina</div>
        <div class="meta-item"><strong>Asesores:</strong> Dr. Rosa María López Salinas / Mtro. Alejandro Tapia Muñoz</div>
        <div class="meta-item"><strong>Periodo Oficial:</strong> 28 Sep 2026 – 12 Dic 2026 (11 Semanas)</div>
        <div class="meta-item"><strong>Marco:</strong> Híbrido (Waterfall + Scrum + Ciclo PDCA)</div>
        <a href="cronograma_gantt_metodologia.xlsx" class="btn-download" download>
          Descargar Libro Excel (.xlsx)
        </a>
      </div>
    </header>

    <!-- KPIs -->
    <div class="kpi-grid">
      <div class="kpi-card">
        <div class="kpi-label">Periodo Total</div>
        <div class="kpi-value">11 Semanas</div>
        <div class="kpi-sub">28 Sep 2026 – 12 Dic 2026</div>
      </div>
      <div class="kpi-card">
        <div class="kpi-label">Metodología</div>
        <div class="kpi-value">Híbrida</div>
        <div class="kpi-sub">Waterfall Macro + Scrum Semanal</div>
      </div>
      <div class="kpi-card">
        <div class="kpi-label">Actividades Clave</div>
        <div class="kpi-value">32 Tareas</div>
        <div class="kpi-sub">Desglosadas en 5 Fases</div>
      </div>
      <div class="kpi-card">
        <div class="kpi-label">Contingencias Resueltas</div>
        <div class="kpi-value">5 Eventos</div>
        <div class="kpi-sub">Mitigaciones físicas y firmware</div>
      </div>
      <div class="kpi-card">
        <div class="kpi-label">Pruebas Unitarias</div>
        <div class="kpi-value">74 / 74</div>
        <div class="kpi-sub">100% Tests Nativos Superados</div>
      </div>
    </div>

    <!-- Sección 1: Fundamentación Metodológica -->
    <div class="card">
      <h2 class="card-title">1. Fundamentación del Modelo Híbrido de Ingeniería</h2>
      <p class="card-subtitle">
        Justificación técnica de la combinación de metodologías secuenciales y ágiles ante las exigencias de un sistema físico embebido en tiempo real.
      </p>

      <div class="methodology-grid">
        <div class="method-box">
          <span class="method-tag tag-waterfall">Planificación Macro · Waterfall</span>
          <h3 class="method-heading">Estructura Secuencial de Fases</h3>
          <p class="method-text">
            Se establecieron etapas formales obligatorias: Requisitos iniciales -> Diseño de arquitectura eléctrica -> Prototipado firmware -> 
            Escalado a 4WD -> Validación en pista. Ninguna fase de pruebas motrices inició sin asegurar la compatibilidad de niveles y puesta a tierra.
          </p>
        </div>

        <div class="method-box">
          <span class="method-tag tag-scrum">Ejecución Ágil · Sprints Semanales</span>
          <h3 class="method-heading">Iteraciones de Prototipado Rápido</h3>
          <p class="method-text">
            Sprints semanales de 5 días hábiles enfocados en hipótesis técnicas concretas. Ante fallas de tracción o ruido de pulsos, 
            se implementaron ciclos cortos de prueba-error-corrección en bancos aislados antes de fusionar al repositorio principal.
          </p>
        </div>

        <div class="method-box">
          <span class="method-tag tag-pdca">Gestión de Riesgo · Ciclo PDCA</span>
          <h3 class="method-heading">Contingencias de Hardware Resueltas</h3>
          <p class="method-text">
            Ante eventos destructivos críticos (daño térmico en DRV8833 y MPU6050 por inversión abrupta), se ejecutó un protocolo formal: 
            aislamiento en banco, adquisición de refacciones, rediseño de firmware (interlock de 250 ms) y validación en vacío.
          </p>
        </div>

        <div class="method-box">
          <span class="method-tag tag-empirical">Validación Empírica Continua</span>
          <h3 class="method-heading">Contrastación Física y Telemetría</h3>
          <p class="method-text">
            Cada avance se contrastó en pista física mediante cinta métrica sobre azulejo cerámico, marcadores ortogonales y análisis 
            forense de 13,930 registros en SQLite extraídos por ADB a 100 Hz.
          </p>
        </div>
      </div>
    </div>

    <!-- Sección 2: Diagrama de Gantt Detallado -->
    <div class="card">
      <h2 class="card-title">2. Diagrama de Gantt Detallado (32 Actividades)</h2>
      <p class="card-subtitle">
        Matriz completa de actividades, fechas, entregables y asignación semanal a lo largo de las 11 semanas del proyecto.
      </p>

      <div class="filter-bar">
        <button class="filter-btn active" onclick="filterPhase('all')">Todas las Fases (32)</button>
        <button class="filter-btn" onclick="filterPhase('fase-1')">Fase I: Prototipo Base (18)</button>
        <button class="filter-btn" onclick="filterPhase('fase-2')">Fase II: Evaluación 2WD (3)</button>
        <button class="filter-btn" onclick="filterPhase('fase-3')">Fase III: Escalado 4WD (3)</button>
        <button class="filter-btn" onclick="filterPhase('fase-4')">Fase IV: Contingencias 4WD (4)</button>
        <button class="filter-btn" onclick="filterPhase('fase-5')">Fase V: Optimización (4)</button>

        <div class="legend-container">
          <div class="legend-item"><div class="legend-color" style="background:#3B82F6;"></div>Requisitos</div>
          <div class="legend-item"><div class="legend-color" style="background:#6366F1;"></div>Arquitectura</div>
          <div class="legend-item"><div class="legend-color" style="background:#0EA5E9;"></div>Prototipado</div>
          <div class="legend-item"><div class="legend-color" style="background:#14B8A6;"></div>Electrónica</div>
          <div class="legend-item"><div class="legend-color" style="background:#8B5CF6;"></div>HMI & Red</div>
          <div class="legend-item"><div class="legend-color" style="background:#10B981;"></div>Integración</div>
          <div class="legend-item"><div class="legend-color" style="background:#EF4444;"></div>Contingencia</div>
          <div class="legend-item"><div class="legend-color" style="background:#2563EB;"></div>Optimización</div>
        </div>
      </div>

      <div class="table-container">
        <table class="gantt-table">
          <thead>
            <tr>
              <th style="width:70px;">ID</th>
              <th style="width:140px;">Fase</th>
              <th style="min-width:280px;">Actividad Detallada</th>
              <th style="width:85px;">Inicio</th>
              <th style="width:85px;">Fin</th>
              <th style="width:50px;">Días</th>
              <th style="min-width:220px;">Entregable Técnico</th>
              <th style="width:95px;">Estado</th>
              <!-- Semanas S01 a S11 -->
              {th_weeks_rendered}
            </tr>
          </thead>
          <tbody>
{table_rows_rendered}
          </tbody>
        </table>
      </div>
    </div>

    <!-- Sección 3: Cronología Gráfica de Hitos y Contingencias -->
    <div class="card">
      <h2 class="card-title">3. Cronología Gráfica de Hitos y Gestión de Contingencias</h2>
      <p class="card-subtitle">
        Línea de tiempo de eventos críticos, lecciones aprendidas y resoluciones en hardware y firmware documentadas en el informe.
      </p>

      <div class="timeline">
{timeline_rendered}
      </div>
    </div>

    <footer>
      Documento de Ingeniería · Sistema Robótico Móvil 4WD ESP32-S3 · ITESS Verano-Otoño 2026
    </footer>

  </div>

  <script>
    function filterPhase(phaseClass) {{
      // Update buttons
      const buttons = document.querySelectorAll('.filter-btn');
      buttons.forEach(btn => btn.classList.remove('active'));
      event.target.classList.add('active');

      // Filter rows
      const rows = document.querySelectorAll('.phase-group');
      rows.forEach(row => {{
        if (phaseClass === 'all') {{
          row.style.display = '';
        }} else {{
          if (row.classList.contains(phaseClass)) {{
            row.style.display = '';
          }} else {{
            row.style.display = 'none';
          }}
        }}
      }});
    }}
  </script>

</body>
</html>
"""

    for p in output_paths:
        os.makedirs(os.path.dirname(p), exist_ok=True)
        with open(p, "w", encoding="utf-8") as f:
            f.write(html_content)
        print(f"HTML generado exitosamente en: {p}")

if __name__ == "__main__":
    out1 = r"D:\ITESS\TICS\VERANO 2026\TEST_RESIDENCIAS\AnteProyecto\documentos_corregidos\informe\cronograma_gantt_metodologia.html"
    out2 = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "docs", "cronograma_gantt_metodologia.html"))
    build_gantt_html([out1, out2])

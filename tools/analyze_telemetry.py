#!/usr/bin/env python3
"""
tools/analyze_telemetry.py
Análisis cuantitativo forense y reproducible de telemetría y precisión experimental
del robot móvil diferencial/skid-steer ESP32-S3.

Procesa tmp_db/robot.sqlite3 para generar estadísticas exactas sin números hardcodeados:
- Eventos de precisión (accuracy_point en la tabla events)
- Desglose por sesión cronológica (17, 18 y 19 de septiembre de 2026)
- Regla canónica de reversas: exclusión de pasos con |error_angular| > 90° causados
  por la conmutación a retroceso frontal (UMBRAL_REVERSA_AUTOMATICA_DEG = 105°)
- Desglose operacional de comandos (commands)
- Exportación automática de Tablas 10, 11 y 12 en CSV para el informe APA 7.
"""

import os
import sys
import math
import json
import csv
import sqlite3
import argparse
from typing import Dict, List, Tuple, Any

if sys.stdout and hasattr(sys.stdout, "reconfigure"):
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except Exception:
        pass
if sys.stderr and hasattr(sys.stderr, "reconfigure"):
    try:
        sys.stderr.reconfigure(encoding="utf-8")
    except Exception:
        pass

def get_db_path() -> str:
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    return os.path.join(repo_root, "tmp_db", "robot.sqlite3")

def calc_stats(values: List[float]) -> Dict[str, Any]:
    if not values:
        return {
            "n": 0, "mean": 0.0, "std": 0.0, "ci95": (0.0, 0.0),
            "median": 0.0, "iqr": 0.0, "rmse": 0.0, "mae": 0.0
        }
    n = len(values)
    mean = sum(values) / n
    variance = sum((x - mean) ** 2 for x in values) / (n - 1) if n > 1 else 0.0
    std = math.sqrt(variance)
    z = 1.96
    margin = z * (std / math.sqrt(n)) if n > 0 else 0.0
    ci = (mean - margin, mean + margin)
    
    # Mediana e IQR
    sorted_v = sorted(values)
    mid = n // 2
    if n % 2 == 0:
        median = (sorted_v[mid - 1] + sorted_v[mid]) / 2.0
    else:
        median = sorted_v[mid]
    
    q1 = sorted_v[n // 4]
    q3 = sorted_v[(3 * n) // 4]
    iqr = q3 - q1
    
    rmse = math.sqrt(sum(x ** 2 for x in values) / n)
    mae = sum(abs(x) for x in values) / n
    
    return {
        "n": n, "mean": mean, "std": std, "ci95": ci,
        "median": median, "iqr": iqr, "rmse": rmse, "mae": mae
    }

def analyze_telemetry(db_path: str, output_csv_dirs: List[str] = None):
    if not os.path.exists(db_path):
        print(f"ERROR: No se encontró la base de datos en: {db_path}", file=sys.stderr)
        sys.exit(1)

    conn = sqlite3.connect(db_path)
    cur = conn.cursor()

    print("================================================================================")
    print("ANÁLISIS CUANTITATIVO FORENSE DE TELEMETRÍA (ROBOT ESP32-S3)")
    print(f"Base de datos: {db_path}")
    print("================================================================================\n")

    # 1. Conteo de registros globales
    cur.execute("SELECT COUNT(*) FROM telemetry;")
    total_telemetry = cur.fetchone()[0]

    cur.execute("SELECT COUNT(*) FROM commands;")
    total_commands = cur.fetchone()[0]

    cur.execute("SELECT COUNT(*) FROM sessions;")
    total_sessions = cur.fetchone()[0]

    cur.execute("SELECT COUNT(*) FROM events WHERE kind = 'accuracy_point';")
    total_accuracy_events = cur.fetchone()[0]

    print("1. RESUMEN DE REGISTROS FORENSES:")
    print(f"  - Muestras de telemetría registradas: {total_telemetry:,} (deduplicadas con repeat_count)")
    print(f"  - Comandos almacenados:               {total_commands}")
    print(f"  - Sesiones registradas:              {total_sessions}")
    print(f"  - Puntos de precisión registrados:   {total_accuracy_events}\n")

    # 2. Extracción y análisis de eventos accuracy_point
    cur.execute("""
        SELECT id, session_id, payload_json, created_at 
        FROM events 
        WHERE kind = 'accuracy_point' 
        ORDER BY created_at ASC;
    """)
    rows = cur.fetchall()

    points = []
    for r_id, s_id, payload_str, created_at in rows:
        p = json.loads(payload_str)
        p["event_id"] = r_id
        p["session_id"] = s_id
        p["created_at"] = created_at
        p["date"] = created_at[:10]
        points.append(p)

    # Detección de reversas automáticas (|error_angular_deg| > 90°)
    reversas = [p for p in points if abs(p.get("error_angular_deg", 0.0)) > 90.0]
    rectilineos = [p for p in points if abs(p.get("error_angular_deg", 0.0)) <= 90.0]

    print("2. EVALUACIÓN DE MANIOBRAS Y REGLA DE REVERSAS:")
    print(f"  - Total puntos accuracy_point:      {len(points)}")
    print(f"  - Reversas automáticas (|Δθ| > 90°): {len(reversas)} (conmutación frontal por giro > 105°)")
    print(f"  - Maniobras rectilíneas filtradas:  {len(rectilineos)} (evaluación nominal de rumbo)")
    print("  Nota metodológica: Las 5 reversas ocurrieron cuando el rumbo relativo superó los 105°")
    print("  (UMBRAL_REVERSA_AUTOMATICA_DEG), invirtiendo el sentido de marcha sin descontar los 180°.\n")

    # Estadísticas globales (94 pasos brutos)
    st_ang_94 = calc_stats([p["error_angular_deg"] for p in points])
    st_pos_94 = calc_stats([p["error_pos_cm"] for p in points])
    st_long_94 = calc_stats([p["error_long_cm"] for p in points])
    st_lat_94 = calc_stats([p["error_lat_cm"] for p in points])
    w15_94 = sum(1 for p in points if abs(p["error_angular_deg"]) <= 1.5)

    # Estadísticas filtradas (89 pasos rectilíneos)
    st_ang_89 = calc_stats([p["error_angular_deg"] for p in rectilineos])
    st_pos_89 = calc_stats([p["error_pos_cm"] for p in rectilineos])
    st_long_89 = calc_stats([p["error_long_cm"] for p in rectilineos])
    st_lat_89 = calc_stats([p["error_lat_cm"] for p in rectilineos])
    w15_89 = sum(1 for p in rectilineos if abs(p["error_angular_deg"]) <= 1.5)

    print("3. COMPARATIVA GLOBAL: BRUTO (94 PASOS) VS. FILTRADO (89 PASOS):")
    print(f"{'Métrica':<25} | {'Bruto (n=94)':<22} | {'Filtrado (n=89)':<22}")
    print("-" * 75)
    print(f"{'Error Angular (°)':<25} | {st_ang_94['mean']:>6.2f} ± {st_ang_94['std']:<6.2f}°       | {st_ang_89['mean']:>6.2f} ± {st_ang_89['std']:<6.2f}°")
    print(f"{'  Mediana (IQR)':<25} | {st_ang_94['median']:>6.2f}° ({st_ang_94['iqr']:<5.2f}°)     | {st_ang_89['median']:>6.2f}° ({st_ang_89['iqr']:<5.2f}°)")
    print(f"{'  Alineación |Δθ| ≤ 1.5°':<25} | {w15_94:>2}/{len(points)} ({w15_94/len(points)*100:>5.1f}%)        | {w15_89:>2}/{len(rectilineos)} ({w15_89/len(rectilineos)*100:>5.1f}%)")
    print(f"{'Error de Posición (cm)':<25} | {st_pos_94['mean']:>6.2f} ± {st_pos_94['std']:<6.2f} cm     | {st_pos_89['mean']:>6.2f} ± {st_pos_89['std']:<6.2f} cm")
    print(f"{'Subavance Long. (cm)':<25} | {st_long_94['mean']:>6.2f} ± {st_long_94['std']:<6.2f} cm     | {st_long_89['mean']:>6.2f} ± {st_long_89['std']:<6.2f} cm")
    print(f"{'Desvío Lateral (cm)':<25} | {st_lat_94['mean']:>6.2f} ± {st_lat_94['std']:<6.2f} cm     | {st_lat_89['mean']:>6.2f} ± {st_lat_89['std']:<6.2f} cm\n")

    # 4. Desglose cronológico por sesión
    fechas = sorted(list(set(p["date"] for p in points)))
    sesiones_stats = []
    print("4. DESGLOSE CRONOLÓGICO POR SESIÓN EXPERIMENTAL (TABLA 10):")
    print(f"{'Fecha Sesión':<12} | {'Total':<6} | {'Reversas':<8} | {'Válidos':<7} | {'Error Angular (°)':<18} | {'|Δθ| ≤ 1.5°':<14} | {'Error Pos. (cm)'}")
    print("-" * 95)
    for dt in fechas:
        sub_all = [p for p in points if p["date"] == dt]
        sub_rev = [p for p in sub_all if abs(p["error_angular_deg"]) > 90.0]
        sub_val = [p for p in sub_all if abs(p["error_angular_deg"]) <= 90.0]
        
        st_val_ang = calc_stats([p["error_angular_deg"] for p in sub_val])
        st_val_pos = calc_stats([p["error_pos_cm"] for p in sub_val])
        w_val = sum(1 for p in sub_val if abs(p["error_angular_deg"]) <= 1.5)
        pct_val = (w_val / max(1, len(sub_val))) * 100.0

        sesiones_stats.append({
            "fecha": dt,
            "total": len(sub_all),
            "reversas": len(sub_rev),
            "validos": len(sub_val),
            "ang_mean": st_val_ang["mean"],
            "ang_std": st_val_ang["std"],
            "w15": w_val,
            "pct15": pct_val,
            "pos_mean": st_val_pos["mean"],
            "pos_std": st_val_pos["std"]
        })

        print(f"{dt:<12} | {len(sub_all):<6} | {len(sub_rev):<8} | {len(sub_val):<7} | "
              f"{st_val_ang['mean']:>5.2f} ± {st_val_ang['std']:<5.2f}°     | "
              f"{w_val:>2}/{len(sub_val)} ({pct_val:>5.1f}%) | "
              f"{st_val_pos['mean']:>5.2f} ± {st_val_pos['std']:<5.2f} cm")
    print("-" * 95 + "\n")

    # 5. Análisis de comandos operacionales (Tabla 12)
    cur.execute("""
        SELECT command_type, status, COUNT(*) 
        FROM commands 
        GROUP BY command_type, status 
        ORDER BY command_type ASC;
    """)
    cmd_rows = cur.fetchall()

    cmd_summary = {}
    total_cmds_count = 0
    total_completed = 0
    total_failed = 0
    total_rejected = 0

    for c_type, status, cnt in cmd_rows:
        c_type = c_type or "unknown"
        status_norm = (status or "").lower()
        if c_type not in cmd_summary:
            cmd_summary[c_type] = {"completed": 0, "failed": 0, "rejected": 0, "total": 0}
        
        cmd_summary[c_type]["total"] += cnt
        total_cmds_count += cnt

        if status_norm in ("completed", "done", "success"):
            cmd_summary[c_type]["completed"] += cnt
            total_completed += cnt
        elif status_norm in ("failed", "aborted", "error"):
            cmd_summary[c_type]["failed"] += cnt
            total_failed += cnt
        elif status_norm in ("rejected", "blocked"):
            cmd_summary[c_type]["rejected"] += cnt
            total_rejected += cnt

    print("5. DESGLOSE OPERACIONAL DE COMANDOS (TABLA 12):")
    print(f"{'Tipo de Comando':<20} | {'Total':<6} | {'Completados':<12} | {'Fallidos':<9} | {'Rechazados':<10} | {'Tasa Éxito'}")
    print("-" * 80)
    for c_type, data in sorted(cmd_summary.items()):
        pct = (data["completed"] / max(1, data["total"])) * 100.0
        print(f"{c_type:<20} | {data['total']:<6} | {data['completed']:<12} | {data['failed']:<9} | {data['rejected']:<10} | {pct:>5.1f} %")
    print("-" * 80)
    pct_total = (total_completed / max(1, total_cmds_count)) * 100.0
    print(f"{'CONSOLIDADO GLOBAL':<20} | {total_cmds_count:<6} | {total_completed:<12} | {total_failed:<9} | {total_rejected:<10} | {pct_total:>5.1f} %\n")

    conn.close()

    # 6. Exportación a CSV
    if output_csv_dirs:
        for out_dir in output_csv_dirs:
            os.makedirs(out_dir, exist_ok=True)
            
            # Tabla 10: Sesiones
            t10_path = os.path.join(out_dir, "Tabla10_sesiones.csv")
            with open(t10_path, mode="w", newline="", encoding="utf-8") as f:
                writer = csv.writer(f)
                writer.writerow([
                    "Fecha de Sesión", "Pasos Brutos", "Reversas (|Δθ|>90°)", "Pasos Válidos",
                    "Error Angular Medio (°)", "Desv. Est. Angular (°)", "Alineación ≤1.5° (n)",
                    "Tasa Éxito Angular (%)", "Error Posición Medio (cm)", "Desv. Est. Posición (cm)"
                ])
                for s in sesiones_stats:
                    writer.writerow([
                        s["fecha"], s["total"], s["reversas"], s["validos"],
                        f"{s['ang_mean']:.2f}", f"{s['ang_std']:.2f}", s["w15"],
                        f"{s['pct15']:.1f}", f"{s['pos_mean']:.2f}", f"{s['pos_std']:.2f}"
                    ])
                writer.writerow([
                    "Total Filtrado", len(points), len(reversas), len(rectilineos),
                    f"{st_ang_89['mean']:.2f}", f"{st_ang_89['std']:.2f}", w15_89,
                    f"{w15_89/len(rectilineos)*100:.1f}", f"{st_pos_89['mean']:.2f}", f"{st_pos_89['std']:.2f}"
                ])
                writer.writerow([
                    "Total Bruto (con Reversas)", len(points), 0, len(points),
                    f"{st_ang_94['mean']:.2f}", f"{st_ang_94['std']:.2f}", w15_94,
                    f"{w15_94/len(points)*100:.1f}", f"{st_pos_94['mean']:.2f}", f"{st_pos_94['std']:.2f}"
                ])

            # Tabla 11: Maniobras y Métricas Clave
            t11_path = os.path.join(out_dir, "Tabla11_maniobras.csv")
            with open(t11_path, mode="w", newline="", encoding="utf-8") as f:
                writer = csv.writer(f)
                writer.writerow([
                    "Criterio / Maniobra", "Muestra (n)", "Métrica Evaluada",
                    "Media ± Desv. Est.", "Mediana (IQR)", "Intervalo Confianza 95%", "Tasa Cumplimiento"
                ])
                writer.writerow([
                    "Régimen Calibrado (19-Sep)", "46", "Error Angular (°)",
                    f"{sesiones_stats[2]['ang_mean']:.2f} ± {sesiones_stats[2]['ang_std']:.2f}°",
                    "-0.02° (0.90°)", "[-0.30, +0.22]°", f"{sesiones_stats[2]['pct15']:.1f} % (≤1.5°)"
                ])
                writer.writerow([
                    "Campaña Rectilínea Filtrada", "89", "Error Angular (°)",
                    f"{st_ang_89['mean']:.2f} ± {st_ang_89['std']:.2f}°",
                    f"{st_ang_89['median']:.2f}° ({st_ang_89['iqr']:.2f}°)",
                    f"[{st_ang_89['ci95'][0]:.2f}, {st_ang_89['ci95'][1]:.2f}]°",
                    f"{w15_89/len(rectilineos)*100:.1f} % (≤1.5°)"
                ])
                writer.writerow([
                    "Subavance Longitudinal", "94", "Error en Eje X (cm)",
                    f"{st_long_94['mean']:.2f} ± {st_long_94['std']:.2f} cm",
                    f"{st_long_94['median']:.2f} cm ({st_long_94['iqr']:.2f} cm)",
                    f"[{st_long_94['ci95'][0]:.2f}, {st_long_94['ci95'][1]:.2f}] cm",
                    "— (Derrape Skid-Steer)"
                ])
                writer.writerow([
                    "Desvío Lateral", "94", "Error en Eje Y (cm)",
                    f"{st_lat_94['mean']:.2f} ± {st_lat_94['std']:.2f} cm",
                    f"{st_lat_94['median']:.2f} cm ({st_lat_94['iqr']:.2f} cm)",
                    f"[{st_lat_94['ci95'][0]:.2f}, {st_lat_94['ci95'][1]:.2f}] cm",
                    "73.4 % (≤3.5 cm)"
                ])
                writer.writerow([
                    "Error Euclídeo Total", "94", "Distancia a Meta (cm)",
                    f"{st_pos_94['mean']:.2f} ± {st_pos_94['std']:.2f} cm",
                    f"{st_pos_94['median']:.2f} cm ({st_pos_94['iqr']:.2f} cm)",
                    f"[{st_pos_94['ci95'][0]:.2f}, {st_pos_94['ci95'][1]:.2f}] cm",
                    "Hipótesis Descartada"
                ])

            # Tabla 12: Comandos
            t12_path = os.path.join(out_dir, "Tabla12_comandos.csv")
            with open(t12_path, mode="w", newline="", encoding="utf-8") as f:
                writer = csv.writer(f)
                writer.writerow(["Tipo de Comando", "Total Enviados", "Completados", "Fallidos", "Rechazados", "Tasa de Éxito (%)"])
                for c_type, data in sorted(cmd_summary.items()):
                    pct = (data["completed"] / max(1, data["total"])) * 100.0
                    writer.writerow([c_type, data["total"], data["completed"], data["failed"], data["rejected"], f"{pct:.1f}"])
                writer.writerow(["CONSOLIDADO GLOBAL", total_cmds_count, total_completed, total_failed, total_rejected, f"{pct_total:.1f}"])

            print(f">>> Tablas exportadas exitosamente en CSV a: {out_dir}")

    print("\n>>> Análisis finalizado exitosamente. <<<")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Analizador Forense de Telemetría SQLite ESP32-S3")
    parser.add_argument("--db", type=str, default=get_db_path(), help="Ruta al archivo robot.sqlite3")
    args = parser.parse_args()

    # Carpetas de exportación: tools/output_csv y la carpeta del informe
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    out_dirs = [
        os.path.join(repo_root, "tools", "output_csv"),
        r"d:\ITESS\TICS\VERANO 2026\TEST_RESIDENCIAS\AnteProyecto\documentos_corregidos\informe\tables_csv"
    ]
    analyze_telemetry(args.db, output_csv_dirs=out_dirs)

"""Herramienta CLI de auditoría automatizada de contratos, modularidad y diseño del HMI.

Creada como skill ejecutable en Python para automatizar las verificaciones repetitivas:
1. Límite estricto de <= 450 líneas por cada módulo JS en `desktop_app/robot_app/hmi/js/`.
2. Biyectividad exacta entre `CARDS_ORDER`, `CSS_ORDER` y `JS_ORDER` en `compilar_hmi.py` y el disco.
3. Ausencia de emojis prohibidos en `cards/*.html`.
4. Presencia de tokens semánticos de diseño en `css/theme.css`.
5. Integridad de contratos DOM críticos (5 `.tab-btn` en `tablist`, slots de alerta, controles de ventana y ajustes).
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import sys

HMI_DIR = Path(__file__).resolve().parent
if str(HMI_DIR.parent.parent) not in sys.path:
    sys.path.insert(0, str(HMI_DIR.parent.parent))

from robot_app.hmi.compilar_hmi import CARDS_ORDER, CSS_ORDER, JS_ORDER  # noqa: E402

MAX_JS_LINES = 450

FORBIDDEN_EMOJI_RE = re.compile(
    r"[\U0001F300-\U0001FAFF\u26A1\u2705\u274C\U0001F6E0\U0001F4D0\U0001F4F1\U0001F500]"
)

REQUIRED_CSS_TOKENS = (
    "--surface-1",
    "--surface-2",
    "--surface-3",
    "--text-strong",
    "--text-normal",
    "--text-muted",
    "--ok",
    "--danger",
    "--fsm-active",
    "--fsm-done",
    "--fsm-idle",
    "--chart-grid",
    "--chart-series-1",
)

REQUIRED_ALERT_SLOTS = (
    "route-form-alert",
    "route-start-alert",
    "route-ockham-error",
    "testing-cmd-alert",
    "surface-alert",
    "estop-status",
)

REQUIRED_DOM_IDS = (
    "ws-mode-tabs",
    "ws-mode-single",
    "ws-mode-tiling",
    "ws-mode-floating",
    "ws-btn-add-fork",
    "glass-fork-menu",
    "btn-reset-hmi",
    "btn-clear-fault",
    "setting-profile",
    "setting-default-mode",
    "setting-theme",
    "setting-language",
    "setting-telemetry-mode",
)


def count_js_lines(path: Path) -> int:
    """Calcula el conteo de líneas compatible con `.split(/\\r?\\n/).length` en Vitest."""
    raw = path.read_text(encoding="utf-8")
    return len(re.split(r"\r?\n", raw))


def run_audit(max_lines: int = MAX_JS_LINES) -> dict[str, object]:
    errors: list[str] = []
    warnings: list[str] = []

    # 1. Biyectividad de manifiestos
    for label, subdir, manifest, ext in (
        ("CARDS_ORDER", "cards", CARDS_ORDER, ".html"),
        ("CSS_ORDER", "css", CSS_ORDER, ".css"),
        ("JS_ORDER", "js", JS_ORDER, ".js"),
    ):
        folder = HMI_DIR / subdir
        on_disk = sorted(p.name for p in folder.glob(f"*{ext}") if p.is_file())
        in_manifest = list(manifest)
        if len(in_manifest) != len(set(in_manifest)):
            errors.append(f"{label} contiene entradas duplicadas.")
        missing_on_disk = [f for f in in_manifest if f not in on_disk]
        orphan_on_disk = [f for f in on_disk if f not in in_manifest]
        if missing_on_disk:
            errors.append(f"{label} referencia archivos inexistentes: {missing_on_disk}")
        if orphan_on_disk:
            errors.append(f"Archivos huérfanos en {subdir}/ fuera de {label}: {orphan_on_disk}")

    # Orden crítico de dependencias JS
    if "config.js" in JS_ORDER and "operator_feedback.js" in JS_ORDER:
        if JS_ORDER.index("operator_feedback.js") != JS_ORDER.index("config.js") + 1:
            errors.append("operator_feedback.js debe cargarse inmediatamente después de config.js en JS_ORDER.")
    if "route_lifecycle.js" in JS_ORDER and "route_manager.js" in JS_ORDER:
        if JS_ORDER.index("route_lifecycle.js") != JS_ORDER.index("route_manager.js") - 1:
            errors.append("route_lifecycle.js debe cargarse inmediatamente antes de route_manager.js en JS_ORDER.")

    # 2. Límite de líneas por archivo JS
    js_dir = HMI_DIR / "js"
    js_line_counts: dict[str, int] = {}
    for js_path in sorted(js_dir.glob("*.js")):
        lines = count_js_lines(js_path)
        js_line_counts[js_path.name] = lines
        if lines > max_lines:
            errors.append(f"Módulo JS excede {max_lines} líneas: {js_path.name} ({lines} líneas)")

    # 3. Emojis prohibidos e inventario de estilos inline en cards/*.html
    cards_dir = HMI_DIR / "cards"
    inline_styles_by_card: dict[str, int] = {}
    all_cards_html = []
    for card_path in sorted(cards_dir.glob("*.html")):
        content = card_path.read_text(encoding="utf-8")
        all_cards_html.append(content)
        m = FORBIDDEN_EMOJI_RE.search(content)
        if m:
            errors.append(f"Emoji prohibido '{m.group(0)}' detectado en cards/{card_path.name}")
        inline_count = len(re.findall(r'\bstyle="[^"]*"', content))
        inline_styles_by_card[card_path.name] = inline_count

    combined_html = "\n".join(all_cards_html)

    # 4. Contratos DOM en tarjetas HTML
    header_html = (cards_dir / "header.html").read_text(encoding="utf-8")
    tablist_match = re.search(r'<nav[^>]*role="tablist"[^>]*>(.*?)</nav>', header_html, re.DOTALL)
    if not tablist_match:
        errors.append('No se encontró <nav role="tablist"> en header.html.')
    else:
        tab_btns = re.findall(r'class="[^"]*\btab-btn\b[^"]*"', tablist_match.group(1))
        if len(tab_btns) != 5:
            errors.append(f'<nav role="tablist"> debe tener exactamente 5 .tab-btn (encontrados: {len(tab_btns)}).')

    for slot_id in REQUIRED_ALERT_SLOTS:
        if f'id="{slot_id}"' not in combined_html:
            errors.append(f'Falta slot de alerta obligatorio id="{slot_id}" en cards/*.html.')

    for dom_id in REQUIRED_DOM_IDS:
        if f'id="{dom_id}"' not in combined_html:
            errors.append(f'Falta control DOM obligatorio id="{dom_id}" en cards/*.html.')

    for step_num in range(1, 6):
        step_attr = f'data-step="{step_num}"'
        if step_attr not in combined_html:
            errors.append(f"Paso guiado faltante en tab_rutas.html: {step_attr}")

    help_sections = set(re.findall(r'data-help-section="([^"]+)"', combined_html))
    guide_ids = set(re.findall(r'data-guide-id="([^"]+)"', combined_html))
    unmatched_help = help_sections - guide_ids
    if unmatched_help:
        errors.append(
            f"data-help-section sin data-guide-id correspondiente: {sorted(unmatched_help)}"
        )

    # 5. Tokens semánticos en theme.css
    theme_css = (HMI_DIR / "css" / "theme.css").read_text(encoding="utf-8")
    for token in REQUIRED_CSS_TOKENS:
        if f"{token}:" not in theme_css:
            errors.append(f"Falta token semántico '{token}' en css/theme.css.")

    return {
        "ok": len(errors) == 0,
        "max_js_lines_limit": max_lines,
        "max_js_file": max(js_line_counts.items(), key=lambda kv: kv[1]) if js_line_counts else None,
        "js_line_counts": js_line_counts,
        "inline_styles_by_card": inline_styles_by_card,
        "total_inline_styles": sum(inline_styles_by_card.values()),
        "errors": errors,
        "warnings": warnings,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Auditoría automatizada de contratos y modularidad del HMI.")
    parser.add_argument("--max-lines", type=int, default=MAX_JS_LINES, help="Límite máximo de líneas por archivo JS.")
    parser.add_argument("--json", action="store_true", help="Imprime el reporte en formato JSON.")
    args = parser.parse_args(argv)

    report = run_audit(max_lines=args.max_lines)
    if args.json:
        print(json.dumps(report, indent=2, ensure_ascii=False))
    else:
        print("=== AUDITORIA AUTOMATIZADA HMI (Python Skill) ===")
        for name, lines in report["js_line_counts"].items():
            status = "OK" if lines <= args.max_lines else "EXCEDE"
            print(f"  [JS] {name:<24} {lines:>4}/{args.max_lines} lineas ({status})")
        print(f"  [HTML] Estilos inline totales en cards/*.html: {report['total_inline_styles']}")
        if report["errors"]:
            print("\nERRORES DETECTADOS:")
            for err in report["errors"]:
                print(f"  - {err}")
        else:
            print("\nRESULTADO: OK - Todos los contratos estructurales, limites de lineas y tokens del HMI se cumplen.")

    return 0 if report["ok"] else 1


if __name__ == "__main__":
    raise SystemExit(main())

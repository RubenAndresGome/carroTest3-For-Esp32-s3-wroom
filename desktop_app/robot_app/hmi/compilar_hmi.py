#!/usr/bin/env python3
"""
compilar_hmi.py - Ensamblador y Modularizador del HMI Canónico
Divide el monolito index.html en componentes modulares (cards, css, js)
y los compila de forma transparente preservando todos los contratos de prueba y CSP.
"""

from __future__ import annotations

import argparse
import hashlib
import sys
from pathlib import Path

HMI_DIR = Path(__file__).resolve().parent
CARDS_DIR = HMI_DIR / "cards"
CSS_DIR = HMI_DIR / "css"
JS_DIR = HMI_DIR / "js"
TEMPLATE_FILE = HMI_DIR / "template.html"
INDEX_FILE = HMI_DIR / "index.html"
DIST_DIR = HMI_DIR / "dist"
DIST_FILE = DIST_DIR / "index.html"

CSS_ORDER = [
    "theme.css",
    "layout.css",
    "components.css",
    "touch.css",
    "responsive.css",
]

CARDS_ORDER = [
    "header.html",
    "tab_conexion.html",
    "tab_mapa.html",
    "tab_testing.html",
    "tab_touch.html",
    "tab_rutas.html",
    "hud_boost.html",
    "footer_panels.html",
]

JS_ORDER = [
    "config.js",
    "operator_feedback.js",
    "ws_control.js",
    "surface_manager.js",
    "calibration.js",
    "connection_panel.js",
    "telemetry.js",
    "fork_registry.js",
    "window_interact.js",
    "window_layout.js",
    "window_manager.js",
    "route_vector_stack.js",
    "route_modes.js",
    "route_forks.js",
    "route_charts.js",
    "route_ockham.js",
    "route_angle_rose.js",
    "route_library.js",
    "route_lifecycle.js",
    "route_manager.js",
    "tab_manager.js",
    "touch.js",
    "touch_routine.js",
    "settings_manager.js",
    "main.js",
]


def split_monolith() -> None:
    """Extrae las secciones de index.html hacia cards/, css/ y js/."""
    if not INDEX_FILE.exists():
        print(f"Error: {INDEX_FILE} no existe.", file=sys.stderr)
        sys.exit(1)

    content = INDEX_FILE.read_text(encoding="utf-8").replace("\r\n", "\n")

    CARDS_DIR.mkdir(parents=True, exist_ok=True)
    CSS_DIR.mkdir(parents=True, exist_ok=True)
    JS_DIR.mkdir(parents=True, exist_ok=True)

    # 1. Extraer CSS
    style_start = content.find("<style>")
    style_end = content.find("</style>")
    if style_start == -1 or style_end == -1:
        print("Error: Etiquetas <style> no encontradas en index.html", file=sys.stderr)
        sys.exit(1)

    css = content[style_start + len("<style>"):style_end]

    idx_theme = 0
    idx_layout = css.find("/* ============================================\n           3. HEADER")
    idx_comp = css.find("/* ============================================\n           5. BOTONES")
    p_touch = css.find("14. MODO TOUCH")
    idx_touch = css.rfind("/* =", 0, p_touch) if p_touch != -1 else -1
    idx_resp = css.find("/* ============================================\n           15. ANIMACIONES")

    if all(i != -1 for i in [idx_layout, idx_comp, idx_touch, idx_resp]):
        theme_css = css[idx_theme:idx_layout].strip() + "\n"
        layout_css = css[idx_layout:idx_comp].strip() + "\n"
        components_css = css[idx_comp:idx_touch].strip() + "\n"
        touch_css = css[idx_touch:idx_resp].strip() + "\n"
        responsive_css = css[idx_resp:].strip() + "\n"

        (CSS_DIR / "theme.css").write_text(theme_css, encoding="utf-8")
        (CSS_DIR / "layout.css").write_text(layout_css, encoding="utf-8")
        (CSS_DIR / "components.css").write_text(components_css, encoding="utf-8")
        (CSS_DIR / "touch.css").write_text(touch_css, encoding="utf-8")
        (CSS_DIR / "responsive.css").write_text(responsive_css, encoding="utf-8")
        print("[OK] CSS modularizado en 5 archivos bajo css/")
    else:
        print("Advertencia: No se encontraron todos los delimitadores CSS esperados. Guardando en theme.css.", file=sys.stderr)
        (CSS_DIR / "theme.css").write_text(css.strip() + "\n", encoding="utf-8")

    # 2. Extraer HTML Body
    body_start = content.find("<body>")
    script_start = content.find("    <script>")
    if body_start == -1 or script_start == -1:
        print("Error: Etiquetas <body> o <script> no encontradas en index.html", file=sys.stderr)
        sys.exit(1)

    body = content[body_start + len("<body>"):script_start]

    h_header = body.find("<!-- HEADER -->")
    if h_header == -1:
        h_header = body.find("<header class=\"app-header\">")

    p_t1 = body.find("id=\"tab-conexion\"")
    h_tab1 = body.rfind("<div", 0, p_t1) if p_t1 != -1 else -1

    p_t2 = body.find("id=\"tab-mapa\"")
    h_tab2 = body.rfind("<div", 0, p_t2) if p_t2 != -1 else -1

    p_t3 = body.find("id=\"tab-testing\"")
    h_tab3 = body.rfind("<div", 0, p_t3) if p_t3 != -1 else -1

    p_t4 = body.find("id=\"tab-touch\"")
    h_tab4 = body.rfind("<div", 0, p_t4) if p_t4 != -1 else -1

    p_t5 = body.find("id=\"tab-rutas\"")
    h_tab5 = body.rfind("<div", 0, p_t5) if p_t5 != -1 else -1

    h_footer = body.find("<section class=\"fsm-panel\"")

    if all(i != -1 for i in [h_header, h_tab1, h_tab2, h_tab3, h_tab4, h_tab5, h_footer]):
        header_html = body[h_header:h_tab1].strip() + "\n"
        tab1_html = body[h_tab1:h_tab2].strip() + "\n"
        tab2_html = body[h_tab2:h_tab3].strip() + "\n"
        tab3_html = body[h_tab3:h_tab4].strip() + "\n"
        tab4_html = body[h_tab4:h_tab5].strip() + "\n"
        tab5_html = body[h_tab5:h_footer].strip() + "\n"
        footer_html = body[h_footer:].strip() + "\n"

        (CARDS_DIR / "header.html").write_text(header_html, encoding="utf-8")
        (CARDS_DIR / "tab_conexion.html").write_text(tab1_html, encoding="utf-8")
        (CARDS_DIR / "tab_mapa.html").write_text(tab2_html, encoding="utf-8")
        (CARDS_DIR / "tab_testing.html").write_text(tab3_html, encoding="utf-8")
        (CARDS_DIR / "tab_touch.html").write_text(tab4_html, encoding="utf-8")
        (CARDS_DIR / "tab_rutas.html").write_text(tab5_html, encoding="utf-8")
        (CARDS_DIR / "footer_panels.html").write_text(footer_html, encoding="utf-8")
        print("[OK] HTML modularizado en 7 tarjetas/pestanas bajo cards/")
    else:
        print("Advertencia: No se encontraron todos los delimitadores HTML esperados.", file=sys.stderr)

    # 3. Extraer JavaScript
    script_end = content.find("    </script>")
    if script_end == -1:
        script_end = content.find("</script>")
    js = content[script_start + len("    <script>"):script_end]

    j_cfg = 0
    p_ws = js.find("const WSControl = {")
    j_ws = js.rfind("/* =", 0, p_ws) if p_ws != -1 else -1

    j_cal = js.find("const Calibration = {")

    p_tlm = js.find("const FSMDisplay = {")
    j_tlm = js.rfind("/* =", 0, p_tlm) if p_tlm != -1 else -1

    p_rm = js.find("const RouteManager = {")
    j_rm = js.rfind("/* =", 0, p_rm) if p_rm != -1 else -1

    p_tab = js.find("const TabManager = {")
    j_tab = js.rfind("/* =", 0, p_tab) if p_tab != -1 else -1

    p_touch = js.find("const Touch = {")
    j_touch = js.rfind("/* =", 0, p_touch) if p_touch != -1 else -1

    p_main = js.find("function init() {")
    j_main = js.rfind("/* =", 0, p_main) if p_main != -1 else -1

    if all(i != -1 for i in [j_ws, j_cal, j_tlm, j_rm, j_tab, j_touch, j_main]):
        config_js = js[j_cfg:j_ws].strip() + "\n"
        ws_js = js[j_ws:j_cal].strip() + "\n"
        cal_js = js[j_cal:j_tlm].strip() + "\n"
        tlm_js = js[j_tlm:j_rm].strip() + "\n"
        rm_js = js[j_rm:j_tab].strip() + "\n"
        tab_js = js[j_tab:j_touch].strip() + "\n"
        touch_js = js[j_touch:j_main].strip() + "\n"
        main_js = js[j_main:].strip() + "\n"

        (JS_DIR / "config.js").write_text(config_js, encoding="utf-8")
        (JS_DIR / "ws_control.js").write_text(ws_js, encoding="utf-8")
        (JS_DIR / "calibration.js").write_text(cal_js, encoding="utf-8")
        (JS_DIR / "telemetry.js").write_text(tlm_js, encoding="utf-8")
        (JS_DIR / "route_manager.js").write_text(rm_js, encoding="utf-8")
        (JS_DIR / "tab_manager.js").write_text(tab_js, encoding="utf-8")
        (JS_DIR / "touch.js").write_text(touch_js, encoding="utf-8")
        (JS_DIR / "main.js").write_text(main_js, encoding="utf-8")
        print("[OK] JavaScript modularizado en 8 modulos bajo js/")
    else:
        print("Advertencia: No se encontraron todos los delimitadores JS esperados.", file=sys.stderr)

    # 4. Crear template.html base
    template = f"""{content[:style_start].rstrip()}
    <style>
/* <!-- INCLUDE_CSS --> */
    </style>
</head>
<body>
<!-- <!-- INCLUDE_CARDS --> -->
    <script>
/* <!-- INCLUDE_JS --> */
    </script>
</body>
</html>
"""
    TEMPLATE_FILE.write_text(template, encoding="utf-8")
    print(f"[OK] Plantilla base guardada en {TEMPLATE_FILE}")


def compile_hmi() -> str:
    """Compila las fuentes modulares (template/index, cards, css, js) en una cadena HTML."""
    source_file = TEMPLATE_FILE if TEMPLATE_FILE.exists() else INDEX_FILE
    if not source_file.exists():
        raise FileNotFoundError(f"No se encontro archivo base ({TEMPLATE_FILE} o {INDEX_FILE}).")

    template = source_file.read_text(encoding="utf-8")

    # Ensamblar CSS
    css_parts: list[str] = []
    for css_name in CSS_ORDER:
        css_path = CSS_DIR / css_name
        if css_path.exists():
            css_parts.append(css_path.read_text(encoding="utf-8").strip())
        else:
            print(f"Aviso: {css_path} no encontrado, omitiendo.", file=sys.stderr)
    assembled_css = "\n\n".join(css_parts)

    # Ensamblar Cards / HTML Body
    cards_parts: list[str] = []
    for card_name in CARDS_ORDER:
        card_path = CARDS_DIR / card_name
        if card_path.exists():
            cards_parts.append(card_path.read_text(encoding="utf-8").strip())
        else:
            print(f"Aviso: {card_path} no encontrado, omitiendo.", file=sys.stderr)
    assembled_cards = "\n\n    ".join(cards_parts)

    # Ensamblar JS
    js_parts: list[str] = []
    for js_name in JS_ORDER:
        js_path = JS_DIR / js_name
        if js_path.exists():
            js_parts.append(js_path.read_text(encoding="utf-8").strip())
        else:
            print(f"Aviso: {js_path} no encontrado, omitiendo.", file=sys.stderr)
    assembled_js = "\n\n".join(js_parts)

    # Inyectar en template
    output = template
    output = output.replace("/* <!-- INCLUDE_CSS --> */", assembled_css)
    output = output.replace("<!-- <!-- INCLUDE_CARDS --> -->", assembled_cards)
    output = output.replace("/* <!-- INCLUDE_JS --> */", assembled_js)

    return output


def main() -> None:
    parser = argparse.ArgumentParser(description="Compilador/modularizador del HMI canonico del robot.")
    parser.add_argument("--split", action="store_true", help="Descompone index.html en cards/, css/ y js/")
    parser.add_argument("--check", action="store_true", help="Verifica si index.html coincide con las fuentes modulares")
    parser.add_argument("--watch", action="store_true", help="Vigila cambios en cards/, css/ y js/ para recompilar")
    args = parser.parse_args()

    if args.split:
        print("Descomponiendo index.html en componentes modulares...")
        split_monolith()
        print("Validando recompilacion de prueba...")
        compiled = compile_hmi()
        print(f"Compilacion exitosa ({len(compiled)} bytes generados).")
        return

    if args.check:
        if not INDEX_FILE.exists():
            print("ERROR: index.html esqueleto ligero no existe.", file=sys.stderr)
            sys.exit(1)
        skeleton = INDEX_FILE.read_text(encoding="utf-8")
        if "/* <!-- INCLUDE_CSS --> */" not in skeleton or \
           "<!-- <!-- INCLUDE_CARDS --> -->" not in skeleton or \
           "/* <!-- INCLUDE_JS --> */" not in skeleton:
            print("ERROR: index.html no contiene los marcadores de ensamblado modulares.", file=sys.stderr)
            sys.exit(1)

        missing = []
        for c in CSS_ORDER:
            if not (CSS_DIR / c).exists(): missing.append(f"css/{c}")
        for cd in CARDS_ORDER:
            if not (CARDS_DIR / cd).exists(): missing.append(f"cards/{cd}")
        for j in JS_ORDER:
            if not (JS_DIR / j).exists(): missing.append(f"js/{j}")
        if missing:
            print(f"ERROR: Faltan archivos modulares requeridos: {', '.join(missing)}", file=sys.stderr)
            sys.exit(1)

        try:
            compiled = compile_hmi()
        except Exception as e:
            print(f"ERROR: Fallo al compilar fuentes modulares: {e}", file=sys.stderr)
            sys.exit(1)

        if len(compiled) < 10000:
            print(f"ERROR: Compilacion sospechosamente pequena ({len(compiled)} bytes).", file=sys.stderr)
            sys.exit(1)

        DIST_DIR.mkdir(parents=True, exist_ok=True)
        DIST_FILE.write_text(compiled, encoding="utf-8")
        print("OK: Fuentes modulares del HMI ensamblan limpiamente y dist/index.html actualizado.")
        sys.exit(0)

    if args.watch:
        import time
        print("Modo watch activo. Presiona Ctrl+C para salir.")
        last_mtime = 0.0
        while True:
            try:
                all_files = list(CARDS_DIR.glob("*")) + list(CSS_DIR.glob("*")) + list(JS_DIR.glob("*")) + [TEMPLATE_FILE]
                current_mtime = max(f.stat().st_mtime for f in all_files if f.exists())
                if current_mtime > last_mtime:
                    last_mtime = current_mtime
                    compiled = compile_hmi()
                    DIST_DIR.mkdir(parents=True, exist_ok=True)
                    DIST_FILE.write_text(compiled, encoding="utf-8")
                    print(f"[{time.strftime('%H:%M:%S')}] HMI recompilado exitosamente ({len(compiled)} bytes en dist/index.html).")
                time.sleep(0.5)
            except KeyboardInterrupt:
                break
        return

    # Compilacion por defecto
    compiled = compile_hmi()
    DIST_DIR.mkdir(parents=True, exist_ok=True)
    DIST_FILE.write_text(compiled, encoding="utf-8")
    print(f"[OK] dist/index.html generado exitosamente ({len(compiled)} bytes) desde fuentes modulares.")


if __name__ == "__main__":
    main()

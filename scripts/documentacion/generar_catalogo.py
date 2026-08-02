"""Genera el catálogo trazable de funciones activas y UML por carpeta.

Usa Universal Ctags para los lenguajes soportados y completa el JavaScript
embebido de la HMI con un analizador conservador basado en patrones. El
resultado es reproducible y excluye deliberadamente código histórico,
dependencias, compilados y pruebas.
"""

from __future__ import annotations

import hashlib
import csv
import json
import os
import re
import subprocess
from collections import Counter, defaultdict
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
OUTPUT_JSON = ROOT / "docs" / "catalogo_funciones.json"
OUTPUT_CSV = ROOT / "docs" / "catalogo_funciones.csv"
OUTPUT_UML = ROOT / "docs" / "uml"
OUTPUT_MERMAID = OUTPUT_UML / "mermaid"
OUTPUT_PLANTUML = OUTPUT_UML / "plantuml"

EXCLUDED_PARTS = {
    "archive", "build", "dist", "node_modules", "vendor", ".pio",
    "__pycache__", ".venv", ".test-venv", ".build-venv", "tests",
}


def active_files() -> list[Path]:
    roots = [
        ROOT / "src",
        ROOT / "include",
        ROOT / "desktop_app" / "robot_app",
        ROOT / "desktop_app" / "frontend" / "src",
        ROOT / "android_app" / "app" / "src" / "main",
        ROOT / "scripts",
    ]
    allowed = {".cpp", ".h", ".py", ".ts", ".kt", ".js", ".ps1", ".bat", ".html"}
    files: list[Path] = [
        ROOT / "desktop_app" / "app.py",
        ROOT / "consultar_db.py",
        ROOT / "SistemaDeDescomposicionDePuntosAVectoresYARutaOrtogonal.html",
    ]
    for base in roots:
        for path in base.rglob("*"):
            if not path.is_file() or path.suffix.lower() not in allowed:
                continue
            relative_parts = set(path.relative_to(ROOT).parts)
            if relative_parts & EXCLUDED_PARTS or path.name.endswith(".disabled"):
                continue
            if "static" in relative_parts or ("hmi" in relative_parts and "vendor" in relative_parts):
                continue
            files.append(path)
    return sorted(set(files))


def subsystem(path: Path) -> str:
    rel = path.relative_to(ROOT).as_posix()
    if rel.startswith("src/"):
        return "firmware/src"
    if rel.startswith("include/"):
        return "firmware/include"
    if rel == "desktop_app/robot_app/hmi/index.html":
        return "hmi/canonica"
    if rel.startswith("desktop_app/robot_app/") or rel == "desktop_app/app.py":
        return "backend/python"
    if rel.startswith("desktop_app/frontend/"):
        return "frontend/typescript"
    if rel.endswith("mobile_entry.py"):
        return "android/python"
    if rel.startswith("android_app/"):
        return "android/kotlin"
    return "herramientas/scripts"


def language(path: Path) -> str:
    return {
        ".cpp": "C++", ".h": "C++", ".py": "Python", ".ts": "TypeScript",
        ".kt": "Kotlin", ".js": "JavaScript", ".ps1": "PowerShell",
        ".bat": "Batch", ".html": "JavaScript embebido",
    }.get(path.suffix.lower(), path.suffix.lstrip("."))


def run_ctags(files: list[Path]) -> list[dict[str, Any]]:
    supported = [path for path in files if path.suffix.lower() != ".html"]
    command = [
        "ctags", "--output-format=json", "--fields=+neKSt", "--extras=-F",
        "--sort=no", "-o", "-", *[str(path.relative_to(ROOT)) for path in supported],
    ]
    result = subprocess.run(command, cwd=ROOT, check=True, capture_output=True, text=True, encoding="utf-8")
    tags: list[dict[str, Any]] = []
    for line in result.stdout.splitlines():
        try:
            item = json.loads(line)
        except json.JSONDecodeError:
            continue
        if item.get("_type") == "tag":
            tags.append(item)
    return tags


def embedded_html_functions(path: Path) -> list[dict[str, Any]]:
    text = path.read_text(encoding="utf-8")
    lines = text.splitlines()
    patterns = [
        re.compile(r"^\s*(?:async\s+)?function\s+([A-Za-z_$][\w$]*)\s*\(([^)]*)\)"),
        re.compile(r"^\s*(?:async\s+)?([A-Za-z_$][\w$]*)\s*\(([^)]*)\)\s*\{"),
        re.compile(r"^\s*(?:const|let|var)\s+([A-Za-z_$][\w$]*)\s*=\s*(?:async\s*)?\(([^)]*)\)\s*=>"),
    ]
    excluded = {"if", "for", "while", "switch", "catch", "confirm"}

    def find_end(start: int) -> int:
        depth = 0
        opened = False
        for index in range(start - 1, len(lines)):
            clean = re.sub(r"(['\"]).*?\1", "", lines[index])
            depth += clean.count("{")
            if clean.count("{"):
                opened = True
            depth -= clean.count("}")
            if opened and depth <= 0:
                return index + 1
        return start

    result: list[dict[str, Any]] = []
    for number, line in enumerate(lines, start=1):
        for pattern in patterns:
            match = pattern.search(line)
            if not match or match.group(1) in excluded:
                continue
            result.append({
                "_type": "tag",
                "name": match.group(1),
                "path": path.relative_to(ROOT).as_posix(),
                "line": number,
                "end": find_end(number),
                "kind": "method" if pattern is patterns[1] else "function",
                "signature": f"({match.group(2).strip()})",
                "scope": "HMI" if "hmi" in path.parts else path.stem,
                "scopeKind": "object",
            })
            break
    unique: dict[tuple[str, int], dict[str, Any]] = {}
    for item in result:
        unique[(item["name"], item["line"])] = item
    return list(unique.values())


def scripted_functions(files: list[Path]) -> list[dict[str, Any]]:
    """Completa lenguajes de automatización que Ctags no siempre reconoce."""
    result: list[dict[str, Any]] = []
    for path in files:
        suffix = path.suffix.lower()
        if suffix not in {".ps1", ".bat"}:
            continue
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
        for number, source in enumerate(lines, start=1):
            if suffix == ".ps1":
                match = re.match(r"^\s*function\s+([A-Za-z_][\w-]*)\s*(?:\{|$)", source, re.I)
            else:
                match = re.match(r"^:([A-Za-z_][\w.-]*)\s*$", source)
                if match and match.group(1).lower() in {"eof"}:
                    match = None
            if not match:
                continue
            result.append({
                "_type": "tag",
                "name": match.group(1),
                "path": path.relative_to(ROOT).as_posix(),
                "line": number,
                "end": number,
                "kind": "function",
                "signature": "()",
                "scope": path.stem,
                "scopeKind": "script",
            })
    return result


def visibility(item: dict[str, Any], source_line: str) -> str:
    name = str(item.get("name", ""))
    if name.startswith("_") or "static " in source_line or "private " in source_line:
        return "interna"
    return "pública"


def normalize_tags(tags: list[dict[str, Any]]) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    callable_kinds = {"function", "method", "member", "prototype"}
    type_kinds = {"class", "struct", "enum", "interface"}
    functions: list[dict[str, Any]] = []
    types: list[dict[str, Any]] = []
    for raw in tags:
        kind = str(raw.get("kind", ""))
        if kind not in callable_kinds | type_kinds:
            continue
        rel = Path(str(raw["path"]).replace("/", "\\"))
        absolute = ROOT / rel
        if not absolute.exists():
            continue
        line = int(raw.get("line", 1))
        end = int(raw.get("end", line))
        source_lines = absolute.read_text(encoding="utf-8", errors="replace").splitlines()
        source_line = source_lines[line - 1] if line <= len(source_lines) else ""
        if language(absolute) == "Kotlin":
            declared = re.search(r"\bfun\s+([A-Za-z_][\w]*)", source_line)
            if not declared or declared.group(1) != str(raw.get("name", "")):
                continue
        entry = {
            "name": str(raw.get("name", "")),
            "qualified_name": ".".join(filter(None, [str(raw.get("scope", "")), str(raw.get("name", ""))])),
            "path": absolute.relative_to(ROOT).as_posix(),
            "folder": subsystem(absolute),
            "language": language(absolute),
            "kind": kind,
            "scope": str(raw.get("scope", "")),
            "signature": str(raw.get("signature", "()")),
            "line": line,
            "end": max(line, end),
            "lines": max(1, end - line + 1),
            "visibility": visibility(raw, source_line),
            "calls": [],
            "callers": [],
            "complexity": 1,
            "interaction": "síncrona",
            "owner": "",
            "platform": "",
            "risk": "Bajo",
            "status": "interno",
            "shared_state": [],
        }
        (functions if kind in callable_kinds else types).append(entry)
    functions.sort(key=lambda item: (item["folder"], item["path"], item["line"], item["name"]))
    types.sort(key=lambda item: (item["folder"], item["path"], item["line"], item["name"]))
    return functions, types


def _function_body(item: dict[str, Any]) -> str:
    path = ROOT / item["path"]
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    return "\n".join(lines[item["line"] - 1:item["end"]])


def _owner(folder: str) -> str:
    if folder.startswith("firmware/"):
        return "ESP32 / tiempo real"
    if folder in {"backend/python", "android/python"}:
        return "Python / misión e historial"
    if folder in {"hmi/canonica", "frontend/typescript"}:
        return "HMI / operador"
    if folder == "android/kotlin":
        return "Android / ciclo de vida"
    return "Mantenimiento / herramientas"


def _platform(folder: str) -> str:
    if folder.startswith("firmware/"):
        return "ESP32-S3"
    if folder.startswith("android/"):
        return "Android"
    if folder in {"backend/python", "hmi/canonica"}:
        return "Windows/Android"
    if folder == "frontend/typescript":
        return "Windows (validación)"
    return "Desarrollo"


def _complexity(body: str) -> int:
    clean = re.sub(r"//.*?$|/\*.*?\*/|#.*?$", "", body, flags=re.M | re.S)
    decisions = re.findall(r"\b(?:if|elif|for|while|case|catch|except)\b|&&|\|\||\?", clean)
    return 1 + len(decisions)


def _interaction(body: str) -> str:
    if re.search(r"\b(?:xQueueSend|xQueueReceive|queue|colaComandos|colaEventos)\b", body, re.I):
        return "cola/evento"
    if re.search(r"\b(?:async|await|thread|websocket|eventsource|SSE)\b", body, re.I):
        return "asíncrona"
    if re.search(r"\b(?:callback|listener|emitirEvento|encolarEvento)\b", body, re.I):
        return "evento"
    return "síncrona"


def _shared_state(body: str) -> list[str]:
    candidates = {
        "estado": r"\b(?:estado|g_estado|estadoRobot)\b",
        "misión activa": r"\b(?:_active_mission|active_mission|misionActiva)\b",
        "comando activo": r"\b(?:_active_command|comandoActivo|cmdActivo)\b",
        "cola de comandos": r"\bcolaComandos\b",
        "cola de eventos": r"\bcolaEventos\b",
        "telemetría": r"\b(?:telemetry|telemetria|g_telemetria)\b",
        "sesión": r"\b(?:session|sesion|g_session)\b",
        "parada/cierre": r"\b(?:shutdown|closing|estop|eStop)\b",
    }
    return [label for label, pattern in candidates.items() if re.search(pattern, body, re.I)]


def infer_calls(functions: list[dict[str, Any]], routes: list[dict[str, Any]]) -> None:
    known = {item["name"] for item in functions if len(item["name"]) > 2}
    by_name: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for item in functions:
        by_name[item["name"]].append(item)
    for item in functions:
        body = _function_body(item)
        found = {
            name for name in re.findall(r"\b([A-Za-z_$][\w$]*)\s*\(", body)
            if name in known and name != item["name"]
        }
        item["calls"] = sorted(found)
        item["complexity"] = _complexity(body)
        item["interaction"] = _interaction(body)
        item["owner"] = _owner(item["folder"])
        item["platform"] = _platform(item["folder"])
        item["shared_state"] = _shared_state(body)
        if re.search(r"\b(?:ledcWrite|aplicarVelocidades|activarMotores|PWM|DRV8833)\b", body):
            item["risk"] = "Alto"
        elif re.search(r"\b(?:stop|estop|mission|mision|session|sesion|websocket|sqlite)\b", body, re.I):
            item["risk"] = "Medio"

    for source in functions:
        for called in source["calls"]:
            for target in by_name.get(called, []):
                target["callers"].append(source["name"])
    route_functions = {route["function"] for route in routes}
    entry_names = {"main", "setup", "loop", "onCreate", "onDestroy", "doGet", "doPost"}
    for item in functions:
        item["callers"] = sorted(set(item["callers"]))
        if item["name"] in route_functions or item["name"] in entry_names:
            item["status"] = "entrada/framework"
        elif not item["callers"]:
            item["status"] = "sin llamada interna detectada"


def dependency_cycles(functions: list[dict[str, Any]]) -> list[list[str]]:
    """Devuelve componentes fuertemente conexas del grafo nominal de llamadas."""
    graph = {item["name"]: set(item["calls"]) for item in functions}
    index = 0
    stack: list[str] = []
    on_stack: set[str] = set()
    indices: dict[str, int] = {}
    low: dict[str, int] = {}
    cycles: list[list[str]] = []

    def visit(node: str) -> None:
        nonlocal index
        indices[node] = low[node] = index
        index += 1
        stack.append(node)
        on_stack.add(node)
        for target in graph.get(node, set()):
            if target not in graph:
                continue
            if target not in indices:
                visit(target)
                low[node] = min(low[node], low[target])
            elif target in on_stack:
                low[node] = min(low[node], indices[target])
        if low[node] == indices[node]:
            component: list[str] = []
            while stack:
                current = stack.pop()
                on_stack.remove(current)
                component.append(current)
                if current == node:
                    break
            if len(component) > 1:
                cycles.append(sorted(component))

    for node in sorted(graph):
        if node not in indices:
            visit(node)
    return sorted(cycles, key=lambda group: (-len(group), group))


def extract_routes(files: list[Path]) -> list[dict[str, Any]]:
    routes: list[dict[str, Any]] = []
    route_pattern = re.compile(r'^@web\.(get|post|put|delete|route)\("([^"]+)"(?:,\s*methods=\[([^]]+)\])?')
    for path in files:
        if path.suffix != ".py":
            continue
        pending: dict[str, Any] | None = None
        for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
            match = route_pattern.match(line.strip())
            if match:
                pending = {"decorator": match.group(1), "path": match.group(2), "methods": match.group(3), "line": number}
            elif pending:
                function = re.match(r"def\s+(\w+)", line.strip())
                if function:
                    methods = pending["methods"] or pending["decorator"].upper()
                    routes.append({**pending, "methods": methods.replace('"', "").replace("'", ""),
                                   "function": function.group(1), "file": path.relative_to(ROOT).as_posix()})
                    pending = None
    return routes


def node_id(item: dict[str, Any]) -> str:
    raw = f'{item["path"]}:{item["line"]}:{item["name"]}'.encode()
    return "n" + hashlib.sha1(raw).hexdigest()[:10]


def mermaid_for_folder(folder: str, functions: list[dict[str, Any]]) -> str:
    selected = [item for item in functions if item["folder"] == folder]
    ids_by_name: dict[str, list[str]] = defaultdict(list)
    for item in selected:
        ids_by_name[item["name"]].append(node_id(item))
    output = ["flowchart LR"]
    by_file: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for item in selected:
        by_file[item["path"]].append(item)
    for index, (path, entries) in enumerate(sorted(by_file.items())):
        output.append(f'  subgraph f{index}["{Path(path).name}"]')
        for item in entries:
            label = f'{item["name"]}{item["signature"]}'.replace('"', "'")
            output.append(f'    {node_id(item)}["{label}"]')
        output.append("  end")
    edges: set[tuple[str, str, str]] = set()
    for item in selected:
        source = node_id(item)
        for called in item["calls"]:
            for target in ids_by_name.get(called, [])[:1]:
                if source != target:
                    edges.add((source, target, item["interaction"]))
    for source, target, interaction in sorted(edges):
        arrow = "-.->" if interaction in {"evento", "cola/evento"} else "-->"
        output.append(f"  {source} {arrow} {target}")
    output.extend([
        "  classDef alto fill:#5b1f2a,stroke:#ff7a7a,color:#fff",
        "  classDef medio fill:#4a3717,stroke:#ffca67,color:#fff",
        "  classDef bajo fill:#123b3a,stroke:#39e6aa,color:#fff",
    ])
    for item in selected:
        output.append(f'  class {node_id(item)} {item["risk"].lower()}')
    if not selected:
        output.append('  empty["Sin funciones detectadas"]')
    return "\n".join(output)


def plantuml_for_folder(folder: str, functions: list[dict[str, Any]]) -> str:
    selected = [item for item in functions if item["folder"] == folder]
    by_name: dict[str, str] = {}
    lines = [
        "@startuml", "!pragma layout smetana", "left to right direction",
        "skinparam backgroundColor #07111F", "skinparam componentStyle rectangle",
        "skinparam componentBackgroundColor #102238",
        "skinparam componentBorderColor #39E6AA",
        "skinparam componentFontColor white",
        "skinparam ArrowColor #5BD8FF", f'title {folder} — grafo funcional',
    ]
    for item in selected:
        alias = node_id(item)
        label = f'{item["name"]}\\nCC={item["complexity"]} · {item["risk"]}'.replace('"', "'")
        lines.append(f'component "{label}" as {alias}')
        by_name.setdefault(item["name"], alias)
    for item in selected:
        for called in item["calls"]:
            target = by_name.get(called)
            if not target or target == node_id(item):
                continue
            arrow = "..>" if item["interaction"] != "síncrona" else "-->"
            lines.append(f'{node_id(item)} {arrow} {target} : {item["interaction"]}')
    lines.extend(["legend", "|= Flecha |= Interacción |", "| --> | síncrona |", "| ..> | asíncrona, evento o cola |", "endlegend", "@enduml"])
    return "\n".join(lines)


def write_markdown(functions: list[dict[str, Any]], types: list[dict[str, Any]]) -> None:
    OUTPUT_UML.mkdir(parents=True, exist_ok=True)
    OUTPUT_MERMAID.mkdir(parents=True, exist_ok=True)
    OUTPUT_PLANTUML.mkdir(parents=True, exist_ok=True)
    folders = sorted({item["folder"] for item in functions})
    index_lines = [
        "# Catálogo UML por carpeta activa",
        "",
        "Generado con Universal Ctags y análisis estático de llamadas. Los enlaces apuntan al código operativo.",
        "",
    ]
    for folder in folders:
        slug = folder.replace("/", "_")
        selected = [item for item in functions if item["folder"] == folder]
        selected_types = [item for item in types if item["folder"] == folder]
        index_lines.append(f'- [{folder}]({slug}.md) — {len(selected)} funciones, {len(selected_types)} tipos; [Mermaid](mermaid/{slug}.mmd) · [PlantUML](plantuml/{slug}.puml)')
        mermaid_source = mermaid_for_folder(folder, functions)
        plantuml_source = plantuml_for_folder(folder, functions)
        (OUTPUT_MERMAID / f"{slug}.mmd").write_text(mermaid_source + "\n", encoding="utf-8")
        (OUTPUT_PLANTUML / f"{slug}.puml").write_text(plantuml_source + "\n", encoding="utf-8")
        lines = [
            f"# UML funcional: `{folder}`", "",
            f"Funciones detectadas: **{len(selected)}**. Tipos detectados: **{len(selected_types)}**.", "",
            "## Grafo de llamadas", "", "```mermaid", mermaid_source, "```", "",
            f"Fuentes: [Mermaid](mermaid/{slug}.mmd) · [PlantUML](plantuml/{slug}.puml). Las flechas continuas son llamadas síncronas; las discontinuas representan asincronía, eventos o colas. El color del nodo indica riesgo estático.", "",
            "## Inventario", "",
            "| Función | Archivo | CC | Propietario | Riesgo/estado | Entra desde | Sale hacia | Estado compartido |", "|---|---|---:|---|---|---|---|---|",
        ]
        for item in selected:
            calls = ", ".join(f"`{name}`" for name in item["calls"]) or "—"
            callers = ", ".join(f"`{name}`" for name in item["callers"]) or "—"
            shared = ", ".join(item["shared_state"]) or "—"
            lines.append(
                f'| `{item["qualified_name"]}` | [`{item["path"]}`](../../{item["path"]}#L{item["line"]}) '
                f'| {item["complexity"]} | {item["owner"]} | {item["risk"]}; {item["status"]}; {item["interaction"]} | {callers} | {calls} | {shared} |'
            )
        (OUTPUT_UML / f"{slug}.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    index_lines.extend([
        "",
        "## Vistas transversales",
        "",
        "- [PCNT/MPU/pose, memoria compartida, JSON v3 y cadena PWM](vistas_especializadas.md)",
        "- [Atlas general de arquitectura](../../DIAGRAMA_SISTEMA_GENERAL.md)",
        "",
        "El análisis estático detecta un único componente fuertemente conexo nominal: ",
        "`rPts ↔ rmPt`, dentro del descomponedor HTML independiente. El catálogo conserva ",
        "el ciclo en `dependency_cycles`; no lo clasifica automáticamente como fallo.",
    ])
    (OUTPUT_UML / "README.md").write_text("\n".join(index_lines) + "\n", encoding="utf-8")


def main() -> None:
    files = active_files()
    tags = run_ctags(files)
    for html in [path for path in files if path.suffix.lower() == ".html"]:
        tags.extend(embedded_html_functions(html))
    tags.extend(scripted_functions(files))
    functions, types = normalize_tags(tags)
    routes = extract_routes(files)
    infer_calls(functions, routes)
    cycles = dependency_cycles(functions)
    by_folder = Counter(item["folder"] for item in functions)
    by_language = Counter(item["language"] for item in functions)
    payload = {
        "generated_at": os.environ.get("CATALOG_GENERATED_AT", datetime.now(timezone.utc).isoformat()),
        "generator": "Universal Ctags 6.x + scripts/documentacion/generar_catalogo.py",
        "scope": "Código operativo; excluye archive, pruebas, vendor, build, dist y .disabled",
        "files": [path.relative_to(ROOT).as_posix() for path in files],
        "stats": {
            "files": len(files), "functions": len(functions), "types": len(types), "routes": len(routes),
            "dependency_cycles": len(cycles),
            "by_folder": dict(sorted(by_folder.items())), "by_language": dict(sorted(by_language.items())),
        },
        "functions": functions,
        "types": types,
        "http_routes": routes,
        "dependency_cycles": cycles,
    }
    OUTPUT_JSON.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    with OUTPUT_CSV.open("w", encoding="utf-8-sig", newline="") as handle:
        fieldnames = ["qualified_name", "path", "line", "folder", "platform", "owner", "risk", "status", "interaction", "complexity", "callers", "calls", "shared_state"]
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        for item in functions:
            writer.writerow({key: "; ".join(item[key]) if isinstance(item[key], list) else item[key] for key in fieldnames})
    write_markdown(functions, types)
    print(json.dumps(payload["stats"], ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()

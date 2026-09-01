"""Valida destinos locales de la documentación Markdown activa."""

from __future__ import annotations

import re
import sys
from pathlib import Path
from urllib.parse import unquote


ROOT = Path(__file__).resolve().parents[2]
LINK_RE = re.compile(r"!?\[[^\]]*\]\(([^)]+)\)")
LEGACY_DOCS = ROOT / "docs_markdowns"
LEGACY_SITE = ROOT / "documentacion_completa_localServer"


def markdown_files() -> list[Path]:
    files = [ROOT / "README.md", ROOT / "DIAGRAMA_SISTEMA_GENERAL.md", ROOT / "scripts" / "README.md"]
    import os
    for folder in (ROOT / "docs_markdowns", ROOT / "evidencia", ROOT / "documentacionCompleta"):
        for root_dir, dirs, filenames in os.walk(folder):
            if "node_modules" in dirs:
                dirs.remove("node_modules")
            if "dist" in dirs:
                dirs.remove("dist")
            for filename in filenames:
                if filename.endswith(".md"):
                    files.append(Path(root_dir) / filename)
    return sorted(set(files))


def normalize_target(raw: str) -> str | None:
    target = raw.strip()
    if target.startswith("<") and target.endswith(">"):
        target = target[1:-1]
    if target.startswith(("http://", "https://", "mailto:", "data:", "#")):
        return None
    target = unquote(target.split("#", 1)[0])
    return target or None


def candidates_for(document: Path, target: str) -> list[Path]:
    """Acepta enlaces históricos mientras las fuentes viven en docs_markdowns."""
    local_target = (document.parent / target).resolve()
    candidates = [local_target]
    try:
        local_parts = local_target.relative_to(ROOT).parts
    except ValueError:
        local_parts = ()
    if local_parts and local_parts[0] == "docs":
        candidates.append(LEGACY_DOCS.joinpath(*local_parts[1:]).resolve())
    if local_parts and local_parts[0] == "documentacionCompleta":
        candidates.append(LEGACY_SITE.joinpath(*local_parts[1:]).resolve())
    root_target = (ROOT / target).resolve()
    parts = Path(target).parts
    if parts and parts[0] == "docs":
        candidates.append(LEGACY_DOCS.joinpath(*parts[1:]).resolve())
    if parts and parts[0] == "documentacionCompleta":
        candidates.append(LEGACY_SITE.joinpath(*parts[1:]).resolve())
    if root_target not in candidates:
        candidates.append(root_target)
    return candidates


def main() -> int:
    missing: list[tuple[Path, int, str]] = []
    checked = 0
    for document in markdown_files():
        if not document.exists():
            missing.append((document, 0, "documento de entrada"))
            continue
        for line_number, line in enumerate(document.read_text(encoding="utf-8").splitlines(), start=1):
            for match in LINK_RE.finditer(line):
                target = normalize_target(match.group(1))
                if target is None:
                    continue
                # Los enlaces Markdown admiten un título opcional después del destino.
                if ' "' in target:
                    target = target.split(' "', 1)[0]
                checked += 1
                if not any(candidate.exists() for candidate in candidates_for(document, target)):
                    missing.append((document.relative_to(ROOT), line_number, target))

    if missing:
        print(f"Enlaces locales inválidos ({len(missing)}):")
        for document, line_number, target in missing:
            print(f"- {document}:{line_number}: {target}")
        return 1

    print(f"Enlaces Markdown válidos: {checked} destinos locales en {len(markdown_files())} documentos.")
    return 0


if __name__ == "__main__":
    sys.exit(main())

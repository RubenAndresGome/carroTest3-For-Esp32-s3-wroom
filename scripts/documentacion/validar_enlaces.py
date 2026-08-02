"""Valida destinos locales de la documentación Markdown activa."""

from __future__ import annotations

import re
import sys
from pathlib import Path
from urllib.parse import unquote


ROOT = Path(__file__).resolve().parents[2]
LINK_RE = re.compile(r"!?\[[^\]]*\]\(([^)]+)\)")


def markdown_files() -> list[Path]:
    files = [ROOT / "README.md", ROOT / "DIAGRAMA_SISTEMA_GENERAL.md", ROOT / "scripts" / "README.md"]
    for folder in (ROOT / "docs", ROOT / "evidencia", ROOT / "documentacionCompleta"):
        files.extend(
            path
            for path in folder.rglob("*.md")
            if "node_modules" not in path.parts and "dist" not in path.parts
        )
    return sorted(set(files))


def normalize_target(raw: str) -> str | None:
    target = raw.strip()
    if target.startswith("<") and target.endswith(">"):
        target = target[1:-1]
    if target.startswith(("http://", "https://", "mailto:", "data:", "#")):
        return None
    target = unquote(target.split("#", 1)[0])
    return target or None


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
                candidate = (document.parent / target).resolve()
                checked += 1
                if not candidate.exists():
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

"""Renderiza los PlantUML del catálogo como SVG y PNG reproducibles.

La primera ejecución descarga una versión fijada de PlantUML desde Maven
Central a `.codex_tools/`; el JAR no se confirma en Git. Los diagramas fuente y
las exportaciones sí forman parte de la documentación versionada.
"""

from __future__ import annotations

import hashlib
import subprocess
import urllib.request
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
VERSION = "1.2025.4"
TOOL_DIR = ROOT / ".codex_tools"
JAR = TOOL_DIR / f"plantuml-{VERSION}.jar"
BASE_URL = "https://repo1.maven.org/maven2/net/sourceforge/plantuml/plantuml"
SOURCE_DIR = ROOT / "docs" / "uml" / "plantuml"
OUTPUT_DIR = ROOT / "docs" / "uml" / "exportados"


def download_if_needed() -> None:
    if JAR.exists():
        return
    TOOL_DIR.mkdir(parents=True, exist_ok=True)
    url = f"{BASE_URL}/{VERSION}/plantuml-{VERSION}.jar"
    checksum_url = f"{url}.sha256"
    with urllib.request.urlopen(checksum_url, timeout=60) as response:
        expected = response.read().decode("ascii").strip().split()[0].lower()
    temporary = JAR.with_suffix(".jar.part")
    urllib.request.urlretrieve(url, temporary)
    actual = hashlib.sha256(temporary.read_bytes()).hexdigest()
    if actual != expected:
        temporary.unlink(missing_ok=True)
        raise RuntimeError(f"SHA-256 inválido para PlantUML: {actual} != {expected}")
    temporary.replace(JAR)


def render(format_name: str) -> None:
    sources = sorted(SOURCE_DIR.glob("*.puml"))
    if not sources:
        raise RuntimeError("No existen fuentes PlantUML; ejecuta generar_catalogo.py primero")
    command = [
        "java", "-Djava.awt.headless=true", "-jar", str(JAR),
        "-charset", "UTF-8", f"-t{format_name}", "-o", str(OUTPUT_DIR),
        *[str(path) for path in sources],
    ]
    subprocess.run(command, cwd=ROOT, check=True)


def main() -> None:
    download_if_needed()
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    render("svg")
    render("png")
    exported = sorted(path for path in OUTPUT_DIR.iterdir() if path.suffix in {".svg", ".png"})
    print(f"PlantUML {VERSION}: {len(exported)} exportaciones en {OUTPUT_DIR.relative_to(ROOT)}")


if __name__ == "__main__":
    main()

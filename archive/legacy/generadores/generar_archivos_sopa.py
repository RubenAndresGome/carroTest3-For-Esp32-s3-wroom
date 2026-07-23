"""Regenera snapshots históricos monolíticos para consulta manual."""

from __future__ import annotations

import re
from pathlib import Path


RAIZ = Path(__file__).resolve().parents[3]
SALIDA = RAIZ / "archive" / "legacy" / "generados"
CABECERAS = [
    "Config.h", "Debug.h", "Estado.h", "Comandos.h", "Eventos.h",
    "PoseEstimator.h", "Sensores.h", "Motores.h", "Cinematica.h",
    "Seguridad.h", "Red.h",
]
MODULOS = [
    "Estado.cpp", "Eventos.cpp", "PoseEstimator.cpp", "Sensores.cpp",
    "Motores.cpp", "Cinematica.cpp", "Seguridad.cpp", "Red.cpp", "main.cpp",
]
ASSETS = [
    "Index.html", "IndexBeta.html", "chart.js",
    "chartjs-plugin-datalabels.js", "chartjs-plugin-zoom.js", "hammer.js",
]


def resolver_fuente(ruta: Path) -> Path | None:
    if ruta.exists():
        return ruta
    deshabilitada = ruta.with_name(ruta.name + ".disabled")
    return deshabilitada if deshabilitada.exists() else None


def generar_firmware() -> None:
    includes: set[str] = set()
    contenido: list[str] = []
    for relativa in [*(Path("include") / n for n in CABECERAS), *(Path("src") / n for n in MODULOS)]:
        fuente = resolver_fuente(RAIZ / relativa)
        if fuente is None:
            continue
        for linea in fuente.read_text(encoding="utf-8").splitlines(keepends=True):
            if linea.startswith("#pragma once"):
                continue
            if re.match(r'#include\s*<.*>', linea):
                includes.add(linea.strip())
            elif not re.match(r'#include\s*".*"', linea):
                contenido.append(linea)
    destino = SALIDA / "carro_test3_sopa.ino"
    destino.write_text(
        "// SNAPSHOT HISTORICO GENERADO; NO COMPILAR COMO FUENTE CANONICA\n"
        + "\n".join(sorted(includes)) + "\n\n" + "".join(contenido),
        encoding="utf-8",
    )


def generar_assets() -> None:
    origen = RAIZ / "archive" / "legacy" / "IUInWeb"
    bloques: list[str] = []
    for nombre in ASSETS:
        ruta = origen / nombre
        if ruta.exists():
            bloques.append(f"\n\n{'=' * 41}\nARCHIVO: {nombre}\n{'=' * 41}\n\n{ruta.read_text(encoding='utf-8')}")
    (SALIDA / "web_assets_sopa.txt").write_text("".join(bloques), encoding="utf-8")


if __name__ == "__main__":
    SALIDA.mkdir(parents=True, exist_ok=True)
    generar_firmware()
    generar_assets()
    print(f"Snapshots históricos actualizados en {SALIDA}")

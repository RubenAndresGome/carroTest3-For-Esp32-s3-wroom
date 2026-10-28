"""Verifica en vivo que GitHub Pages sirve los videos, fotogramas y páginas de evidencia.

Construye la lista de recursos leyendo el repositorio local (no hay URLs fijas),
pide cada uno con `Range: bytes=0-511` y detecta respuestas inválidas: estados
distintos de 200/206, punteros de Git LFS servidos en lugar del archivo, o
videos MP4 con un Content-Type que no sea `video/*`.
"""

from __future__ import annotations

import argparse
import sys
import urllib.error
import urllib.request
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
EVIDENCIA = ROOT / "evidencia"
BASE_DEFAULT = "https://rubenandresgome.github.io/carroTest3-For-Esp32-s3-wroom/"
VIDEO_EXCLUIDO = "ejes_invertidos_2026-09-08.mp4"
PAGINAS = ["incidentes/2026-07-30_reinicio_y_calibracion.md", "evidencia.html"]
TIMEOUT_S = 15
LFS_PREFIJO = b"version https://git-lfs"


def recursos(base: str, muestra: int) -> list[str]:
    """Devuelve las URLs a verificar a partir de los archivos locales de evidencia/."""
    base = base.rstrip("/") + "/"
    videos = sorted(p.name for p in (EVIDENCIA / "videos").glob("*.mp4") if p.name != VIDEO_EXCLUIDO)
    fotos = sorted(
        p.relative_to(EVIDENCIA / "fotogramas").as_posix()
        for p in (EVIDENCIA / "fotogramas").rglob("*")
        if p.suffix.lower() in (".jpg", ".webp")
    )
    if muestra > 0:
        fotos = fotos[:muestra]
    urls = [f"{base}videos/{nombre}" for nombre in videos]
    urls += [f"{base}fotogramas/{ruta}" for ruta in fotos]
    urls += [f"{base}{pagina}" for pagina in PAGINAS]
    return urls


def verificar(url: str) -> str | None:
    """Devuelve el motivo del fallo, o None si el recurso es correcto."""
    peticion = urllib.request.Request(url, headers={"Range": "bytes=0-511"})
    try:
        with urllib.request.urlopen(peticion, timeout=TIMEOUT_S) as respuesta:
            estado = respuesta.status
            tipo = respuesta.headers.get("Content-Type", "")
            cuerpo = respuesta.read(512)
    except urllib.error.HTTPError as error:
        return f"HTTP {error.code}"
    except (urllib.error.URLError, TimeoutError, OSError) as error:
        return f"error de red: {error}"

    if estado not in (200, 206):
        return f"estado HTTP inesperado {estado}"
    if cuerpo.startswith(LFS_PREFIJO):
        return "sirve un puntero de Git LFS en vez del archivo"
    if url.lower().endswith(".mp4") and not tipo.startswith("video/"):
        return f"Content-Type no es video/* ({tipo or 'vacío'})"
    return None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--base", default=BASE_DEFAULT, help="URL base del sitio publicado")
    parser.add_argument("--muestra", type=int, default=0, help="Verifica solo los primeros N fotogramas (0 = todos)")
    args = parser.parse_args()

    urls = recursos(args.base, args.muestra)
    fallos = 0
    for url in urls:
        motivo = verificar(url)
        if motivo:
            fallos += 1
            print(f"FALLO {url} -> {motivo}")

    print(f"OK {len(urls) - fallos} / Fallos {fallos}")
    return 1 if fallos else 0


if __name__ == "__main__":
    sys.exit(main())

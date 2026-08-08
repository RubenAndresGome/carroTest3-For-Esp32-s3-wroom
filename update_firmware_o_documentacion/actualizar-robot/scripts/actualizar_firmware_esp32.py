#!/usr/bin/env python3
"""Compila y carga el firmware del ESP32-S3 con una precondición de seguridad."""

from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import subprocess
import sys
from typing import Sequence


RAIZ = Path(__file__).resolve().parents[3]
ENTORNO_PREDETERMINADO = "esp32-s3-devkitc-1"


def ejecutar(comando: Sequence[str]) -> int:
    print("$", subprocess.list2cmdline(list(comando)))
    return subprocess.run(list(comando), cwd=RAIZ, check=False).returncode


def localizar_platformio() -> Path:
    candidatos: list[Path] = []
    if pio_en_path := shutil.which("platformio"):
        candidatos.append(Path(pio_en_path))
    candidatos.append(Path.home() / ".platformio" / "penv" / "Scripts" / "platformio.exe")
    for pio in candidatos:
        if pio.is_file():
            return pio
    raise FileNotFoundError("No se encontró PlatformIO. Instálalo o agrega platformio al PATH.")


def argumentos() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", help="Puerto serie del ESP32-S3, por ejemplo COM5.")
    parser.add_argument("--environment", default=ENTORNO_PREDETERMINADO,
                        help=f"Entorno PlatformIO (predeterminado: {ENTORNO_PREDETERMINADO}).")
    parser.add_argument("--solo-compilar", action="store_true", help="Compila sin cargar hardware.")
    parser.add_argument("--listar-puertos", action="store_true", help="Muestra los puertos que PlatformIO detecta.")
    parser.add_argument("--confirmar-vmot-apagado", action="store_true",
                        help="Confirmación obligatoria: VMOT está apagado durante la carga.")
    return parser.parse_args()


def main() -> int:
    args = argumentos()
    if not (RAIZ / "platformio.ini").is_file():
        print(f"ERROR: No se encontró platformio.ini en {RAIZ}.", file=sys.stderr)
        return 2
    try:
        pio = localizar_platformio()
    except FileNotFoundError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 2

    if args.listar_puertos:
        return ejecutar([str(pio), "device", "list"])

    codigo = ejecutar([str(pio), "run", "--environment", args.environment])
    if codigo or args.solo_compilar:
        return codigo

    if not args.port:
        print("ERROR: Indica --port COMx para evitar cargar por error otro dispositivo.", file=sys.stderr)
        return 2
    if not args.confirmar_vmot_apagado:
        print("ERROR: Antes de cargar, apaga físicamente VMOT y repite con --confirmar-vmot-apagado.", file=sys.stderr)
        return 2

    print("Carga autorizada: VMOT apagado. Si el ESP32-S3 no entra solo en bootloader, mantén BOOT y pulsa RST.")
    codigo = ejecutar([
        str(pio), "run", "--environment", args.environment, "--target", "upload", "--upload-port", args.port,
    ])
    if codigo:
        print(
            "La carga falló. Revisa cable USB de datos, puerto COM, monitor serie cerrado y la secuencia BOOT/RST. "
            "No energices VMOT para resolver un error de conexión.",
            file=sys.stderr,
        )
    return codigo


if __name__ == "__main__":
    raise SystemExit(main())

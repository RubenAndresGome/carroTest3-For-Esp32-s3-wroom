#!/usr/bin/env python3
"""Compila e instala la HMI Android Robot S3 sin borrar datos de la tablet."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
from typing import Sequence


RAIZ = Path(__file__).resolve().parents[3]
ANDROID_APP = RAIZ / "android_app"
APK = ANDROID_APP / "app" / "build" / "outputs" / "apk" / "debug" / "app-debug.apk"
PAQUETE = "mx.ik.robots3"


def ejecutar(comando: Sequence[str], *, cwd: Path | None = None, env: dict[str, str] | None = None) -> int:
    print("$", subprocess.list2cmdline(list(comando)))
    return subprocess.run(list(comando), cwd=cwd, env=env, check=False).returncode


def localizar_sdk() -> Path:
    candidatos: list[Path] = []
    for variable in ("ANDROID_HOME", "ANDROID_SDK_ROOT"):
        if valor := os.environ.get(variable):
            candidatos.append(Path(valor))
    if local := os.environ.get("LOCALAPPDATA"):
        candidatos.append(Path(local) / "Android" / "Sdk")
    for sdk in candidatos:
        if (sdk / "platform-tools" / "adb.exe").is_file():
            return sdk
    raise FileNotFoundError("No se encontró Android SDK Platform-Tools. Configura ANDROID_HOME o instala Android Studio.")


def localizar_adb() -> Path:
    try:
        return localizar_sdk() / "platform-tools" / "adb.exe"
    except FileNotFoundError:
        pass
    if adb_en_path := shutil.which("adb"):
        return Path(adb_en_path)
    raise FileNotFoundError("No se encontró adb. Instala Android SDK Platform-Tools o configura ANDROID_HOME.")


def preparar_entorno_android() -> dict[str, str]:
    entorno = os.environ.copy()
    sdk = localizar_sdk()
    entorno["ANDROID_HOME"] = str(sdk)
    entorno["ANDROID_SDK_ROOT"] = str(sdk)
    if not entorno.get("JAVA_HOME"):
        jbr = Path(r"C:\Program Files\Android\Android Studio\jbr")
        if (jbr / "bin" / "java.exe").is_file():
            entorno["JAVA_HOME"] = str(jbr)
    return entorno


def tablet_autorizada(adb: Path, serial_solicitado: str | None) -> str:
    resultado = subprocess.run([str(adb), "devices"], capture_output=True, text=True, check=False)
    if resultado.returncode:
        raise RuntimeError("ADB no pudo enumerar dispositivos.")
    dispositivos = [
        linea.split()[0]
        for linea in resultado.stdout.splitlines()[1:]
        if len(linea.split()) >= 2 and linea.split()[1] == "device"
    ]
    if serial_solicitado:
        if serial_solicitado not in dispositivos:
            raise RuntimeError(f"La tablet ADB '{serial_solicitado}' no está autorizada.")
        return serial_solicitado
    if len(dispositivos) != 1:
        raise RuntimeError(
            "Conecta exactamente una tablet con depuración USB autorizada o indica --serial. "
            f"Detectadas: {len(dispositivos)}."
        )
    return dispositivos[0]


def argumentos() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--solo-compilar", action="store_true", help="Genera APK pero no usa ADB.")
    parser.add_argument("--sin-compilar", action="store_true", help="Instala la APK existente sin ejecutar Gradle.")
    parser.add_argument("--serial", help="Serie ADB de la tablet a actualizar.")
    return parser.parse_args()


def main() -> int:
    args = argumentos()
    gradle = ANDROID_APP / "gradlew.bat"
    if not gradle.is_file():
        print(f"ERROR: No se encontró {gradle}.", file=sys.stderr)
        return 2

    if not args.sin_compilar:
        print("Aviso: no ejecutes simultáneamente el controlador Windows y la HMI de la tablet.")
        try:
            entorno = preparar_entorno_android()
        except FileNotFoundError as error:
            print(f"ERROR: {error}", file=sys.stderr)
            return 2
        codigo = ejecutar([str(gradle), "--no-daemon", "assembleDebug", "--console=plain"], cwd=ANDROID_APP, env=entorno)
        if codigo:
            return codigo
    if not APK.is_file():
        print(f"ERROR: No existe la APK esperada: {APK}", file=sys.stderr)
        return 2

    print(f"APK lista: {APK} ({APK.stat().st_size / 1024 / 1024:.2f} MiB)")
    if args.solo_compilar:
        return 0

    try:
        adb = localizar_adb()
        serial = tablet_autorizada(adb, args.serial)
    except (FileNotFoundError, RuntimeError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 2
    codigo = ejecutar([str(adb), "-s", serial, "install", "-r", str(APK)])
    if codigo:
        return codigo
    comprobacion = subprocess.run([str(adb), "-s", serial, "shell", "pm", "path", PAQUETE],
                                  capture_output=True, text=True, check=False)
    if comprobacion.returncode or not comprobacion.stdout.strip():
        print(f"ERROR: No se pudo comprobar el paquete {PAQUETE} tras instalar.", file=sys.stderr)
        return 1
    print(f"HMI instalada en {serial}: {comprobacion.stdout.strip()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

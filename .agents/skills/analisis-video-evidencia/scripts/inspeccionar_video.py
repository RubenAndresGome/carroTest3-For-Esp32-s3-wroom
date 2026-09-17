#!/usr/bin/env python3
"""
inspeccionar_video.py
Herramienta CLI para inspección forense de videos de pruebas del robot ESP32-S3.

Permite:
- Obtener metadatos (duración, resolución, fps, códec).
- Extraer N fotogramas representativos espaciados uniformemente.
- Generar hojas de contacto (contact sheet) con cuadrícula de muestras.
- Extraer fotogramas en marcas de tiempo específicas para correlacionar con telemetría.

Uso:
  python inspeccionar_video.py --video "evidencia/videos/mi_video.mp4" --info
  python inspeccionar_video.py --video "evidencia/videos/mi_video.mp4" --frames 24
  python inspeccionar_video.py --video "evidencia/videos/mi_video.mp4" --contact-sheet
  python inspeccionar_video.py --video "evidencia/videos/mi_video.mp4" --extract-at "00:04.500" --output "evidencia/fotogramas/muestra.jpg"
"""

import argparse
import json
import math
import os
import re
import subprocess
import sys
from pathlib import Path


def get_video_slug(video_path: str) -> str:
    base = os.path.basename(video_path)
    name, _ = os.path.splitext(base)
    # Normalizar a caracteres alfanuméricos y guiones bajos
    slug = re.sub(r"[^\w\-]", "_", name)
    return slug


def probe_video(video_path: str) -> dict:
    if not os.path.exists(video_path):
        raise FileNotFoundError(f"No se encontró el archivo de video en: {video_path}")

    cmd = [
        "ffprobe",
        "-v", "quiet",
        "-print_format", "json",
        "-show_format",
        "-show_streams",
        video_path,
    ]
    try:
        res = subprocess.run(cmd, capture_output=True, text=True, check=True)
        data = json.loads(res.stdout)
    except subprocess.CalledProcessError as e:
        sys.stderr.write(f"Error al ejecutar ffprobe: {e.stderr}\n")
        raise
    except FileNotFoundError:
        sys.stderr.write("ffprobe no está instalado o no está en el PATH del sistema.\n")
        raise

    format_info = data.get("format", {})
    video_stream = None
    audio_stream = None

    for s in data.get("streams", []):
        if s.get("codec_type") == "video" and video_stream is None:
            video_stream = s
        elif s.get("codec_type") == "audio" and audio_stream is None:
            audio_stream = s

    duration_sec = float(format_info.get("duration", 0.0))
    if duration_sec == 0.0 and video_stream:
        duration_sec = float(video_stream.get("duration", 0.0))

    fps = 0.0
    if video_stream:
        r_frame_rate = video_stream.get("r_frame_rate", "0/0")
        if "/" in r_frame_rate:
            num, den = r_frame_rate.split("/")
            if float(den) > 0:
                fps = float(num) / float(den)
        else:
            fps = float(r_frame_rate or 0)

    width = int(video_stream.get("width", 0)) if video_stream else 0
    height = int(video_stream.get("height", 0)) if video_stream else 0
    codec_name = video_stream.get("codec_name", "desconocido") if video_stream else "ninguno"
    size_bytes = int(format_info.get("size", 0))

    return {
        "file_path": str(Path(video_path).resolve()),
        "file_name": os.path.basename(video_path),
        "duration_sec": round(duration_sec, 3),
        "duration_formatted": format_seconds(duration_sec),
        "width": width,
        "height": height,
        "resolution": f"{width}x{height}",
        "fps": round(fps, 2),
        "video_codec": codec_name,
        "has_audio": audio_stream is not None,
        "audio_codec": audio_stream.get("codec_name", "") if audio_stream else None,
        "size_mb": round(size_bytes / (1024 * 1024), 2),
    }


def format_seconds(seconds: float) -> str:
    m = int(seconds // 60)
    s = seconds % 60
    return f"{m:02d}:{s:06.3f}"


def parse_timestamp_to_seconds(ts_str: str) -> float:
    # Acepta HH:MM:SS, MM:SS, MM:SS.mmm o simplemente segundos en float
    parts = ts_str.strip().split(":")
    if len(parts) == 3:
        h = float(parts[0])
        m = float(parts[1])
        s = float(parts[2])
        return h * 3600 + m * 60 + s
    elif len(parts) == 2:
        m = float(parts[0])
        s = float(parts[1])
        return m * 60 + s
    elif len(parts) == 1:
        return float(parts[0])
    else:
        raise ValueError(f"Formato de marca de tiempo no válido: '{ts_str}'")


def extract_frame_at(video_path: str, timestamp_sec: float, output_path: str) -> None:
    os.makedirs(os.path.dirname(os.path.abspath(output_path)), exist_ok=True)
    cmd = [
        "ffmpeg",
        "-y",
        "-ss", str(timestamp_sec),
        "-i", video_path,
        "-vframes", "1",
        "-q:v", "2",
        output_path,
    ]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        sys.stderr.write(f"Error al extraer fotograma en {timestamp_sec}s: {res.stderr}\n")
        raise RuntimeError(f"FFmpeg falló al extraer fotograma: {res.stderr}")


def extract_sample_frames(video_path: str, count: int = 24, output_dir: str = None) -> list:
    info = probe_video(video_path)
    duration = info["duration_sec"]
    slug = get_video_slug(video_path)

    if not output_dir:
        output_dir = os.path.join("evidencia", "fotogramas", slug)
    os.makedirs(output_dir, exist_ok=True)

    if count <= 1:
        timestamps = [duration / 2.0]
    else:
        # Espaciamiento uniforme evitando exactamente los extremos 0 y final
        margin = min(0.5, duration / (count + 1))
        step = (duration - 2 * margin) / (count - 1) if count > 1 else 0
        timestamps = [margin + i * step for i in range(count)]

    extracted = []
    print(f"Extrayendo {count} fotogramas en '{output_dir}'...")
    for idx, ts in enumerate(timestamps, start=1):
        filename = f"fotograma_{idx:02d}.jpg"
        target_path = os.path.join(output_dir, filename)
        extract_frame_at(video_path, ts, target_path)
        extracted.append({
            "index": idx,
            "timestamp_sec": round(ts, 3),
            "timestamp_formatted": format_seconds(ts),
            "file": target_path,
        })
        print(f"  • [{idx:02d}/{count:02d}] {format_seconds(ts)} -> {filename}")

    return extracted


def generate_contact_sheet(video_path: str, count: int = 24, output_path: str = None) -> str:
    info = probe_video(video_path)
    duration = info["duration_sec"]
    slug = get_video_slug(video_path)

    if not output_path:
        output_path = os.path.join("evidencia", "fotogramas", f"contacto_{slug}.jpg")
    os.makedirs(os.path.dirname(os.path.abspath(output_path)), exist_ok=True)

    # Determinar columnas y filas (ej. 24 -> 4x6, 12 -> 4x3, etc.)
    cols = 4
    rows = math.ceil(count / cols)

    print(f"Generando hoja de contacto ({cols}x{rows}, {count} muestras) en '{output_path}'...")

    # Usar filtro select y tile de ffmpeg
    fps = info["fps"] if info["fps"] > 0 else 25.0
    total_frames = int(duration * fps)
    step_frames = max(1, total_frames // count)

    vf_filter = f"select='not(mod(n\\,{step_frames}))',scale=320:-1,tile={cols}x{rows}"

    cmd = [
        "ffmpeg",
        "-y",
        "-i", video_path,
        "-vf", vf_filter,
        "-frames:v", "1",
        "-q:v", "2",
        output_path,
    ]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        # Método alternativo si el filtro directo select falla: extraer temporalmente y combinar
        sys.stderr.write(f"Aviso con filtro directo tile: {res.stderr}\n")
        sys.stderr.write("Intentando fallback mediante extracción temporal...\n")
        temp_dir = os.path.join("evidencia", "fotogramas", f"_tmp_{slug}")
        samples = extract_sample_frames(video_path, count, temp_dir)
        # Combinar usando montajes de ffmpeg si están disponibles o conservar muestras
        return samples[0]["file"]

    print(f"Hoja de contacto generada exitosamente: {output_path}")
    return output_path


def main():
    parser = argparse.ArgumentParser(
        description="Inspección forense y extracción de evidencia audiovisual para el Robot ESP32-S3."
    )
    parser.add_argument(
        "--video", "-v", required=True, help="Ruta al archivo de video (.mp4, .webm, etc.)"
    )
    parser.add_argument(
        "--info", action="store_true", help="Mostrar metadatos detallados del video (JSON/texto)"
    )
    parser.add_argument(
        "--frames", type=int, default=0, help="Extraer N fotogramas distribuidos uniformemente"
    )
    parser.add_argument(
        "--contact-sheet", action="store_true", help="Generar hoja de contacto con fotogramas"
    )
    parser.add_argument(
        "--extract-at", type=str, default=None, help="Extraer fotograma en tiempo específico (ej. '00:04.5' o '4.5')"
    )
    parser.add_argument(
        "--output", "-o", type=str, default=None, help="Ruta de salida para fotograma o carpeta"
    )

    args = parser.parse_args()

    if not os.path.exists(args.video):
        print(f"[ERROR] El archivo de video no existe en: {args.video}", file=sys.stderr)
        sys.exit(1)

    info = probe_video(args.video)

    if args.info or (not args.frames and not args.contact_sheet and not args.extract_at):
        print("==================================================")
        print(" METADATOS FORENSES DEL VIDEO")
        print("==================================================")
        print(f"  • Archivo:     {info['file_name']}")
        print(f"  • Ruta:        {info['file_path']}")
        print(f"  • Duración:    {info['duration_formatted']} ({info['duration_sec']} s)")
        print(f"  • Resolución:  {info['resolution']}")
        print(f"  • Tasa FPS:    {info['fps']} fps")
        print(f"  • Códec video: {info['video_codec']}")
        print(f"  • Audio:       {'Sí (' + info['audio_codec'] + ')' if info['has_audio'] else 'No'}")
        print(f"  • Tamaño:      {info['size_mb']} MB")
        print("==================================================")

    if args.extract_at:
        ts_sec = parse_timestamp_to_seconds(args.extract_at)
        slug = get_video_slug(args.video)
        out_file = args.output or os.path.join(
            "evidencia", "fotogramas", slug, f"captura_{ts_sec:.2f}s.jpg"
        )
        print(f"Extrayendo fotograma en {format_seconds(ts_sec)} -> {out_file}...")
        extract_frame_at(args.video, ts_sec, out_file)
        print("Extracción completada.")

    if args.frames > 0:
        extract_sample_frames(args.video, count=args.frames, output_dir=args.output)

    if args.contact_sheet:
        generate_contact_sheet(args.video, count=args.frames if args.frames > 0 else 24, output_path=args.output)


if __name__ == "__main__":
    main()

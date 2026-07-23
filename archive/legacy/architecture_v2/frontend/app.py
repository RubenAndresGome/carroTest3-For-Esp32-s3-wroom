"""Lanzador de compatibilidad; la implementación vive en desktop_app."""

from __future__ import annotations

import runpy
from pathlib import Path


if __name__ == "__main__":
    runpy.run_path(str(Path(__file__).resolve().parents[2] / "desktop_app" / "app.py"), run_name="__main__")

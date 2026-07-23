"""Entrada Android: aloja Flask únicamente en loopback dentro del APK."""

from __future__ import annotations

import logging
import threading
from pathlib import Path
from typing import Any

from waitress import create_server

from robot_app.app_factory import create_app
from robot_app.config import AppConfig


_lock = threading.RLock()
_server: Any | None = None
_application: Any | None = None


def run(android_files_dir: str, port: int = 8080) -> None:
    """Bloquea la hebra del servicio Android mientras Waitress está activo."""
    global _server, _application
    data_dir = Path(android_files_dir) / "robot_s3"
    data_dir.mkdir(parents=True, exist_ok=True)
    settings = AppConfig(
        data_dir=data_dir,
        database_path=data_dir / "robot.sqlite3",
        start_gateway=False,
    )
    logging.basicConfig(level=logging.INFO)
    application = create_app(settings)
    server = create_server(
        application,
        host="127.0.0.1",
        port=int(port),
        threads=8,
        ident="RobotS3-Android",
    )
    with _lock:
        _application = application
        _server = server
    try:
        server.run()
    finally:
        application.extensions["robot_service"].close()
        with _lock:
            _server = None
            _application = None


def stop() -> None:
    """Detiene Waitress; el finally de run cierra gateway, sesión y SQLite."""
    with _lock:
        server = _server
    if server is not None:
        server.close()


def is_ready() -> bool:
    with _lock:
        return _server is not None

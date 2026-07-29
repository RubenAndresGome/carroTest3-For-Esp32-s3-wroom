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
_stopping = False


def run(android_files_dir: str, port: int = 8080) -> None:
    """Bloquea la hebra del servicio Android mientras Waitress está activo."""
    global _server, _application, _stopping
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
        _stopping = False
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
    global _stopping
    with _lock:
        if _stopping:
            return
        _stopping = True
        server = _server
    if server is not None:
        server.close()
        for channel in list(server._map.values()):
            try:
                channel.close()
            except Exception:
                pass
        server.task_dispatcher.shutdown(cancel_pending=True)


def prepare_close(force: bool = False) -> dict[str, Any]:
    """Usado por la notificación Android antes de destruir el servicio."""
    with _lock:
        application = _application
    if application is None:
        return {"safe_to_close": True, "stop_required": False, "stop_confirmed": True, "forced": force}
    result = application.extensions["robot_service"].prepare_close(force=bool(force))
    return dict(result)


def is_ready() -> bool:
    with _lock:
        return _server is not None

"""Configuración explícita y validada de la aplicación."""

from __future__ import annotations

import os
from dataclasses import dataclass
from pathlib import Path
from urllib.parse import urlparse

from platformdirs import user_data_path


DEFAULT_ROBOT_HOST = "192.168.4.1"


def normalize_robot_host(value: object) -> str:
    raw = str(value or "").strip()
    if not raw:
        return DEFAULT_ROBOT_HOST
    parsed = urlparse(raw if "://" in raw else f"//{raw}")
    host = parsed.hostname
    if not host or any(char.isspace() for char in host):
        raise ValueError("La IP o nombre del robot no es válido")
    port = parsed.port
    return f"{host}:{port}" if port else host


@dataclass(frozen=True, slots=True)
class AppConfig:
    data_dir: Path
    database_path: Path
    robot_host: str = DEFAULT_ROBOT_HOST
    start_gateway: bool = False
    telemetry_queue_size: int = 256
    event_queue_size: int = 64
    max_message_bytes: int = 4096

    @classmethod
    def from_environment(cls) -> "AppConfig":
        data_dir = Path(
            os.environ.get("ROBOT_APP_DATA_DIR")
            or user_data_path("RobotS3", "IK", ensure_exists=True)
        )
        data_dir.mkdir(parents=True, exist_ok=True)
        return cls(
            data_dir=data_dir,
            database_path=data_dir / "robot.sqlite3",
            robot_host=normalize_robot_host(os.environ.get("ROBOT_HOST", DEFAULT_ROBOT_HOST)),
            # La red del robot se elige primero en Windows. El WebSocket solo
            # arranca cuando el operador pulsa Conectar en la HMI.
            start_gateway=os.environ.get("ROBOT_AUTO_CONNECT", "0") == "1",
        )

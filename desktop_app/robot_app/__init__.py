"""Aplicación local para el Robot ESP32-S3."""

from typing import Any


def create_app(*args: Any, **kwargs: Any) -> Any:
    """Importación diferida para mantener el dominio independiente de Flask."""
    from .app_factory import create_app as factory
    return factory(*args, **kwargs)


__all__ = ["create_app"]

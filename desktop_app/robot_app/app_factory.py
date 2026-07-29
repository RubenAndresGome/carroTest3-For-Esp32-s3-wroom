"""Composition root: ensambla adaptadores y servicios."""

from __future__ import annotations

import atexit
import secrets
from typing import Any

from flask import Flask, g

from .config import AppConfig
from .database import Database
from .services import RobotService
from .web import web


def create_app(config: AppConfig | None = None) -> Flask:
    settings = config or AppConfig.from_environment()
    app = Flask(__name__, template_folder=None, static_folder=None)
    app.config.update(
        JSON_SORT_KEYS=False,
        MAX_CONTENT_LENGTH=16 * 1024,
        TRUSTED_HOSTS=["127.0.0.1", "localhost", "[::1]"],
    )
    database = Database(settings.database_path)
    database.initialize()
    if database.get_setting("robot_host") is None:
        database.set_setting("robot_host", settings.robot_host)
    service = RobotService(database, start_gateway=settings.start_gateway, max_message_bytes=settings.max_message_bytes)
    app.extensions["robot_service"] = service
    app.extensions["app_token"] = secrets.token_urlsafe(24)
    app.extensions["host_shutdown"] = None
    app.register_blueprint(web)

    @app.before_request
    def create_csp_nonce() -> None:
        g.csp_nonce = secrets.token_urlsafe(18)

    @app.after_request
    def secure_headers(response: Any) -> Any:
        response.headers["X-Content-Type-Options"] = "nosniff"
        response.headers["Referrer-Policy"] = "no-referrer"
        response.headers["Cache-Control"] = "no-store"
        nonce = getattr(g, "csp_nonce", "")
        response.headers["Content-Security-Policy"] = (
            f"default-src 'self'; script-src 'self' 'nonce-{nonce}'; style-src-elem 'self' 'nonce-{nonce}'; "
            "style-src-attr 'unsafe-inline'; img-src 'self' data:; "
            "connect-src 'self'; object-src 'none'; base-uri 'none'; frame-ancestors 'none'"
        )
        return response

    atexit.register(service.close)
    return app

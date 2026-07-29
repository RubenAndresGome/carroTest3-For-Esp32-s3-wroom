"""Adaptador HTTP/SSE de Flask; no contiene lógica del robot."""

from __future__ import annotations

import csv
import io
import json
import queue
import secrets
import threading
from pathlib import Path
from functools import wraps
from typing import Any, Callable

from flask import Blueprint, Response, current_app, g, jsonify, request, send_from_directory, stream_with_context

from .services import RobotService


web = Blueprint("web", __name__)
HMI_ROOT = Path(__file__).resolve().parent / "hmi"


def _service() -> RobotService:
    return current_app.extensions["robot_service"]


def _require_token(function: Callable[..., Any]) -> Callable[..., Any]:
    @wraps(function)
    def wrapper(*args: Any, **kwargs: Any) -> Any:
        expected = current_app.extensions["app_token"]
        if not secrets.compare_digest(request.headers.get("X-App-Token", ""), expected):
            return jsonify({"error": "token_local_invalido"}), 403
        return function(*args, **kwargs)
    return wrapper


@web.get("/")
def index() -> Response:
    html = (HMI_ROOT / "index.html").read_text(encoding="utf-8")
    nonce = g.csp_nonce
    html = html.replace('<meta name="app-token" content="">', f'<meta name="app-token" content="{current_app.extensions["app_token"]}">')
    html = html.replace("<style>", f'<style nonce="{nonce}">', 1)
    html = html.replace("    <script>\n", f'    <script nonce="{nonce}">\n', 1)
    return Response(html, mimetype="text/html")


@web.get("/<path:filename>")
def ui_asset(filename: str) -> Response:
    allowed = {"chart.js", "chartjs-plugin-datalabels.js", "hammer.js", "chartjs-plugin-zoom.js"}
    if filename not in allowed:
        return jsonify({"error": "recurso_no_encontrado"}), 404
    return send_from_directory(HMI_ROOT / "vendor", filename)


@web.get("/api/v1/status")
def status() -> Response:
    return jsonify(_service().status())


@web.route("/api/v1/config/robot", methods=["GET", "PUT"])
@_require_token
def robot_config() -> Response | tuple[Response, int]:
    service = _service()
    if request.method == "GET":
        return jsonify({"robot_host": service.get_robot_host()})
    body = request.get_json(silent=True) or {}
    try:
        host = service.set_robot_host(body.get("robot_host"))
    except ValueError as exc:
        return jsonify({"error": str(exc)}), 400
    return jsonify({"robot_host": host})


@web.post("/api/v1/connection/<action>")
@_require_token
def connection(action: str) -> Response | tuple[Response, int]:
    service = _service()
    operations = {"connect": service.connect, "disconnect": service.disconnect, "reconnect": service.reconnect}
    operation = operations.get(action)
    if operation is None:
        return jsonify({"error": "accion_de_conexion_invalida"}), 404
    operation()
    return jsonify(service.status())


@web.post("/api/v1/commands")
@_require_token
def command() -> tuple[Response, int]:
    body = request.get_json(silent=True) or {}
    try:
        result = _service().send_command(body.get("name"), body.get("payload"))
    except ValueError as exc:
        return jsonify({"error": str(exc)}), 400
    except RuntimeError as exc:
        return jsonify({"error": str(exc)}), 503
    return jsonify({"id": result.command_id, "status": "queued"}), 202


@web.post("/api/v1/estop")
@_require_token
def estop() -> tuple[Response, int]:
    result = _service().send_command("estop", {})
    return jsonify({"id": result.command_id, "status": "queued"}), 202


@web.route("/api/v1/missions", methods=["GET", "POST", "DELETE"])
@_require_token
def missions() -> Response | tuple[Response, int]:
    service = _service()
    if request.method == "GET":
        return jsonify(service.mission_status())
    if request.method == "DELETE":
        return jsonify(service.stop_mission())
    body = request.get_json(silent=True) or {}
    try:
        return jsonify(service.start_mission(body.get("points"))), 202
    except ValueError as exc:
        return jsonify({"error": str(exc)}), 400
    except RuntimeError as exc:
        return jsonify({"error": str(exc)}), 409


@web.delete("/api/v1/missions/robot-memory")
@_require_token
def clear_robot_mission_memory() -> Response:
    return jsonify(_service().clear_robot_mission_memory())


@web.post("/api/v1/missions/return-home")
@_require_token
def return_home() -> tuple[Response, int]:
    try:
        return jsonify(_service().start_return_home()), 202
    except RuntimeError as exc:
        return jsonify({"error": str(exc)}), 409


@web.post("/api/v1/app/close")
@_require_token
def close_application() -> Response | tuple[Response, int]:
    body = request.get_json(silent=True) or {}
    result = _service().prepare_close(force=body.get("force") is True)
    if not result.get("safe_to_close"):
        return jsonify(result), 409
    shutdown = current_app.extensions.get("host_shutdown")
    if callable(shutdown):
        threading.Timer(0.25, shutdown).start()
    return jsonify(result)


@web.post("/api/v1/sessions/start")
@_require_token
def start_session() -> Response:
    return jsonify({"session_id": _service().start_session(), "recording": True})


@web.post("/api/v1/sessions/stop")
@_require_token
def stop_session() -> Response:
    return jsonify({"session_id": _service().stop_session("manual"), "recording": False})


@web.post("/api/v1/sessions/cleanup")
@_require_token
def cleanup_sessions() -> Response:
    body = request.get_json(silent=True) or {}
    try:
        days = int(body.get("days", 7))
    except (ValueError, TypeError):
        days = 7
    deleted_count = _service().purge_sessions(days)
    return jsonify({"deleted_sessions": deleted_count, "message": f"Se eliminaron {deleted_count} registros de sesiones"})



@web.get("/api/v1/sessions/<int:session_id>/telemetry.csv")
def export_telemetry(session_id: int) -> Response:
    output = io.StringIO(newline="")
    writer = csv.writer(output)
    writer.writerow(["seq", "received_at", "robot_uptime_ms", "state", "x_mm", "y_mm", "yaw_deg", "pfl", "pfr", "pbl", "pbr", "pwm_l", "pwm_r", "mpu_present", "mpu_stale", "i2c_ok", "pin_state_json", "payload_json"])
    for row in _service().database.telemetry_rows(session_id):
        writer.writerow(tuple(row))
    return Response(
        output.getvalue(), mimetype="text/csv",
        headers={"Content-Disposition": f"attachment; filename=telemetry-session-{session_id}.csv"},
    )


@web.get("/api/v1/sessions")
def sessions() -> Response:
    return jsonify([dict(row) for row in _service().database.session_rows()])


@web.get("/api/v1/sessions/<int:session_id>.json")
def export_session_json(session_id: int) -> Response | tuple[Response, int]:
    session = _service().database.session_row(session_id)
    if session is None:
        return jsonify({"error": "sesion_no_encontrada"}), 404
    database = _service().database
    payload = {
        "session": dict(session),
        "telemetry": [dict(row) for row in database.telemetry_rows(session_id)],
        "commands": [dict(row) for row in database.command_rows(session_id)],
        "events": [dict(row) for row in database.event_rows(session_id)],
    }
    return Response(
        json.dumps(payload, ensure_ascii=False, separators=(",", ":")), mimetype="application/json",
        headers={"Content-Disposition": f"attachment; filename=session-{session_id}.json"},
    )


@web.get("/api/v1/events")
def events() -> Response:
    service = _service()
    subscriber = service.events.subscribe()

    @stream_with_context
    def stream() -> Any:
        try:
            yield f"event: snapshot\ndata: {__import__('json').dumps(service.status(), separators=(',', ':'))}\n\n"
            while True:
                try:
                    payload = subscriber.get(timeout=15.0)
                    yield f"data: {payload}\n\n"
                except queue.Empty:
                    yield ": keepalive\n\n"
        finally:
            service.events.unsubscribe(subscriber)

    return Response(stream(), mimetype="text/event-stream", headers={"Cache-Control": "no-store", "X-Accel-Buffering": "no"})

"""Casos de uso: conexión, comandos, sesiones, eventos y telemetría."""

from __future__ import annotations

import json
import math
import queue
import threading
import time
import uuid
from typing import Any

from .config import DEFAULT_ROBOT_HOST, normalize_robot_host
from .database import Database
from .domain import (MAX_SEGMENT_MM, CommandStatus, ConnectionState, RobotCommand, Severity,
                     TelemetrySnapshot, split_segment_mm)
from .gateway import RobotGateway


class EventHub:
    def __init__(self, subscriber_size: int = 64) -> None:
        self._subscriber_size = subscriber_size
        self._subscribers: set[queue.Queue[str]] = set()
        self._lock = threading.Lock()

    def subscribe(self) -> queue.Queue[str]:
        subscriber: queue.Queue[str] = queue.Queue(maxsize=self._subscriber_size)
        with self._lock:
            self._subscribers.add(subscriber)
        return subscriber

    def unsubscribe(self, subscriber: queue.Queue[str]) -> None:
        with self._lock:
            self._subscribers.discard(subscriber)

    def publish(self, kind: str, payload: dict[str, Any]) -> None:
        encoded = json.dumps({"type": kind, "payload": payload}, ensure_ascii=False, separators=(",", ":"))
        with self._lock:
            subscribers = tuple(self._subscribers)
        for subscriber in subscribers:
            try:
                subscriber.put_nowait(encoded)
            except queue.Full:
                try:
                    subscriber.get_nowait()
                    subscriber.put_nowait(encoded)
                except (queue.Empty, queue.Full):
                    pass


class TelemetryRecorder:
    def __init__(self, database: Database, queue_size: int = 256) -> None:
        self._database = database
        self._queue: queue.Queue[tuple[int, TelemetrySnapshot]] = queue.Queue(maxsize=queue_size)
        self._stop = threading.Event()
        self.last_error: str | None = None
        self._thread = threading.Thread(target=self._run, name="sqlite-telemetry", daemon=True)
        self._thread.start()

    def submit(self, session_id: int, snapshot: TelemetrySnapshot) -> None:
        try:
            self._queue.put_nowait((session_id, snapshot))
        except queue.Full:
            try:
                self._queue.get_nowait()
                self._queue.put_nowait((session_id, snapshot))
            except (queue.Empty, queue.Full):
                pass

    def stop(self) -> None:
        self._stop.set()
        self._thread.join(timeout=3.0)

    def _run(self) -> None:
        while not self._stop.is_set() or not self._queue.empty():
            try:
                session_id, snapshot = self._queue.get(timeout=0.2)
                self._database.insert_telemetry(session_id, snapshot)
                self.last_error = None
            except queue.Empty:
                continue
            except Exception as exc:
                self.last_error = str(exc)[:160]


class RobotService:
    MISSION_SEGMENT_TIMEOUT_S = 240.0
    TELEMETRY_FRESHNESS_S = 2.0
    CLOSE_STOP_TIMEOUT_S = 5.0
    PROTOCOL = "steps-v3"

    def __init__(self, database: Database, start_gateway: bool = True, max_message_bytes: int = 4096) -> None:
        self.database = database
        self.events = EventHub()
        self._lock = threading.RLock()
        self._robot_host = normalize_robot_host(database.get_setting("robot_host", DEFAULT_ROBOT_HOST))
        self._session_id: int | None = None
        self._last_telemetry: TelemetrySnapshot | None = None
        self._telemetry_sequence = 0
        self._last_recorded_at = 0.0
        self._identity_session_id: int | None = None
        self._last_terminal_id: str | None = None
        self._last_manual_recorded_at = 0.0
        self._tracked_command_ids: set[str] = set()
        self._command_ids_by_seq: dict[int, str] = {}
        self._command_terminal_events: dict[str, threading.Event] = {}
        self._command_terminal_results: dict[str, tuple[str, str | None]] = {}
        self._controller_session = uuid.uuid4().hex[:16]
        self._next_seq = 1
        database.set_setting("controller_session", self._controller_session)
        database.set_setting("next_command_seq", self._next_seq)
        self._closing = False
        self._closed = False
        self._close_prepared = False
        self._close_result: dict[str, Any] | None = None
        self._startup_stop_required = False
        self._startup_stop_command_id: str | None = None
        self._mission_id: str | None = None
        self._mission_points: list[dict[str, float]] = []
        self._mission_index = 0
        self._mission_command_id: str | None = None
        self._mission_command_started_at: float | None = None
        self._mission_error: str | None = None
        self._mission_blocked = False
        self._mission_revision = 1
        self._mission_stage = "idle"
        self._mission_origin: dict[str, float] = {"x_mm": 0.0, "y_mm": 0.0}
        self._mission_seq: int | None = None
        self._mission_kind = "outbound"
        self._mission_final_heading: float | None = None
        saved_mission = database.get_setting("active_mission", None)
        if isinstance(saved_mission, dict) and saved_mission.get("id") and isinstance(saved_mission.get("points"), list):
            self._mission_id = str(saved_mission["id"])
            self._mission_points = list(saved_mission["points"])
            self._mission_index = int(saved_mission.get("current_index", 0))
            self._mission_revision = int(saved_mission.get("revision", 1))
            self._mission_stage = str(saved_mission.get("stage") or "executing")
            self._mission_blocked = bool(saved_mission.get("blocked", False))
            self._mission_error = str(saved_mission.get("error") or "") or None
            self._mission_command_id = str(saved_mission.get("active_command_id") or "") or None
            self._mission_kind = str(saved_mission.get("kind") or "outbound")
            final_heading = saved_mission.get("final_heading")
            self._mission_final_heading = float(final_heading) if final_heading is not None else None
            origin = saved_mission.get("origin", {})
            if isinstance(origin, dict):
                self._mission_origin = {"x_mm": float(origin.get("x_mm", 0)), "y_mm": float(origin.get("y_mm", 0))}
            self._mission_seq = int(saved_mission.get("active_seq", 0) or 0) or None
            if self._mission_stage not in {"completed", "abandoned", "blocked"} and not self._mission_blocked:
                self._mission_stage = "abandoned"
                self._mission_blocked = True
                self._mission_error = "app_restarted_with_pending_mission"
                self._mission_command_id = None
                self._mission_command_started_at = None
                self._mission_seq = None
                self._startup_stop_required = True
        failed_commands = database.fail_nonterminal_commands("app_restarted_command_abandoned")
        orphan_sessions = database.close_orphan_sessions("app_restarted_uncleanly")
        if failed_commands:
            self._startup_stop_required = True
        if self._mission_kind == "ockham_return" and self._mission_stage == "abandoned":
            route = database.get_setting("last_completed_route", None)
            if isinstance(route, dict) and route.get("return_state") == "in_progress":
                route["return_state"] = "blocked"
                route["return_error"] = "app_restarted_with_pending_mission"
                database.set_setting("last_completed_route", route)
        if self._mission_id is not None:
            self._persist_mission()
        if failed_commands or orphan_sessions or self._startup_stop_required:
            database.insert_event(None, "app_restart_quarantine", Severity.WARNING.value, {
                "failed_commands": failed_commands,
                "orphan_sessions": orphan_sessions,
                "stop_required": self._startup_stop_required,
                "mission_id": self._mission_id,
            })
        self._recorder = TelemetryRecorder(database)
        self.gateway = RobotGateway(
            host_getter=self.get_robot_host,
            on_message=self._on_robot_message,
            on_state=self._on_connection_state,
            on_sent=self._on_command_sent,
            session_getter=lambda: self._controller_session,
            max_message_bytes=max_message_bytes,
        )
        if start_gateway:
            self.gateway.start()

    def close(self) -> None:
        with self._lock:
            if self._closed:
                return
            self._closed = True
        self.gateway.stop()
        with self._lock:
            if self._session_id is not None:
                self.database.stop_session(self._session_id, "application_closed")
                self._session_id = None
        self._recorder.stop()

    def get_robot_host(self) -> str:
        with self._lock:
            return self._robot_host

    def set_robot_host(self, value: object) -> str:
        host = normalize_robot_host(value)
        with self._lock:
            changed = host != self._robot_host
            self._robot_host = host
        self.database.set_setting("robot_host", host)
        if changed:
            self.gateway.request_reconnect()
        self.events.publish("config", {"robot_host": host})
        return host

    def connect(self) -> None:
        self.gateway.start()

    def disconnect(self) -> None:
        self.gateway.stop()

    def reconnect(self) -> None:
        if self.gateway.snapshot()["state"] == ConnectionState.STOPPED.value:
            self.gateway.start()
        else:
            self.gateway.request_reconnect()

    def status(self) -> dict[str, Any]:
        with self._lock:
            telemetry = self._last_telemetry.public_dict() if self._last_telemetry else None
            session_id = self._session_id
            host = self._robot_host
        return {
            "connection": self.gateway.snapshot(),
            "robot_host": host,
            "websocket_url": f"ws://{host}/ws",
            "recording": session_id is not None,
            "session_id": session_id,
            "telemetry": telemetry,
            "storage_error": self._recorder.last_error,
            "mission": self.mission_status(),
            "controls_ready": not self._startup_stop_required and not self._closing,
            "closing": self._closing,
        }

    def mission_status(self) -> dict[str, Any]:
        with self._lock:
            return {
                "id": self._mission_id,
                "running": self._mission_id is not None and not self._mission_blocked
                and self._mission_stage in {"executing", "aligning_final"},
                "blocked": self._mission_blocked,
                "current_index": self._mission_index,
                "total_segments": len(self._mission_points),
                "active_command_id": self._mission_command_id,
                "segment_elapsed_s": (
                    max(0.0, time.monotonic() - self._mission_command_started_at)
                    if self._mission_command_started_at is not None else None
                ),
                "error": self._mission_error,
                "revision": self._mission_revision if self._mission_id else None,
                "stage": self._mission_stage,
                "active_seq": self._mission_seq,
                "kind": self._mission_kind if self._mission_id else None,
                "final_heading": self._mission_final_heading,
            }

    def _persist_mission(self) -> None:
        with self._lock:
            value = None if self._mission_id is None else {
                "id": self._mission_id, "revision": self._mission_revision,
                "points": self._mission_points, "current_index": self._mission_index,
                "stage": self._mission_stage,
                "origin": self._mission_origin, "active_seq": self._mission_seq,
                "blocked": self._mission_blocked, "error": self._mission_error,
                "active_command_id": self._mission_command_id,
                "kind": self._mission_kind, "final_heading": self._mission_final_heading,
            }
        self.database.set_setting("active_mission", value)

    def _mission_wire_points(self) -> list[dict[str, Any]]:
        with self._lock:
            mission_id = self._mission_id or ""
            points = list(self._mission_points)
        return [
            {**point, "step_id": f"{mission_id[:24]}{index:08x}"}
            for index, point in enumerate(points, start=1)
        ]

    def _require_ready_robot(self) -> TelemetrySnapshot:
        connection = self.gateway.snapshot()
        if connection["state"] != ConnectionState.CONNECTED.value or connection["protocol"] != self.PROTOCOL:
            raise RuntimeError("El robot no está conectado con protocolo robot-s3-steps-v3")
        with self._lock:
            if self._startup_stop_required:
                raise RuntimeError("La reconciliación de parada tras reinicio aún no ha terminado")
            if self._closing:
                raise RuntimeError("La aplicación está preparando su cierre")
            telemetry = self._last_telemetry
        if telemetry is None or not telemetry.calibrated:
            raise RuntimeError("El robot debe estar conectado y con la MPU de arranque lista")
        if time.time() - telemetry.received_at > self.TELEMETRY_FRESHNESS_S:
            raise RuntimeError("La telemetría del robot está obsoleta; reconecte antes de iniciar")
        if telemetry.state.lower() != "listo":
            raise RuntimeError(f"El robot debe estar LISTO; estado actual: {telemetry.state}")
        return telemetry

    def _build_segments(self, points: list[Any], mission_origin: dict[str, float]) -> list[dict[str, float]]:
        origin_x, origin_y = mission_origin["x_mm"], mission_origin["y_mm"]
        segments: list[dict[str, float]] = []
        for point in points:
            if not isinstance(point, dict):
                raise ValueError("Cada punto debe ser un objeto")
            validated = RobotCommand.create("move", point).payload
            target_x, target_y = validated["x_mm"], validated["y_mm"]
            dx = target_x - origin_x
            dy = target_y - origin_y
            
            # Descomposición automática en 2 tramos ortogonales si el punto es diagonal
            sub_targets = []
            if abs(dx) > 1.0 and abs(dy) > 1.0:
                sub_targets.append((target_x, origin_y))
                sub_targets.append((target_x, target_y))
            else:
                sub_targets.append((target_x, target_y))
                
            for tx, ty in sub_targets:
                sub_dx = tx - origin_x
                sub_dy = ty - origin_y
                if abs(sub_dx) <= 1.0:
                    tx = origin_x
                if abs(sub_dy) <= 1.0:
                    ty = origin_y
                new_segments = split_segment_mm(origin_x, origin_y, tx, ty)
                segments.extend(new_segments)
                origin_x, origin_y = tx, ty
        if not segments:
            raise ValueError("La misión no contiene desplazamiento")
        if len(segments) > 32:
            raise ValueError("La descomposición excede los 32 segmentos disponibles en el ESP32")
        return segments

    def _activate_mission(self, segments: list[dict[str, float]], mission_origin: dict[str, float],
                          kind: str, final_heading: float | None = None) -> dict[str, Any]:
        if kind == "outbound":
            previous_route = self.database.get_setting("last_completed_route", None)
            if isinstance(previous_route, dict) and previous_route.get("return_state") == "available":
                previous_route["return_state"] = "blocked"
                previous_route["return_error"] = "superseded_by_new_mission"
                self.database.set_setting("last_completed_route", previous_route)
        with self._lock:
            if self._mission_id is not None and not self._mission_blocked and self._mission_stage in {
                "executing", "aligning_final"
            }:
                raise RuntimeError("Ya existe una misión activa")
            self._mission_id = uuid.uuid4().hex
            self._mission_points = segments
            self._mission_index = 0
            self._mission_command_id = None
            self._mission_command_started_at = None
            self._mission_origin = mission_origin
            self._mission_seq = None
            self._mission_error = None
            self._mission_blocked = False
            self._mission_revision = 1
            self._mission_stage = "executing"
            self._mission_kind = kind
            self._mission_final_heading = final_heading
            self._next_seq = 1
            self.database.set_setting("next_command_seq", 1)
        self._queue_current_mission_step()
        self._persist_mission()
        snapshot = self.mission_status()
        self.events.publish("mission", snapshot)
        return snapshot

    def start_mission(self, points: object) -> dict[str, Any]:
        if not isinstance(points, list) or not points or len(points) > 32:
            raise ValueError("La misión requiere entre 1 y 32 puntos")
        telemetry = self._require_ready_robot()
        mission_origin = {"x_mm": telemetry.x_mm, "y_mm": telemetry.y_mm}
        segments = self._build_segments(points, mission_origin)
        return self._activate_mission(segments, mission_origin, "outbound")

    def start_return_home(self) -> dict[str, Any]:
        telemetry = self._require_ready_robot()
        route = self.database.get_setting("last_completed_route", None)
        if not isinstance(route, dict) or route.get("return_state") != "available":
            raise RuntimeError("No existe una ruta saliente completada disponible para regresar")
        source_origin = route.get("origin")
        source_points = route.get("points")
        if not isinstance(source_origin, dict) or not isinstance(source_points, list) or not source_points:
            raise RuntimeError("El registro de la ruta completada no es válido")
        try:
            previous = {"x_mm": float(source_origin["x_mm"]), "y_mm": float(source_origin["y_mm"])}
            vectors: list[tuple[float, float]] = []
            for point in source_points:
                current = {"x_mm": float(point["x_mm"]), "y_mm": float(point["y_mm"])}
                vectors.append((current["x_mm"] - previous["x_mm"], current["y_mm"] - previous["y_mm"]))
                previous = current
        except (KeyError, TypeError, ValueError) as exc:
            raise RuntimeError("El registro de la ruta completada no es válido") from exc
        return_origin = {"x_mm": telemetry.x_mm, "y_mm": telemetry.y_mm}
        x_mm, y_mm = telemetry.x_mm, telemetry.y_mm
        return_points: list[dict[str, float]] = []
        for dx, dy in reversed(vectors):
            x_mm -= dx
            y_mm -= dy
            return_points.append({"x_mm": x_mm, "y_mm": y_mm})
        if len(return_points) > 32:
            raise RuntimeError("La ruta inversa excede los 32 pasos atómicos")
        route["return_state"] = "in_progress"
        route["return_error"] = None
        self.database.set_setting("last_completed_route", route)
        try:
            snapshot = self._activate_mission(return_points, return_origin, "ockham_return", final_heading=0.0)
        except Exception:
            route["return_state"] = "available"
            self.database.set_setting("last_completed_route", route)
            raise
        snapshot["planned_points"] = return_points
        return snapshot

    def stop_mission(self, reason: str = "operator_stop") -> dict[str, Any]:
        with self._lock:
            was_running = (
                self._mission_id is not None
                and not self._mission_blocked
                and self._mission_index < len(self._mission_points)
            )
            had_active_command = self._mission_command_id is not None
            active_command_id = self._mission_command_id
            mission_kind = self._mission_kind
            mission_id = self._mission_id
            # DELETE descarta la ruta. No republicar un fallo anterior como si
            # lo hubiera provocado una orden manual posterior.
            self._mission_error = None
            self._mission_id = None
            self._mission_points = []
            self._mission_index = 0
            self._mission_command_id = None
            self._mission_command_started_at = None
            self._mission_seq = None
            self._mission_blocked = False
            self._mission_stage = "idle"
            self._mission_kind = "outbound"
            self._mission_final_heading = None
        self.gateway.cancel(active_command_id)
        if active_command_id:
            self._tracked_command_ids.discard(active_command_id)
            self.database.update_command(active_command_id, CommandStatus.FAILED.value, reason)
        if mission_kind == "ockham_return":
            self._set_return_state("blocked", reason)
        elif mission_kind == "outbound":
            route = self.database.get_setting("last_completed_route", None)
            if isinstance(route, dict) and route.get("route_id") == mission_id and route.get("return_state") == "available":
                route["return_state"] = "blocked"
                route["return_error"] = reason
                self.database.set_setting("last_completed_route", route)
        if was_running or had_active_command:
            try:
                self.send_command("stop", {})
            except RuntimeError:
                pass
        snapshot = self.mission_status()
        self._persist_mission()
        self.events.publish("mission", snapshot)
        return snapshot

    def clear_robot_mission_memory(self) -> dict[str, Any]:
        """Detiene el paso y borra la misión que pertenece exclusivamente a Python."""
        snapshot = self.stop_mission("operator_clear")
        snapshot["clear_command_id"] = None
        return snapshot

    def _advance_mission(self) -> None:
        queue_step = False
        queue_alignment = False
        completed_kind: str | None = None
        with self._lock:
            if not self._mission_id or self._mission_blocked:
                return
            if self._mission_stage == "aligning_final":
                self._mission_stage = "completed"
                completed_kind = self._mission_kind
            else:
                self._mission_index += 1
            self._mission_command_id = None
            self._mission_command_started_at = None
            self._mission_seq = None
            if completed_kind is None and self._mission_index < len(self._mission_points):
                queue_step = True
            elif completed_kind is None and self._mission_final_heading is not None:
                self._mission_stage = "aligning_final"
                queue_alignment = True
            elif completed_kind is None:
                self._mission_stage = "completed"
                completed_kind = self._mission_kind
        if queue_step:
            self._queue_current_mission_step()
        elif queue_alignment:
            self._queue_final_alignment()
        elif completed_kind == "outbound":
            with self._lock:
                route = {
                    "route_id": self._mission_id,
                    "origin": self._mission_origin,
                    "points": self._mission_points,
                    "return_state": "available",
                    "return_error": None,
                }
            self.database.set_setting("last_completed_route", route)
        elif completed_kind == "ockham_return":
            self._set_return_state("completed")
        self._persist_mission()
        self.events.publish("mission", self.mission_status())

    def _set_return_state(self, state: str, error: str | None = None) -> None:
        route = self.database.get_setting("last_completed_route", None)
        if isinstance(route, dict):
            route["return_state"] = state
            route["return_error"] = error
            self.database.set_setting("last_completed_route", route)

    def _queue_current_mission_step(self, seq_override: int | None = None) -> None:
        with self._lock:
            if not self._mission_id or self._mission_index >= len(self._mission_points):
                return
            start = self._mission_origin if self._mission_index == 0 else self._mission_points[self._mission_index - 1]
            target = self._mission_points[self._mission_index]
            dx = target["x_mm"] - start["x_mm"]
            dy = target["y_mm"] - start["y_mm"]
            reuse_command_id = self._mission_command_id if seq_override is not None else None
        heading = math.degrees(math.atan2(dx, dy)) % 360.0
        distance_cm = math.hypot(dx, dy) / 10.0
        if (abs(dx) > 1.0 and abs(dy) > 1.0) or not 0.5 <= distance_cm <= MAX_SEGMENT_MM / 10.0:
            raise RuntimeError("Segmento de misión no ortogonal o fuera de rango")
        # El waypoint no se reconstruye desde la odometría desviada: el ESP32
        # recibe el punto almacenado en la ruta para poder recuperar el
        # desplazamiento lateral antes de confirmar el paso.
        command = self.send_command("step", {
            "heading": heading, "cm": distance_cm, "drive_mode": "auto",
            "target_x_mm": target["x_mm"], "target_y_mm": target["y_mm"],
        },
                                    seq_override=seq_override, command_id_override=reuse_command_id)
        with self._lock:
            self._mission_command_id = command.command_id
            self._mission_seq = command.seq
            self._mission_command_started_at = None
        self._persist_mission()

    def _queue_final_alignment(self, seq_override: int | None = None) -> None:
        with self._lock:
            heading = self._mission_final_heading
            if not self._mission_id or self._mission_stage != "aligning_final" or heading is None:
                return
            reuse_command_id = self._mission_command_id if seq_override is not None else None
        command = self.send_command("turn_to", {"heading": heading}, seq_override=seq_override,
                                    command_id_override=reuse_command_id)
        with self._lock:
            self._mission_command_id = command.command_id
            self._mission_seq = command.seq
            self._mission_command_started_at = None
        self._persist_mission()

    def _expire_stalled_mission(self) -> None:
        with self._lock:
            started = self._mission_command_started_at
            running = self._mission_command_id is not None
            # Es un límite de coordinación; los watchdogs eléctricos siguen
            # siendo responsabilidad del ESP32.
            expired = (self._mission_stage in {"executing", "aligning_final"} and running and started is not None
                       and time.monotonic() - started >= self.MISSION_SEGMENT_TIMEOUT_S)
        if expired:
            self._block_mission("mission_segment_timeout")

    def _block_mission(self, reason: str) -> None:
        with self._lock:
            if self._mission_id is None or self._mission_stage not in {"executing", "aligning_final"}:
                return
            active_command_id = self._mission_command_id
            active_seq = self._mission_seq
            self._mission_error = reason
            self._mission_blocked = True
            self._mission_stage = "blocked"
            self._mission_command_id = None
            self._mission_command_started_at = None
            self._mission_seq = None
            mission_kind = self._mission_kind
        self.gateway.cancel(active_command_id)
        if active_command_id is not None:
            self._tracked_command_ids.discard(active_command_id)
            if active_seq is not None:
                self._command_ids_by_seq.pop(active_seq, None)
            self.database.update_command(active_command_id, CommandStatus.FAILED.value, reason)
        try:
            self.send_command("stop", {})
        except RuntimeError:
            pass
        if mission_kind == "ockham_return":
            self._set_return_state("blocked", reason)
        self._persist_mission()
        self.events.publish("mission", self.mission_status())

    def _reconcile_mission_snapshot(self, snapshot: TelemetrySnapshot) -> None:
        firmware = snapshot.mission
        if firmware:
            self._reconcile_firmware_mission(firmware)
            return
        with self._lock:
            expected = self._mission_command_id
            blocked = self._mission_blocked
            started = self._mission_command_started_at
        if not expected or blocked:
            return
        active = snapshot.active_command_id
        if active == expected:
            return
        terminal_id = str(snapshot.last_terminal.get("id", ""))
        if terminal_id == expected:
            return
        if active:
            self._block_mission("mission_active_command_mismatch")
            return
        elapsed = time.monotonic() - started if started is not None else 0.0
        if elapsed >= 2.0 and snapshot.state.upper() in {
            "IDLE", "FAULT", "UNCALIBRATED", "ESTOP_LATCHED", "SAFE_STOP_COMMS"
        }:
            self._block_mission("mission_reconciliation_missing_command")

    def _reconcile_firmware_mission(self, firmware: dict[str, Any]) -> None:
        firmware_id = str(firmware.get("id") or "")
        revision = int(firmware.get("revision", 0) or 0)
        changed = False
        with self._lock:
            if not self._mission_id or firmware_id != self._mission_id or revision != self._mission_revision:
                return
            checkpoint = max(self._mission_index, int(firmware.get("completed_steps", 0) or 0))
            if checkpoint != self._mission_index:
                self._mission_index = checkpoint
                changed = True
            state = str(firmware.get("state", "")).lower()
            if state in {"running", "executing", "between_steps", "loaded"}:
                next_stage = "executing" if state != "loaded" else self._mission_stage
                if next_stage != self._mission_stage:
                    self._mission_stage = next_stage
                    changed = True
            elif state == "completed":
                changed = self._mission_index != len(self._mission_points) or self._mission_stage != "completed"
                self._mission_index, self._mission_stage = len(self._mission_points), "completed"
                self._mission_command_id = None
                self._mission_command_started_at = None
            elif state in {"interrupted", "checkpoint_only"}:
                changed = (self._mission_stage != "interrupted" or not self._mission_blocked or
                           self._mission_error != "robot_restarted_mid_mission")
                self._mission_stage = "interrupted"
                self._mission_blocked = True
                self._mission_error = "robot_restarted_mid_mission"
                self._mission_command_id = None
                self._mission_command_started_at = None
        if changed:
            self._persist_mission()
            self.events.publish("mission", self.mission_status())

    def send_command(self, name: object, payload: dict[str, Any] | None,
                     seq_override: int | None = None, command_id_override: str | None = None) -> RobotCommand:
        normalized_name = str(name or "").lower()
        with self._lock:
            if self._closed:
                raise RuntimeError("La aplicación ya está cerrada")
            if self._closing and normalized_name not in {"stop", "estop"}:
                raise RuntimeError("La aplicación está preparando su cierre")
            if self._startup_stop_required and normalized_name not in {"stop", "estop"}:
                raise RuntimeError("La parada de reconciliación tras reinicio aún no ha sido confirmada")
            seq = seq_override if seq_override is not None else self._next_seq
            if seq_override is None:
                self._next_seq += 1
                self.database.set_setting("next_command_seq", self._next_seq)
        command = RobotCommand.create(name, payload, seq=seq, command_id=command_id_override)
        with self._lock:
            session_id = self._session_id
        now = time.monotonic()
        persist = command.name != "manual" or now - self._last_manual_recorded_at >= 1.0
        if persist and command_id_override is None:
            self.database.insert_command(command.command_id, session_id, command.name, command.payload, CommandStatus.QUEUED.value)
        if persist:
            self._tracked_command_ids.add(command.command_id)
            self._command_ids_by_seq[command.seq] = command.command_id
            if command.name == "stop":
                self._command_terminal_events.setdefault(command.command_id, threading.Event())
            if command.name == "manual":
                self._last_manual_recorded_at = now
        if not self.gateway.enqueue(command):
            if persist:
                self.database.update_command(command.command_id, CommandStatus.FAILED.value, "outgoing_queue_full")
            raise RuntimeError("La cola de comandos está llena")
        if command.name != "manual" or persist:
            self.events.publish("command", {"id": command.command_id, "name": command.name, "status": "queued"})
        return command

    def start_session(self) -> int:
        with self._lock:
            if self._session_id is None:
                self._session_id = self.database.create_session()
                self._identity_session_id = None
            session_id = self._session_id
        self.events.publish("session", {"recording": True, "session_id": session_id})
        return session_id

    def stop_session(self, reason: str | None = None) -> int | None:
        with self._lock:
            session_id = self._session_id
            self._session_id = None
        if session_id is not None:
            self.database.stop_session(session_id, reason)
        self.events.publish("session", {"recording": False, "session_id": session_id})
        return session_id

    def _on_connection_state(self, state: ConnectionState, detail: str | None) -> None:
        payload = {"state": state.value, "detail": detail}
        self.events.publish("connection", payload)
        if state == ConnectionState.CONNECTED:
            self.start_session()
            self.database.insert_event(self._session_id, "connection", Severity.INFO.value, payload)
        elif state in {ConnectionState.STOPPED, ConnectionState.BACKOFF}:
            if state == ConnectionState.BACKOFF:
                self.database.insert_event(self._session_id, "connection", Severity.WARNING.value, payload)
            self.stop_session(detail or state.value)

    def _on_command_sent(self, command: RobotCommand) -> None:
        if command.command_id in self._tracked_command_ids:
            self.database.update_command(command.command_id, CommandStatus.SENT.value)
        with self._lock:
            if command.command_id == self._mission_command_id:
                self._mission_command_started_at = time.monotonic()
        if command.name != "manual" and command.command_id in self._tracked_command_ids:
            self.events.publish("command", {"id": command.command_id, "name": command.name, "status": "sent"})

    def _on_robot_message(self, message: dict[str, Any]) -> None:
        kind = str(message.get("evt", message.get("type", "")))
        if kind in {"welcome", "hello_ack"}:
            self.events.publish("robot_event", message)
            if kind == "hello_ack":
                self._reconcile_short_memory_hello(message)
            return
        is_telemetry = kind == "telemetry"
        if is_telemetry:
            self._telemetry_sequence += 1
            snapshot = TelemetrySnapshot.from_message(message, self._telemetry_sequence)
            with self._lock:
                self._last_telemetry = snapshot
                session_id = self._session_id
            if session_id is not None and self._identity_session_id != session_id:
                self.database.update_session_identity(
                    session_id, snapshot.robot_id, snapshot.firmware_version, self.gateway.snapshot()["protocol"]
                )
                if snapshot.robot_id or snapshot.firmware_version:
                    self._identity_session_id = session_id
            public_snapshot = snapshot.public_dict()
            public_snapshot["protocol"] = self.gateway.snapshot()["protocol"]
            self.events.publish("telemetry", public_snapshot)
            self._expire_stalled_mission()
            if session_id is not None and snapshot.received_at - self._last_recorded_at >= 0.2:
                self._last_recorded_at = snapshot.received_at
                self._recorder.submit(session_id, snapshot)
            return

        seq = int(message.get("seq", 0) or 0)
        command_id = self._command_ids_by_seq.get(seq, "")
        status_map = {
            "accepted": CommandStatus.ACKNOWLEDGED,
            "completed": CommandStatus.COMPLETED,
            "already_done": CommandStatus.COMPLETED,
            "rejected": CommandStatus.REJECTED,
            "fault": CommandStatus.FAILED,
        }
        if command_id and kind in status_map:
            terminal_detail = str(message.get("reason") or message.get("detail") or "") or None
            self.database.update_command(command_id, status_map[kind].value, terminal_detail)
            if kind in {"completed", "already_done", "rejected", "fault"}:
                self._last_terminal_id = command_id
            if kind in {"completed", "already_done", "rejected", "fault"}:
                waiter = self._command_terminal_events.get(command_id)
                if waiter is not None:
                    self._command_terminal_results[command_id] = (kind, terminal_detail)
                    waiter.set()
                self._tracked_command_ids.discard(command_id)
                self._command_ids_by_seq.pop(seq, None)
        severity = Severity.ERROR if kind == "fault" else Severity.INFO
        self.database.insert_event(self._session_id, kind or "message", severity.value, message)
        public_event = {**message, "type": kind, "id": command_id or None}
        self.events.publish("robot_event", public_event)
        with self._lock:
            mission_match = bool(command_id and command_id == self._mission_command_id)
            startup_stop_match = bool(command_id and command_id == self._startup_stop_command_id)
        if startup_stop_match and kind in {"completed", "already_done", "rejected", "fault"}:
            detail = str(message.get("detail") or message.get("reason") or "") or None
            stop_confirmed = kind == "already_done" or (kind == "completed" and detail == "stop_ok")
            with self._lock:
                self._startup_stop_command_id = None
                if stop_confirmed:
                    self._startup_stop_required = False
            self._command_terminal_events.pop(command_id, None)
            self._command_terminal_results.pop(command_id, None)
            if stop_confirmed:
                self.database.insert_event(self._session_id, "app_restart_stop_confirmed", Severity.INFO.value, {
                    "command_id": command_id, "detail": detail,
                })
                self.events.publish("lifecycle", {"controls_ready": True, "reason": "startup_stop_confirmed"})
            else:
                self.database.insert_event(self._session_id, "app_restart_stop_unconfirmed", Severity.ERROR.value, {
                    "command_id": command_id, "event": kind, "detail": detail,
                })
        if mission_match and kind in {"completed", "already_done"}:
            self._advance_mission()
        elif mission_match and kind in {"rejected", "fault"}:
            self._block_mission(str(message.get("detail") or message.get("reason") or kind))

    def _reconcile_short_memory_hello(self, message: dict[str, Any]) -> None:
        with self._lock:
            startup_stop_required = self._startup_stop_required
            startup_stop_sent = self._startup_stop_command_id is not None
        if startup_stop_required:
            if not startup_stop_sent:
                command = self.send_command("stop", {})
                with self._lock:
                    self._startup_stop_command_id = command.command_id
                self.database.insert_event(self._session_id, "app_restart_stop_queued", Severity.WARNING.value, {
                    "command_id": command.command_id,
                })
            return
        last_seq = int(message.get("last_seq", 0) or 0)
        state = str(message.get("state", "")).lower()
        with self._lock:
            mission_seq = self._mission_seq
            aligning_final = self._mission_stage == "aligning_final"
            has_mission = bool(self._mission_id and (
                self._mission_index < len(self._mission_points) or aligning_final
            ))
        if not has_mission or mission_seq is None:
            return
        if last_seq >= mission_seq:
            with self._lock:
                command_id = self._mission_command_id
            if command_id:
                self.database.update_command(command_id, CommandStatus.COMPLETED.value, "reconciled_last_seq")
                self._tracked_command_ids.discard(command_id)
                self._command_ids_by_seq.pop(mission_seq, None)
            self._advance_mission()
            return
        if state in {"ejecutando", "calibrando"}:
            return
        if state == "listo":
            if aligning_final:
                self._queue_final_alignment(seq_override=mission_seq)
            else:
                self._queue_current_mission_step(seq_override=mission_seq)
            return
        self._block_mission("robot_restarted_mid_step")

    def _movement_requires_stop(self) -> bool:
        with self._lock:
            mission_active = self._mission_stage in {"executing", "aligning_final"}
            telemetry = self._last_telemetry
            startup_pending = self._startup_stop_required
        if mission_active or startup_pending:
            return True
        if telemetry is None:
            return False
        return telemetry.state.lower() in {"ejecutando", "calibrando"} or any(telemetry.pwm)

    def _archive_mission_for_close(self) -> bool:
        with self._lock:
            if self._mission_id is None or self._mission_stage in {"completed", "abandoned", "idle"}:
                return False
            active_command_id = self._mission_command_id
            self._mission_stage = "abandoned"
            self._mission_blocked = True
            self._mission_error = "operator_app_close"
            self._mission_command_id = None
            self._mission_command_started_at = None
            self._mission_seq = None
            mission_kind = self._mission_kind
        self.gateway.cancel(active_command_id)
        if mission_kind == "ockham_return":
            self._set_return_state("blocked", "operator_app_close")
        self._persist_mission()
        self.events.publish("mission", self.mission_status())
        return True

    def _finish_close_preparation(self, force: bool, stop_required: bool, stop_confirmed: bool) -> dict[str, Any]:
        mission_abandoned = self._archive_mission_for_close()
        failed_commands = self.database.fail_nonterminal_commands(
            "forced_app_close_without_stop_ack" if force and not stop_confirmed else "operator_app_close"
        )
        if stop_confirmed:
            self.database.insert_event(self._session_id, "robot_stop_confirmed", Severity.INFO.value, {})
        if mission_abandoned:
            self.database.insert_event(self._session_id, "mission_abandoned_on_close", Severity.WARNING.value, {
                "mission": self.mission_status(),
            })
        if force and not stop_confirmed:
            self.database.insert_event(self._session_id, "forced_app_close_without_stop_ack", Severity.CRITICAL.value, {
                "stop_required": stop_required,
            })
        self.stop_session("operator_app_close" if not force else "forced_app_close")
        result = {
            "safe_to_close": True,
            "stop_required": stop_required,
            "stop_confirmed": stop_confirmed,
            "forced": force,
            "failed_commands": failed_commands,
            "mission_stage": self._mission_stage,
        }
        with self._lock:
            self._close_prepared = True
            self._close_result = result
        return result

    def prepare_close(self, force: bool = False, timeout_s: float | None = None) -> dict[str, Any]:
        with self._lock:
            if self._close_prepared and self._close_result is not None:
                return dict(self._close_result)
            if self._closing:
                return {"safe_to_close": False, "error": "close_already_in_progress", "retryable": True}
            self._closing = True
            session_id = self._session_id
        stop_required = self._movement_requires_stop()
        self.database.insert_event(session_id, "app_close_requested", Severity.WARNING.value, {
            "force": force, "stop_required": stop_required,
        })
        if force:
            return self._finish_close_preparation(True, stop_required, False)
        if not stop_required:
            return self._finish_close_preparation(False, False, True)
        connection = self.gateway.snapshot()
        if connection["state"] != ConnectionState.CONNECTED.value or connection["protocol"] != self.PROTOCOL:
            with self._lock:
                self._closing = False
            self.database.insert_event(session_id, "app_close_blocked", Severity.ERROR.value, {
                "reason": "stop_not_confirmed", "connection": connection,
            })
            return {"safe_to_close": False, "error": "stop_not_confirmed", "retryable": True,
                    "stop_required": True, "stop_confirmed": False, "forced": False}
        with self._lock:
            command_id = self._startup_stop_command_id
        if command_id is None:
            try:
                command_id = self.send_command("stop", {}).command_id
            except RuntimeError:
                with self._lock:
                    self._closing = False
                return {"safe_to_close": False, "error": "stop_not_confirmed", "retryable": True,
                        "stop_required": True, "stop_confirmed": False, "forced": False}
        waiter = self._command_terminal_events.setdefault(command_id, threading.Event())
        terminal = self._command_terminal_results.get(command_id)
        if terminal is None:
            waiter.wait(self.CLOSE_STOP_TIMEOUT_S if timeout_s is None else max(0.0, timeout_s))
            terminal = self._command_terminal_results.get(command_id)
        self._command_terminal_events.pop(command_id, None)
        self._command_terminal_results.pop(command_id, None)
        confirmed = bool(terminal and (
            terminal[0] == "already_done" or (terminal[0] == "completed" and terminal[1] == "stop_ok")
        ))
        if not confirmed:
            with self._lock:
                self._closing = False
            self.database.insert_event(session_id, "app_close_blocked", Severity.ERROR.value, {
                "reason": "stop_not_confirmed", "command_id": command_id,
            })
            return {"safe_to_close": False, "error": "stop_not_confirmed", "retryable": True,
                    "stop_required": True, "stop_confirmed": False, "forced": False}
        return self._finish_close_preparation(False, True, True)

    def purge_sessions(self, days: int) -> int:
        return self.database.purge_sessions(days)

"""Casos de uso: conexión, comandos, sesiones, eventos y telemetría."""

from __future__ import annotations

import json
import queue
import threading
import time
import uuid
from typing import Any

from .config import DEFAULT_ROBOT_HOST, normalize_robot_host
from .database import Database
from .domain import CommandStatus, ConnectionState, RobotCommand, Severity, TelemetrySnapshot, split_segment_mm
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
        self._mission_id: str | None = None
        self._mission_points: list[dict[str, float]] = []
        self._mission_index = 0
        self._mission_command_id: str | None = None
        self._mission_command_started_at: float | None = None
        self._mission_error: str | None = None
        self._mission_blocked = False
        self._mission_revision = 1
        self._mission_stage = "idle"
        saved_mission = database.get_setting("active_mission", None)
        if isinstance(saved_mission, dict) and saved_mission.get("id") and isinstance(saved_mission.get("points"), list):
            self._mission_id = str(saved_mission["id"])
            self._mission_points = list(saved_mission["points"])
            self._mission_index = int(saved_mission.get("current_index", 0))
            self._mission_revision = int(saved_mission.get("revision", 1))
            self._mission_stage = "reconnecting"
        self._recorder = TelemetryRecorder(database)
        self.gateway = RobotGateway(
            host_getter=self.get_robot_host,
            on_message=self._on_robot_message,
            on_state=self._on_connection_state,
            on_sent=self._on_command_sent,
            max_message_bytes=max_message_bytes,
        )
        if start_gateway:
            self.gateway.start()

    def close(self) -> None:
        self.gateway.stop()
        with self._lock:
            if self._session_id is not None:
                self.database.stop_session(self._session_id)
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
        }

    def mission_status(self) -> dict[str, Any]:
        with self._lock:
            return {
                "id": self._mission_id,
                "running": (
                    self._mission_id is not None
                    and not self._mission_blocked
                    and self._mission_index < len(self._mission_points)
                ),
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
            }

    def _persist_mission(self) -> None:
        with self._lock:
            value = None if self._mission_id is None else {
                "id": self._mission_id, "revision": self._mission_revision,
                "points": self._mission_points, "current_index": self._mission_index,
                "stage": self._mission_stage,
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

    def start_mission(self, points: object) -> dict[str, Any]:
        if not isinstance(points, list) or not points or len(points) > 32:
            raise ValueError("La misión requiere entre 1 y 32 puntos")
        connection = self.gateway.snapshot()
        if connection["state"] != ConnectionState.CONNECTED.value or connection["protocol"] != "v1":
            raise RuntimeError("El robot no está conectado con protocolo v1")
        with self._lock:
            if self._mission_blocked:
                raise RuntimeError("La misión está bloqueada; bórrela antes de iniciar otra")
            if self._mission_id is not None and self._mission_index < len(self._mission_points):
                raise RuntimeError("Ya existe una misión activa")
            telemetry = self._last_telemetry
            if telemetry is None or not telemetry.calibrated:
                raise RuntimeError("El robot debe estar conectado y con la MPU de arranque lista")
            if time.time() - telemetry.received_at > self.TELEMETRY_FRESHNESS_S:
                raise RuntimeError("La telemetría del robot está obsoleta; reconecte antes de iniciar")
            if telemetry.state.upper() != "IDLE":
                raise RuntimeError(f"El robot debe estar en IDLE; estado actual: {telemetry.state}")
            origin_x, origin_y = telemetry.x_mm, telemetry.y_mm
        segments: list[dict[str, float]] = []
        for waypoint_index, point in enumerate(points, start=1):
            if not isinstance(point, dict):
                raise ValueError("Cada punto debe ser un objeto")
            validated = RobotCommand.create("move", point).payload
            dx = validated["x_mm"] - origin_x
            dy = validated["y_mm"] - origin_y
            if abs(dx) > 1.0 and abs(dy) > 1.0:
                raise ValueError(
                    f"Tramo no ortogonal en waypoint {waypoint_index}: "
                    f"cambian X ({dx:.1f} mm) e Y ({dy:.1f} mm)"
                )
            new_segments = split_segment_mm(origin_x, origin_y, validated["x_mm"], validated["y_mm"])
            segments.extend(new_segments)
            origin_x, origin_y = validated["x_mm"], validated["y_mm"]
        if not segments:
            raise ValueError("La misión no contiene desplazamiento")
        if len(segments) > 32:
            raise ValueError("La descomposición excede los 32 segmentos disponibles en el ESP32")
        with self._lock:
            self._mission_id = uuid.uuid4().hex
            self._mission_points = segments
            self._mission_index = 0
            self._mission_command_id = None
            self._mission_command_started_at = None
            self._mission_error = None
            self._mission_blocked = False
            self._mission_revision = 1
            self._mission_stage = "uploading"
            command = self.send_command("mission_upload", {
                "mission_id": self._mission_id, "revision": self._mission_revision,
                "points": self._mission_wire_points(),
            })
            self._mission_command_id = command.command_id
            self._mission_command_started_at = None
        self._persist_mission()
        snapshot = self.mission_status()
        self.events.publish("mission", snapshot)
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
            # DELETE descarta la ruta. No republicar un fallo anterior como si
            # lo hubiera provocado una orden manual posterior.
            self._mission_error = None
            self._mission_id = None
            self._mission_points = []
            self._mission_index = 0
            self._mission_command_id = None
            self._mission_command_started_at = None
            self._mission_blocked = False
            self._mission_stage = "idle"
        self.gateway.cancel(active_command_id)
        if active_command_id:
            self._tracked_command_ids.discard(active_command_id)
            self.database.update_command(active_command_id, CommandStatus.FAILED.value, reason)
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
        """Detiene la ejecución y borra ruta/checkpoint tanto en ESP32 como en SQLite."""
        try:
            command = self.send_command("mission_clear", {})
            command_id = command.command_id
        except RuntimeError:
            command_id = None
        with self._lock:
            self._mission_id = None
            self._mission_points = []
            self._mission_index = 0
            self._mission_command_id = None
            self._mission_command_started_at = None
            self._mission_error = None
            self._mission_blocked = False
            self._mission_stage = "idle"
        self._persist_mission()
        snapshot = self.mission_status()
        snapshot["clear_command_id"] = command_id
        self.events.publish("mission", snapshot)
        return snapshot

    def _advance_mission(self) -> None:
        # Compatibilidad interna: la ruta completa vive en el ESP32; Python ya
        # no alimenta un waypoint por cada reconexión.
        self.events.publish("mission", self.mission_status())

    def _expire_stalled_mission(self) -> None:
        with self._lock:
            started = self._mission_command_started_at
            running = self._mission_command_id is not None
            # Durante EXECUTING manda el ESP32 y sus watchdogs físicos; Python
            # nunca detiene una ruta sólo porque el WebSocket tardó o cayó.
            expired = (self._mission_stage == "uploading" and running and started is not None
                       and time.monotonic() - started >= self.MISSION_SEGMENT_TIMEOUT_S)
        if expired:
            self._block_mission("mission_segment_timeout")

    def _block_mission(self, reason: str) -> None:
        with self._lock:
            if self._mission_id is None or self._mission_index >= len(self._mission_points):
                return
            self._mission_error = reason
            self._mission_blocked = True
            self._mission_command_id = None
            self._mission_command_started_at = None
        try:
            self.send_command("stop", {})
        except RuntimeError:
            pass
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

    def send_command(self, name: object, payload: dict[str, Any] | None) -> RobotCommand:
        command = RobotCommand.create(name, payload)
        with self._lock:
            session_id = self._session_id
        now = time.monotonic()
        persist = command.name != "manual" or now - self._last_manual_recorded_at >= 1.0
        if persist:
            self.database.insert_command(command.command_id, session_id, command.name, command.payload, CommandStatus.QUEUED.value)
            self._tracked_command_ids.add(command.command_id)
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
        kind = str(message.get("type", ""))
        if kind in {"heartbeat_ack", "hello", "hello_ack"}:
            # Mensajes de control del enlace: no llenar SQLite a 5 Hz.
            if kind in {"hello", "hello_ack"}:
                mission = message.get("mission")
                if isinstance(mission, dict):
                    self._reconcile_firmware_mission(mission)
                self.events.publish("robot_event", message)
            return
        is_telemetry = kind == "telemetry" or any(key in message for key in ("s", "pfl", "x"))
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
            terminal_id = str(snapshot.last_terminal.get("id", ""))
            terminal_reconciled = False
            if terminal_id and terminal_id != self._last_terminal_id:
                self._last_terminal_id = terminal_id
                reconciled = dict(snapshot.last_terminal)
                reconciled["reconciled"] = True
                self._on_robot_message(reconciled)
                terminal_reconciled = True
            if not terminal_reconciled:
                self._reconcile_mission_snapshot(snapshot)
                self._expire_stalled_mission()
            if session_id is not None and snapshot.received_at - self._last_recorded_at >= 0.2:
                self._last_recorded_at = snapshot.received_at
                self._recorder.submit(session_id, snapshot)
            return

        command_id = str(message.get("id", ""))
        if str(message.get("cmd", "")) == "manual" and command_id not in self._tracked_command_ids:
            return
        status_map = {
            "accepted": CommandStatus.ACKNOWLEDGED,
            "queued": CommandStatus.ACKNOWLEDGED,
            "completed": CommandStatus.COMPLETED,
            "done": CommandStatus.COMPLETED,
            "rejected": CommandStatus.REJECTED,
            "error": CommandStatus.FAILED,
            "fault": CommandStatus.FAILED,
        }
        if command_id and kind in status_map:
            terminal_detail = str(message.get("reason") or message.get("detail") or "") or None
            self.database.update_command(command_id, status_map[kind].value, terminal_detail)
            if kind in {"completed", "rejected", "error", "fault"}:
                self._last_terminal_id = command_id
            if kind in {"completed", "rejected", "error", "fault"} or str(message.get("cmd", "")) == "manual":
                self._tracked_command_ids.discard(command_id)
        severity = Severity.ERROR if kind in {"error", "fault"} else Severity.INFO
        self.database.insert_event(self._session_id, kind or "message", severity.value, message)
        self.events.publish("robot_event", message)
        with self._lock:
            mission_match = bool(command_id and command_id == self._mission_command_id)
        if mission_match and kind == "completed":
            with self._lock:
                stage = self._mission_stage
                mission_id = self._mission_id
                revision = self._mission_revision
            if stage == "uploading" and mission_id:
                command = self.send_command("mission_start", {"mission_id": mission_id, "revision": revision})
                with self._lock:
                    self._mission_stage = "executing"
                    self._mission_command_id = command.command_id
                    self._mission_command_started_at = None
                self._persist_mission()
                self.events.publish("mission", self.mission_status())
            elif stage == "executing":
                with self._lock:
                    self._mission_index = len(self._mission_points)
                    self._mission_stage = "completed"
                    self._mission_command_id = None
                    self._mission_command_started_at = None
                self._persist_mission()
                self.events.publish("mission", self.mission_status())
        elif mission_match and kind in {"rejected", "fault", "error"}:
            self._block_mission(str(message.get("detail") or message.get("reason") or kind))

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
        self._command_ids_by_seq: dict[int, str] = {}
        saved_controller_session = str(database.get_setting("controller_session", "") or "")
        self._controller_session = saved_controller_session if len(saved_controller_session) == 16 else uuid.uuid4().hex[:16]
        self._next_seq = max(1, int(database.get_setting("next_command_seq", 1) or 1))
        database.set_setting("controller_session", self._controller_session)
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
        saved_mission = database.get_setting("active_mission", None)
        if isinstance(saved_mission, dict) and saved_mission.get("id") and isinstance(saved_mission.get("points"), list):
            self._mission_id = str(saved_mission["id"])
            self._mission_points = list(saved_mission["points"])
            self._mission_index = int(saved_mission.get("current_index", 0))
            self._mission_revision = int(saved_mission.get("revision", 1))
            self._mission_stage = "reconnecting"
            origin = saved_mission.get("origin", {})
            if isinstance(origin, dict):
                self._mission_origin = {"x_mm": float(origin.get("x_mm", 0)), "y_mm": float(origin.get("y_mm", 0))}
            self._mission_seq = int(saved_mission.get("active_seq", 0) or 0) or None
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
                "active_seq": self._mission_seq,
            }

    def _persist_mission(self) -> None:
        with self._lock:
            value = None if self._mission_id is None else {
                "id": self._mission_id, "revision": self._mission_revision,
                "points": self._mission_points, "current_index": self._mission_index,
                "stage": self._mission_stage,
                "origin": self._mission_origin, "active_seq": self._mission_seq,
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
        if connection["state"] != ConnectionState.CONNECTED.value or connection["protocol"] != "steps-v2":
            raise RuntimeError("El robot no está conectado con protocolo robot-s3-steps-v2")
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
            if telemetry.state.lower() != "listo":
                raise RuntimeError(f"El robot debe estar LISTO; estado actual: {telemetry.state}")
            origin_x, origin_y = telemetry.x_mm, telemetry.y_mm
            mission_origin = {"x_mm": origin_x, "y_mm": origin_y}
        segments: list[dict[str, float]] = []
        for waypoint_index, point in enumerate(points, start=1):
            if not isinstance(point, dict):
                raise ValueError("Cada punto debe ser un objeto")
            validated = RobotCommand.create("move", point).payload
            target_x, target_y = validated["x_mm"], validated["y_mm"]
            dx = target_x - origin_x
            dy = target_y - origin_y
            if abs(dx) > 1.0 and abs(dy) > 1.0:
                raise ValueError(
                    f"Tramo no ortogonal en waypoint {waypoint_index}: "
                    f"cambian X ({dx:.1f} mm) e Y ({dy:.1f} mm)"
                )
            if abs(dx) <= 1.0:
                target_x = origin_x
            if abs(dy) <= 1.0:
                target_y = origin_y
            new_segments = split_segment_mm(origin_x, origin_y, target_x, target_y)
            segments.extend(new_segments)
            origin_x, origin_y = target_x, target_y
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
            self._mission_origin = mission_origin
            self._mission_seq = None
            self._mission_error = None
            self._mission_blocked = False
            self._mission_revision = 1
            self._mission_stage = "executing"
        self._queue_current_mission_step()
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
            self._mission_seq = None
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
        """Detiene el paso y borra la misión que pertenece exclusivamente a Python."""
        snapshot = self.stop_mission("operator_clear")
        snapshot["clear_command_id"] = None
        return snapshot

    def _advance_mission(self) -> None:
        with self._lock:
            if not self._mission_id or self._mission_blocked:
                return
            self._mission_index += 1
            self._mission_command_id = None
            self._mission_command_started_at = None
            self._mission_seq = None
            terminado = self._mission_index >= len(self._mission_points)
            if terminado:
                self._mission_stage = "completed"
        if not terminado:
            self._queue_current_mission_step()
        self._persist_mission()
        self.events.publish("mission", self.mission_status())

    def _queue_current_mission_step(self, seq_override: int | None = None) -> None:
        with self._lock:
            if not self._mission_id or self._mission_index >= len(self._mission_points):
                return
            start = self._mission_origin if self._mission_index == 0 else self._mission_points[self._mission_index - 1]
            target = self._mission_points[self._mission_index]
            dx = target["x_mm"] - start["x_mm"]
            dy = target["y_mm"] - start["y_mm"]
        heading = math.degrees(math.atan2(dx, dy)) % 360.0
        distance_cm = math.hypot(dx, dy) / 10.0
        command = self.send_command("step", {"heading": heading, "cm": distance_cm}, seq_override=seq_override)
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
            expired = (self._mission_stage == "executing" and running and started is not None
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

    def send_command(self, name: object, payload: dict[str, Any] | None,
                     seq_override: int | None = None) -> RobotCommand:
        with self._lock:
            seq = seq_override if seq_override is not None else self._next_seq
            if seq_override is None:
                self._next_seq += 1
                self.database.set_setting("next_command_seq", self._next_seq)
        command = RobotCommand.create(name, payload, seq=seq)
        with self._lock:
            session_id = self._session_id
        now = time.monotonic()
        persist = command.name != "manual" or now - self._last_manual_recorded_at >= 1.0
        if persist:
            self.database.insert_command(command.command_id, session_id, command.name, command.payload, CommandStatus.QUEUED.value)
            self._tracked_command_ids.add(command.command_id)
            self._command_ids_by_seq[command.seq] = command.command_id
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
                self._tracked_command_ids.discard(command_id)
                self._command_ids_by_seq.pop(seq, None)
        severity = Severity.ERROR if kind == "fault" else Severity.INFO
        self.database.insert_event(self._session_id, kind or "message", severity.value, message)
        public_event = {**message, "type": kind, "id": command_id or None}
        self.events.publish("robot_event", public_event)
        with self._lock:
            mission_match = bool(command_id and command_id == self._mission_command_id)
        if mission_match and kind in {"completed", "already_done"}:
            self._advance_mission()
        elif mission_match and kind in {"rejected", "fault"}:
            self._block_mission(str(message.get("detail") or message.get("reason") or kind))

    def _reconcile_short_memory_hello(self, message: dict[str, Any]) -> None:
        last_seq = int(message.get("last_seq", 0) or 0)
        state = str(message.get("state", "")).lower()
        with self._lock:
            mission_seq = self._mission_seq
            has_mission = bool(self._mission_id and self._mission_index < len(self._mission_points))
        if not has_mission or mission_seq is None:
            return
        if last_seq >= mission_seq:
            with self._lock:
                command_id = self._mission_command_id
            if command_id:
                self.database.update_command(command_id, CommandStatus.COMPLETED.value, "reconciled_last_seq")
            self._advance_mission()
            return
        if state in {"ejecutando", "calibrando"}:
            return
        if state == "listo":
            self._queue_current_mission_step(seq_override=mission_seq)
            return
        self._block_mission("robot_restarted_mid_step")

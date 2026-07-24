"""Único propietario del WebSocket hacia el ESP32."""

from __future__ import annotations

import itertools
import json
import logging
import queue
import random
import socket
import threading
import time
from collections.abc import Callable
from dataclasses import dataclass, field
from typing import Any

import websocket

from .domain import ConnectionState, RobotCommand


logger = logging.getLogger(__name__)


@dataclass(order=True, slots=True)
class _Outgoing:
    priority: int
    sequence: int
    command: RobotCommand = field(compare=False)


class RobotGateway:
    """Serializa conexión, recepción y envío en una sola hebra."""

    MAX_CONNECT_ATTEMPTS = 5

    def __init__(
        self,
        host_getter: Callable[[], str],
        on_message: Callable[[dict[str, Any]], None],
        on_state: Callable[[ConnectionState, str | None], None],
        on_sent: Callable[[RobotCommand], None],
        session_getter: Callable[[], str] = lambda: "",
        max_message_bytes: int = 4096,
    ) -> None:
        self._host_getter = host_getter
        self._on_message = on_message
        self._on_state = on_state
        self._on_sent = on_sent
        self._session_getter = session_getter
        self._max_message_bytes = max_message_bytes
        self._outgoing: queue.PriorityQueue[_Outgoing] = queue.PriorityQueue(maxsize=64)
        self._cancelled: set[str] = set()
        self._cancelled_lock = threading.Lock()
        self._counter = itertools.count()
        self._stop = threading.Event()
        self._reconnect = threading.Event()
        self._thread: threading.Thread | None = None
        self._state_lock = threading.Lock()
        self._state = ConnectionState.STOPPED
        self._detail: str | None = None
        self._protocol_v1 = False
        self._last_heartbeat_ack: float | None = None
        self._handshake_started: float | None = None
        self._connect_attempt = 0

    def start(self) -> None:
        if self._thread and self._thread.is_alive():
            if self.snapshot()["state"] == ConnectionState.STOPPED.value:
                self._reconnect.set()
            return
        self._stop.clear()
        self._reconnect.clear()
        self._thread = threading.Thread(target=self._run, name="robot-websocket", daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()
        self._reconnect.set()
        if self._thread and self._thread.is_alive():
            self._thread.join(timeout=3.0)
        self._set_state(ConnectionState.STOPPED, None)

    def request_reconnect(self) -> None:
        if self._thread and self._thread.is_alive():
            self._reconnect.set()

    def enqueue(self, command: RobotCommand) -> bool:
        priority = 0 if command.name == "estop" else 10
        try:
            self._outgoing.put_nowait(_Outgoing(priority, next(self._counter), command))
            return True
        except queue.Full:
            return False

    def cancel(self, command_id: str | None) -> None:
        if command_id:
            with self._cancelled_lock:
                self._cancelled.add(command_id)

    def snapshot(self) -> dict[str, Any]:
        with self._state_lock:
            heartbeat_age_ms = (
                int((time.monotonic() - self._last_heartbeat_ack) * 1000)
                if self._last_heartbeat_ack is not None else None
            )
            return {
                "state": self._state.value,
                "detail": self._detail,
                "protocol": "steps-v2" if self._protocol_v1 else "negotiating",
                "protocol_name": "robot-s3-steps-v2",
                "heartbeat_age_ms": heartbeat_age_ms,
                "connect_attempt": self._connect_attempt,
                "connect_attempt_max": self.MAX_CONNECT_ATTEMPTS,
            }

    def _set_state(self, state: ConnectionState, detail: str | None) -> None:
        with self._state_lock:
            self._state, self._detail = state, detail
        log = logger.warning if state == ConnectionState.BACKOFF else logger.info
        log("WebSocket robot: %s%s", state.value, f" - {detail}" if detail else "")
        self._on_state(state, detail)

    def _run(self) -> None:
        delay = 0.5
        attempts = 0
        while not self._stop.is_set():
            host = self._host_getter()
            url = f"ws://{host}/ws"
            attempts += 1
            with self._state_lock:
                self._connect_attempt = attempts
            self._set_state(ConnectionState.CONNECTING, url)
            connection = None
            try:
                connection = websocket.create_connection(url, timeout=3, enable_multithread=True)
                connection.settimeout(0.1)
                self._protocol_v1 = False
                self._last_heartbeat_ack = None
                self._handshake_started = time.monotonic()
                self._reconnect.clear()
                delay = 0.5
                attempts = 0
                with self._state_lock:
                    self._connect_attempt = 0
                self._set_state(ConnectionState.CONNECTED, url)
                connection.send(json.dumps(
                    {"cmd": "hello", "session": self._session_getter(), "seq": 0},
                    separators=(",", ":"),
                ))
                while not self._stop.is_set() and not self._reconnect.is_set():
                    self._drain_one(connection)
                    try:
                        raw = connection.recv()
                        if raw:
                            self._receive(raw)
                    except (websocket.WebSocketTimeoutException, TimeoutError, socket.timeout):
                        pass
                    if not self._protocol_v1 and self._handshake_started is not None:
                        if time.monotonic() - self._handshake_started > 3.0:
                            raise RuntimeError("El robot no confirmó robot-s3-steps-v2")
            except Exception as exc:  # frontera de E/S: se reporta sin derribar la app
                logger.warning("Fallo del WebSocket hacia %s: %s", host, exc, exc_info=True)
                if attempts >= self.MAX_CONNECT_ATTEMPTS:
                    self._set_state(
                        ConnectionState.STOPPED,
                        f"No se alcanzó {host}. Conecta el dispositivo a "
                        "ROBOT_S3_LOCAL y pulsa Conectar.",
                    )
                else:
                    self._set_state(
                        ConnectionState.BACKOFF,
                        f"Intento {attempts}/{self.MAX_CONNECT_ATTEMPTS}: "
                        f"{str(exc)[:100]}",
                    )
            finally:
                if connection is not None:
                    try:
                        connection.close()
                    except Exception:
                        pass
            if self._stop.is_set():
                break
            if attempts >= self.MAX_CONNECT_ATTEMPTS:
                # No dejar la UI eternamente en REINTENTANDO. La hebra queda
                # dormida y un nuevo clic en Conectar la reactiva de inmediato.
                self._reconnect.wait()
                self._reconnect.clear()
                attempts = 0
                delay = 0.5
                continue
            reconnect_requested = self._reconnect.wait(
                delay + random.uniform(0, delay * 0.2)
            )
            self._reconnect.clear()
            if reconnect_requested:
                attempts = 0
                delay = 0.5
            else:
                delay = min(delay * 2, 10.0)

    def _drain_one(self, connection: Any) -> None:
        if not self._protocol_v1:
            return
        while True:
            try:
                item = self._outgoing.get_nowait()
            except queue.Empty:
                return
            with self._cancelled_lock:
                cancelled = item.command.command_id in self._cancelled
                if cancelled:
                    self._cancelled.discard(item.command.command_id)
            if not cancelled:
                break
        envelope = item.command.protocol_envelope()
        try:
            connection.send(json.dumps(envelope, separators=(",", ":")))
            self._on_sent(item.command)
        except Exception:
            self._outgoing.put_nowait(item)
            raise

    def _receive(self, raw: str | bytes) -> None:
        encoded = raw.encode("utf-8") if isinstance(raw, str) else raw
        if len(encoded) > self._max_message_bytes:
            raise ValueError("Mensaje del robot demasiado grande")
        message = json.loads(encoded.decode("utf-8"))
        if not isinstance(message, dict):
            raise ValueError("Mensaje del robot no es un objeto JSON")
        kind = message.get("evt")
        if kind == "hello_ack":
            if message.get("protocol") != "robot-s3-steps-v2":
                raise ValueError("Protocolo del robot incompatible")
            if message.get("session") != self._session_getter():
                raise ValueError("El robot confirmó una sesión distinta")
            self._protocol_v1 = True
        if kind in {"rejected", "error", "fault"}:
            logger.error("Evento del robot %s: %s", kind, message)
        self._on_message(message)

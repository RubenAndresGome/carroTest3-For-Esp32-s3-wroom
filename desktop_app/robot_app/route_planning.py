"""Compilación de pasos lógicos a segmentos atómicos de misión."""

from __future__ import annotations

import math
from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Any, Mapping

from .domain import MAX_SEGMENT_MM, RobotCommand, split_segment_mm


MAX_COMPILED_SEGMENTS = 32
MAX_VECTOR_LENGTH_CM = 10_000.0


def _finite_number(value: object, name: str, low: float, high: float) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ValueError(f"{name} debe ser numérico")
    result = float(value)
    if not math.isfinite(result) or not low <= result <= high:
        raise ValueError(f"{name} debe estar entre {low:g} y {high:g}")
    return result


def normalize_signed_degrees(value: float) -> float:
    normalized = (value + 180.0) % 360.0 - 180.0
    return 0.0 if abs(normalized) < 1e-12 else normalized


@dataclass(frozen=True, slots=True)
class RouteCompilation:
    mode: str
    logical_steps: list[dict[str, Any]]
    segments: list[dict[str, Any]]


class RouteExecutionStrategy(ABC):
    """Convierte una representación lógica en segmentos ejecutables comunes."""

    mode: str

    @abstractmethod
    def compile(self, request: Mapping[str, Any], origin: Mapping[str, float]) -> RouteCompilation:
        raise NotImplementedError

    @staticmethod
    def _validated_origin(origin: Mapping[str, float]) -> tuple[float, float]:
        return float(origin["x_mm"]), float(origin["y_mm"])

    @staticmethod
    def _finish(mode: str, logical_steps: list[dict[str, Any]],
                segments: list[dict[str, Any]]) -> RouteCompilation:
        if not segments:
            raise ValueError("La misión no contiene desplazamiento")
        if len(segments) > MAX_COMPILED_SEGMENTS:
            raise ValueError("La compilación excede los 32 segmentos disponibles en el ESP32")
        return RouteCompilation(mode=mode, logical_steps=logical_steps, segments=segments)

    @staticmethod
    def _append_segment(
        segments: list[dict[str, Any]], start_x: float, start_y: float,
        target_x: float, target_y: float, logical_step_id: int, component: str,
    ) -> tuple[float, float]:
        previous_x, previous_y = start_x, start_y
        for target in split_segment_mm(start_x, start_y, target_x, target_y):
            dx = target["x_mm"] - previous_x
            dy = target["y_mm"] - previous_y
            distance_mm = math.hypot(dx, dy)
            segments.append({
                "x_mm": target["x_mm"],
                "y_mm": target["y_mm"],
                "logical_step_id": logical_step_id,
                "component": component,
                "heading_deg": math.degrees(math.atan2(dx, dy)) % 360.0,
                "length_mm": distance_mm,
            })
            previous_x, previous_y = target["x_mm"], target["y_mm"]
        return target_x, target_y


class RectangularRouteStrategy(RouteExecutionStrategy):
    """Contrato histórico: puntos absolutos y descomposición diagonal X→Y."""

    mode = "rectangular"

    def compile(self, request: Mapping[str, Any], origin: Mapping[str, float]) -> RouteCompilation:
        points = request.get("points")
        if not isinstance(points, list) or not points or len(points) > MAX_COMPILED_SEGMENTS:
            raise ValueError("La misión requiere entre 1 y 32 puntos")
        x_mm, y_mm = self._validated_origin(origin)
        logical_steps: list[dict[str, Any]] = []
        segments: list[dict[str, Any]] = []
        for logical_id, point in enumerate(points, start=1):
            if not isinstance(point, Mapping):
                raise ValueError("Cada punto debe ser un objeto")
            validated = RobotCommand.create("move", point).payload
            target_x, target_y = validated["x_mm"], validated["y_mm"]
            start_x, start_y = x_mm, y_mm
            dx, dy = target_x - start_x, target_y - start_y
            logical_steps.append({
                "id": logical_id, "type": "rectangular",
                "origin": {"x_mm": start_x, "y_mm": start_y},
                "target": {"x_mm": target_x, "y_mm": target_y},
                "dx_mm": dx, "dy_mm": dy,
            })
            if abs(dx) > 1.0 and abs(dy) > 1.0:
                x_mm, y_mm = self._append_segment(
                    segments, x_mm, y_mm, target_x, y_mm, logical_id, "x",
                )
                x_mm, y_mm = self._append_segment(
                    segments, x_mm, y_mm, target_x, target_y, logical_id, "y",
                )
            else:
                if abs(dx) <= 1.0:
                    target_x = x_mm
                if abs(dy) <= 1.0:
                    target_y = y_mm
                component = "x" if abs(target_x - x_mm) > 1.0 else "y"
                x_mm, y_mm = self._append_segment(
                    segments, x_mm, y_mm, target_x, target_y, logical_id, component,
                )
        return self._finish(self.mode, logical_steps, segments)


class _AngularRouteStrategy(RouteExecutionStrategy):
    direct = False

    def compile(self, request: Mapping[str, Any], origin: Mapping[str, float]) -> RouteCompilation:
        vectors = request.get("vectors")
        if not isinstance(vectors, list) or not vectors or len(vectors) > MAX_COMPILED_SEGMENTS:
            raise ValueError("La misión angular requiere entre 1 y 32 vectores")
        x_mm, y_mm = self._validated_origin(origin)
        phi_unwrapped = 0.0
        logical_steps: list[dict[str, Any]] = []
        segments: list[dict[str, Any]] = []
        for logical_id, vector in enumerate(vectors, start=1):
            if not isinstance(vector, Mapping):
                raise ValueError("Cada vector debe ser un objeto")
            length_cm = _finite_number(
                vector.get("length_cm"), "length_cm", 0.0, MAX_VECTOR_LENGTH_CM,
            )
            theta_deg = _finite_number(
                vector.get("relative_angle_deg"), "relative_angle_deg", -180.0, 180.0,
            )
            reference_deg = normalize_signed_degrees(phi_unwrapped)
            phi_unwrapped += theta_deg
            phi_deg = normalize_signed_degrees(phi_unwrapped)
            radians = math.radians(phi_deg)
            dx_mm = length_cm * 10.0 * math.cos(radians)
            dy_mm = length_cm * 10.0 * math.sin(radians)
            if abs(dx_mm) < 1e-9:
                dx_mm = 0.0
            if abs(dy_mm) < 1e-9:
                dy_mm = 0.0
            start_x, start_y = x_mm, y_mm
            target_x, target_y = start_x + dx_mm, start_y + dy_mm
            validated_target = RobotCommand.create(
                "move", {"x_mm": target_x, "y_mm": target_y},
            ).payload
            target_x, target_y = validated_target["x_mm"], validated_target["y_mm"]
            logical_steps.append({
                "id": logical_id, "type": "angular", "length_cm": length_cm,
                "relative_angle_deg": theta_deg, "reference_angle_deg": reference_deg,
                "absolute_angle_deg": phi_deg, "absolute_angle_unwrapped_deg": phi_unwrapped,
                "firmware_heading_deg": (90.0 - phi_deg) % 360.0,
                "origin": {"x_mm": start_x, "y_mm": start_y},
                "target": {"x_mm": target_x, "y_mm": target_y},
                "dx_mm": dx_mm, "dy_mm": dy_mm,
            })
            if length_cm == 0.0:
                continue
            if self.direct:
                x_mm, y_mm = self._append_segment(
                    segments, x_mm, y_mm, target_x, target_y, logical_id, "vector",
                )
            else:
                if abs(dx_mm) > 1.0:
                    x_mm, y_mm = self._append_segment(
                        segments, x_mm, y_mm, target_x, y_mm, logical_id, "x",
                    )
                if abs(dy_mm) > 1.0:
                    x_mm, y_mm = self._append_segment(
                        segments, x_mm, y_mm, target_x, target_y, logical_id, "y",
                    )
        return self._finish(self.mode, logical_steps, segments)


class AngularDecompositionRouteStrategy(_AngularRouteStrategy):
    mode = "angular_decomposition"


class AngularVectorialRouteStrategy(_AngularRouteStrategy):
    mode = "angular_vectorial"
    direct = True


ROUTE_EXECUTION_STRATEGIES: dict[str, RouteExecutionStrategy] = {
    "rectangular": RectangularRouteStrategy(),
    "angular_decomposition": AngularDecompositionRouteStrategy(),
    "angular_vectorial": AngularVectorialRouteStrategy(),
}


def strategy_for_request(request: Mapping[str, Any]) -> RouteExecutionStrategy:
    raw_mode = request.get("mode", "rectangular")
    if raw_mode == "polar":
        raw_mode = "angular_decomposition"
    mode = str(raw_mode or "rectangular")
    strategy = ROUTE_EXECUTION_STRATEGIES.get(mode)
    if strategy is None:
        raise ValueError("Modo de misión no permitido")
    return strategy

"""Compilación de pasos lógicos a segmentos atómicos de misión."""

from __future__ import annotations

import math
from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Any, Mapping

from .domain import MAX_SEGMENT_MM, RobotCommand, split_segment_mm


MAX_COMPILED_SEGMENTS = 256
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
            raise ValueError("La compilación excede los 256 segmentos disponibles en el ESP32")
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
            raise ValueError("La misión requiere entre 1 y 256 puntos")
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
            raise ValueError("La misión angular requiere entre 1 y 256 vectores")
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


def simplify_rdp(points: list[dict[str, float]], tolerance_mm: float = 20.0) -> list[dict[str, float]]:
    """Simplifica una polilínea conservando extremos y cambios relevantes."""
    if len(points) <= 2:
        return list(points)
    tolerance = max(0.0, float(tolerance_mm))

    def distance(point: dict[str, float], start: dict[str, float], end: dict[str, float]) -> float:
        dx, dy = end["x_mm"] - start["x_mm"], end["y_mm"] - start["y_mm"]
        if dx == dy == 0:
            return math.hypot(point["x_mm"] - start["x_mm"], point["y_mm"] - start["y_mm"])
        t = max(0.0, min(1.0, ((point["x_mm"] - start["x_mm"]) * dx + (point["y_mm"] - start["y_mm"]) * dy) / (dx * dx + dy * dy)))
        return math.hypot(point["x_mm"] - (start["x_mm"] + t * dx), point["y_mm"] - (start["y_mm"] + t * dy))

    # Implementación iterativa: una grabación de 3000 muestras con giros
    # alternados puede superar el límite de recursión de Python.
    keep = {0, len(points) - 1}
    pending = [(0, len(points) - 1)]
    while pending:
        start_index, end_index = pending.pop()
        furthest, maximum = -1, 0.0
        for index in range(start_index + 1, end_index):
            current = distance(points[index], points[start_index], points[end_index])
            if current > maximum:
                furthest, maximum = index, current
        if furthest >= 0 and maximum > tolerance:
            keep.add(furthest)
            pending.append((start_index, furthest))
            pending.append((furthest, end_index))
    return [points[index] for index in sorted(keep)]


def compile_orthogonal_points(points: list[dict[str, float]], max_segment_mm: float = MAX_SEGMENT_MM,
                              drive_mode: str = "auto") -> list[dict[str, Any]]:
    """Convierte puntos absolutos a tramos ortogonales X y luego Y."""
    if len(points) < 2:
        raise ValueError("La ruta requiere al menos dos muestras")
    result: list[dict[str, Any]] = []
    x, y = float(points[0]["x_mm"]), float(points[0]["y_mm"])
    for logical_step_id, point in enumerate(points[1:], start=1):
        target_x, target_y = float(point["x_mm"]), float(point["y_mm"])
        for next_x, next_y, component in ((target_x, y, "x"), (target_x, target_y, "y")):
            if math.hypot(next_x - x, next_y - y) <= 1.0:
                x, y = next_x, next_y
                continue
            for target in split_segment_mm(x, y, next_x, next_y, max_segment_mm):
                result.append({"start_x_mm": x, "start_y_mm": y,
                               "x_mm": target["x_mm"], "y_mm": target["y_mm"], "component": component,
                               "logical_step_id": logical_step_id,
                               "heading_deg": math.degrees(math.atan2(target["x_mm"] - x, target["y_mm"] - y)) % 360.0,
                               "length_mm": math.hypot(target["x_mm"] - x, target["y_mm"] - y), "drive_mode": drive_mode})
                x, y = target["x_mm"], target["y_mm"]
    if len(result) > MAX_COMPILED_SEGMENTS:
        raise ValueError("La ruta excede los 256 segmentos disponibles")
    return result


def compile_touch_path(points: list[dict[str, float]]) -> tuple[list[dict[str, float]], list[dict[str, Any]],
                                                                 list[dict[str, Any]], float]:
    """Simplifica adaptativamente una grabación y conserva geometría lógica y física."""
    if len(points) < 2:
        raise ValueError("La grabación requiere al menos dos muestras")
    tolerance = 20.0
    while True:
        simplified = simplify_rdp(points, tolerance)
        try:
            segments = compile_orthogonal_points(simplified)
            break
        except ValueError as exc:
            if "256" not in str(exc) or len(simplified) <= 2:
                raise
            tolerance *= 1.5
            if tolerance > 100_000.0:
                raise ValueError("No fue posible reducir la grabación a 256 segmentos") from exc
    logical: list[dict[str, Any]] = []
    previous_angle = 0.0
    for index, (start, end) in enumerate(zip(simplified, simplified[1:]), start=1):
        dx = float(end["x_mm"]) - float(start["x_mm"])
        dy = float(end["y_mm"]) - float(start["y_mm"])
        absolute = math.degrees(math.atan2(dy, dx))
        relative = (absolute - previous_angle + 180.0) % 360.0 - 180.0
        logical.append({
            "id": f"touch-{index}", "origin": dict(start), "destination": dict(end),
            "dx_mm": dx, "dy_mm": dy, "length_mm": math.hypot(dx, dy),
            "relative_angle_deg": relative, "absolute_angle_deg": absolute,
        })
        previous_angle = absolute
    return simplified, logical, segments, tolerance

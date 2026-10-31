"""Módulo de dominio puro para Rutinas Programadas del Modo Touch.

Maneja validación de acciones discretas (turn, move), inversión cinemática G^-1
y compilación de segmentos para ejecución en lazo cerrado IMU+PCNT.
"""

from __future__ import annotations

import math
import uuid
from typing import Any

# Límites de seguridad operativa S8 y S9
MIN_TURN_DEG: float = 2.0
MAX_TURN_DEG: float = 180.0
MIN_MOVE_CM: float = 1.0
MAX_MOVE_CM: float = 200.0
MAX_ACTIONS: int = 128
MAX_TOTAL_DISTANCE_CM: float = 2000.0  # 20 metros acumulados
DEFAULT_SUBSEGMENT_MM: float = 2000.0  # 2 metros máx por step de firmware


def normalize_angle_deg(angle: float) -> float:
    """Normaliza un ángulo al rango [0, 360)."""
    return float(angle) % 360.0


def normalize_delta_deg(delta: float) -> float:
    """Normaliza un delta angular al rango [-180, 180]."""
    d = (float(delta) + 180.0) % 360.0 - 180.0
    if d == -180.0 and delta > 0:
        return 180.0
    return d


def validate_actions(actions: list[dict[str, Any]]) -> None:
    """Valida los límites operativos S8 y S9 para una lista de acciones."""
    if not isinstance(actions, list):
        raise ValueError("Las acciones deben ser una lista")
    if not actions:
        raise ValueError("La rutina requiere al menos una acción")
    if len(actions) > MAX_ACTIONS:
        raise ValueError(f"La rutina excede el límite de {MAX_ACTIONS} acciones")

    total_dist_cm = 0.0
    for idx, act in enumerate(actions):
        if not isinstance(act, dict):
            raise ValueError(f"Acción #{idx + 1} no es un diccionario válido")
        act_type = str(act.get("type", "")).lower()
        if act_type == "turn":
            delta = act.get("delta_deg")
            if delta is None or isinstance(delta, bool) or not isinstance(delta, (int, float)):
                raise ValueError(f"Acción #{idx + 1} (turn) requiere delta_deg numérico")
            f_delta = float(delta)
            if math.isnan(f_delta) or math.isinf(f_delta):
                raise ValueError(f"Acción #{idx + 1} (turn) contiene delta_deg inválido (NaN o Inf)")
            abs_delta = abs(f_delta)
            if not (MIN_TURN_DEG <= abs_delta <= MAX_TURN_DEG):
                raise ValueError(
                    f"Acción #{idx + 1} (turn): |delta_deg| ({abs_delta:.1f}°) debe estar entre "
                    f"{MIN_TURN_DEG}° y {MAX_TURN_DEG}°"
                )
        elif act_type == "move":
            dist = act.get("distance_cm")
            if dist is None or isinstance(dist, bool) or not isinstance(dist, (int, float)):
                raise ValueError(f"Acción #{idx + 1} (move) requiere distance_cm numérico")
            f_dist = float(dist)
            if math.isnan(f_dist) or math.isinf(f_dist) or f_dist <= 0:
                raise ValueError(f"Acción #{idx + 1} (move) contiene distance_cm inválido")
            if not (MIN_MOVE_CM <= f_dist <= MAX_MOVE_CM):
                raise ValueError(
                    f"Acción #{idx + 1} (move): distance_cm ({f_dist:.1f} cm) debe estar entre "
                    f"{MIN_MOVE_CM} y {MAX_MOVE_CM} cm"
                )
            direction = str(act.get("direction", "")).lower()
            if direction not in {"forward", "reverse"}:
                raise ValueError(f"Acción #{idx + 1} (move): direction debe ser 'forward' o 'reverse'")
            total_dist_cm += f_dist
        else:
            raise ValueError(f"Acción #{idx + 1} tiene tipo desconocido '{act_type}' (esperado 'turn' o 'move')")

    if total_dist_cm > MAX_TOTAL_DISTANCE_CM:
        raise ValueError(
            f"La distancia acumulada ({total_dist_cm:.1f} cm) excede el máximo permitido de "
            f"{MAX_TOTAL_DISTANCE_CM:.1f} cm"
        )


def invert_mirror(actions: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """Aplica la inversa cinemática exacta G^-1 en modo espejo.

    Invierte el orden de la lista, niega los deltas de giro (-delta_deg)
    e intercambia el sentido de traslación ('forward' <-> 'reverse').
    """
    validate_actions(actions)
    inverted: list[dict[str, Any]] = []
    for act in reversed(actions):
        act_type = str(act.get("type", "")).lower()
        new_act = dict(act)
        new_act["id"] = act.get("id") or uuid.uuid4().hex[:8]
        if act_type == "turn":
            new_act["delta_deg"] = -float(act["delta_deg"])
        elif act_type == "move":
            curr_dir = str(act.get("direction", "forward")).lower()
            new_act["direction"] = "reverse" if curr_dir == "forward" else "forward"
        inverted.append(new_act)
    return inverted


def compile_routine(
    actions: list[dict[str, Any]],
    start_pose: dict[str, float] | None = None,
    direction: str = "forward",
    strategy: str = "mirror",
) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    """Compila una lista de acciones a segmentos de misión ejecutables y poses acumuladas.

    Parámetros:
      actions: Lista de acciones {type: 'turn'|'move', ...}
      start_pose: Pose inicial {'x_mm', 'y_mm', 'yaw_deg'}. Default: (0, 0, 0)
      direction: 'forward' para orden directo, 'reverse' para desandar
      strategy: 'mirror' (inversión espejo exacta G^-1) o 'pivot_forward' (retorno frontal estilo Ockham)

    Retorna:
      (segments, poses):
        segments: Segmentos listos para RobotService con 'kind', 'heading_deg', 'x_mm', 'y_mm', etc.
        poses: Waypoints acumulados {'x_mm', 'y_mm', 'yaw_deg', 'action_id'}
    """
    validate_actions(actions)
    effective_direction = str(direction or "forward").lower()
    effective_strategy = str(strategy or "mirror").lower()
    if effective_direction == "pivot_forward":
        effective_direction = "reverse"
        effective_strategy = "pivot_forward"

    if start_pose is None:
        start_pose = {"x_mm": 0.0, "y_mm": 0.0, "yaw_deg": 0.0}

    cur_x = float(start_pose.get("x_mm", 0.0))
    cur_y = float(start_pose.get("y_mm", 0.0))
    cur_yaw = normalize_angle_deg(float(start_pose.get("yaw_deg", 0.0)))

    # Pose de origen
    poses: list[dict[str, Any]] = [{"x_mm": cur_x, "y_mm": cur_y, "yaw_deg": cur_yaw, "action_id": "origin"}]
    segments: list[dict[str, Any]] = []

    # Enfoque A: Espejo (G^-1 directo)
    if effective_direction == "reverse" and effective_strategy == "mirror":
        actions_to_run = invert_mirror(actions)
    elif effective_direction == "reverse" and effective_strategy == "pivot_forward":
        # Si la rutina consta exclusivamente de giros en el lugar (sin traslación),
        # desandar la rutina frontalmente equivale a invertir los giros discretos.
        if all(str(act.get("type", "")).lower() == "turn" for act in actions):
            actions_to_run = invert_mirror(actions)
        else:
            # Enfoque B: Ockham - Proyecta la ruta de ida desde (0, 0, theta_0) para hallar los waypoints geométricos
            # respetando la orientación inicial real, luego desanda frontalmente hacia cada waypoint anterior.
            total_delta_yaw = sum(
                float(act.get("delta_deg", 0.0))
                for act in actions
                if str(act.get("type", "")).lower() == "turn"
            )
            theta_0 = normalize_angle_deg(cur_yaw - total_delta_yaw)
            out_segs, out_poses = compile_routine(
                actions, start_pose={"x_mm": 0.0, "y_mm": 0.0, "yaw_deg": theta_0}, direction="forward"
            )
            target_waypoints = list(reversed(out_poses[:-1]))  # P_{n-1} hasta P_0
            start_x = cur_x
            start_y = cur_y
            fwd_final_x = float(out_poses[-1]["x_mm"])
            fwd_final_y = float(out_poses[-1]["y_mm"])

            prev_x = start_x
            prev_y = start_y
            prev_yaw = cur_yaw

            for idx, wp in enumerate(target_waypoints):
                target_x = start_x + (float(wp["x_mm"]) - fwd_final_x)
                target_y = start_y + (float(wp["y_mm"]) - fwd_final_y)
                dx = target_x - prev_x
                dy = target_y - prev_y
                dist_mm = math.hypot(dx, dy)
                if dist_mm < 1.0:
                    continue  # Punto redundante
                heading = normalize_angle_deg(math.degrees(math.atan2(dx, dy)))

                rem_mm = dist_mm
                while rem_mm > 0:
                    step_mm = min(rem_mm, DEFAULT_SUBSEGMENT_MM)
                    rad = math.radians(heading)
                    sub_dx = step_mm * math.sin(rad)
                    sub_dy = step_mm * math.cos(rad)
                    next_x = prev_x + sub_dx
                    next_y = prev_y + sub_dy
                    segments.append({
                        "kind": "move",
                        "start_x_mm": prev_x,
                        "start_y_mm": prev_y,
                        "x_mm": next_x,
                        "y_mm": next_y,
                        "heading_deg": heading,
                        "distance_cm": round(step_mm / 10.0, 2),
                        "drive_mode": "forward",
                        "action_id": f"ockham_ret_{idx + 1}",
                    })
                    prev_x, prev_y, prev_yaw = next_x, next_y, heading
                    rem_mm -= step_mm

                poses.append({"x_mm": prev_x, "y_mm": prev_y, "yaw_deg": prev_yaw, "action_id": f"ockham_ret_{idx + 1}"})

            # Giro final a la orientación inicial
            orig_yaw = out_poses[0]["yaw_deg"]
            delta_final = (orig_yaw - prev_yaw + 180.0) % 360.0 - 180.0
            if abs(delta_final) >= MIN_TURN_DEG:
                segments.append({
                    "kind": "turn",
                    "heading_deg": orig_yaw,
                    "delta_deg": delta_final,
                    "x_mm": prev_x,
                    "y_mm": prev_y,
                    "action_id": "ockham_final_turn",
                })
                poses.append({"x_mm": prev_x, "y_mm": prev_y, "yaw_deg": orig_yaw, "action_id": "ockham_final_turn"})

            return segments, poses
    else:
        actions_to_run = actions

    # Compilación estándar (directa o inversa espejo)
    for act in actions_to_run:
        act_id = str(act.get("id") or uuid.uuid4().hex[:8])
        act_type = str(act.get("type", "")).lower()

        if act_type == "turn":
            delta = float(act["delta_deg"])
            cur_yaw = normalize_angle_deg(cur_yaw + delta)
            segments.append({
                "kind": "turn",
                "heading_deg": cur_yaw,
                "delta_deg": delta,
                "x_mm": cur_x,
                "y_mm": cur_y,
                "action_id": act_id,
            })
            poses.append({"x_mm": cur_x, "y_mm": cur_y, "yaw_deg": cur_yaw, "action_id": act_id})

        elif act_type == "move":
            dist_cm = float(act["distance_cm"])
            direction_mode = str(act.get("direction", "forward")).lower()
            dist_mm = dist_cm * 10.0

            # Rumbo del vector de traslación
            if direction_mode == "forward":
                travel_heading = cur_yaw
            else:
                # En reversa el desplazamiento físico ocurre hacia yaw + 180°
                travel_heading = normalize_angle_deg(cur_yaw + 180.0)

            # Subsegmentación si la distancia excede el límite del firmware
            remaining_mm = dist_mm
            while remaining_mm > 0:
                step_mm = min(remaining_mm, DEFAULT_SUBSEGMENT_MM)
                rad = math.radians(travel_heading)
                step_dx = step_mm * math.sin(rad)
                step_dy = step_mm * math.cos(rad)
                next_x = cur_x + step_dx
                next_y = cur_y + step_dy

                segments.append({
                    "kind": "move",
                    "start_x_mm": cur_x,
                    "start_y_mm": cur_y,
                    "x_mm": next_x,
                    "y_mm": next_y,
                    "heading_deg": travel_heading,
                    "distance_cm": round(step_mm / 10.0, 2),
                    "drive_mode": direction_mode,
                    "action_id": act_id,
                })
                cur_x, cur_y = next_x, next_y
                remaining_mm -= step_mm

            poses.append({"x_mm": cur_x, "y_mm": cur_y, "yaw_deg": cur_yaw, "action_id": act_id})

    return segments, poses

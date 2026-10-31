"""Pruebas unitarias para el módulo de dominio puro routines.py."""

import math
import unittest

from robot_app.routines import (
    MAX_ACTIONS,
    MAX_TOTAL_DISTANCE_CM,
    compile_routine,
    invert_mirror,
    normalize_angle_deg,
    normalize_delta_deg,
    validate_actions,
)


class TestRoutinesDomain(unittest.TestCase):
    def test_normalize_angles(self) -> None:
        self.assertEqual(normalize_angle_deg(0.0), 0.0)
        self.assertEqual(normalize_angle_deg(360.0), 0.0)
        self.assertEqual(normalize_angle_deg(450.0), 90.0)
        self.assertEqual(normalize_angle_deg(-90.0), 270.0)

        self.assertEqual(normalize_delta_deg(90.0), 90.0)
        self.assertEqual(normalize_delta_deg(-90.0), -90.0)
        self.assertEqual(normalize_delta_deg(270.0), -90.0)
        self.assertEqual(normalize_delta_deg(180.0), 180.0)

    def test_validate_actions_valid(self) -> None:
        actions = [
            {"id": "a1", "type": "turn", "delta_deg": 90.0},
            {"id": "a2", "type": "move", "distance_cm": 50.0, "direction": "forward"},
            {"id": "a3", "type": "turn", "delta_deg": -30.0},
            {"id": "a4", "type": "move", "distance_cm": 20.0, "direction": "reverse"},
        ]
        validate_actions(actions)  # No debe lanzar excepción

    def test_validate_actions_invalid_boundaries(self) -> None:
        # Turn demasiado pequeño (< 2°)
        with self.assertRaises(ValueError):
            validate_actions([{"type": "turn", "delta_deg": 1.5}])

        # Turn demasiado grande (> 180°)
        with self.assertRaises(ValueError):
            validate_actions([{"type": "turn", "delta_deg": 181.0}])

        # Move demasiado pequeño (< 1 cm)
        with self.assertRaises(ValueError):
            validate_actions([{"type": "move", "distance_cm": 0.5, "direction": "forward"}])

        # Move demasiado grande (> 200 cm)
        with self.assertRaises(ValueError):
            validate_actions([{"type": "move", "distance_cm": 201.0, "direction": "forward"}])

        # NaN o Inf
        with self.assertRaises(ValueError):
            validate_actions([{"type": "turn", "delta_deg": float("nan")}])
        with self.assertRaises(ValueError):
            validate_actions([{"type": "move", "distance_cm": float("inf"), "direction": "forward"}])

        # Dirección inválida
        with self.assertRaises(ValueError):
            validate_actions([{"type": "move", "distance_cm": 10.0, "direction": "sideways"}])

        # Lista vacía
        with self.assertRaises(ValueError):
            validate_actions([])

        # Límite de acciones (> 128)
        many_actions = [{"type": "turn", "delta_deg": 10.0} for _ in range(MAX_ACTIONS + 1)]
        with self.assertRaises(ValueError):
            validate_actions(many_actions)

        # Distancia total acumulada (> 2000 cm)
        long_actions = [{"type": "move", "distance_cm": 150.0, "direction": "forward"} for _ in range(14)]
        with self.assertRaises(ValueError):
            validate_actions(long_actions)

    def test_invert_mirror_opus_example(self) -> None:
        # Ejemplo formal de Opus §3.2:
        # Directo: [Giro +90°, Avance 50 cm, Giro -30°, Reversa 20 cm]
        # Inverso: [Avance 20 cm, Giro +30°, Reversa 50 cm, Giro -90°]
        actions = [
            {"id": "1", "type": "turn", "delta_deg": 90.0},
            {"id": "2", "type": "move", "distance_cm": 50.0, "direction": "forward"},
            {"id": "3", "type": "turn", "delta_deg": -30.0},
            {"id": "4", "type": "move", "distance_cm": 20.0, "direction": "reverse"},
        ]
        inverted = invert_mirror(actions)

        self.assertEqual(len(inverted), 4)
        self.assertEqual(inverted[0]["type"], "move")
        self.assertEqual(inverted[0]["distance_cm"], 20.0)
        self.assertEqual(inverted[0]["direction"], "forward")

        self.assertEqual(inverted[1]["type"], "turn")
        self.assertEqual(inverted[1]["delta_deg"], 30.0)

        self.assertEqual(inverted[2]["type"], "move")
        self.assertEqual(inverted[2]["distance_cm"], 50.0)
        self.assertEqual(inverted[2]["direction"], "reverse")

        self.assertEqual(inverted[3]["type"], "turn")
        self.assertEqual(inverted[3]["delta_deg"], -90.0)

    def test_compile_routine_and_invert_mirror_identity(self) -> None:
        # Ejecutar directo y luego inverso espejo debe cerrar el ciclo en (0, 0)
        actions = [
            {"id": "1", "type": "turn", "delta_deg": 90.0},
            {"id": "2", "type": "move", "distance_cm": 50.0, "direction": "forward"},
            {"id": "3", "type": "turn", "delta_deg": -30.0},
            {"id": "4", "type": "move", "distance_cm": 20.0, "direction": "reverse"},
        ]

        # 1. Compilación directa desde (0, 0, 0)
        direct_segs, direct_poses = compile_routine(actions, start_pose={"x_mm": 0.0, "y_mm": 0.0, "yaw_deg": 0.0})
        end_pose = direct_poses[-1]

        # 2. Compilación inversa desde la pose final
        rev_segs, rev_poses = compile_routine(
            actions, start_pose=end_pose, direction="reverse", strategy="mirror"
        )
        final_pose = rev_poses[-1]

        # El robot debe haber regresado al origen con error sub-milimétrico y yaw 0°
        self.assertAlmostEqual(final_pose["x_mm"], 0.0, delta=0.01)
        self.assertAlmostEqual(final_pose["y_mm"], 0.0, delta=0.01)
        self.assertAlmostEqual(final_pose["yaw_deg"], 0.0, delta=0.01)

    def test_compile_routine_pivot_forward_ockham(self) -> None:
        actions = [
            {"id": "1", "type": "turn", "delta_deg": 90.0},
            {"id": "2", "type": "move", "distance_cm": 50.0, "direction": "forward"},
        ]
        direct_segs, direct_poses = compile_routine(actions, start_pose={"x_mm": 0.0, "y_mm": 0.0, "yaw_deg": 0.0})
        end_pose = direct_poses[-1]

        rev_segs, rev_poses = compile_routine(
            actions, start_pose=end_pose, direction="reverse", strategy="pivot_forward"
        )
        final_pose = rev_poses[-1]

        self.assertAlmostEqual(final_pose["x_mm"], 0.0, delta=0.01)
        self.assertAlmostEqual(final_pose["y_mm"], 0.0, delta=0.01)
        # En pivot_forward el paso es con forward
        for seg in rev_segs:
            if seg["kind"] == "move":
                self.assertEqual(seg["drive_mode"], "forward")

    def test_compile_routine_pure_turns_360(self) -> None:
        actions = [
            {"id": "1", "type": "turn", "delta_deg": 90.0},
            {"id": "2", "type": "turn", "delta_deg": 90.0},
            {"id": "3", "type": "turn", "delta_deg": 90.0},
            {"id": "4", "type": "turn", "delta_deg": 90.0},
        ]
        segs, poses = compile_routine(actions, start_pose={"x_mm": 0.0, "y_mm": 0.0, "yaw_deg": 0.0})
        self.assertEqual(len(segs), 4)
        self.assertAlmostEqual(poses[-1]["yaw_deg"], 0.0, delta=0.01)
        self.assertAlmostEqual(poses[-1]["x_mm"], 0.0, delta=0.01)

        # Invertir 360° devuelve la rotación opuesta cerrando en 0°
        rev_segs, rev_poses = compile_routine(actions, start_pose=poses[-1], direction="reverse", strategy="mirror")
        self.assertEqual(len(rev_segs), 4)
        self.assertAlmostEqual(rev_poses[-1]["yaw_deg"], 0.0, delta=0.01)

    def test_compile_routine_pure_turns_pivot_forward_fallback(self) -> None:
        # Giros puros con strategy pivot_forward deben desandar los giros en vez de quedar vacíos
        actions = [
            {"id": "1", "type": "turn", "delta_deg": 90.0},
            {"id": "2", "type": "turn", "delta_deg": 45.0},
        ]
        rev_segs, rev_poses = compile_routine(actions, direction="pivot_forward")
        self.assertTrue(len(rev_segs) > 0)
        self.assertEqual(rev_segs[0]["kind"], "turn")
        self.assertAlmostEqual(rev_segs[0]["delta_deg"], -45.0, delta=0.01)

    def test_compile_routine_pivot_forward_arbitrary_origin(self) -> None:
        origin = {"x_mm": 1200.0, "y_mm": 3400.0, "yaw_deg": 30.0}
        actions = [
            {"id": "1", "type": "turn", "delta_deg": 60.0},
            {"id": "2", "type": "move", "distance_cm": 40.0, "direction": "forward"},
        ]
        direct_segs, direct_poses = compile_routine(actions, start_pose=origin)
        end_pose = direct_poses[-1]

        rev_segs, rev_poses = compile_routine(
            actions, start_pose=end_pose, direction="pivot_forward"
        )
        final_pose = rev_poses[-1]

        self.assertAlmostEqual(final_pose["x_mm"], origin["x_mm"], delta=0.05)
        self.assertAlmostEqual(final_pose["y_mm"], origin["y_mm"], delta=0.05)
        self.assertAlmostEqual(final_pose["yaw_deg"], origin["yaw_deg"], delta=0.05)


if __name__ == "__main__":
    unittest.main()

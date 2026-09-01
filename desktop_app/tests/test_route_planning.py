import math
import unittest

from robot_app.route_planning import (
    AngularDecompositionRouteStrategy,
    AngularVectorialRouteStrategy,
    RectangularRouteStrategy,
)


class RoutePlanningTests(unittest.TestCase):
    def setUp(self) -> None:
        self.origin = {"x_mm": 0.0, "y_mm": 0.0}

    def test_angular_decomposition_compiles_x_then_y(self) -> None:
        compilation = AngularDecompositionRouteStrategy().compile({
            "vectors": [{"length_cm": 50, "relative_angle_deg": 135}],
        }, self.origin)
        self.assertEqual(compilation.mode, "angular_decomposition")
        self.assertEqual([segment["component"] for segment in compilation.segments], ["x", "y"])
        self.assertAlmostEqual(compilation.segments[0]["x_mm"], -353.5533906)
        self.assertEqual(compilation.segments[0]["y_mm"], 0.0)
        self.assertAlmostEqual(compilation.segments[1]["y_mm"], 353.5533906)

    def test_vectorial_compiles_one_diagonal_with_firmware_heading(self) -> None:
        compilation = AngularVectorialRouteStrategy().compile({
            "vectors": [{"length_cm": 50, "relative_angle_deg": 135}],
        }, self.origin)
        self.assertEqual(len(compilation.segments), 1)
        segment = compilation.segments[0]
        self.assertEqual(segment["component"], "vector")
        self.assertAlmostEqual(segment["length_mm"], 500.0)
        self.assertAlmostEqual(segment["heading_deg"], 315.0)
        self.assertAlmostEqual(compilation.logical_steps[0]["firmware_heading_deg"], 315.0)

    def test_relative_angles_wrap_and_zero_length_only_changes_reference(self) -> None:
        compilation = AngularVectorialRouteStrategy().compile({
            "vectors": [
                {"length_cm": 10, "relative_angle_deg": 170},
                {"length_cm": 0, "relative_angle_deg": 30},
                {"length_cm": 10, "relative_angle_deg": 0},
            ],
        }, self.origin)
        self.assertEqual([step["absolute_angle_deg"] for step in compilation.logical_steps], [170.0, -160.0, -160.0])
        self.assertEqual(len(compilation.segments), 2)
        self.assertEqual([segment["logical_step_id"] for segment in compilation.segments], [1, 3])

    def test_vector_larger_than_limit_is_split_collinearly(self) -> None:
        compilation = AngularVectorialRouteStrategy().compile({
            "vectors": [{"length_cm": 450, "relative_angle_deg": 45}],
        }, self.origin)
        self.assertEqual(len(compilation.segments), 3)
        self.assertTrue(all(math.isclose(segment["heading_deg"], 45.0) for segment in compilation.segments))
        self.assertTrue(all(math.isclose(segment["length_mm"], 1500.0) for segment in compilation.segments))

    def test_zero_only_route_is_rejected(self) -> None:
        with self.assertRaisesRegex(ValueError, "no contiene desplazamiento"):
            AngularVectorialRouteStrategy().compile({
                "vectors": [{"length_cm": 0, "relative_angle_deg": 90}],
            }, self.origin)

    def test_legacy_rectangular_behavior_keeps_diagonal_decomposition(self) -> None:
        compilation = RectangularRouteStrategy().compile({
            "points": [{"x_mm": 1000, "y_mm": 1000}],
        }, self.origin)
        self.assertEqual([(segment["x_mm"], segment["y_mm"]) for segment in compilation.segments], [
            (1000.0, 0.0), (1000.0, 1000.0),
        ])


if __name__ == "__main__":
    unittest.main()

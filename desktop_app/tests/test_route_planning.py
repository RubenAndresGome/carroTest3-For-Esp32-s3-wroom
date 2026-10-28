import math
import unittest

from robot_app.route_planning import (
    AngularDecompositionRouteStrategy,
    AngularVectorialRouteStrategy,
    OrthogonalRealignmentPolicy,
    RectangularRouteStrategy,
    VectorialRealignmentPolicy,
    compile_orthogonal_points,
    compile_touch_path,
    get_realignment_policy,
    select_optimal_drive_mode,
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

    def test_rectangular_snaps_subcentimeter_residual(self) -> None:
        # Pose real 2 mm por debajo del waypoint planificado: el residuo debe
        # absorberse, no compilarse como micro-paso (el firmware exige >= 5 mm).
        compilation = RectangularRouteStrategy().compile({
            "points": [
                {"x_mm": 0.0, "y_mm": 1550.0},
                {"x_mm": 0.0, "y_mm": 1000.0},
                {"x_mm": 0.0, "y_mm": 0.0},
            ],
        }, {"x_mm": 2000.0, "y_mm": 1548.0})
        lengths = [segment["length_mm"] for segment in compilation.segments]
        self.assertEqual(len(lengths), 3)
        self.assertTrue(all(length >= 5.0 for length in lengths))
        self.assertAlmostEqual(lengths[0], 2000.0)

    def test_touch_compilation_is_orthogonal_exact_and_bounded(self) -> None:
        points = [{"x_mm": 0.0, "y_mm": 0.0}, {"x_mm": 4500.0, "y_mm": 3100.0}]
        segments = compile_orthogonal_points(points)
        self.assertLessEqual(len(segments), 256)
        self.assertEqual((segments[-1]["x_mm"], segments[-1]["y_mm"]), (4500.0, 3100.0))
        for segment in segments:
            dx = segment["x_mm"] - segment["start_x_mm"]
            dy = segment["y_mm"] - segment["start_y_mm"]
            self.assertTrue(math.isclose(dx, 0.0) or math.isclose(dy, 0.0))
            self.assertLessEqual(segment["length_mm"], 2000.0)

    def test_touch_rdp_adapts_until_compilation_fits_256(self) -> None:
        points = [
            {"x_mm": float(index * 10), "y_mm": 40.0 if index % 2 else -40.0}
            for index in range(300)
        ]
        simplified, logical, segments, tolerance = compile_touch_path(points)
        self.assertLess(len(simplified), len(points))
        self.assertEqual(len(logical), len(simplified) - 1)
        self.assertLessEqual(len(segments), 256)
        self.assertGreaterEqual(tolerance, 20.0)


    def test_orthogonal_points_subdivides_into_500mm_default(self) -> None:
        points = [{"x_mm": 0.0, "y_mm": 0.0}, {"x_mm": 0.0, "y_mm": 1000.0}]
        segments = compile_orthogonal_points(points)
        self.assertEqual(len(segments), 2)
        self.assertEqual(segments[0]["length_mm"], 500.0)
        self.assertEqual(segments[1]["length_mm"], 500.0)
        self.assertEqual(segments[1]["y_mm"], 1000.0)

    def test_orthogonal_points_supports_custom_subsegments(self) -> None:
        points = [{"x_mm": 0.0, "y_mm": 0.0}, {"x_mm": 0.0, "y_mm": 1000.0}]
        segments = compile_orthogonal_points(points, max_segment_mm=200.0)
        self.assertEqual(len(segments), 5)
        for segment in segments:
            self.assertEqual(segment["length_mm"], 200.0)

    def test_orthogonal_points_deducts_chassis_offset(self) -> None:
        points = [{"x_mm": 0.0, "y_mm": 0.0}, {"x_mm": 0.0, "y_mm": 1000.0}]
        segments = compile_orthogonal_points(points, deduct_chassis_offset=True)
        self.assertEqual(len(segments), 2)
        self.assertEqual(segments[-1]["y_mm"], 860.0)

    def test_realignment_policies(self) -> None:
        ortho = get_realignment_policy("rectangular")
        self.assertIsInstance(ortho, OrthogonalRealignmentPolicy)
        self.assertEqual(ortho.target_heading(12.0), 0.0)
        self.assertEqual(ortho.target_heading(44.0), 0.0)
        self.assertEqual(ortho.target_heading(46.0), 90.0)
        self.assertEqual(ortho.target_heading(182.0), 180.0)
        self.assertEqual(ortho.target_heading(358.0), 0.0)

        vec = get_realignment_policy("angular_vectorial")
        self.assertIsInstance(vec, VectorialRealignmentPolicy)
        self.assertAlmostEqual(vec.target_heading(42.5), 42.5)
        self.assertAlmostEqual(vec.target_heading(375.0), 15.0)

        touch_policy = get_realignment_policy("touch_reverse")
        self.assertIsInstance(touch_policy, OrthogonalRealignmentPolicy)

    def test_select_optimal_drive_mode(self) -> None:
        # Facing 0° (+Y), heading is 10° -> turn is only 10° for forward, 170° for reverse -> forward
        self.assertEqual(select_optimal_drive_mode(10.0, 0.0), "forward")
        # Facing 0° (+Y), heading is 170° -> turn is 170° forward, but 10° reverse -> reverse
        self.assertEqual(select_optimal_drive_mode(170.0, 0.0), "reverse")
        # Facing 90° (+X), heading is 260° -> delta_fwd = 170°, delta_rev = 10° -> reverse
        self.assertEqual(select_optimal_drive_mode(260.0, 90.0), "reverse")
        # Facing 90° (+X), heading is 80° -> forward
        self.assertEqual(select_optimal_drive_mode(80.0, 90.0), "forward")

    def test_chained_vectorial_route_notebook_trig(self) -> None:
        compilation = AngularVectorialRouteStrategy().compile({
            "vectors": [
                {"length_cm": 100, "relative_angle_deg": 30},
                {"length_cm": 100, "relative_angle_deg": -30},
            ],
        }, self.origin)
        self.assertEqual(len(compilation.logical_steps), 2)
        step1, step2 = compilation.logical_steps
        self.assertAlmostEqual(step1["relative_angle_deg"], 30.0)
        self.assertAlmostEqual(step1["absolute_angle_deg"], 30.0)
        self.assertAlmostEqual(step2["relative_angle_deg"], -30.0)
        self.assertAlmostEqual(step2["absolute_angle_deg"], 0.0)
        self.assertAlmostEqual(step1["firmware_heading_deg"], 60.0)
        self.assertAlmostEqual(step2["firmware_heading_deg"], 90.0)
        self.assertAlmostEqual(step2["target"]["x_mm"], 1866.0254, places=2)
        self.assertAlmostEqual(step2["target"]["y_mm"], 500.0, places=2)


if __name__ == "__main__":
    unittest.main()

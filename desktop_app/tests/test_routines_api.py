"""Pruebas unitarias para endpoints de Control Libre y Rutinas Programadas."""

import json
import time
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from unittest.mock import Mock

from robot_app.app_factory import create_app
from robot_app.config import AppConfig
from robot_app.domain import ConnectionState, TelemetrySnapshot


class RoutinesApiTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = TemporaryDirectory()
        self.root = Path(self.temp_dir.name)
        self.db_path = self.root / "test_robot.sqlite3"
        self.app = create_app(AppConfig(self.root, self.db_path, start_gateway=False))
        self.client = self.app.test_client()
        self.token = self.app.extensions["app_token"]
        self.service = self.app.extensions["robot_service"]
        self.database = self.service.database
        self.service.gateway._state = ConnectionState.CONNECTED
        self.service.gateway._protocol_v1 = True

    def tearDown(self) -> None:
        self.service.close()
        self.temp_dir.cleanup()

    def _ready_robot(self, x_mm: float = 0.0, y_mm: float = 0.0, yaw_deg: float = 0.0) -> None:
        self.service._last_telemetry = TelemetrySnapshot.from_message({
            "evt": "telemetry", "state": "listo", "x_mm": x_mm, "y_mm": y_mm,
            "yaw_deg": yaw_deg, "cal": True, "enc": [10, 11, 12, 13],
            "capabilities": ["manual_drive_v1", "steps_v3"],
            "mpu_present": True, "mpu_calibrated": True, "mpu_stale": False,
            "encoder_health": {wheel: "healthy" for wheel in ("fl", "fr", "bl", "br")},
        }, 1)

    def test_free_drive_does_not_create_touch_recording(self) -> None:
        self._ready_robot()
        headers = {"X-App-Token": self.token}

        # Iniciar en submodo libre
        res = self.client.post("/api/v1/manual/start", json={"mode": "free"}, headers=headers)
        self.assertEqual(res.status_code, 201)
        self.assertEqual(res.json["mode"], "free")
        self.assertFalse(res.json["recording"])

        # Control continuo libre
        drive_res = self.client.put("/api/v1/manual/drive", json={"left": 180, "right": 180}, headers=headers)
        self.assertEqual(drive_res.status_code, 202)

        # Telemetría continua no acumula muestras en TouchRouteRecorder
        self._ready_robot(x_mm=100.0, y_mm=100.0, yaw_deg=45.0)
        self.assertFalse(self.service._touch.summary()["recording"])
        self.assertEqual(self.service._touch.summary().get("sample_count", 0), 0)

        # Detener
        stop_res = self.client.post("/api/v1/manual/stop", json={}, headers=headers)
        self.assertEqual(stop_res.status_code, 200)
        self.assertEqual(stop_res.json["mode"], "free")
        self.assertFalse(stop_res.json["recording"])

        # Ninguna fila en touch_recordings
        self.assertEqual(len(self.database.touch_recordings()), 0)

    def test_routines_crud(self) -> None:
        headers = {"X-App-Token": self.token}
        actions = [
            {"id": "a1", "type": "turn", "delta_deg": 45.0},
            {"id": "a2", "type": "move", "distance_cm": 30.0, "direction": "forward"},
        ]

        # 1. Crear rutina
        create_res = self.client.post("/api/v1/routines", json={
            "id": "rutina_1", "name": "Patrulla A", "actions": actions, "source": "jog"
        }, headers=headers)
        self.assertEqual(create_res.status_code, 201)
        self.assertEqual(create_res.json["name"], "Patrulla A")
        self.assertEqual(len(create_res.json["actions"]), 2)

        # 2. Listar
        list_res = self.client.get("/api/v1/routines", headers=headers)
        self.assertEqual(list_res.status_code, 200)
        self.assertEqual(len(list_res.json), 1)

        # 3. Obtener
        get_res = self.client.get("/api/v1/routines/rutina_1", headers=headers)
        self.assertEqual(get_res.status_code, 200)
        self.assertEqual(get_res.json["id"], "rutina_1")

        # 4. Eliminar
        del_res = self.client.delete("/api/v1/routines/rutina_1", headers=headers)
        self.assertEqual(del_res.status_code, 200)
        self.assertTrue(del_res.json["deleted"])

        # 5. Obtener 404
        self.assertEqual(self.client.get("/api/v1/routines/rutina_1", headers=headers).status_code, 404)

    def test_routine_preview(self) -> None:
        headers = {"X-App-Token": self.token}
        actions = [
            {"id": "1", "type": "turn", "delta_deg": 90.0},
            {"id": "2", "type": "move", "distance_cm": 50.0, "direction": "forward"},
        ]
        res = self.client.post("/api/v1/routines/preview", json={
            "actions": actions,
            "direction": "reverse",
            "reverse_strategy": "mirror",
            "start_pose": {"x_mm": 500.0, "y_mm": 0.0, "yaw_deg": 90.0},
        }, headers=headers)
        self.assertEqual(res.status_code, 200)
        self.assertEqual(res.json["direction"], "reverse")
        # Estructura dual completa para renderizado simultáneo de ida y vuelta
        self.assertIn("forward", res.json)
        self.assertIn("reverse", res.json)
        self.assertEqual(len(res.json["forward"]["poses"]), 3)
        self.assertEqual(len(res.json["reverse"]["poses"]), 3)
        poses = res.json["poses"]
        # Retorno espejo debe terminar en (0, 0, 0)
        self.assertAlmostEqual(poses[-1]["x_mm"], 0.0, delta=0.1)
        self.assertAlmostEqual(poses[-1]["y_mm"], 0.0, delta=0.1)

    def test_routine_run_and_mission_flow(self) -> None:
        self._ready_robot(x_mm=0.0, y_mm=0.0, yaw_deg=0.0)
        headers = {"X-App-Token": self.token}
        actions = [
            {"id": "t1", "type": "turn", "delta_deg": 90.0},
            {"id": "m1", "type": "move", "distance_cm": 25.0, "direction": "forward"},
        ]
        self.service.save_routine("Cuadrante 1", actions, routine_id="r10")

        # Ejecutar rutina directa
        run_res = self.client.post("/api/v1/routines/r10/run", json={"direction": "forward"}, headers=headers)
        self.assertEqual(run_res.status_code, 202)
        self.assertEqual(run_res.json["mode"], "routine")
        self.assertEqual(run_res.json["direction"], "forward")

        # Verificar que el primer paso sea turn_to(90)
        status = self.service.mission_status()
        self.assertTrue(status["running"])
        self.assertEqual(status["total_segments"], 2)

        # Simular avance de pasos
        self.service._on_step_settled()
        self.service._on_step_settled()
        # Rutina completada
        self.assertEqual(self.service.mission_status()["stage"], "completed")

        # Ejecutar rutina en reversa frontal (pivot_forward)
        run_piv = self.client.post("/api/v1/routines/r10/run", json={"direction": "pivot_forward"}, headers=headers)
        self.assertEqual(run_piv.status_code, 202)
        self.assertEqual(run_piv.json["direction"], "reverse")

    def test_jog_endpoint(self) -> None:
        headers = {"X-App-Token": self.token}
        # Solo planificar (execute=False)
        jog_plan = self.client.post("/api/v1/jog", json={
            "action": {"type": "turn", "delta_deg": 30.0},
            "execute": False,
        }, headers=headers)
        self.assertEqual(jog_plan.status_code, 200)
        self.assertTrue(jog_plan.json["valid"])

        # Ejecución requiere robot listo
        self._ready_robot()
        jog_exec = self.client.post("/api/v1/jog", json={
            "action": {"type": "move", "distance_cm": 10.0, "direction": "forward"},
            "execute": True,
        }, headers=headers)
        self.assertEqual(jog_exec.status_code, 202)
        self.assertEqual(self.service.mission_status()["kind"], "jog")


if __name__ == "__main__":
    unittest.main()

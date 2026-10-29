import asyncio
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / ".build-tools/test-deps"))
sys.path.insert(0, str(ROOT))
import httpx
from carrier_openwebui_tool import Tools


def status(state="idle", action_id=0, stopped=True, healthy=True):
    return {"boot_id": 42, "id": action_id, "state": state, "kind": "move",
            "motor_stopped": stopped, "reason": state, "target": 50,
            "progress": 46 if state == "completed" else 0,
            "pose": {"x_mm": 48, "y_mm": 0, "heading_deg": 0, "healthy": healthy}}


class Robot:
    def __init__(self):
        self.calls = []
        self.current = status()
        self.mode = "normal"
        self.tracking = True

    def handle(self, request):
        path = request.url.path
        params = dict(request.url.params)
        self.calls.append((request.method, path, params))
        if path == "/api/telemetry":
            return httpx.Response(200, json={"firmware": "2.4", "boot_id": 42,
                "pose": {"ready": True, "valid": self.tracking, "incomplete": False,
                         "origin_pending": False, "origin_id": 1}})
        if path == "/api/action" and request.method == "GET":
            return httpx.Response(200, json=self.current)
        if path == "/api/action" and request.method == "POST":
            if self.mode == "rejected":
                return httpx.Response(409, json={"error": "busy"})
            self.current = status("running", 1, False)
            if self.mode == "start_timeout":
                raise httpx.ReadTimeout("start reply lost", request=request)
            return httpx.Response(200, json=self.current)
        if path == "/api/action/heartbeat":
            if self.mode == "heartbeat_timeout":
                raise httpx.ReadTimeout("heartbeat reply lost", request=request)
            self.current = status("fault" if self.mode == "fault" else "completed", 1)
            if self.mode == "reboot":
                self.current["boot_id"] = 43
            if self.mode == "unhealthy_finish":
                self.current["pose"]["healthy"] = False
            return httpx.Response(200, json=self.current)
        if path == "/api/action/cancel":
            self.current = status("cancelled", 1)
            return httpx.Response(200, json=self.current)
        if path == "/api/motor":
            self.current = status("cancelled", self.current["id"])
            return httpx.Response(200, json={"ok": True})
        raise AssertionError((path, params))


class ClientTests(unittest.IsolatedAsyncioTestCase):
    def setUp(self):
        self.robot = Robot()
        self.tool = Tools()
        self.tool._client = lambda: httpx.AsyncClient(
            base_url="http://robot", transport=httpx.MockTransport(self.robot.handle))

    def count(self, method, path):
        return sum(m == method and p == path for m, p, _ in self.robot.calls)

    async def test_complete_move_and_keep_scalers_untouched(self):
        result = await self.tool.carrier_move(50)
        self.assertTrue(result["ok"])
        self.assertEqual(result["action"]["pose"]["x_mm"], 48)
        self.assertEqual(self.count("POST", "/api/action"), 1)
        self.assertEqual(self.count("POST", "/api/action/heartbeat"), 1)
        self.assertEqual(self.count("POST", "/api/motor-scale"), 0)
        self.assertEqual(self.count("POST", "/api/action/cancel"), 0)
        start = next(p for m, path, p in self.robot.calls if m == "POST" and path == "/api/action")
        self.assertEqual(start, {"boot_id": "42", "id": "1", "origin_id": "1", "kind": "move", "value": "50", "speed": "20"})

    async def test_signed_turn(self):
        result = await self.tool.carrier_turn(-30)
        self.assertTrue(result["ok"])
        start = next(p for m, path, p in self.robot.calls if m == "POST" and path == "/api/action")
        self.assertEqual((start["kind"], start["value"]), ("turn", "-30"))

    async def test_invalid_arguments_never_contact_robot(self):
        for value in (0, 1, 501, float("nan"), float("inf"), True, "20"):
            self.assertFalse((await self.tool.carrier_move(value))["ok"])
        self.assertFalse((await self.tool.carrier_turn(181))["ok"])
        self.assertFalse(self.robot.calls)

    async def test_unhealthy_tracking_never_starts(self):
        self.robot.tracking = False
        self.assertFalse((await self.tool.carrier_move(50))["ok"])
        self.assertEqual(self.count("POST", "/api/action"), 0)

    async def test_fault_is_not_success(self):
        self.robot.mode = "fault"
        self.assertFalse((await self.tool.carrier_move(50))["ok"])
        self.assertEqual(self.count("POST", "/api/action"), 1)

    async def test_unhealthy_final_pose_is_not_success(self):
        self.robot.mode = "unhealthy_finish"
        self.assertFalse((await self.tool.carrier_move(50))["ok"])

    async def test_lost_start_reply_cancels_without_retry(self):
        self.robot.mode = "start_timeout"
        self.assertFalse((await self.tool.carrier_move(50))["ok"])
        self.assertEqual(self.count("POST", "/api/action"), 1)
        self.assertEqual(self.count("POST", "/api/action/cancel"), 1)

    async def test_lost_heartbeat_cancels_without_retry(self):
        self.robot.mode = "heartbeat_timeout"
        self.assertFalse((await self.tool.carrier_move(50))["ok"])
        self.assertEqual(self.count("POST", "/api/action"), 1)
        self.assertEqual(self.count("POST", "/api/action/cancel"), 1)

    async def test_rejected_start_does_not_cancel_other_movement(self):
        self.robot.mode = "rejected"
        self.assertFalse((await self.tool.carrier_move(50))["ok"])
        self.assertEqual(self.count("POST", "/api/action/cancel"), 0)

    async def test_reboot_is_failure(self):
        self.robot.mode = "reboot"
        self.assertFalse((await self.tool.carrier_move(50))["ok"])

    async def test_stop_available_while_movement_lock_held(self):
        async with self.tool._movement_lock:
            self.assertFalse((await self.tool.carrier_move(50))["ok"])
            self.assertTrue((await self.tool.carrier_stop())["ok"])

    async def test_cancelled_tool_cancels_robot(self):
        real_request = self.tool._request
        started = asyncio.Event()
        async def waiting_request(client, method, path, params=None):
            result = await real_request(client, method, path, params)
            if method == "POST" and path == "/api/action":
                started.set()
                await asyncio.Event().wait()
            return result
        self.tool._request = waiting_request
        task = asyncio.create_task(self.tool.carrier_move(50))
        await started.wait()
        task.cancel()
        with self.assertRaises(asyncio.CancelledError):
            await task
        self.assertEqual(self.count("POST", "/api/action/cancel"), 1)


if __name__ == "__main__":
    unittest.main(verbosity=2)

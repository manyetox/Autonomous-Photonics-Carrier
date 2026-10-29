"""
title: Photonics Carrier
description: Read the carrier's position and execute bounded distance and turn actions using firmware 2.4.
author: Photonics Carrier project
version: 2.4.0
"""

import asyncio
import json
import math
import time

import httpx
from pydantic import BaseModel, Field


class Tools:
    class Valves(BaseModel):
        ROBOT_URL: str = Field(
            default="http://192.168.4.1",
            description="Carrier address reachable from the Open WebUI server.",
        )
        POWER_PERCENT: int = Field(
            default=20, ge=15, le=40,
            description="Base action power. Firmware applies your separate left/right factors afterward.",
        )

    def __init__(self):
        self.valves = self.Valves()
        self._movement_lock = asyncio.Lock()

    def _client(self):
        return httpx.AsyncClient(
            base_url=self.valves.ROBOT_URL.rstrip("/"),
            timeout=httpx.Timeout(0.6), follow_redirects=False, trust_env=False,
        )

    async def _request(self, client, method, path, params=None):
        response = await client.request(method, path, params=params)
        response.raise_for_status()
        data = response.json()
        if not isinstance(data, dict) or "error" in data:
            raise ValueError(f"Carrier response: {data}")
        return data

    async def carrier_get_position(self) -> dict:
        """Read real carrier position in mm and heading in degrees. Coordinates are relative to the manually set origin, can drift, and do not describe obstacles. Use this before moving. Do not invent position or claim successful movement without a completed result."""
        try:
            async with self._client() as client:
                d = await self._request(client, "GET", "/api/telemetry")
                return {key: d.get(key) for key in
                        ("firmware", "boot_id", "sample", "pose", "motor", "action")}
        except Exception as exc:
            return {"ok": False, "error": str(exc)}

    async def carrier_move(self, distance_mm: float) -> dict:
        """Move a requested signed distance relative to the carrier's current heading and wait for the measured result. Positive is forward, negative is backward. Magnitude must be 10 to 500 mm. Call movements sequentially only when the user requests them in a clear area. This cannot detect obstacles or edges. Stop the plan on any fault; never automatically retry a failed or uncertain movement.
        :param distance_mm: Signed millimetres; positive forward, negative backward; magnitude 10 through 500.
        """
        return await self._movement("move", distance_mm)

    async def carrier_turn(self, angle_deg: float) -> dict:
        """Turn relative to the current heading and wait for the measured result. Positive is left/counterclockwise, negative is right/clockwise. Magnitude must be 5 to 180 degrees. Call movements sequentially. Stop the plan on any fault; never automatically retry a failed or uncertain movement.
        :param angle_deg: Signed degrees; positive left, negative right; magnitude 5 through 180.
        """
        return await self._movement("turn", angle_deg)

    async def carrier_stop(self) -> dict:
        """Stop and cancel any carrier movement immediately. This is available even while another movement tool is waiting. Does not reset coordinates or change motor factors."""
        try:
            async with self._client() as client:
                await self._request(client, "POST", "/api/motor", {"cmd": "S"})
                # Firmware PWM writes run independently at 10 ms intervals.
                for _ in range(5):
                    await asyncio.sleep(0.04)
                    status = await self._request(client, "GET", "/api/action")
                    if status.get("motor_stopped"):
                        return {"ok": True, "motor_stopped": True, "action": status}
                return {"ok": False, "error": "STOP sent but motor stop not confirmed"}
        except Exception as exc:
            return {"ok": False, "error": str(exc), "motor_stopped": "unconfirmed"}

    async def _movement(self, kind, value):
        limit = (10, 500) if kind == "move" else (5, 180)
        if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
            return {"ok": False, "error": "A finite numeric movement value is required"}
        if not limit[0] <= abs(value) <= limit[1]:
            return {"ok": False, "error": f"Magnitude must be {limit[0]}..{limit[1]}"}
        if self._movement_lock.locked():
            return {"ok": False, "error": "Another movement is active. Run actions sequentially."}
        async with self._movement_lock:
            async with self._client() as client:
                identity = None
                uncertain_or_active = False
                try:
                    d = await self._request(client, "GET", "/api/telemetry")
                    if d.get("firmware") != "2.4":
                        raise ValueError("Install PhotonicsCarrierAgent firmware 2.4 first")
                    p = d["pose"]
                    if not p["ready"] or not p["valid"] or p["incomplete"] or p["origin_pending"]:
                        raise ValueError("Tracking is not complete and ready. Check the surface, then manually set level/origin.")
                    current = await self._request(client, "GET", "/api/action")
                    if current["boot_id"] != d["boot_id"]:
                        raise ValueError("Robot restarted during preparation")
                    if current["state"] == "running" or not current["motor_stopped"]:
                        raise ValueError("Carrier is already moving")
                    identity = {"boot_id": d["boot_id"], "id": current["id"] + 1}
                    params = dict(identity, origin_id=p["origin_id"], kind=kind,
                                  value=value, speed=self.valves.POWER_PERCENT)
                    uncertain_or_active = True
                    try:
                        result = await self._request(client, "POST", "/api/action", params)
                    except httpx.HTTPStatusError:
                        # A rejected start did not authorize this tool to cancel another action.
                        uncertain_or_active = False
                        raise
                    deadline = time.monotonic() + 23
                    while True:
                        if result.get("boot_id") != identity["boot_id"] or result.get("id") != identity["id"]:
                            raise ValueError("Robot boot or active action changed")
                        if result.get("state") not in ("running", "completed", "cancelled", "fault"):
                            raise ValueError("Unexpected action status")
                        if result["state"] != "running" and result.get("motor_stopped"):
                            # Return the measured pose after a short settling interval, not only
                            # the position that originally crossed the controller's tolerance.
                            await asyncio.sleep(0.25)
                            settled = await self._request(client, "GET", "/api/action")
                            if settled.get("boot_id") != identity["boot_id"] or settled.get("id") != identity["id"]:
                                raise ValueError("Action changed while settling")
                            if not settled.get("motor_stopped"):
                                raise ValueError("Motor activity resumed while settling")
                            uncertain_or_active = False
                            return {"ok": settled["state"] == "completed" and settled["pose"]["healthy"],
                                    "action": settled,
                                    "note": "Pose is measured relative odometry; progress is the value at action termination."}
                        if time.monotonic() >= deadline:
                            raise TimeoutError("Action did not terminate within its deadline")
                        await asyncio.sleep(0.2)
                        result = await self._request(client, "POST", "/api/action/heartbeat", identity)
                except asyncio.CancelledError:
                    # The finally clause cancels this action; firmware also expires heartbeats.
                    raise
                except Exception as exc:
                    return {"ok": False, "error": str(exc),
                            "instruction": "Do not automatically repeat this movement. Read status and resolve the cause first."}
                finally:
                    if uncertain_or_active and identity is not None:
                        try:
                            await self._request(client, "POST", "/api/action/cancel", identity)
                        except Exception:
                            pass  # No renewals remain; firmware's client/motor timeouts still apply.


if __name__ == "__main__":
    # Local command-line check uses exactly the same tool as Open WebUI.
    import argparse
    parser = argparse.ArgumentParser(description="Photonics Carrier firmware 2.4 client")
    parser.add_argument("command", choices=["status", "move", "turn", "stop"])
    parser.add_argument("value", type=float, nargs="?")
    parser.add_argument("--url", default="http://192.168.4.1")
    parser.add_argument("--power", type=int, default=20)
    args = parser.parse_args()
    tool = Tools()
    tool.valves.ROBOT_URL = args.url
    tool.valves.POWER_PERCENT = args.power
    if args.command in ("move", "turn") and args.value is None:
        parser.error("move and turn require a signed numeric value")
    if not 15 <= args.power <= 40:
        parser.error("power must be 15..40")
    call = {"status": tool.carrier_get_position, "move": tool.carrier_move,
            "turn": tool.carrier_turn, "stop": tool.carrier_stop}[args.command]
    result = asyncio.run(call(args.value) if args.command in ("move", "turn") else call())
    print(json.dumps(result, indent=2))

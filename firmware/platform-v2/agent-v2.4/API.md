# Photonics Carrier HTTP API — firmware 2.4

Connect to the carrier AP; use its printed IP as the base URL. Parameters are
URL query parameters on POST requests, matching the existing web server. JSON
request bodies are not used. Responses are JSON with `Cache-Control: no-store`.

## Read state

`GET /api/telemetry` retains all v2.3 fields, sets `schema: 3` and `firmware: "2.4"`,
and adds `action: {id, state, kind, reason, target, progress}`. It is a cached
snapshot refreshed about every 100 ms. Check `boot_id`, `sample`, sensor ages,
and pose health; fetching a cached snapshot does not force a new sensor sample.

`GET /api/action` returns the current/most recent action directly:

```json
{
  "boot_id": 123,
  "id": 1,
  "state": "running",
  "kind": "move",
  "reason": "running",
  "target": 50,
  "progress": 20,
  "units": "mm",
  "speed_percent": 20,
  "origin_id": 1,
  "elapsed_ms": 600,
  "motor_stopped": false,
  "pose": {"x_mm": 20, "y_mm": 0, "heading_deg": 0, "healthy": true}
}
```

States: `idle`, `running`, `completed`, `cancelled`, `fault`.
`elapsed_ms` is age since action start (continues increasing after termination).
`progress` freezes at termination; `pose` continues reflecting measured robot
state. `motor_stopped` means commanded/applied PWM is zero, not independently
measured mechanical standstill. The supplied client waits an additional 250 ms
and returns the then-current pose after a terminal result and zero PWM.

## Start a bounded action

```text
POST /api/action?boot_id=123&origin_id=1&id=1&kind=move&value=50&speed=20
POST /api/action?boot_id=123&origin_id=1&id=2&kind=turn&value=-30&speed=20
```

- Read `boot_id` and `pose.origin_id` from fresh telemetry.
- Read the latest action ID from `/api/action`; use a strictly larger positive
  unsigned 32-bit ID for the next action. IDs are reset by reboot.
- `kind=move`: signed millimetres, magnitude 10–500. Positive forward, negative backward.
- `kind=turn`: signed degrees, magnitude 5–180. Positive left/CCW, negative right/CW.
- `speed`: integer base PWM percentage, 15–40. Wheel factors still apply once.
- Start requires stopped motors, valid complete tracking, correct boot/origin,
  an operational motor timer and both wheel factors greater than zero.

Response is the same action object as GET; success starts asynchronously.
To prevent duplicate motion after a lost reply, the same ID and identical
parameters return the existing result **without restarting or renewing it**.
Reusing that ID with different parameters, or sending an older ID, returns 409.
The firmware retains only the most recent action, not a queue/history.

## Maintain or cancel

```text
POST /api/action/heartbeat?boot_id=123&id=1
POST /api/action/cancel?boot_id=123&id=1
```

Send heartbeats about every **200 ms** while running. Deadline: **1,000 ms**.
GET polling alone does not renew a movement. The supplied tool manages this
while waiting, independently of the language model. Do not expose heartbeats
as model-generated individual actions.

Both routes require the current boot and action ID. Heartbeats cannot restart
completed/failed/cancelled actions. An overdue heartbeat faults the action
rather than reviving it. Cancel for an old ID does not affect a newer action.

`POST /api/motor?cmd=S` remains the global STOP. It requires no lease and cancels
an automated action as well as stopping manual movement.

## Existing interfaces retained

- `/api/arm`, `/api/motor?cmd=F|B|L|R&epoch=...&seq=...`: existing manual lease
  and increasing sequence protocol; renew within 750 ms. Rejected while an
  automated action is running. STOP remains available.
- `/api/speed?value=0..100`: manual speed; cancels an active automated action.
- `/api/motor-scale?left=0..2&right=0..2`: stop and apply independent factors.
- `/api/invert?side=L|R`, `/api/level`, `/api/zero`, `/api/config`: preserved.
- `/imu`: existing telemetry alias.
- `/`: existing page plus measured-move controls. Loading the page sends STOP.

The automation controller adjusts pre-scaled left/right commands to maintain
the starting heading during straight movement. It tapers base power near the
target. It never changes your calibrated factors or automatically resets pose.

The standard rotation-vector sensor driver, optical fusion, motor pins,
independent PWM timer, Wi-Fi AP and browser data export remain in place.

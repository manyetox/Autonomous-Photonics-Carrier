# Photonics Carrier — agent control, firmware 2.4

This update lets a local model in Ollama/Open WebUI read the carrier's position,
move a signed distance, turn a signed angle, and stop. The ESP32 controls the
motors using the existing BNO086 + PAW3395 odometry. The laptop tool renews the
connection while waiting for each action; model generation speed does not
control motor timing. A camera is not needed for this movement demonstration.

## Files

- `PhotonicsCarrierAgent/PhotonicsCarrierAgent.ino`: updated firmware.
- `PhotonicsCarrierAgent/MotionController.h`: controller used by the firmware.
  **Keep this in the same folder as the .ino file.**
- `carrier_openwebui_tool.py`: paste this whole file into an Open WebUI Workspace Tool.
  It can also run as a command-line client.
- `requirements-client.txt`: dependencies for the standalone client.
- `API.md`: the firmware's HTTP interface and limits.

## 1. Upload the firmware

1. Extract the package. Open `PhotonicsCarrierAgent/PhotonicsCarrierAgent.ino`
   in Arduino IDE. Both sketch files should be present in that folder.
2. Select **ESP32 Dev Module**, your carrier's port, and the same flash/upload
   settings that worked with the previous sketch. This version was compiled
   against ESP32 Arduino core **3.0.7** and Adafruit BNO08x **1.2.5**.
3. Upload. The motor pins, BNO086 initialization, PAW3395 initialization and
   odometry calculation are retained from the supplied v2.3 sketch.
4. At 115200 baud, the serial output prints the carrier's Wi-Fi name and IP.
   Join **PhotonicsCarrier** using the existing password (`carrier123`).
   Open the reported address; the normal ESP32 AP address is `http://192.168.4.1`.

This package has been compiled, but has not been flashed or physically tested
on your carrier by the assistant.

## 2. Set up your two different motors

**Both independent motor power factors remain available.**

- Left factor and right factor each accept **0.00–2.00**.
- 1.00 applies the normal command; 0.80 applies 80%; 1.20 applies 120%.
- Final PWM is capped at 255. Each factor is applied exactly once, followed by
  the existing left/right inversion settings.
- The factors affect **both manual and automated movement**, including turns.
- Applying factors stops/cancels any active movement.
- Factors and inversion settings are still **RAM only**, as in v2.3. Reapply
  your calibrated values after each restart. No flash storage was added.
- Automated actions require both factors to be greater than zero; manual
  driving still allows a zero factor for motor testing.

Use the existing left/right factor controls and **STOP & APPLY MOTOR FACTORS**
button. Set motor inversions as needed so FORWARD really moves forward and
LEFT really turns counterclockwise. The two motors need not be identical, but
large mismatches, stiction and different forward/reverse behaviour still need
physical tuning. A factor scales PWM; it does not measure wheel RPM.

## 3. Check positioning and measured moves

1. Place the carrier flat on its normal textured driving surface. Keep it still
   for 3–5 seconds, then press **SET LEVEL & RESET ORIGIN**.
2. Confirm the position status says **Tracking**, without an incomplete warning.
3. Check optical direction/scale against a ruler. If the optical sensor is
   off-centre, enter its actual offsets so rotation does not appear as
   translation of the carrier centre. The IMU's physical offset is not the
   optical sensor offset.
4. In **Measured movements**, try **50 mm**, action power **20%**, then a **30°**
   turn, on a clear floor. The normal STOP button cancels either action.
5. Measure the actual movement. Tune your factors and optical calibration before
   asking for larger moves. The initial controller tolerances are 5 mm and 2°;
   they are stop thresholds in the estimated pose, not guaranteed physical accuracy.

Positive distance means forward. Negative distance means backward. Positive
angle means left/counterclockwise; negative angle means right/clockwise.

The manual speed slider is separate from action power. Agent actions use the
Open WebUI tool's POWER_PERCENT setting. Both use the same wheel factors.

Page-started actions require that page to stay visible. Opening, reloading,
leaving or closing the control page sends STOP, preserving the old manual
control behaviour. **Finish setup and close the carrier page before starting
an agent demonstration.** Use the agent's position tool to monitor it instead.

## 4. Connect Open WebUI

1. In **Workspace → Tools**, create a tool and paste the complete contents of
   `carrier_openwebui_tool.py`. Save it as **Photonics Carrier**.
2. Set its Valves: `ROBOT_URL` to the printed carrier address and
   `POWER_PERCENT` initially to **20**. The permitted base-power range is 15–40%.
   Wheel factors are applied afterward, so a factor above 1 can increase final PWM.
3. Enable this tool in the chat with your local Gemma model. Use **Native**
   function calling in the model/chat advanced settings. Your particular Ollama
   model and version must support tool calls.
4. Ask: **“Use the carrier tool to report its current position. Do not move yet.”**
5. Once the returned readings are correct, ask: **“Move forward 50 mm, then
   report the measured final position. Stop if any action fails.”**
6. Try a turn separately before asking for a sequence.

The tool exposes only position, move, turn and stop. It does not let the model
change motor factors, invert motors, or reset/calibrate the coordinate frame.
Make those changes yourself in the carrier page.

If Open WebUI runs in Docker or WSL, **its server environment**, not just your
Windows browser, must reach the carrier's Wi-Fi address. A timeout from the
position tool means to check that network route first. The tool does not need
an internet connection after the model/software are installed.

Suggested model instruction:

> You operate the Photonics Carrier only through its four carrier tools. Read
> position before movement. Coordinates are relative odometry, not obstacle
> detection. Execute requested movements one at a time and wait for each tool
> result. Report actual measurements. A cancelled, faulted, uncertain or unhealthy
> result is not success. Stop the plan on failure and do not retry automatically.
> Do not reset the origin or invent calibration. Move only when the user asks.

Open WebUI documentation:
- https://docs.openwebui.com/features/extensibility/plugin/tools/
- https://docs.openwebui.com/features/extensibility/plugin/tools/development/

## Optional: test from the laptop without the model

With Python installed, run from the extracted package folder:

```powershell
python -m pip install -r requirements-client.txt
python carrier_openwebui_tool.py status
python carrier_openwebui_tool.py move 50 --power 20
python carrier_openwebui_tool.py turn -30 --power 20
python carrier_openwebui_tool.py stop
```

Add `--url http://YOUR-CARRIER-IP` if the address differs. Commands that move
wait for the result. The tool sends heartbeats without asking the model to
generate repeated commands. The firmware continues enforcing its own limits.

## What stops an automated action

- STOP, calibration, origin reset, inversion/factor changes, or manual speed changes.
- Invalid/incomplete tracking, stale sensors, excessive tilt, or a changed origin.
- Missing client heartbeat for 1 second.
- Independent 750 ms motor watchdog if the firmware's main loop stops renewing
  output. A recovered loop cannot silently restart that action.
- No measured forward progress for 2.5 seconds, or a total duration over 20 seconds.
- More than 30 mm lateral deviation or 20° heading deviation during a straight move.
- More than 50 mm translation during a turn, or substantial wrong-direction progress.

Faults remain visible; they are not automatically retried. If tracking became
incomplete, inspect the cause and set a new origin manually. The firmware still
allows manual driving with unreliable odometry for calibration, as v2.3 did.
Automated actions do not.

The robot does not detect obstacles or edges. Keep the first tests short and
slow on a clear floor. Magnetic disturbances, surface quality, wheel mismatch
and coasting can affect the achieved position. This version provides bounded
distance/turn actions, not autonomous navigation to arbitrary coordinates.

## Verification performed

- Compiled successfully for ESP32 Dev Module, Arduino ESP32 core 3.0.7.
- Program: 1,039,353 bytes (79% of the selected app partition).
- Static RAM: 55,416 bytes (16%).
- Native C++ controller tests: signed movements, coordinate rotation, heading
  wrap, timeout/stall handling, origin/health checks, cancellation and clock wrap.
- Tests of actual firmware HTTP handlers with simulated hardware: one-time
  scaling/inversion, manual controls, action conflicts, repeat requests,
  heartbeat/watchdog expiry, parameter validation and scoped cancellation.
- 12 simulated Open WebUI client tests: success/fault reporting, invalid inputs,
  lost start/heartbeat replies, restart detection, cancellation and concurrent STOP.
- Embedded browser JavaScript syntax checked.

Physical accuracy and your installed Ollama/Open WebUI integration still need
the short tests above; no robot movement was performed during software testing.

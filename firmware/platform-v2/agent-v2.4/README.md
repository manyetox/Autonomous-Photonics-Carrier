# Platform V2 — Agent firmware v2.4

This is the latest local V2 firmware package from the carrier project, published on 29 October 2026. The sketch and MotionController.h are copied without changes. It targets **ESP32 + BNO086 + PAW3395**, with manual web control, independent left/right motor factors, level/origin calibration, relative positioning, telemetry exports, measured movement actions and the bundled Open WebUI client.

Start with [START_HERE.md](START_HERE.md) and the [HTTP API](API.md). Keep both sketch files in `PhotonicsCarrierAgent/`. V3 LSM6DS3 support is pending; this package is not compatible with the V3 IMU.

## Evidence

The author reports V2 motors, IMU and optical flow working. That does not establish physical validation of every v2.4 automated action. This publication reran software checks without flashing or moving the carrier:

- ESP32 Arduino core 3.0.7 compile passed with Adafruit BNO08x 1.2.5.
- Sketch: 1,039,353 bytes flash (79%); global variables: 55,416 bytes RAM (16%).
- Motion-controller host tests, firmware API/watchdog/replay/motor-scaling tests, and embedded JavaScript syntax check passed.
- All 12 simulated client tests passed, including failures, cancellation, reboot and lost replies.

## Reproduce host tests

From this directory, with Python, a C++17 compiler and Node.js installed:

```sh
python -m pip install -r requirements-client.txt
python tests/test_firmware.py
python -m unittest discover -s tests -p test_client.py
g++ -std=c++17 tests/test_motion.cpp -o motion-test
./motion-test
```

`CXX` and `NODE` may select compiler/Node executables for the firmware test harness. The published harness uses portable executable names in place of the original local Windows paths. Generated test files go in `.build-tools/`.

Motor power scales PWM, not measured wheel RPM. The carrier has no obstacle/edge detector; calibration, direction checks and physical movement verification remain necessary. The package has no magnetic-encoder integration.

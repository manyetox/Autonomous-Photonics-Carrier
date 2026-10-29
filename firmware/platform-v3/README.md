# Platform V3 firmware — pending

At the author's request, this update publishes the existing V2 firmware and keeps V3 firmware pending.

The V3 motor/IMU board uses LSM6DS3 instead of BNO086. The [V2 Agent v2.4 package](../platform-v2/agent-v2.4/README.md) must not be described as LSM6DS3 firmware. V3 needs its own driver, orientation estimation, confirmed ESP32 wiring, calibration and hardware tests.

The V3 board connector is J2 with 12 pins, whereas V2 MDI-R2.0 used a 13-pin connector. Consult the [V3 pinout](../../docs/platform-v3.md) before wiring.

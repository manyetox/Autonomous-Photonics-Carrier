# Platform V2 — working modular prototype

Platform V2 is the working modular carrier baseline; Platform V3 is now in development. It is a **system generation**, not a PCB revision number.

Current boards:

- **MB-R2.1** — main controller/power/breakout
- **MDI-R2.0** — motor driver + BNO086 IMU
- **OFS-R2.1.1** — PAW3395 optical-flow sensor
- **ENC-R1.0** — MA730 wheel encoder

As of 29 October 2026, the author confirms V2 works, including the motors, BNO086 IMU, and PAW3395 optical-flow sensor. The MA730 magnetic encoders remain **untested because of component-ordering delays**.

Earlier documentation incorrectly labelled the encoder as validated and optical flow as unvalidated. These status records are corrected; measured positioning accuracy is still to be established.

These files describe V2 hardware. The lower-cost LSM6DS3 belongs to [V3 development](../../docs/platform-v3.md), not to the unchanged MDI-R2.0 board.

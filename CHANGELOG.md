# Changelog / design lineage

This file records architectural changes rather than every KiCad edit.

## Platform V3 — development started (29 October 2026)

- Selected **STMicroelectronics LSM6DS3** as the lower-cost replacement for the V2 BNO086, confirmed from the 27 October schematic discussion.
- Imported native MDI-R3.0 and OFS-R2.2 KiCad designs and supplied draft fabrication outputs from the author’s archive; both boards use two copper layers.
- Added schematic-derived component lists, import checksums and KiCad DRC reports; outstanding DRC errors are documented.
- Published the existing V2 Agent v2.4 firmware/client and retained V3 firmware as pending at the author’s request.
- Preserve V2 design files as the BNO086 hardware baseline.

## Platform V2 — working prototype update (29 October 2026)

- Author confirms V2 works; motors, BNO086 IMU, and PAW3395 optical-flow sensing operate.
- Correct earlier documentation: **PAW3395 is working; MA730 magnetic encoders are untested** because component ordering is delayed.
- Board bring-up does not establish quantitative positioning accuracy or complete autonomous optical alignment.

## Platform V2 — modular architecture (August–September 2026)

- Reworked the carrier into a modular architecture centred on an **ESP32-WROOM-32UE** mainboard.
- Replaced the V1 integrated motor/stepper approach with a dedicated **TC78H651** dual H-bridge board for low-speed DC gearmotors.
- Retained **BNO086** inertial sensing on the motor/IMU daughterboard.
- Added a dedicated **PAW3395** bottom-facing optical-flow board with local **1.8 V regulation**.
- Added a dedicated **MA730** magnetic wheel-angle encoder board.
- Removed UWB from the current core sensing architecture.
- Decoupled fine optomechanical alignment from the mobile-base electronics; the carrier now focuses on robust coarse transport/localisation while fine correction can be added as a separate optomechanical layer.
- At the August–September milestone, the hardware design was complete and assembly was awaiting parts; see the October status update above.

## Platform V1 — integrated architecture (July–August 2026)

- Developed a roughly **50 × 50 mm, 8-layer** integrated carrier PCB.
- Used **ESP32-S3-WROOM-1** as controller.
- Integrated **DWM3000 UWB** and **BNO086 IMU**.
- Included **TMC5041** stepper-control stages and **DRV8231** motor-driver stages.
- Provided the first hardware implementation of the autonomous-photonics carrier concept.
- Retained in the repository as a validated historical design and design-reference point.

## Legacy reference — before Platform V1

The `hardware/legacy-reference/` directory contains an earlier general-purpose ESP32 robot-controller design. It influenced later design choices but is **not** a version of the autonomous photonics carrier.

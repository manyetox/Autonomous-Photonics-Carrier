# Changelog / design lineage

This file records architectural changes rather than every KiCad edit.

## Platform V2 — current architecture (August–September 2026)

- Reworked the carrier into a modular architecture centred on an **ESP32-WROOM-32UE** mainboard.
- Replaced the V1 integrated motor/stepper approach with a dedicated **TC78H651** dual H-bridge board for low-speed DC gearmotors.
- Retained **BNO086** inertial sensing on the motor/IMU daughterboard.
- Added a dedicated **PAW3395** bottom-facing optical-flow board with local **1.8 V regulation**.
- Added a dedicated **MA730** magnetic wheel-angle encoder board.
- Removed UWB from the current core sensing architecture.
- Decoupled fine optomechanical alignment from the mobile-base electronics; the carrier now focuses on robust coarse transport/localisation while fine correction can be added as a separate optomechanical layer.
- Current hardware design completed; system manufacturing remains partially complete while parts arrive.

## Platform V1 — integrated architecture (July–August 2026)

- Developed a roughly **50 × 50 mm, 8-layer** integrated carrier PCB.
- Used **ESP32-S3-WROOM-1** as controller.
- Integrated **DWM3000 UWB** and **BNO086 IMU**.
- Included **TMC5041** stepper-control stages and **DRV8231** motor-driver stages.
- Provided the first hardware implementation of the autonomous-photonics carrier concept.
- Retained in the repository as a validated historical design and design-reference point.

## Legacy reference — before Platform V1

The `hardware/legacy-reference/` directory contains an earlier general-purpose ESP32 robot-controller design. It influenced later design choices but is **not** a version of the autonomous photonics carrier.

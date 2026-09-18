# Platform V1 — integrated carrier

Platform V1 is the first photonics-specific carrier architecture. It is retained as a **validated historical design** and should not be confused with the legacy robot controller.

The current archived board identifier is **INT-R1.0.1**.

Key features visible in the PCB source include ESP32-S3-WROOM-1, DWM3000 UWB, BNO086, dual TMC5041 stepper-control ICs, DRV8231 motor-driver stages, and an 8-layer 50 × 50 mm PCB.

V1 is superseded by Platform V2 for ongoing work because V2 separates controller, motor/IMU, optical-flow, and wheel-encoder functions into smaller boards.

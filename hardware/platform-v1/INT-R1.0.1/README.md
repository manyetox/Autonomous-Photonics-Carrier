# INT-R1.0.1 — Platform V1 integrated PCB

**Platform:** V1  
**Board revision:** INT-R1.0.1  
**Status:** validated historical hardware; superseded by Platform V2  
**PCB:** 8 layers, approximately 50 × 50 mm

## Main subsystems

- ESP32-S3-WROOM-1 controller
- DWM3000 UWB module
- BNO086 IMU
- 2 × TMC5041 stepper-controller ICs
- 2 × DRV8231 motor-driver stages
- TPS56528DDA power stage
- USB/ESD and breakout interfaces

`source/` contains the editable KiCad files. `fabrication/current/` contains the final retained V1 Gerber/drill package. Earlier schematic export is kept only in `archive/`.

> **Image placeholder:** I will add a V1 board photograph or render here later.

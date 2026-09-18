# MDI-R2.0 — motor driver + IMU

**Platform:** V2  
**Board revision:** MDI-R2.0  
**Status:** validated; current  
**PCB:** 4 layers, approximately 25 × 27 mm

## Function

This daughterboard combines low-speed differential-drive actuation with inertial sensing.

## Key hardware

- **TC78H651FGN** dual H-bridge motor driver
- **BNO086** 9-axis smart IMU
- SPI-oriented IMU interface signals
- separate motor output pads

The design is intended for the low-speed DC-gearmotor V2 drivetrain rather than the V1 stepper/flexure architecture.

See `docs/interfaces-and-pinouts.md` for the 13-pin connector net names.

> **Image placeholder:** I will add the assembled motor/IMU board and its motor wiring here later.

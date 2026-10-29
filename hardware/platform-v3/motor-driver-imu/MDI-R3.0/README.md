# MDI-R3.0 — motor driver + LSM6DS3

**Platform:** V3 development snapshot  
**Status:** unvalidated development design  
**PCB:** two copper layers

Replaces the V2 BNO086 with LSM6DS3 and reduces the motor/IMU PCB from four copper layers to two. The PCB is marked V3.0. The schematic contains the TC78H651FNG motor-driver symbol; its component value is `~`, so the exact purchasing code still needs BOM review.

## Files

- `source/`: supplied native KiCad project, schematic and PCB; original filenames retained.
- [Component CSV](bom/components-from-schematic.csv): extracted from the schematic, not a verified purchasing BOM.
- `fabrication/supplied/`: supplied Gerber/drill outputs.

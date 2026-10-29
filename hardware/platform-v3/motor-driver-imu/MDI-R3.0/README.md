# MDI-R3.0 — motor driver + LSM6DS3

**Platform:** V3 development snapshot  
**Status:** unvalidated development design  
**PCB:** two copper layers

Replaces the V2 BNO086 with LSM6DS3 and reduces the motor/IMU PCB from four copper layers to two. The PCB is marked V3.0. The schematic contains the TC78H651FNG motor-driver symbol; its component value is `~`, so the exact purchasing code still needs BOM review.

## Files

- `source/`: supplied native KiCad project, schematic and PCB; original filenames retained.
- [Component CSV](bom/components-from-schematic.csv): extracted from the schematic, not a verified purchasing BOM.
- `fabrication/supplied-draft/`: supplied Gerber/drill outputs; not released for manufacturing.
- [DRC report](validation/drc.json): KiCad 10.0.7 checks on 29 October 2026.

DRC reported **30 errors and 20 warnings**, with zero unconnected items and zero schematic/PCB parity issues. This is not a clean DRC result. See [import validation](../../../../docs/hardware-import-validation.md) for the categories and limitations.

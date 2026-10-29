# Platform V3 — development hardware

Imported from the author's `new files.zip` on 29 October 2026. V3 development replaces the V2 BNO086 with the lower-cost STMicroelectronics LSM6DS3.

| Board | Supplied revision | State |
|---|---|---|
| [Motor driver + IMU](motor-driver-imu/MDI-R3.0/README.md) | MDI-R3.0; PCB marked V3.0 | LSM6DS3, two-layer design; unvalidated |
| [Optical flow](optical-flow/OFS-R2.2/README.md) | OFS-R2.2; PCB marked V2.2 | PAW3395 revision supplied with V3 work; unvalidated revision |

The optical board's V2.2 marking is a board revision, not a carrier generation. PAW3395 operation was verified on the V2 prototype; this does not independently validate the revised OFS-R2.2 board.

No revised mainboard, encoder board, or mechanical CAD was supplied. The existing [V2 hardware](../platform-v2/README.md) remains available as the baseline; final V3 integration is pending.

Each board folder contains the native KiCad project, schematic and PCB, a schematic-derived component CSV, supplied Gerber/drill files, and a KiCad 10.0.7 DRC report. Sources and supplied fabrication files are preserved byte-for-byte; see [import-manifest.json](import-manifest.json).

**These are development files, not a fabrication-approved release.** The supplied Gerbers are in `fabrication/supplied-draft/` because DRC reports errors and Gerber/source equivalence has not been independently established. Review the [hardware import checks](../../docs/hardware-import-validation.md) before manufacturing.

Editor history, backups, local settings, old Gerber directories and unverified older STEP models from the archive were not copied into V3. Earlier V2 design files remain unchanged.

V3 firmware is [pending](../../firmware/platform-v3/README.md). The published [Agent v2.4 firmware](../../firmware/platform-v2/agent-v2.4/README.md) is for V2 BNO086 hardware.

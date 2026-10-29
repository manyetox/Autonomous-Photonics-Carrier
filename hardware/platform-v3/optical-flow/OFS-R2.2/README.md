# OFS-R2.2 — PAW3395 optical flow

**Platform:** V3 development snapshot  
**Status:** unvalidated development design  
**PCB:** two copper layers

The PCB is marked V2.2 and uses two copper layers. This is the revised optical-flow board supplied with the V3 development files; the author-confirmed working V2 sensor does not establish validation of this revision.

## Files

- `source/`: supplied native KiCad project, schematic and PCB; original filenames retained.
- [Component CSV](bom/components-from-schematic.csv): extracted from the schematic, not a verified purchasing BOM.
- `fabrication/supplied-draft/`: supplied Gerber/drill outputs; not released for manufacturing.
- [DRC report](validation/drc.json): KiCad 10.0.7 checks on 29 October 2026.

DRC reported **23 errors and 37 warnings**, with zero unconnected items and zero schematic/PCB parity issues. This is not a clean DRC result. See [import validation](../../../../docs/hardware-import-validation.md) for the categories and limitations.

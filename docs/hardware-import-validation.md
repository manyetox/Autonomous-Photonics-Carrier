# Hardware import validation — 29 October 2026

Imported from `new files.zip`. KiCad 10.0.7 DRC was run against both supplied boards with `--schematic-parity`. No source changes or zone refills were performed. Zero unconnected items and zero schematic/PCB parity issues were reported for each board. DRC errors remain, so the included fabrication outputs are labelled supplied drafts, not approved manufacturing packages.

## MDI-R3.0

| Severity | Rule | Count |
|---|---|---:|
| error | annular_width | 14 |
| error | starved_thermal | 2 |
| error | via_diameter | 14 |
| warning | lib_footprint_mismatch | 1 |
| warning | silk_edge_clearance | 4 |
| warning | text_height | 15 |

[Full DRC report](../hardware/platform-v3/motor-driver-imu/MDI-R3.0/validation/drc.json).

## OFS-R2.2

| Severity | Rule | Count |
|---|---|---:|
| error | annular_width | 17 |
| error | copper_edge_clearance | 1 |
| error | courtyards_overlap | 1 |
| error | starved_thermal | 4 |
| warning | silk_edge_clearance | 2 |
| warning | silk_over_copper | 4 |
| warning | silk_overlap | 2 |
| warning | text_height | 15 |
| warning | text_thickness | 14 |

[Full DRC report](../hardware/platform-v3/optical-flow/OFS-R2.2/validation/drc.json).

## Interpretation

The reports include annular-ring, via-size, thermal-relief and optical-board clearance/overlap issues. Review these against the intended manufacturer and physical assembly. No trace, footprint, via or design-rule edits were made merely to silence DRC. Library-related warnings depend on the installed library versions; the native source files contain their embedded symbol/footprint data.

## Reproducibility

Native sources and supplied Gerber/drill files were copied byte-for-byte; SHA-256 values and archive paths are recorded in [the import manifest](../hardware/platform-v3/import-manifest.json). Component CSVs were generated from schematic netlists. They retain incomplete source values, including the motor driver value `~`, and are not a purchasing BOM.

The supplied Gerbers were not regenerated and have not been proven equivalent to the current source. Older STEP models and previous Gerbers in the ZIP were not promoted to V3. No new hardware was built or physically tested during this update.

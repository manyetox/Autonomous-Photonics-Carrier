# Manufacturing and assembly guide

## Before ordering anything

1. Read `VERSIONING.md`.
2. Use only the `current/` fabrication package for the board you intend to build.
3. Open the corresponding KiCad source and compare the generated Gerbers against the source revision.
4. Check footprints, connector orientation, board stack-up, copper layers, drill files, and outline in the fabricator viewer.
5. Re-check the BOM against current component datasheets and stock. The included CSV BOMs are extraction aids, not purchasing authority.

## Current fabrication targets

| Board | Current fab revision | Layers | Approx. outline |
|---|---|---:|---:|
| Platform V1 integrated PCB | INT-R1.0.1 | 8 | 50 × 50 mm |
| V2 mainboard | MB-R2.1 | 4 | 25 × 27.1 mm |
| V2 motor/IMU | MDI-R2.0 | 4 | 25 × 27 mm |
| V2 PAW3395 optical flow | OFS-R2.1.1 | 2 | 19.3 × 24.0 mm |
| V2 MA730 encoder | ENC-R1.0 | 2 | 20 × 16.8 mm |

## Assembly priorities

### Mainboard

- Confirm 3.3 V regulator orientation and passive values before fitting the ESP32.
- Verify 3.3 V rail resistance to ground before first power-up.
- Bring up the board from a current-limited bench supply when possible.
- Verify EN/BOOT/programming access before connecting daughterboards.

### Motor/IMU board

- Confirm the TC78H651 package orientation and motor-output pads.
- Check the BNO086 orientation relative to the carrier coordinate frame and record the convention in software.
- Power logic and motor rails cautiously; test each H-bridge at low duty cycle with the carrier lifted off the table.

### PAW3395 optical-flow board

This is currently the only V2 board not experimentally validated, so it deserves the most cautious bring-up.

- Check the 1.8 V rail before installing/energising the PAW3395 if the assembly process allows.
- Verify sensor orientation and the optical lens/mechanical height.
- Confirm SPI idle states and nCS behaviour before attempting motion reads.
- The schematic specifies **R3 = 2 MΩ**; do not silently substitute a different value in published builds.
- Treat optical height, surface texture, and lens selection as part of the sensor system, not just PCB assembly.

> **Image placeholder — PAW3395 assembly:** I will add a close-up showing the sensor/lens spacing and the underside mounting here.

### MA730 encoder

- The magnetic target, air gap, centring, and alignment are as important as the PCB.
- Validate angle continuity through a full revolution before using data for odometry.
- Record the exact magnet geometry used in experimental data.

> **Image placeholder — encoder magnet:** I will add the final magnet and wheel/shaft arrangement here after mechanical integration.

## Power and batteries

The repository does not include a certified charger or battery-management product. Use an appropriate protected cell/pack and charger for the selected 1S LiPo system. Never treat the PCB as a substitute for battery protection.

## KiCad-generated local files intentionally removed

The public repository excludes `.history/`, `*-backups/`, `*.kicad_prl`, and lock files. These are editor state/backups rather than reproducible design inputs and would make the project history unnecessarily confusing.

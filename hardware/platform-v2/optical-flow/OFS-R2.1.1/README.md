# OFS-R2.1.1 — PAW3395 optical-flow sensor

**Platform:** V2  
**Board revision:** OFS-R2.1.1  
**Status:** current design; **not yet experimentally validated**  
**PCB:** 2 layers, approximately 19.3 × 24.0 mm

## Function

This board mounts a bottom-facing PAW3395 optical tracking sensor so the carrier can measure relative motion against the tabletop independently of wheel rotation.

## Key hardware

- PAW3395 optical tracking sensor
- AP2127K-1.8 local 1.8 V LDO
- SPI (`SCLK`, `MOSI`, `MISO`, `nCS`)
- `MOTION` output
- lens/mechanical optical stack

The current schematic specifies **R3 = 2 MΩ**.

This board should not be described as working until the 1.8 V rail, SPI initialisation, motion data, lens height, and calibrated displacement have been tested on the assembled hardware.

> **Image placeholder:** I will add a photograph showing the PAW3395 lens and underside mounting here after validation.

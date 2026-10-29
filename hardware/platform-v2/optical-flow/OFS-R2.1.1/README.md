# OFS-R2.1.1 — PAW3395 optical-flow sensor

**Platform:** V2  
**Board revision:** OFS-R2.1.1  
**Status:** **working, author-confirmed** (29 October 2026)<br>
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

The author has verified the PAW3395 on the prototype. This corrects the earlier unvalidated status. Surface sensitivity, lens height, displacement calibration, and repeatability still need documented quantitative results.

> **Image placeholder:** I will add a photograph showing the PAW3395 lens and underside mounting here after validation.

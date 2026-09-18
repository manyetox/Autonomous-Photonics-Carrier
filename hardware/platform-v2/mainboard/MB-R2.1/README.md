# MB-R2.1 — Platform V2 mainboard

**Platform:** V2  
**Board revision:** MB-R2.1  
**Status:** validated; current  
**PCB:** 4 layers, approximately 25 × 27.1 mm

## Function

MB-R2.1 is the central controller and power/breakout board. It intentionally avoids embedding every sensor/actuator subsystem so that daughterboards can be changed independently.

## Key hardware

- ESP32-WROOM-32UE
- NCP186-series 3.3 V LDO design
- TC2030 programming connector
- two 13-pin expansion headers
- IO13 user/status LED
- 3.3 V power indicator

See `docs/interfaces-and-pinouts.md` for connector nets.

## Fabrication

Use only `fabrication/current/MB-R2.1-gerbers.zip` for the current board. `archive/MB-R2.0-gerbers-superseded.zip` is kept only for design history.

> **Image placeholder:** I will add a photograph of the assembled MB-R2.1 here later.

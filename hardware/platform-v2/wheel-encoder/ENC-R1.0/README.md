# ENC-R1.0 — MA730 wheel encoder

**Platform:** V2  
**Board revision:** ENC-R1.0  
**Status:** validated; current  
**PCB:** 2 layers, approximately 20 × 16.8 mm

## Function

ENC-R1.0 measures wheel/shaft angle using the **MA730 magnetic angle sensor**. It replaces earlier exploration of sparse Hall-sensor edge counting with a more direct angle-sensing approach.

## Interfaces

- SPI: MOSI, MISO, SCLK, CS
- 3.3 V and GND
- optional A/B/Z outputs
- PWM output
- MGL/MGH magnetic-field diagnostic outputs

The magnetic target and air gap are part of the measurement system. Experimental records should identify the magnet geometry and mechanical spacing.

> **Image placeholder:** I will add the encoder mounted next to the final wheel/shaft magnet here later.

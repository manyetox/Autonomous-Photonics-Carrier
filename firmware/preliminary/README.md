# Preliminary Platform V2 firmware scaffold

This is a **starting point**, not finished autonomous-carrier firmware.

It is intentionally conservative:

- targets the ESP32-WROOM-32UE main controller through the generic PlatformIO `esp32dev` board definition;
- controls the confirmed **IO13 status LED**;
- exposes a raw ADC diagnostic for the mainboard ADC-labelled net;
- defines a dual-H-bridge motor abstraction but refuses to drive motors until final GPIO mappings are supplied;
- provides serial commands for status/stop/motor testing;
- provides explicit placeholder interfaces for BNO086, PAW3395, and MA730 integration;
- performs no autonomous motion at boot.

The daughterboard connector signal names are known from KiCad, but the final physical signal-to-ESP32 interconnect is not encoded here unless it is unambiguous. This is deliberate: public firmware should not turn a guessed harness mapping into an apparent design fact.

## Build

Install PlatformIO and run:

```bash
pio run
```

Upload/monitor commands depend on the programmer/serial arrangement used with the TC2030/mainboard.

## First bring-up sequence

1. Build and flash with all peripheral pin assignments left disabled.
2. Confirm boot messages and IO13 LED heartbeat.
3. Confirm the actual daughterboard-to-mainboard wiring from the assembled carrier.
4. Populate `include/board_config.h` with the final GPIO numbers.
5. Test one motor at a time with the carrier lifted and a current-limited supply.
6. Integrate BNO086, MA730, then PAW3395 drivers individually.
7. Only after sensor sign/frame checks should closed-loop motion be enabled.

## AI assistance

This preliminary scaffold was written with some assistance from **OpenAI ChatGPT 5.6**, using the project's KiCad net names and existing architecture as context. It has not yet been validated as the final firmware and should be reviewed against device datasheets during implementation.

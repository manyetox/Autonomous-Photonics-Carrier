# Platform V3 — development notes

Updated 29 October 2026.

## Baseline and change

The author confirms V2 works, including motors, BNO086 and PAW3395. MA730 magnetic encoders remain untested because component ordering is delayed; the requested encoder part is MA730GQ-P.

V3 replaces BNO086 with the lower-cost **STMicroelectronics LSM6DS3**. This is confirmed both in the 27 October schematic discussion and in the newly supplied KiCad schematic/PCB. The exact purchasing suffix is not confirmed; do not silently substitute LSM6DS3TR-C.

The [V3 hardware snapshot](../hardware/platform-v3/README.md) contains **MDI-R3.0** and **OFS-R2.2**. The motor/IMU board is now two-layer, and its interface changes from the V2 13-pin connector to a 12-pin J2. No replacement mainboard or encoder source was supplied.

## MDI-R3.0 J2 connector

Extracted from the supplied schematic netlist; this is the board connector, not an ESP32 GPIO mapping.

| Pin | Net |
|---:|---|
| 1 | Vin |
| 2 | GND |
| 3 | +3.3V |
| 4 | MOTOR_R_IN2 |
| 5 | MOTOR_R_IN1 |
| 6 | MOTOR_L_IN2 |
| 7 | MOTOR_L_IN1 |
| 8 | IMU_INT_N |
| 9 | SPI_MISO |
| 10 | SPI_MOSI |
| 11 | SPI_SCLK |
| 12 | IMU_CS_N |

U1 is LSM6DS3: VDDIO (5) and VDD (8) connect to 3.3 V; pins 2, 3, 6 and 7 connect to GND. Pin 1 is MISO, 14 MOSI, 13 SCLK, 12 CS and 4 INT1. INT2 (9) and NC pins 10/11 have no connections. This records the supplied design, not a full electrical-design approval.

## Firmware

The LSM6DS3 contains a three-axis accelerometer and three-axis gyroscope, with SPI/I2C. Replacing BNO086 requires a new driver and firmware orientation estimator instead of its rotation-vector interface. Gyro bias, timing, mounting axes and heading drift require validation.

Reference: [ST LSM6DS3 datasheet, DocID026899 Rev 10](https://community.st.com/ysqtg83639/attachments/ysqtg83639/mems-sensors-forum/21052/1/st_imu_lsm6ds3_datasheet.pdf).

At the author's request, [V3 firmware remains pending](../firmware/platform-v3/README.md); the [published V2 Agent v2.4 package](../firmware/platform-v2/agent-v2.4/README.md) retains BNO086 support.

## Remaining work

- Resolve the errors in the [hardware import checks](hardware-import-validation.md) and regenerate fabrication outputs from the reviewed sources.
- Confirm purchased component variants, BOM and final ESP32 wiring.
- Resolve and retest the reported regulator failures under motor load; their cause and any separate 3.3 V supply redesign remain unconfirmed.
- Implement and test V3 IMU firmware, calibration, sensor freshness and communication-loss stopping.
- Test the magnetic encoders when parts arrive.
- Publish measured positioning and repeatability results.

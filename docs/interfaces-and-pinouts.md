# Electrical interfaces and pinouts

These tables are extracted from the current uploaded KiCad PCB net names. They document what the boards expose without inventing an unverified system-level harness mapping.

## MB-R2.1 mainboard

### J2 — 13-pin expansion header

| Pin | Net |
|---:|---|
| 1 | +3.3V |
| 2 | GND |
| 3 | IO_analog_ADC |
| 4 | IO34 |
| 5 | IO35 |
| 6 | IO32 |
| 7 | IO33 |
| 8 | IO25 |
| 9 | IO26 |
| 10 | IO27 |
| 11 | IO14 |
| 12 | IO12 |
| 13 | IO13 |

### J3 — 13-pin expansion header

| Pin | Net |
|---:|---|
| 1 | GND |
| 2 | IO23 |
| 3 | IO22-SCL |
| 4 | IO21-SDA |
| 5 | IO19 |
| 6 | IO18 |
| 7 | IO17 |
| 8 | IO16 |
| 9 | IO4 |
| 10 | +3.3V |
| 11 | +3.3V |
| 12 | Vin |
| 13 | GND |

### J1 — TC2030 programming header

| Pin | Net |
|---:|---|
| 1 | EN |
| 2 | +3.3V |
| 3 | U0TX |
| 4 | GND |
| 5 | U0RX |
| 6 | ESP_BOOT |

The onboard controllable LED is connected to **IO13** through its series resistor. The second LED is a 3.3 V power indicator.

## MDI-R2.0 motor-driver / IMU board — J1

| Pin | Net |
|---:|---|
| 1 | IMU_RST_N |
| 2 | IMU_CS_N |
| 3 | IMU_INT_N |
| 4 | SPI_SCLK |
| 5 | SPI_MISO |
| 6 | MOTOR_L_IN1 |
| 7 | Vin |
| 8 | MOTOR_R_IN2 |
| 9 | MOTOR_R_IN1 |
| 10 | MOTOR_L_IN2 |
| 11 | SPI_MOSI |
| 12 | PS1 |
| 13 | GND |

Motor outputs are broken out separately as Motor1 +/- and Motor2 +/-.

## OFS-R2.1.1 PAW3395 board — J3

| Pin | Net |
|---:|---|
| 1 | MOTION |
| 2 | SCLK |
| 3 | MOSI |
| 4 | MISO |
| 5 | nCS |
| 6 | VIN |
| 7 | GND |

The board locally generates **1.8 V** for the PAW3395 core using AP2127K-1.8.

## ENC-R1.0 MA730 board

### J1 — SPI / power

| Pin | Net |
|---:|---|
| 1 | MOSI |
| 2 | MISO |
| 3 | SCLK |
| 4 | CS |
| 5 | NC |
| 6 | +3.3V |
| 7 | GND |

### J2 — alternate/diagnostic outputs

| Pin | Net |
|---:|---|
| 1 | Z |
| 2 | B |
| 3 | A |
| 4 | NC |
| 5 | MGL |
| 6 | MGH |
| 7 | PWM |

## Important firmware note

The repository deliberately does not convert the daughterboard connector names above into guessed ESP32 GPIO assignments. The current firmware scaffold requires the final physical interconnect to be confirmed before those mappings are enabled. This is preferable to publishing a firmware pin map that merely looks plausible.

# Platform V2 key component selections

This is a curated component summary, not a complete purchasing BOM.

| Subsystem | Selected part / specification | Notes |
|---|---|---|
| Main MCU | ESP32-WROOM-32UE | External-antenna ESP32 module used on MB-R2.1 |
| 3.3 V regulator | NCP186AMX330TAG | Low-Iq 3.3 V / 1 A class LDO selection |
| Motor driver | TC78H651FGN,EL | Dual H-bridge on MDI-R2.0 |
| IMU | BNO086 | 9-axis smart IMU on MDI-R2.0 |
| Optical flow | PAW3395 | Bottom relative-motion sensor |
| Optical-flow 1.8 V LDO | AP2127K-1.8TRG1 | Local regulator on OFS board |
| Optical sensor lens | LOAE-LSI1 preferred / LM19-LSI alternative | Mechanical optical stack must be validated |
| Wheel angle | MA730 | Magnetic angle encoder on ENC-R1.0 |
| Main bulk capacitor | Murata GRM31CR60J107KEA8K, 100 µF, 6.3 V, X5R | Selected low-profile bulk capacitor where applicable |
| Drivetrain target | Low-speed N20-class geared DC motors, roughly 15–20 rpm target | Mechanical selection/integration; final motor BOM not yet released here |

Generic passives may be substituted only after checking voltage rating, tolerance, package fit, ESR/decoupling role, and the relevant device datasheet.

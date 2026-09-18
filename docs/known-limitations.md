# Known limitations and design-review notes

This file intentionally records unresolved items rather than hiding them.

## System integration is not complete

Platform V2 is a finished electronics design but the physical carrier is still being assembled. Full autonomous operation has not yet been demonstrated in this repository release.

## PAW3395 board remains unvalidated

OFS-R2.1.1 is electrically simple relative to the larger boards, but it should still be treated as **unvalidated until measured**. Optical tracking sensors are sensitive to lens geometry, surface, height, sensor initialisation, and motion conditions.

## Peripheral-to-ESP32 mapping needs final system confirmation

The KiCad source clearly documents each board's connector nets, but the public firmware does not assume a daughterboard-to-mainboard GPIO mapping that has not been proven from the final assembled interconnect. The preliminary firmware therefore leaves these peripheral assignments disabled by default.

## Mainboard ADC network review item

In the uploaded MB-R2.1 PCB source, the network labelled `IO_analog_ADC_Battery_Meter` is connected through R27 to the R25/R26 divider, and R25 is shown tied to **+3.3 V** rather than `Vin`. If this network is intended as a battery-voltage monitor, that connection should be re-checked before relying on it as a battery measurement. The firmware consequently exposes only a raw diagnostic ADC path and does not claim calibrated battery voltage.

## Mechanical design not included yet

The archive supplied for this repository contains PCB STEP files but not the final carrier chassis, wheel geometry, encoder magnet mount, or optical-post adapter. Those mechanical parameters are essential to reproducible odometry and will need to be added when finalised.

## No production battery subsystem

There is no onboard battery charger/BMS presented here as a finished product. Use an appropriate protected cell and charger.

## No measured positioning specification yet

The architecture was designed for research into high-repeatability positioning, but a final x/y/yaw accuracy number would be premature until the complete sensor-fusion and ground-truth experiments are run.

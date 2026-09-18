# Validation status and test plan

## Current evidence level

| Item | Current status | Meaning |
|---|---|---|
| Platform V1 integrated PCB | **Validated** | Historical board has been built/bench checked; retained as superseded hardware |
| MB-R2.1 mainboard | **Validated** | Controller/power board has been bench validated |
| MDI-R2.0 motor/IMU board | **Validated** | Board has been bench validated |
| ENC-R1.0 wheel encoder | **Validated** | Board has been bench validated |
| OFS-R2.1.1 PAW3395 board | **Unvalidated** | Design/fabrication files complete; experimental verification still required |
| Full Platform V2 carrier | **Integration in progress** | Design finished; manufacturing/assembly semi-finished and awaiting parts |
| Sensor-fusion autonomy | **Not yet validated** | Firmware/software still to be developed |
| Autonomous optical alignment | **Not yet validated** | Experimental goal, not a current result |

“Validated” here means practical board-level bring-up/bench validation. It does **not** mean that final localisation accuracy, drift, lifetime, EMC, temperature range, or optical-alignment performance has been fully characterised.

## Recommended board-level tests

### MB-R2.1

- input current at idle;
- 3.3 V rail value and stability;
- boot/programming through TC2030;
- IO13 LED control;
- serial communication;
- header continuity and power-pin verification.

### MDI-R2.0

- H-bridge output polarity for all input states;
- PWM response under representative motor load;
- supply droop and thermal behaviour;
- BNO086 identity/communication;
- interrupt/reset behaviour;
- coordinate-frame verification.

### OFS-R2.1.1

- 1.8 V regulation;
- SPI identity/init sequence;
- MOTION interrupt;
- raw x/y counts under calibrated linear displacement;
- sensitivity versus surface texture, speed, height, and orientation;
- repeatability over repeated traverses.

### ENC-R1.0

- SPI angle continuity through 0/360°;
- repeatability at fixed angular positions;
- magnetic field status (MGL/MGH);
- A/B/Z output validity if used;
- error versus magnet offset and air gap.

## System-level experiments

### 1. Straight-line motion calibration

Command several distances in both directions and compare:

- wheel-derived displacement;
- PAW3395 displacement;
- overhead-camera ground truth;
- fused estimate.

### 2. Rotation calibration

Command repeated yaw changes and compare encoder/kinematic prediction, IMU yaw, visual ground truth, and fused yaw.

### 3. Return-to-pose repeatability

Move away from a reference pose and return to it repeatedly. Report mean error, standard deviation, radial error distribution, and yaw error rather than a single “accuracy” number.

### 4. Static drift

Keep the carrier stationary for a defined duration and record each estimator. Distinguish sensor noise, estimator drift, and actual mechanical motion.

### 5. Slip/recovery test

Deliberately perturb a wheel or move the carrier without corresponding wheel rotation. Measure how quickly optical flow/global vision causes the estimator to recover.

### 6. Optical task

Define a simple measurable optical objective, e.g. beam centring or a polarisation/throughput measurement. Report initial error, convergence criterion, number of attempts, final error, and success rate.

## Data discipline

Raw data should never be overwritten by processed results. Store calibration parameters, board revisions, firmware commit, camera calibration, surface type, motor/battery configuration, and mechanical geometry with every run.

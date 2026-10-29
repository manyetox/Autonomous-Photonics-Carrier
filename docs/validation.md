# Validation status and test plan

## Current evidence level — 29 October 2026

Status below incorporates the author's project updates. V2 is reported working, including the motors, IMU, and optical-flow sensor; no new physical tests were performed as part of this repository update.

| Item | Current status | Meaning |
|---|---|---|
| Platform V1 integrated PCB | **Validated** | Historical board has been built/bench checked; retained as superseded hardware |
| MB-R2.1 mainboard | **Validated** | Controller/power board has been bench validated |
| MDI-R2.0 motor/IMU board | **Validated** | Board has been bench validated |
| ENC-R1.0 wheel encoder | **Untested** | MA730 component ordering is delayed; encoder operation has not been verified |
| OFS-R2.1.1 PAW3395 board | **Working, author-confirmed** | Optical sensor verified on the prototype; quantitative calibration/performance data still to be published |
| Platform V2 prototype | **Working, author-confirmed** | Motors, BNO086 and PAW3395 operate; magnetic encoders remain untested |
| Platform V3 / LSM6DS3 | **In development** | Lower-cost IMU selected; V3 bring-up and performance validation pending |
| Sensor-fusion autonomy | **Not yet validated** | Prototype IMU/optical-flow positioning developed; encoder integration and full autonomy remain unvalidated |
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

## V3 bring-up still required

- Resolve the documented [V3 DRC errors](hardware-import-validation.md); the source import is not fabrication approval.
- Verify the LSM6DS3 identity, SPI communication, acceleration and angular-rate streams.
- Check mounting axes, stationary gyro bias, orientation estimation and drift.
- Verify sensor freshness and motor-stop behaviour after sensor or communication loss.
- Recheck power-rail stability under motor load; regulator failures were reported during development and their cause is unresolved.
- Test MA730 encoders after the required parts arrive.

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

# Research methodology

## Core questions

The platform is designed to make the following questions experimentally answerable:

1. How accurately can a low-cost mobile carrier estimate and control planar **x, y, yaw** pose on an optical table?
2. How repeatable is the carrier when asked to leave and return to the same pose?
3. How much drift is contributed by wheel odometry, optical flow, IMU, and the estimator itself?
4. Does combining wheel angle, optical flow, inertial sensing, and global vision improve recovery from slip or accumulated error?
5. What accuracy/repeatability is required before optical feedback can reliably complete alignment?
6. What is the cost and complexity per carrier, and how does that scale to multiple modules?

## Terminology

### Accuracy

Closeness of an estimated/commanded pose to an external ground-truth pose.

### Precision

Spread of repeated measurements under nominally identical conditions. High precision alone does not imply accuracy.

### Repeatability

Ability to return to the same physical pose under the same procedure. This is especially important for reconfigurable experiments.

### Resolution

Smallest change the sensing/control chain can meaningfully distinguish. Sensor count resolution should not be reported as system positioning accuracy.

### Drift

Change in estimated or physical pose over time without an intended command. Drift must be separated into sensor, estimator, thermal, mechanical, and true-motion components where possible.

## Ground truth

The planned overhead fiducial camera is the natural external reference for planar motion, but it should be calibrated and its own uncertainty reported. A high-resolution camera estimate is still a measurement system, not absolute truth.

For selected tests, independent mechanical references (ruler/linear stage/dial indicator/precision fixture) can be useful to validate the camera calibration.

## Statistical reporting

For repeated experiments, report distributions rather than only best-case values. Useful statistics include:

- sample count;
- mean signed error;
- mean absolute error;
- RMS error;
- standard deviation;
- 50th/95th percentile radial error;
- yaw error separately from translation;
- failure/success count for alignment tasks.

## Avoiding misleading claims

- Do not turn encoder quantisation into a claimed positioning accuracy.
- Do not call a simulated or theoretical value “measured.”
- Do not pool data from different hardware revisions without recording the revision.
- Do not tune on the same dataset used for final evaluation without saying so.
- Do not hide failed runs; define exclusion criteria in advance.

## Suggested experiment record

Each run should record at minimum:

- timestamp/run ID;
- Platform generation;
- MB/MDI/OFS/ENC board revisions;
- firmware commit/tag;
- host-software commit/tag;
- wheel diameter and track width;
- motor/gearbox configuration;
- battery voltage/state;
- tabletop/surface description;
- PAW3395 lens/height configuration;
- encoder magnet configuration;
- camera calibration identifier;
- command trajectory;
- raw sensor streams;
- ground-truth stream;
- processed estimate;
- relevant optical measurement.

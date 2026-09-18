# Design evolution and engineering rationale

## Pre-project legacy controller

The repository includes an older general-purpose ESP32 robot-controller PCB under `hardware/legacy-reference/`. It is included only to show the engineering background that informed later motor-control and embedded-PCB decisions. It is **not Platform V0 or Platform V1**.

## Platform V1: integration-first

The first photonics-specific carrier used a dense 8-layer board. The architecture attempted to keep coarse motion, UWB localisation, inertial sensing, local stepper control, USB protection, and power electronics on one PCB. That approach offered compact integration, but it also coupled many experimental assumptions into one expensive board spin.

The most important V1 elements visible in the source are:

- ESP32-S3-WROOM-1 controller;
- DWM3000 UWB module;
- BNO086 IMU;
- two TMC5041 stepper-controller ICs;
- DRV8231 motor-driver stages;
- TPS56528DDA power stage;
- a 50 × 50 mm, 8-layer PCB.

The local stepper/flexure concept made sense when the carrier was expected to perform coarse and fine motion itself. As the project matured, it became clearer that the mobile base and the fine optomechanical stage have different engineering requirements.

## Platform V2: modularity-first

V2 separates the carrier into replaceable functions. The mainboard is small and generic; the motor/IMU, optical-flow, and wheel-angle functions live on separate boards. This change supports several research goals:

- cheaper iteration on one subsystem;
- clearer fault isolation;
- easier comparison of alternative sensors;
- less dependence on a single monolithic PCB spin;
- simpler scaling to several carriers;
- easier future replacement of the MCU or sensor board.

### Why UWB is no longer core

UWB was useful as a coarse global-localisation idea, but the combination of overhead fiducials, bottom optical flow, wheel angle, and IMU data better matches the tabletop scale of the experiment while reducing radio hardware and calibration complexity. UWB remains documented in V1 rather than being silently erased from the design history.

### Why the stepper/flexure subsystem was removed from V2

Fine optical alignment can require much finer motion and better stiffness than a low-cost mobile base naturally provides. Instead of asking every carrier to contain a complete fine XYZ/yaw mechanism, V2 treats the mobile base as a coarse positioning and sensing platform. A premade piezo/fine stage or experiment-specific adjustment module can be added above the carrier when needed.

### Why PAW3395

The PAW3395 path was chosen after looking for a compact, actively available optical-tracking sensor suitable for high-resolution relative motion. It provides a direct relative-motion signal at the bottom of the carrier and gives the estimator information that is independent of wheel rotation.

### Why MA730

Early encoder exploration considered discrete magnets and Hall sensors. The current board instead uses an MA730 magnetic angle encoder. That moves the design from sparse edge counting toward direct angular measurement and also exposes A/B/Z and diagnostic outputs for alternative experiments.

## Academic interpretation

V1 and V2 should be treated as different experimental hardware architectures, not as a claim that V2 is already proven superior in measured localisation performance. V2 is preferred because of engineering modularity and the current research direction; quantitative comparison requires data.

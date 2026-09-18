# Host software — planned

The host-side autonomy and experiment-control software is still planned rather than presented as complete. This directory documents the intended boundaries so that later software development remains compatible with the hardware.

## Planned modules

- **Global perception:** overhead camera calibration, fiducial/AprilTag detection, carrier identification, and global x/y/yaw pose.
- **Pose fusion:** combine camera, wheel angle, PAW3395 relative motion, and BNO086 information with explicit uncertainty handling.
- **Motion planning/control:** convert target table poses into carrier trajectories while handling obstacles and multiple carriers.
- **Experiment model:** represent optical components, carrier identities, and desired experimental topology.
- **Optical feedback:** use detector/camera/experiment measurements to refine physical alignment.
- **Logging:** synchronised sensor, command, pose, and optical-data capture for reproducible experiments.

## Interface philosophy

The embedded controller should own fast local actuation and low-level safety; the host should own global perception, experiment logic, optimisation, and data logging. This split keeps high-level research algorithms easy to modify without making the real-time motor-control path depend on a desktop process.

## AI assistance

Initial software architecture notes and the preliminary embedded scaffold were prepared with some assistance from OpenAI ChatGPT 5.6. Future implemented algorithms should be reviewed, tested, and documented in the same way as any other research code.

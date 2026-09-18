# System architecture

## Research objective

The carrier is a mobile embedded platform intended to support reconfigurable free-space photonics experiments. Its job is to transport an optical post/holder, estimate its planar motion, and expose control/sensing interfaces that higher-level software can use for closed-loop experiment automation.

The design deliberately separates **coarse mobile positioning** from **fine optical alignment**. The mobile base should move an element close enough and repeatably enough that an independent fine stage or optical-feedback loop can complete alignment when the experiment requires it.

## Platform V2 sensing model

No single low-cost sensor is expected to remain accurate under every condition. The V2 architecture therefore uses complementary information:

- **MA730 wheel encoders** — direct wheel-angle measurement; useful for local odometry but sensitive to slip and wheel/mechanical errors.
- **PAW3395 optical flow** — relative motion against the tabletop; can detect motion that wheel odometry does not explain and can reduce dependence on wheel geometry.
- **BNO086 IMU** — inertial orientation/dynamics; useful for yaw/short-term motion context but can drift if treated as the only absolute reference.
- **Overhead fiducial camera (planned)** — global absolute pose reference and recovery source.
- **Optical experiment signal (future)** — closes the loop on the experimental objective itself rather than pose alone.

The fusion algorithm is intentionally not fixed in hardware. An EKF, factor-graph approach, complementary estimator, or experiment-specific optimiser can be evaluated later against the same sensor suite.

## Motion architecture

The present mechanical concept uses a compact four-wheel carrier with left/right differential drive and low-speed N20-class gearmotors. Earlier design work targeted roughly 15–20 rpm operation from a 1S LiPo to favour controllability and torque over top speed. The exact mechanical chassis and motor BOM are not yet included in this repository release.

> **Image placeholder — chassis:** I will add the final carrier chassis and optical-post mounting arrangement here once the mechanical assembly is complete.

## Electrical partitioning

### MB-R2.1 mainboard

- ESP32-WROOM-32UE-N4-class module footprint/design.
- NCP186-series 3.3 V LDO design.
- TC2030 programming/debug connector.
- Two 13-pin expansion connectors that expose power and ESP32 signals.
- IO13 status LED plus a 3.3 V power indicator.

### MDI-R2.0 motor/IMU board

- TC78H651FGN dual H-bridge.
- BNO086 9-axis smart IMU.
- Motor output pads and power/interface connections.
- SPI-oriented IMU signal set in the current design.

### OFS-R2.1.1 optical-flow board

- PAW3395 optical tracking sensor.
- AP2127K-1.8 local 1.8 V regulator.
- SPI + motion-interrupt interface.
- Dedicated optical/lens mechanical requirements.

### ENC-R1.0 wheel encoder

- MA730 magnetic angle encoder.
- SPI interface.
- Optional A/B/Z, PWM, and magnetic-field status outputs broken out separately.

## Communications

The ESP32 provides the embedded control point. Host communication is expected to use a convenient research interface such as USB-serial during bring-up and Wi-Fi/Bluetooth or a higher-level network protocol later. The repository does not yet define the final multi-carrier networking protocol.

## Power strategy

V2 is built around a low-voltage battery-powered carrier. The mainboard provides 3.3 V logic regulation, while the PAW3395 board derives its required 1.8 V rail locally. Battery charging is expected to be external rather than implemented as an onboard charger in the current electronics.

## Scope boundaries

The following are intentionally outside the present completed-hardware claim:

- final carrier chassis and optical post adapter;
- production-ready battery enclosure/charger;
- complete sensor-fusion firmware;
- overhead-camera perception software;
- multi-carrier coordination;
- automatic closed-loop optical alignment;
- validated positioning-performance numbers.

# Related work and research context

This page places the **Autonomous-Photonics-Carrier** in its research context. It is intended as a concise engineering map of nearby work, not as an exhaustive literature review.

The central idea of this project is to investigate whether **small, comparatively low-cost mobile carriers can reposition standard free-space optical hardware on a tabletop, estimate their own pose from complementary sensors, and provide a hardware layer for later closed-loop optical alignment**. That emphasis is different from building a single general-purpose robotic manipulator: the carrier itself becomes part of the reconfigurable optical table.

## OptoMate / AI-driven robotics for free-space optics

**S. Z. Uddin et al., “AI-Driven Robotics for Free-Space Optics,” arXiv:2505.17985 (2025).**  
<https://arxiv.org/abs/2505.17985>

OptoMate combines generative AI, computer vision, precision robotics, robotic pick-and-place, and fine-alignment tooling to automate free-space optical experiments. It is an important demonstration that optical setup generation, physical assembly, alignment, and measurement can be connected into one autonomous workflow.

### Relationship to this project

The Autonomous-Photonics-Carrier approaches the physical layer from a different direction. Rather than relying primarily on a robot arm to place optical components, this project explores **distributed mobile modules that carry the optical elements themselves**. The two approaches should therefore be viewed as related but not equivalent architectures.

The most relevant lessons for this project are:

- geometric placement alone is not sufficient for a useful optical experiment;
- coarse positioning and fine optical alignment should be treated as different control regimes;
- computer vision can provide a global reference, while local sensors can provide faster relative motion information;
- the final success metric should include the quality of the optical experiment, not only robot pose error.

## Closed-loop robotic optical assembly and self-recovery

**S. Choi et al., “A Framework for Closed-Loop Robotic Assembly, Alignment and Self-Recovery of Precision Optical Systems,” arXiv:2603.21496 (2026).**  
<https://arxiv.org/abs/2603.21496>

This work demonstrates a closed-loop framework for robotic construction, alignment, maintenance, and recovery of precision optical systems, including autonomous construction of a tabletop laser cavity. It is particularly relevant because it treats optical alignment as an iterative feedback problem rather than a one-shot geometric placement task.

### Relationship to this project

For the Autonomous-Photonics-Carrier, this supports the architectural decision to keep a distinction between:

1. **coarse mobile positioning** of the carrier;
2. **pose estimation and recovery** using external and onboard sensing; and
3. **fine alignment using optical feedback and/or a dedicated fine-positioning mechanism**.

The current repository concentrates on the first two hardware layers. Closed-loop optical optimisation remains future experimental and software work and is not claimed as a completed capability.

## UbiSwarm and small tabletop multi-robot systems

**L. H. Kim and S. Follmer, “UbiSwarm: Ubiquitous Robotic Interfaces and Investigation of Abstract Motion as a Display,” Proceedings of the ACM on Interactive, Mobile, Wearable and Ubiquitous Technologies, 1(3), 2017.**  
DOI: <https://doi.org/10.1145/3130931>  
Project page: <https://shape.stanford.edu/research/UbiSwarm/>

UbiSwarm is not a photonics platform, but it is relevant to the **physical architecture of many small mobile robots sharing a tabletop workspace**. It provides a useful precedent for thinking about compact mobile units, multi-robot motion, and scalable distributed physical systems.

### Relationship to this project

The useful connection is at the robotics-system level rather than the optical-control level. The Autonomous-Photonics-Carrier similarly aims to make the mobile unit small enough that multiple carriers could coexist around an experiment, while retaining enough onboard sensing and control to support repeatable motion.

## Position of this repository

The project sits at the intersection of three research directions:

- **autonomous photonics**, where optical experiments are assembled, aligned, measured, and recovered with increasingly closed-loop automation;
- **mobile modular robotics**, where distributed units can reconfigure the physical arrangement of a workspace; and
- **embedded sensor fusion**, where wheel angle, optical flow, inertial sensing, and external vision provide complementary estimates of motion and pose.

The current contribution of this repository is therefore intentionally narrow and testable: **a documented mobile-carrier hardware platform and a reproducible basis for measuring whether this architecture is accurate, repeatable, stable, and practical enough to support later autonomous photonics experiments.**

## Scope note

References on this page are included to explain engineering context. Their inclusion does not imply that this project reproduces or matches the performance of those systems. Any future quantitative comparison should use clearly defined experimental conditions and measured results rather than architectural similarity alone.

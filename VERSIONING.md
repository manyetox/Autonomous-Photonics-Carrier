# Versioning and naming convention

This project uses **two independent version namespaces**. Keeping them separate is important for reproducibility.

## 1. Platform generation

A **Platform** version describes the overall system architecture.

- **Platform V1** — integrated/monolithic carrier electronics.
- **Platform V2** — modular carrier electronics; current architecture.

A daughterboard revision does **not** create a new platform generation.

## 2. Board revision

Each PCB has its own identifier:

| Board identifier | Human-readable name | Platform | Current? |
|---|---|---|---|
| **INT-R1.0.1** | Integrated V1 carrier PCB | Platform V1 | Historical |
| **MB-R2.1** | Mainboard | Platform V2 | Yes |
| **MDI-R2.0** | Motor Driver + IMU | Platform V2 | Yes |
| **OFS-R2.1.1** | Optical-Flow Sensor | Platform V2 | Yes |
| **ENC-R1.0** | Wheel Encoder | Platform V2 | Yes |

Therefore, **OFS-R2.1.1 is not “Platform V2.1.1.”** It is board revision 2.1.1 within Platform V2.

## Historical fabrication outputs

Older Gerbers are retained only inside `archive/` folders and are explicitly labelled as superseded. They are useful for design history but should not be sent to fabrication unless there is a specific reason to reproduce that revision.

## Firmware/software versions

Firmware and host software should be versioned independently from hardware. A future firmware release may support more than one hardware board revision, so firmware tags should not reuse PCB revision numbers.

Recommended future tags:

- `fw-v0.1.0`, `fw-v0.2.0`, ... for embedded firmware releases.
- repository release tags such as `release-2026-09` or semantic versions for curated archival snapshots.

Each experimental dataset should record **Platform generation + every installed board revision + firmware commit/tag**.

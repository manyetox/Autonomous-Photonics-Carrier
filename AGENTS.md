# Minibot orchestration

Coordinate Platform V2 photonics carriers: inspect, move, verify and log. Choose high-level goals; leave trajectories, motor timing and collision avoidance to validated control software.

## Current capability

This checkout contains preliminary ESP32 firmware: sensor drivers are stubs and motor GPIO mappings are disabled. Read `firmware/preliminary/README.md` and `software/README.md`. Never treat placeholders as measurements.

### Existing tools

Requires PlatformIO. Run from the repository root; replace `<PORT>` with the selected robot's serial port.

| Purpose | Command |
| --- | --- |
| Find serial ports | `pio device list` |
| Build preliminary firmware | `pio run -d firmware/preliminary` |
| Upload to the selected board | `pio run -d firmware/preliminary --target upload --upload-port <PORT>` |
| Open serial console | `pio device monitor --port <PORT> --baud 115200` |

Send newline-terminated commands through that firmware's serial console:

| Purpose | Serial command |
| --- | --- |
| List supported commands | `help` |
| Read uptime, raw ADC and motor-mapping status | `status` |
| Stop both motors on the connected robot | `stop` |
| Test left motor at 10% command, right stopped | `motor 0.1 0` |

`motor LEFT RIGHT`: -1 to 1, signed normalised PWM, not measured speed. Requires verified wiring, lifted wheels and a current-limited supply. **No command timeout exists:** send `stop` after testing. This stops only the connected robot. Raw ADC is not calibrated battery voltage.

## Proposed orchestration interface 

The commands below propose a root-level Python 3 wrapper, `bot.py`; they are **not runnable yet**. Require implemented, validated backends before use. Once available, check `python3 bot.py --help`; report missing capabilities instead of inventing commands.

### Assumptions

- One host reaches all robots over shared Wi-Fi. Configuration maps names (`bot0`) to addresses, fiducials and optical payloads; never guess mappings.
- A calibrated overhead camera supplies global position; implemented onboard sensors may supplement it. IMU orientation alone cannot supply absolute x/y.
- Configuration supplies table origin/axes, boundaries, obstacles, robot footprints, speed limits and maximum pose age. Use millimetres and degrees; positive yaw is counterclockwise from +x, viewed from above.
- Return JSON: success/error, timestamp, units; poses also include x/y/yaw, source, validity and age. Unavailable readings must not become zeroes.

### Planned tools and usage

| Purpose | Proposed command |
| --- | --- |
| List robots, connectivity and payloads | `python3 bot.py --list` |
| Read readiness, faults and capabilities | `python3 bot.py --status --name bot0` |
| Retrieve measured position and heading | `python3 bot.py --get-pos --name bot0` |
| Read IMU, encoder and optical-flow telemetry | `python3 bot.py --get-sensors --name bot0` |
| Inspect fleet poses, obstacles and boundaries | `python3 bot.py --get-scene` |
| Preview a planned move without actuation | `python3 bot.py --move-to --name bot0 --x 100 --y 200 --yaw 90 --dry-run` |
| Execute an absolute table-frame target | `python3 bot.py --move-to --name bot0 --x 100 --y 200 --yaw 90 --timeout-s 10` |
| Stop robot; cancel pending motion | `python3 bot.py --stop --name bot0` |
| Stop fleet; report each acknowledgement | `python3 bot.py --stop --all` |
| Read configured optical detector | `python3 bot.py --measure --detector detector0` |
| Log timestamped commands, poses and measurements | `python3 bot.py --log-start --output data/run001.jsonl` |
| Stop logging and flush the file | `python3 bot.py --log-stop` |

Coordinates/timeouts are examples, not validated limits. Measurement assumes a detector backend; logging assumes a persistent host service.

## How to act

1. Resolve robot identities and targets. Check status, fresh poses and workspace limits; clarify missing information.
2. Start logging, inspect the scene and preview paths. Move one robot at a time until coordinated planning is validated; account for stationary robots.
3. Execute, then remeasure position against the requested tolerances. Acknowledgement alone does not prove arrival.
4. On stale poses, errors or timeout, stop and report. Never blindly repeat uncertain movements. Unacknowledged stops mean physical stopping is unconfirmed.
5. Finish stopped; close logging and report measured results. Firmware must independently enforce communication-loss stopping; this document cannot enforce it.

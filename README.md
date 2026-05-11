# OpenHTF Demo for STM32 Firmware Functional Tests

This repository demonstrates how to build a **Hardware-in-the-Loop (HIL) functional test framework** for STM32 firmware using [OpenHTF](https://github.com/google/openhtf) — an open-source hardware testing framework originally built at Google

---

## Table of Contents

- [OpenHTF Demo for STM32 Firmware Functional Tests](#openhtf-demo-for-stm32-firmware-functional-tests)
  - [Table of Contents](#table-of-contents)
  - [1. What is in this repo](#1-what-is-in-this-repo)
  - [2. Demo Hardware](#2-demo-hardware)
  - [3. Firmware Overview](#3-firmware-overview)
  - [4. Test Framework Overview](#4-test-framework-overview)
    - [Key Components](#key-components)
    - [How a test phase looks](#how-a-test-phase-looks)
  - [5. Quick Start](#5-quick-start)
    - [Prerequisites](#prerequisites)
    - [Install](#install)
    - [Configure hardware](#configure-hardware)
    - [Place firmware binary](#place-firmware-binary)
  - [6. Running the Tests](#6-running-the-tests)
  - [7. Writing a New Test](#7-writing-a-new-test)
    - [Conventions to follow](#conventions-to-follow)
    - [Minimal new test template](#minimal-new-test-template)
  - [8. Repository Structure](#8-repository-structure)
  - [License](#license)

---

## 1. What is in this repo

| Layer | What it is |
|---|---|
| `src/` | STM32 embedded firmware (C, FreeRTOS, UART CLI, CAN/UDS) |
| `tests/functional/` | OpenHTF-based HIL functional test framework (Python) |
| `tests/unit/` | Unit test scaffolding |
| `docs/` | Feature specs, CLI docs, speaker notes |

The firmware is the **Device Under Test (DUT)**. The test framework runs on a host PC, programs the DUT via ST-Link, and validates behavior over a UART serial console — all automatically, with structured PASS/FAIL reports.

---

## 2. Demo Hardware

- **MCU**: STM32C092 (Arm Cortex-M0+)
- **Board**: STM32 Nucleo-C092RC
- **Programmer**: Onboard ST-Link (SWD) — no external programmer needed
- **Serial console**: Onboard ST-Link Virtual COM Port (VCP) at 115200 baud
- **Host**: Linux PC — Ubuntu 20.04+ is the tested environment. Windows and macOS should work (Python, STM32CubeProgrammer, and pyserial are all cross-platform) but have not been validated yet.

> * The STM32 Nucleo can be replaced by any STM32 board with an SWD interface and a UART (or USB CDC) for the console.
> * There is a can_sensors_demo.ioc that you can use as reference in case you want to port to different STM32 Nucleo board
```
┌─────────────────────────────────────┐
│         Host PC (Linux)             │
│                                     │
│  OpenHTF Test Runner                │
│   ├── STLinkPlug  ──────────────────┼──► SWD (flash & reset)
│   └── SerialConsolePlug ────────────┼──► UART VCP (CLI monitor)
└─────────────────────────────────────┘
                    │ USB
         ┌──────────┴──────────┐
         │    Nucleo-Board     │
         │     STM32 MCU       │
         │  FreeRTOS firmware  │
         └─────────────────────┘
```

---

## 3. Firmware Overview

The demo firmware (`src/`) is a FreeRTOS application that exposes a **UART CLI** for querying device state. Key firmware features:

> This is a simple demo firmware meant to illustrate the testing framework. It is not meant to be a full production-quality firmware, in fact the idea is that it is used to iterate on the test framework and try to break it and improve it by adding test cases in a Test Driven Development (TDD) style. This way we can replace the system under test by keeping the main CLI interface and just changing the implementation behind it.

| CLI Command | Description |
|---|---|
| `version` | Print firmware name and version (`Test Demo Firmware v0.1.0`) |
| `uid` | Print 96-bit unique device ID (serial number) |
| `devinfo` | Print device ID, revision, and flash size |
| `temp` | Read and print internal temperature sensor (°C) |
| `help` | List all available commands |

Firmware version is defined in [src/app/version.h](src/app/version.h):

```c
#define FW_NAME     "Test Demo Firmware"
#define FW_VERSION  "v0.1.0"
```

**Building the firmware** (requires `arm-none-eabi-gcc` 13.3 or later):

```bash
cd src
make -j8 GCC_PATH=/path/to/arm-gnu-toolchain/bin
```

The built binary is placed in `src/build/`.

---

## 4. Test Framework Overview

The functional test framework lives in `tests/functional/` and is built on **OpenHTF**. The core pattern is:

```
Test Runner  →  Test Case  →  Test Phases  →  Hardware Plugs  →  DUT
```

### Key Components

| Component | Path | Purpose |
|---|---|---|
| **STLinkPlug** | `plugs/stlink_plug.py` | Flash firmware and reset the device via STM32CubeProgrammer CLI |
| **SerialConsolePlug** | `plugs/serial_plug.py` | Connect to UART, capture logs, search patterns, send CLI commands |
| **Test cases** | `tests/` | Numbered test procedures (`test_001_...py`, `test_002_...py`, …) |
| **Test runner** | `run_tests.py` | Entry point; loads config, discovers tests, writes reports |
| **Hardware config** | `config/hw_cfg.yaml` | Serial port, ST-Link settings, flash addresses |
| **Test config** | `config/test_cfg.yaml` | Timeouts, firmware paths, thresholds |

### How a test phase looks

```python
@htf.plug(serial=SerialConsolePlug)
@htf.measures(
    htf.Measurement('firmware_version').with_validator(
        validators.equals('v0.1.0')  # must match FW_VERSION in src/app/version.h
    )
)
def validate_firmware_version(test, serial):
    serial.clear_logs()
    serial.send_command('version')
    serial.wait_for_pattern(r'Test Demo Firmware v\d+\.\d+\.\d+', timeout=5.0)
    version_lines = serial.search_logs(pattern=r'Test Demo Firmware v\d+\.\d+\.\d+', is_regex=True)
    match = re.search(r'(v\d+\.\d+\.\d+)', version_lines[0]) if version_lines else None
    test.measurements.firmware_version = match.group(1) if match else 'NOT_FOUND'
```

Test results are written as **JSON reports** in `tests/functional/reports/`.

---

## 5. Quick Start

### Prerequisites

- Python 3.8+  
- [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html) (v2.19+) with CLI in PATH or set in `hw_cfg.yaml` 
- STM32 Nucleo board connected via USB : [nucleo-c092rc](https://www.st.com/en/evaluation-tools/nucleo-c092rc.html), or any other similar
- Arm GNU Toolchain (`arm-none-eabi-gcc`) for building firmware (https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)

### Install

Run the one-step setup script — it handles everything:

```bash
cd tests/functional
./setup.sh
```

The script will:
1. Check that Python 3 is installed
2. Create a `venv/` virtual environment (skipped if it already exists)
3. Activate the virtual environment
4. Upgrade `pip`
5. Install all Python dependencies from `requirements.txt` (openhtf, pyserial, pyyaml, …)
6. Verify key packages imported correctly
7. Create the `logs/` and `reports/` output directories
8. Check that `STM32_Programmer_CLI` is reachable (warns if not found in PATH)
9. Run `python3 run_tests.py --list` to confirm the test runner is functional

At the end it prints a summary of the available `run_tests.py` commands.

> For full detail on the framework and manual setup steps see [tests/functional/README.md](tests/functional/README.md).

Or install manually:

```bash
cd tests/functional
python3 -m venv venv && source venv/bin/activate
pip install -r requirements.txt
mkdir -p logs reports
```

### Configure hardware

Edit `config/hw_cfg.yaml` to match your setup:

```yaml
stlink:
  programmer_cli: "/opt/st/stm32cubeclt_1.15.1/STM32CubeProgrammer/bin/STM32_Programmer_CLI"

serial:
  port: /dev/ttyACM0     # adjust to your VCP device
  baudrate: 115200
```

### Place firmware binary

Copy your built `.hex` file to `tests/functional/test_binaries/draft/`:

```bash
# Quick helper (copies from src/build/ to test_binaries/draft/)
python3 framework/src_draft_get.py
```

Or copy manually:

```bash
cp src/build/can_sensors_demo.hex tests/functional/test_binaries/draft/
```

---

## 6. Running the Tests

```bash
cd tests/functional
source venv/bin/activate

# List available tests
python3 run_tests.py --list

# Run all tests
python3 run_tests.py

# Run a specific test
python3 run_tests.py --test test_001_my_hello_world_test

# Verbose output (shows all OpenHTF log details)
python3 run_tests.py --verbose
```

Reports are written to `tests/functional/reports/` as timestamped JSON files.

---

## 7. Writing a New Test

### Conventions to follow

- One file per test case in `tests/`, named `test_NNN_short_description.py`
- Phase functions are numbered (`Phase 1`, `Phase 2`, …) and focused on a single concern
- Measurement names are self-documenting (e.g., `uart_alive_lines_captured`, `firmware_version`)
- Use `validators.equals(...)` for exact match, `validators.in_range(...)` for numeric bounds, `validators.matches_regex(...)` for format checks
- When checking firmware version, inline the expected value in `@htf.measures` with a comment pointing to `src/app/version.h` — do not use a YAML config for this

### Minimal new test template

```python
import openhtf as htf
from openhtf.util import validators
from openhtf.output.callbacks import json_factory, console_summary
from plugs import SerialConsolePlug

@htf.plug(serial=SerialConsolePlug)
@htf.measures(
    htf.Measurement('my_measurement').with_validator(validators.equals('expected'))
)
def my_phase(test, serial):
    serial.clear_logs()
    serial.send_command('my_cli_command')
    serial.wait_for_pattern(r'expected pattern', timeout=5.0)
    lines = serial.search_logs(pattern=r'expected pattern', is_regex=True)
    test.measurements.my_measurement = lines[0] if lines else 'NOT_FOUND'

def test_00N_my_test():
    test = htf.Test(
        my_phase,
        test_name='test_00N_my_test',
        test_description='Brief description'
    )
    test.add_output_callbacks(
        json_factory.OutputToJSON('reports/{dut_id}.{metadata[test_name]}.{start_time_millis}.json'),
        console_summary.ConsoleSummary()
    )
    test.execute(test_start=htf.util.argv.test_start_from_args)
```

---

## 8. Repository Structure

```
stm32_openhtf_demo/
├── src/                            # STM32 firmware
│   ├── app/
│   │   ├── version.h               # FW_NAME, FW_VERSION
│   │   ├── cli_demo/               # UART CLI implementation
│   │   └── can_uds_demo/           # CAN/UDS demo module
│   ├── Core/                       # HAL init, main, FreeRTOS tasks
│   ├── Drivers/                    # STM32 HAL + BSP drivers
│   ├── Middlewares/                # FreeRTOS
│   └── Makefile
│
├── tests/
│   └── functional/                 # OpenHTF HIL test framework
│       ├── plugs/
│       │   ├── stlink_plug.py      # STLinkPlug: flash & reset
│       │   └── serial_plug.py      # SerialConsolePlug: UART monitor & CLI
│       ├── tests/
│       │   └── test_001_my_hello_world_test.py   # Example: flash + boot + version
│       ├── config/
│       │   ├── hw_cfg.yaml         # Serial port, ST-Link, flash layout
│       │   └── test_cfg.yaml       # Timeouts, firmware paths, thresholds
│       ├── framework/
│       │   └── src_draft_get.py    # Copy build output to test_binaries/
│       ├── test_binaries/
│       │   ├── draft/              # Nightly/dev builds
│       │   └── release/            # Release builds
│       ├── reports/                # Generated JSON reports (git-ignored)
│       ├── logs/                   # Execution logs (git-ignored)
│       ├── run_tests.py            # Test runner entry point
│       ├── requirements.txt
│       ├── setup.sh                # One-step environment setup
│       └── verify.sh               # Installation verification
│
└── docs/
    ├── DEMO_FIRMWARE.md
    ├── DEMO_SETUP.md
    ├── DEMO_TEST_FUNCTIONAL.md
```

---

## License

See [LICENSE](LICENSE) and [NOTICE](NOTICE).




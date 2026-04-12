# GitHub Copilot Instructions

## Purpose

This file provides custom instructions for GitHub Copilot and Copilot Chat in the stm32_openhtf_demo workspace.  
Use it to clarify coding standards, project structure, preferred libraries, and any special requirements for Copilot suggestions.

---

## Project Structure

- Main firmware code is in `src/` (subfolders: `Core/`, `Drivers/`, `Middlewares/`, `app/`).
- Application modules are in `src/app/` (e.g., `can_uds_demo/`, `cli_demo/`).
- Firmware version is defined in `src/app/version.h` (`FW_VERSION`, `FW_NAME`).
- Build artifacts go in `src/build/` (ignored by git).
- Tests are in `tests/unit/` and `tests/functional/`.
- Documentation is in `docs/`.

---

## Coding Standards

- Use C for embedded code, Python for scripting/tests.
- Follow snake_case for Python, camelCase for C.
- Always use STM32 HAL drivers for hardware access.
- Document all functions with comments or docstrings.
- Prefer standard libraries unless otherwise specified.
- For FreeRTOS, use CMSIS RTOS v2 API when possible.
- Follow K&R style for C code formatting for control statements:
```c
    if (condition) {
        // code
    } else if (other_condition) {
        // code
    } else {
        // code
    }
```

---

## Testing

- Unit tests: `tests/unit/`
- Functional tests: `tests/functional/`
- Test framework: **OpenHTF** (`import openhtf as htf`)
- Hardware plugs are in `tests/functional/plugs/`:
  - `STLinkPlug` — firmware flashing and device reset via STM32CubeProgrammer CLI
  - `SerialConsolePlug` — UART console monitoring, log capture, pattern search, and CLI command sending
- Config files are in `tests/functional/config/`:
  - `hw_cfg.yaml` — serial port, baudrate, ST-Link settings
  - `test_cfg.yaml` — firmware paths, timeouts, test parameters
- Test binaries go in `tests/functional/test_binaries/`
- Reports are written to `tests/functional/reports/`

### Functional Test Conventions

- Each test file is a standalone OpenHTF test in `tests/functional/tests/`.
- Each file defines phase functions decorated with `@htf.plug(...)` and `@htf.measures(...)`, and a main `test_NNN_...()` function that assembles them with `htf.Test(...)`.
- Phase functions follow a numbered naming convention: `Phase 1`, `Phase 2`, etc.
- Measurement names must be self-documenting — name them after what they assert, e.g. `uart_alive_lines_captured`, `welcome_banner_found`, `firmware_version`.
- Use `validators.equals(...)` for exact match, `validators.in_range(minimum=...)` for numeric bounds, `validators.matches_regex(...)` for format checks.
- When validating firmware version, use `validators.equals('vX.Y.Z')` inline in `@htf.measures` with a comment referencing `src/app/version.h`. Do not use a module-level constant or YAML config for the expected version.
- Keep phase functions focused: one concern per phase, minimal measurements per phase.
- Output callbacks used: `json_factory.OutputToJSON` and `console_summary.ConsoleSummary`.

---

## Embedded Guidelines

- Target STM32 microcontrollers (Cortex-M0+).
- Use `arm-none-eabi-gcc` toolchain.
- Linker script: `STM32C092XX_FLASH.ld`
- Startup file: `startup_stm32c092xx.s`
- Use FreeRTOS for RTOS features.
- Firmware exposes a UART CLI; use `version` command to query firmware version and build date.

# GitHub Copilot Instructions

## Purpose

This file provides custom instructions for GitHub Copilot and Copilot Chat in the stm32_openhtf_demo workspace.  
Use it to clarify coding standards, project structure, preferred libraries, and any special requirements for Copilot suggestions.

---

## How to Use

- Add specific instructions, rules, or examples below.
- Update as your project evolves.
- Copilot will reference this file to improve its suggestions.

---

## Project Structure

- Main code is in `src/` (with subfolders: `Core/`, `Drivers/`, `Middlewares/`, `app/`).
- Application modules are in `src/app/` (e.g., `can_uds_demo/`, `cli_demo/`).
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
- Test coverage must include both unit and functional tests.

---

## Embedded Guidelines

- Target STM32 microcontrollers (Cortex-M0+).
- Use `arm-none-eabi-gcc` toolchain.
- Linker script: `STM32C092XX_FLASH.ld`
- Startup file: `startup_stm32c092xx.s`
- Use FreeRTOS for RTOS features.

---

## Custom Instructions

<!-- Add your project-specific instructions here. For example: -->
<!-- - All new drivers should be placed in Drivers/. -->
<!-- - Application logic goes in src/app/. -->
<!-- - Keep build/ and other generated folders out of git. -->
<!-- - Use version.h for versioning info. -->

---

Feel free to expand this template with your own requirements and best practices.

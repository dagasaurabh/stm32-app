# STM32 Firmware Framework

[![CMake](https://img.shields.io/badge/CMake-%E2%89%A53.16-blue?logo=cmake&logoColor=white)](https://cmake.org/cmake/help/latest/)
[![Toolchain](https://img.shields.io/badge/arm--none--eabi--gcc-15.2-brightgreen?logo=arm&logoColor=white)](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
[![Platform](https://img.shields.io/badge/platform-Linux-lightgrey?logo=linux&logoColor=white)](https://www.kernel.org/)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)

A **vendor-IDE-free STM32 firmware framework** for developers who prefer CMake and
command-line tooling over STM32CubeIDE. It provides a layered architecture with
clean separation between MCU, board, and application — reproducible builds and
OpenOCD integration, all driven by a single `make` command.

---

## Contents

- [Features](#features)
- [Supported Hardware](#supported-hardware)
- [Prerequisites](#prerequisites)
- [Quickstart](#quickstart)
- [Project Structure](#project-structure)
- [Adding a New Application](#adding-a-new-application)
- [Adding a New Board](#adding-a-new-board)
- [Running Tests](#running-tests)
- [Contributing](#contributing)
- [License](#license)

---

## Features

- **Single `make` entry point** — downloads the ARM toolchain, initialises HAL
  submodules, runs CMake, and builds all firmware in one step
- **Seven-layer architecture** — strict dependency direction from app down to vendor HAL;
  no layer reaches up or sideways
- **Interrupt-driven peripheral subsystems** — SPI, I2C, UART, GPIO, each with
  both blocking-sync and non-blocking async APIs
- **OpenOCD integration** — per-app `_flash`, `_debug`, and `_gdb` CMake targets
- **CI style enforcement** — GitHub Actions checks `.editorconfig` and `clang-format`
  on every push and pull request

---

## Supported Hardware

| Board | MCU | Core | Flash | RAM | Clock |
|-------|-----|------|-------|-----|-------|
| [`NUCLEO-L552ZE-Q`](https://www.st.com/en/evaluation-tools/nucleo-l552ze-q.html) | STM32L552ZE | Cortex-M33 | 512 KB | 256 KB | 110 MHz |
| [`B-U585I-IOT02A`](https://www.st.com/en/evaluation-tools/b-u585i-iot02a.html) | STM32U585AI | Cortex-M33 | 2 MB | 786 KB | 160 MHz |

---

## Prerequisites

| Tool | Version | Notes |
|------|---------|-------|
| `cmake` | ≥ 3.16 | Must be on `PATH` |
| `make` | any | GNU make |
| `git` | any | For submodules |
| `python3` | ≥ 3.6 | Required by OpenOCD build |
| `libusb-dev` | any | Required by OpenOCD (Debian: `libusb-1.0-0-dev`) |
> The **ARM GNU Toolchain** (arm-none-eabi-gcc 15.2) and **OpenOCD** are
> downloaded and built automatically by `make` — you do not need to install them
> separately.

Install the non-optional dependencies on Debian/Ubuntu:

```bash
sudo apt install cmake make git python3 libusb-1.0-0-dev
```

---

## Quickstart

### 1 — Clone

```bash
git clone https://github.com/dagasaurabh/stm32-app.git
cd stm32-app
```

### 2 — Full build (first time)

The first `make` invocation downloads the ARM toolchain (~150 MB), initialises
HAL submodules for the target board, and builds OpenOCD:

```bash
# Default board: nucleo_l552ze_q, default type: Debug
make

# Choose a different board or build type
make BOARD=b_u585i_iot02a TYPE=Release
```

Subsequent builds after editing source files:

```bash
make build                          # reconfigure + rebuild (current board)
make build-all                      # build every supported board
```

### 3 — Flash an application

Connect your board via USB/ST-Link, then:

```bash
# Flash the helloworld app to nucleo_l552ze_q (Debug build)
cmake --build build/nucleo_l552ze_q/Debug --target helloworld_flash

# Flash to B-U585I-IOT02A
cmake --build build/b_u585i_iot02a/Debug --target helloworld_flash
```

### 4 — Debug (OpenOCD + GDB)

```bash
# Terminal 1 — start OpenOCD GDB server
cmake --build build/nucleo_l552ze_q/Debug --target helloworld_debug

# Terminal 2 — connect GDB client with startup script
cmake --build build/nucleo_l552ze_q/Debug --target helloworld_gdb
```

### Build reference

```bash
make BOARD=<board> TYPE=<type> [target]

# Boards
#   nucleo_l552ze_q   (default)
#   b_u585i_iot02a

# Types
#   Debug             (default)
#   Release

make list-boards    # print supported boards
make help           # print all Makefile targets
make clean          # clean current board build
make distclean      # remove all build output including tools
```

---

## Project Structure

```
stm32-app/
├── app/
│   └── projects/               # One subdirectory per application
│       ├── blinky/             #   main.c + CMakeLists.txt
│       ├── helloworld/
│       ├── spi_loopback_test/
│       ├── spi_loopback_irq_test/
│       ├── hts221_test_sync/   # Sensor apps (sync + async variants)
│       ├── hts221_test_async/
│       └── ...
│
├── platform/
│   ├── devices/                # Portable sensor drivers
│   │   ├── hts221/
│   │   ├── lps22hh/
│   │   ├── ism330dhcx/
│   │   └── iis2mdc/
│   │
│   ├── boards/                 # Board layer — peripheral wiring, MSP callbacks
│   │   ├── nucleo_l552ze_q/
│   │   └── b_u585i_iot02a/
│   │
│   ├── common/                 # Facade APIs (spi, i2c, gpio, serial, regmap, sensor_mgr)
│   │   ├── include/            # one subdirectory per subsystem
│   │   └── src/                #
│   │
│   ├── drivers/                # Hardware drivers — one per peripheral
│   │   ├── spi/                #   *_driver.c + *_hal.c
│   │   ├── i2c/
│   │   ├── gpio/
│   │   └── uart/
│   │
│   └── mcu/                    # MCU-family layer — startup, clocks, HAL config
│       ├── stm32l5/
│       └── stm32u5/
│
├── hal/                        # ST vendor HAL (git submodules, never modified)
│   ├── STM32CubeL5/
│   └── STM32CubeU5/
│
├── cmake/                      # CMake helpers (board selection, toolchain interface)
├── toolchain/                  # ARM GNU toolchain (downloaded by make)
└── ext/                        # External tools (OpenOCD submodule)
```

### Layer dependency rules

```
app/projects/<name>/            ← links against board subsystems + devices
platform/devices/<sensor>/      ← calls only the I2C facade; no board/MCU knowledge
platform/boards/<board>/        ← wires facade ops tables to the driver layer
platform/common/                ← facade APIs; no HAL includes
platform/drivers/               ← calls HAL; no board knowledge
platform/mcu/<family>/          ← startup, HAL module libraries
hal/STM32Cube<family>/          ← vendor HAL (read-only)
```

Each layer depends only on layers below it. Applications link against named
interface libraries (`board_spi`, `board_i2c`, `hts221`, …); CMake pulls in
the correct objects automatically.

---

## Adding a New Application

1. Create `app/projects/<name>/main.c` and `app/projects/<name>/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)
set(P <name>)
project(${P} C ASM)

add_executable(${P}.elf main.c)
target_link_libraries(${P}.elf
    ${MCU_TOOLCHAIN_LIB}
    mcu_core
    board_core
    board_console
    # add board_spi, board_i2c as needed
)
build_app(${P})   # generates _flash, _debug, _gdb targets
```

2. Register it in `app/CMakeLists.txt`:

```cmake
add_subdirectory(projects/<name>)
```

3. Build and flash:

```bash
make build
cmake --build build/<board>/<type> --target <name>_flash
```

**Available link targets**

| Target | Provides |
|--------|----------|
| `mcu_core` | startup, clocks, system init |
| `board_core` | GPIO controller, `board_init()` |
| `board_console` | UART-backed printf / stdin |
| `board_spi` | SPI bus registration + CS management |
| `board_i2c` | I2C bus registration |
| `hts221` / `lps22hh` / `ism330dhcx` / `iis2mdc` | sensor drivers |

---

## Adding a New Board

1. **Create the board cmake file** at
   `platform/boards/<board>/cmake/board.cmake`. It must set:

```cmake
set(MCU_FAMILY    stm32<family>)         # e.g. stm32l5, stm32u5
set(MCU_VARIANT   STM32<VARIANT>xx)      # e.g. STM32L552xx
set(CPU_FLAGS     "-mcpu=cortex-m33 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard")
set(LINKER_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/../linker/<board>.ld")
set(MCU_HAL_ROOT  "${FW_ROOT}/hal/STM32Cube<Family>")
set(MCU_HAL_INCLUDES ...)
```

2. **Create the board directory tree** with these subdirectories:

```
platform/boards/<board>/
├── cmake/board.cmake
├── core/          board_core.c  (board_init, GPIO controller registration)
├── console/       board_console.c  (UART MSP init, IRQ handlers, printf wiring)
├── spi/           board_spi.c  (SPI ops table, bus registration)
├── i2c/           board_i2c.c  (I2C ops table, bus registration)
├── src/           hal_msp.c    (HAL_MspInit — clocks, power domains)
├── include/       board_*.h    (board-specific pin/peripheral defines)
├── linker/        <board>.ld
└── openocd.cfg
```

3. **Create `platform/mcu/<family>/`** if the MCU family is new (startup code,
   `time.c`, `cpu_idle.c`, `hal/CMakeLists.txt`, `stm32<family>xx_hal_conf.h`).

4. **Add the board name** to `SUPPORTED_BOARDS` in the root `Makefile` and map it
   to its HAL submodule.

---

## Running Tests

Host-native unit tests (Unity framework, no ARM toolchain or hardware required)
are planned but not yet committed to this repository.

---

## Contributing

Contributions are welcome. Please follow these conventions:

- **Layer isolation** — no layer may include headers from a layer above it. Sensor
  drivers (`platform/devices/`) must not include any HAL or board headers.
- **CMake library pattern** — use OBJECT + INTERFACE libraries (see existing drivers
  for the pattern). Applications link against the INTERFACE target.
- **No HAL modifications** — `hal/STM32Cube*/` are git submodules and must not be
  edited. All workarounds belong in the driver or MCU layer.
- **Test coverage** — new driver or facade code should be accompanied by a
  host-native unit test under `tests/unit/`.
- **Style** — match the indentation and naming of the surrounding file; code must
  pass the `clang-format` and `.editorconfig` CI checks on push.

### Getting started

```bash
# Fork on GitHub, then clone your fork
git clone https://github.com/<your-username>/stm32-app.git
cd stm32-app

# Build and confirm everything compiles before making changes
make

# Create a feature branch
git checkout -b feature/<short-description>
```

Open a pull request against `main` when ready.

> **CI:** A GitHub Actions workflow enforces `.editorconfig` and `clang-format`
> style checks on every push and pull request (`.github/workflows/style.yml`).

---

## License

This project is licensed under the [MIT License](LICENSE).

Copyright (c) 2026 Saurabh Daga

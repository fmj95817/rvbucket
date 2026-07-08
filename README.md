# RVBucket: ASM and RTL Implementation of a 32-bit RISC-V Architecture SoC

## Overview

This is an open-source 32-bit RISC-V processor, featuring a 2-stage pipelined RV32G instruction set CPU and a simple MCU-class SoC, implemented using both ASM (Architecture Simulation Model) and RTL approaches.

## System Specifications

- **CPU Core**: 2-stage pipelined RV32G (RV32IMACZicsr) CPU with Sv32 MMU; ASM includes TLB, RTL currently uses direct page-table walk
- **L1 Cache**: Configurable set-associative L1 I-Cache and L1 D-Cache with write-back support in ASM; RTL cache optimization is in progress
- **Boot ROM**: 2 KB
- **Flash**: 32 MB
- **ITCM (Instruction Tightly Coupled Memory)**: 128 KB
- **DTCM (Data Tightly Coupled Memory)**: 64 KB
- **DDR**: Configurable size, supports preloading Linux payloads
- **UART**: basic TX/RX function
- **GPIO**: 24-bit bidirectional GPIO with output/input/interrupt modes, Web UI buttons & switches
- **GTimer**: General-purpose countdown timer with IRQ
- **ACLINT**: Core-local interruptor (mtime, mtimecmp, MSWI, SSWI)
- **PLIC**: Platform-level interrupt controller (UART/GPIO/GTimer IRQs)
- **Bus Fabric**: BTI/AXI4/APB protocol bridges, mux, and demux

## Features

- **RV32G Instruction Set**: Supports RV32I + M + A + Zicsr + Zifencei.
- **2-Stage Pipeline**: IF/EX pipeline with branch prediction (BHT).
- **MMU with Sv32 Paging**: ASM supports TLB and page table walker; RTL supports Linux smoke with direct page-table walk.
- **L1 Cache**: Configurable set-associative L1 I-Cache and D-Cache with bypass support in ASM; RTL cache optimization is planned.
- **Trap Handling**: Machine/Supervisor mode traps, interrupts, delegations.
- **Simple SoC System**: Boot ROM, Flash, ITCM/DTCM, DDR, UART, GPIO, GTimer, ACLINT, PLIC.
- **ASM (Architecture Simulation Model)**: Cycle-accurate simulation with VCD waveform dumps and interface transaction traces.
- **Dual UI**: Web UI Dashboard (xterm.js terminal + GPIO panel + reset) and Terminal UI (vim-style cmd mode).
- **RTL**: SystemVerilog implementation with auto-generated interfaces (`itfgen.py`), supports VCS/Verilator/FPGA.
- **Unit Test Framework**: Modular UT framework with scoreboard, colored output, and interface trace verification.
- **Linux Support**: UART console, sysfs GPIO interface.
- **Regression Suite**: `run.sh` automates full SW case + UT regression with single-case mode.

## Quick Start

### Dependencies

- **Simulation Tools**: VCS or Verilator simulator.
- **RISC-V Toolchain**: RISC-V GCC toolchain for compiling test programs. See [RISC-V Toolchain GitHub Repository](https://github.com/riscv/riscv-gnu-toolchain).
- **FPGA Tools**: Xilinx Vivado (for FPGA target).
- **Python (Web UI)**: Python 3 + aiohttp (`pip3 install aiohttp`).

### Build Commands

| Command | Description |
|---------|-------------|
| `./build.sh hw model` | Build ASM simulation |
| `./build.sh hw rtl vcs` | Build RTL simulation (VCS) |
| `./build.sh hw rtl vcs debug` | Build RTL simulation (VCS debug + FSDB) |
| `./build.sh hw rtl verilator` | Build RTL simulation (Verilator) |
| `./build.sh hw fpga xilinx` | Generate Vivado FPGA project |
| `./build.sh sw` | Build all bare-metal C test cases |
| `./build.sh sw <name>` | Build a single C test case |
| `./build.sh sw linux` | Build Linux kernel + OpenSBI |
| `./build.sh sw linux clean` | Clean Linux kernel + OpenSBI build |
| `./build.sh ut model` | Build all ASM unit tests |
| `./build.sh ut model <name>` | Build a single UT or UT subdirectory |

*Note: For FPGA, add board description files under `fpga/xilinx/boards` and update `board` in `fpga/xilinx/proj.json`.*

### Running Simulations

**ASM:**
```bash
cd build/hw/model

# Terminal mode:
./sim_top ../../sw/<case>/<case>.bin

# Web UI dashboard (open http://localhost:5000):
./sim_top --web-ui ../../sw/<case>/<case>.bin

# Linux with fast preload to DDR:
./sim_top --fast-load-linux --web-ui ../../sw/linux/linux.bin

# Disable end-of-sim detection (interactive use):
./sim_top --no-end-detect --web-ui ../../sw/<case>/<case>.bin
```

**Web UI Dashboard:**
- xterm.js full terminal emulation (ANSI, cursor keys, paste, backspace)
- 16-LED GPIO output panel + 4 buttons + 4 toggle switches
- Reset button and auto-reconnect with history replay
- Light/dark theme toggle with persistent preference

**Terminal UI:**
- Raw-mode interactive UART console
- `ESC` enters vim-style cmd mode: `p1`–`p4` toggle buttons, `s1`–`s4` toggle switches, `r` reset, `i` back to interactive

**Regression Suite:**
```bash
./run.sh model sw              # all SW cases (batch)
./run.sh model sw <name>       # single SW case
./run.sh model ut              # all UTs (batch)
./run.sh model ut <name>       # single UT
./run.sh model                 # full SW + UT regression
```

**ASM Unit Tests (stand-alone):**
```bash
./build.sh ut model            # build all UTs
./run.sh model ut axi2bti      # run a single UT (cd to its build dir)
```

**RTL (VCS):**
```bash
cd build/hw/vcs
./sim_top +program=../../sw/<case>/<case>.hex
```

**RTL (Verilator):**
```bash
cd build/hw/verilator
./obj_dir/Vsim_top +program=../../sw/<case>/<case>.hex
```

## Project Structure

```
.
├── base/                    Shared library (ASM + RTL)
│   ├── model/base/          types, itf, fifo, arbiter
│   ├── model/dbg/           debug: vcd, log, pcm, chk
│   └── rtl/                 RTL base
├── design/
│   ├── model/               ASM source
│   │   ├── bus/             fabric: bridge, mux, demux
│   │   ├── core/            CPU & peripherals
│   │   ├── itf/             interface definitions
│   │   ├── mem/             memory: ram, rom, L1
│   │   ├── peri/            uart, gpio, gtimer
│   │   └── spec/            ISA, CSR, SoC config
│   └── rtl/                 RTL source (SystemVerilog)
├── sim/
│   ├── model/               ASM simulation top & UI
│   └── rtl/                 RTL simulation top
├── ut/model/                ASM unit tests (mirrors design/model/)
├── cases/                   Software test cases
├── sdk/                     Software SDK (CRT, drivers)
├── tools/                   Build tools, Web UI, itfgen
├── thirdparty/              Linux kernel, OpenSBI
└── build/                   Build outputs
```

## Development Status

- [x] ASM RV32G (I/M/A/Zicsr/Zifencei) instruction set
- [x] ASM MMU with Sv32 paging and TLB
- [x] ASM L1 I-Cache and D-Cache with bypass
- [x] ASM boots Linux kernel
- [x] ASM bus fabric with BTI/AXI4/APB protocol bridges, mux, and demux
- [x] ASM GPIO (24-bit bidirectional, output/input/interrupt modes)
- [x] ASM GTimer (general-purpose countdown timer)
- [x] ASM Web UI Dashboard (xterm.js, GPIO LEDs/buttons/switches, reset, theme toggle)
- [x] ASM Terminal UI (vim-style cmd mode, ESC-key reset)
- [x] ASM module-level unit test framework
- [x] ASM regression suite (run.sh)
- [x] Linux RVBucket GPIO driver (gpio-rvbucket, sysfs interface)
- [x] RTL RV32G instruction set
- [x] RTL MMU with Sv32 direct page-table walk
- [x] RTL reaches Linux smoke stage
- [ ] RTL TLB optimization
- [ ] RTL L1 cache optimization
- [ ] FPGA boots Linux kernel
- [ ] ASM/RTL F/D instruction set extensions

## Contributions

Contributions are welcome! Please follow these steps:

1. Fork this project.
2. Create your feature branch (`git checkout -b feature/YourFeature`).
3. Commit your changes (`git commit -am 'Add some feature'`).
4. Push to the branch (`git push origin feature/YourFeature`).
5. Create a Pull Request.

## License

This project is licensed under the [GPLv3 License](LICENSE).

## Contact

For any questions or suggestions, please contact azfmj22@hotmail.com.

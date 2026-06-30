# RVBucket: C-Model and RTL Implementation of a 32-bit RISC-V Architecture SoC

## Overview

This is an open-source 32-bit RISC-V processor, featuring a 2-stage pipelined RV32G instruction set CPU and a simple MCU-class SoC, implemented using both C-Model and RTL approaches.

## System Specifications

- **CPU Core**: 2-stage pipelined RV32G (RV32IMACZicsr) CPU with MMU and TLB
- **L1 Cache**: Configurable set-associative L1 I-Cache and L1 D-Cache with write-back support
- **Boot ROM**: 1 KB
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
- **MMU with Sv32 Paging**: TLB support, page table walker, boots Linux kernel.
- **L1 Cache**: Configurable set-associative L1 I-Cache and D-Cache with bypass support.
- **Trap Handling**: Machine/Supervisor mode traps, interrupts, delegations.
- **Simple SoC System**: Boot ROM, Flash, ITCM/DTCM, DDR, UART, GPIO, GTimer, ACLINT, PLIC.
- **C-Model**: Cycle-accurate simulation with VCD waveform dumps and interface transaction dumps.
- **Dual UI**: Web UI Dashboard (xterm.js terminal + GPIO panel + reset) and Terminal UI (vim-style cmd mode).
- **RTL**: SystemVerilog implementation, supports VCS and Verilator simulation, FPGA synthesis.
- **Unit Test Framework**: Modular UT framework with scoreboard, colored output, and interface dump verification.
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
| `./build.sh hw model` | Build C-Model simulation |
| `./build.sh hw rtl vcs` | Build RTL simulation (VCS) |
| `./build.sh hw rtl verilator` | Build RTL simulation (Verilator) |
| `./build.sh hw fpga xilinx` | Generate Vivado FPGA project |
| `./build.sh sw` | Build all bare-metal C test cases |
| `./build.sh sw <name>` | Build a single C test case |
| `./build.sh sw linux` | Build Linux kernel + OpenSBI |
| `./build.sh sw linux clean` | Clean Linux kernel + OpenSBI build |
| `./build.sh ut model` | Build all C-Model unit tests |
| `./build.sh ut model <name>` | Build a single UT or UT subdirectory |

*Note: For FPGA, add board description files under `fpga/xilinx/boards` and update `board` in `fpga/xilinx/proj.json`.*

### Running Simulations

**C-Model:**
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

**C-Model Unit Tests (stand-alone):**
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
model/            C-Model source
├── base/         Base types, ITF, FIFO, arbiter
├── bus/          Bus fabric (bridge, mux, demux)
│   ├── bridge/   bti2apb, bti2axi, axi2bti
│   ├── mux/      BTI mux, AXI mux
│   └── demux/    BTI demux, APB demux, AXI demux
├── core/         CPU core + peripherals
│   └── hart/     Hart sub-modules (ifu, exu, mmu, csr, trap)
├── dbg/          Debug infrastructure (VCD, log, PCM)
├── itf/          Interface definitions (BTI, AXI4, APB, GPIO, etc.)
├── mem/          Memory models (RAM, ROM, L1 cache)
├── peri/         Peripherals (UART, GPIO, GTimer)
└── spec/         Architecture specifications (CSR, ISA, SoC config)
sim/              Simulation environment
├── model/        C-Model simulation top, UI interface & implementations
└── rtl/          RTL simulation top
ut/model/         Unit tests (mirrors model/ structure)
├── utils.h/c     UT framework (scoreboard, macros)
├── bus/          Bus UTs (bridge, mux, demux)
├── core/         Core UTs (aclint, hart/ifu)
├── mem/          Memory UTs (ram, rom, l1)
└── peri/         Peripheral UTs (uart, gpio, gtimer, peri)
rtl/              RTL source (SystemVerilog)
cases/            Software test cases
sdk/              Software SDK (CRT, drivers for UART/GPIO/GTimer/syscalls)
tools/            Build tools (bin2x, mkbin) + Web UI (web_ui_v2.py/html)
thirdparty/       Third-party code (Linux kernel, OpenSBI)
build/            Build outputs
├── hw/model/     C-Model binary + UT binaries
├── hw/vcs/       VCS simulation
├── hw/verilator/ Verilator simulation
├── hw/vivado/    FPGA project
└── sw/           Software build outputs
```

## Development Status

- [x] C-Model RV32G (I/M/A/Zicsr/Zifencei) instruction set
- [x] C-Model MMU with Sv32 paging and TLB
- [x] C-Model L1 I-Cache and D-Cache with bypass
- [x] C-Model boots Linux kernel
- [x] C-Model bus fabric with BTI/AXI4/APB protocol bridges, mux, and demux
- [x] C-Model GPIO (24-bit bidirectional, output/input/interrupt modes)
- [x] C-Model GTimer (general-purpose countdown timer)
- [x] C-Model Web UI Dashboard (xterm.js, GPIO LEDs/buttons/switches, reset, theme toggle)
- [x] C-Model Terminal UI (vim-style cmd mode, ESC-key reset)
- [x] C-Model module-level unit test framework
- [x] C-Model regression suite (run.sh)
- [x] Linux RVBucket GPIO driver (gpio-rvbucket, sysfs interface)
- [ ] RTL RV32G instruction set
- [ ] RTL MMU with paging
- [ ] RTL L1 cache
- [ ] RTL boots Linux kernel
- [ ] FPGA boots Linux kernel
- [ ] C-Model/RTL F/D instruction set extensions

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

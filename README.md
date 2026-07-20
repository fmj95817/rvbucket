# RVBucket: ASM and RTL Implementation of a 32-bit RISC-V Architecture SoC

## Overview

This is an open-source 32-bit RISC-V processor, featuring a 2-stage pipelined RV32G instruction set CPU and a simple MCU-class SoC, implemented using both ASM (Architecture Simulation Model) and RTL approaches.

## System Specifications

- **CPU Core**: 2-stage pipelined RV32G (RV32IMAZicsr_Zicbom) CPU; both ASM and RTL include Sv32 MMU with TLB
- **L1 Cache**: Configurable set-associative L1 I-Cache and L1 D-Cache with write-back support in ASM; RTL cache optimization is in progress
- **Boot ROM**: 4 KB
- **Flash**: 32 MB
- **DDR**: Configurable size, supports preloading Linux payloads
- **UART**: basic TX/RX function
- **GPIO**: 24-bit bidirectional GPIO with output/input/interrupt modes, Web UI buttons & switches
- **GTimer**: General-purpose countdown timer with IRQ
- **SD-SPI Host**: Protocol-level controller with AXI DMA in ASM/RTL and an FPGA SPI PHY
- **ACLINT**: Core-local interruptor (mtime, mtimecmp, MSWI, SSWI)
- **PLIC**: Platform-level interrupt controller (UART/GPIO/GTimer IRQs)
- **Bus Fabric**: BTI/AXI4/APB protocol bridges, mux, and demux

## Features

- **RV32G Instruction Set**: Supports RV32I + M + A + Zicsr + Zifencei.
- **2-Stage Pipeline**: IF/EX pipeline with branch prediction (BHT).
- **MMU with Sv32 Paging**: ASM and RTL both support TLB and page table walker; RTL can run Linux smoke.
- **L1 Cache**: Configurable set-associative L1 I-Cache and D-Cache with bypass support in ASM; RTL cache optimization is planned.
- **Trap Handling**: Machine/Supervisor mode traps, interrupts, delegations.
- **Simple SoC System**: Boot ROM, Flash, DDR, UART, GPIO, GTimer, ACLINT, PLIC.
- **ASM (Architecture Simulation Model)**: Cycle-accurate simulation with VCD waveform dumps and interface transaction traces.
- **Dual UI**: Web UI Dashboard (xterm.js terminal + GPIO panel + reset) and Terminal UI (vim-style cmd mode).
- **RTL**: SystemVerilog implementation with auto-generated interfaces (`itfgen.py`), supports VCS/Verilator/FPGA.
- **Unit Test Framework**: Modular UT framework with scoreboard, colored output, and interface trace verification.
- **Linux Support**: UART console, sysfs GPIO interface.
- **Linux Storage**: Custom MMC host driver and ext4 root filesystem on SD.
- **Regression Suite**: `run.sh` automates full SW case + UT regression with single-case mode.

## Quick Start

### Dependencies

- **Simulation Tools**: VCS or Verilator simulator.
- **RISC-V Toolchain**: RISC-V GCC toolchain for compiling test programs. See [RISC-V Toolchain GitHub Repository](https://github.com/riscv/riscv-gnu-toolchain).
- **FPGA Tools**: Xilinx Vivado (for FPGA target).
- **Python (Web UI)**: Python 3 + aiohttp (`pip3 install aiohttp`).
- **SD Image Tools**: `sfdisk` and `mke2fs` from util-linux and e2fsprogs.

### Build Commands

| Command | Description |
|---------|-------------|
| `./build.sh hw model` | Build ASM simulation |
| `./build.sh hw rtl vcs` | Build RTL simulation (VCS) |
| `./build.sh hw rtl vcs debug` | Build RTL simulation (VCS debug + FSDB) |
| `./build.sh hw rtl verilator` | Build RTL simulation (Verilator) |
| `./build.sh hw fpga xilinx` | Generate Vivado FPGA project |
| `./build.sh sw` | Build all simulation bare-metal C test cases under `build/sw/sim` |
| `./build.sh sw sim <name>` | Build a single simulation C test case |
| `./build.sh sw board` | Build all board/real-hardware bare-metal C test cases under `build/sw/board` |
| `./build.sh sw board <name>` | Build a single board/real-hardware C test case |
| `./build.sh sw sim opensbi` / `./build.sh sw board opensbi` | Build the standalone OpenSBI case; currently both profiles share one output |
| `./build.sh sw sim linux` / `./build.sh sw board linux` | Build Linux kernel + OpenSBI; currently both profiles share one output |
| `./build.sh sw sim linux clean` / `./build.sh sw board linux clean` | Clean Linux kernel + OpenSBI build |
| `./build.sh ut model` | Build all ASM unit tests |
| `./build.sh ut model <name>` | Build a single UT or UT subdirectory |

*Note: For FPGA, add board description files under `fpga/xilinx/boards` and update `board` in `fpga/xilinx/proj.json`.*

### Running Simulations

**ASM:**
```bash
cd build/hw/model

# Terminal mode:
./sim_top ../../sw/sim/<case>/<case>.bin

# Web UI dashboard (open http://localhost:5000):
./sim_top --web-ui ../../sw/sim/<case>/<case>.bin

# Linux with fast preload to DDR:
./sim_top --fast-load-linux --web-ui ../../sw/linux/linux.bin

# Boot from QSPI while attaching an SD root filesystem:
./sim_top --boot qspi --sd-image <sd.img> <program.bin>

# Boot the kernel image stored in SD partition 1:
./sim_top --boot sdcard --sd-image <sd.img>

# Disable end-of-sim detection (interactive use):
./sim_top --no-end-detect --web-ui ../../sw/sim/<case>/<case>.bin
```

**SD Linux cold boot:**
```bash
./build.sh sw sim linux
./build.sh sw sim sdspi
./tools/mksdcard.sh build/sw/linux/linux.bin \
    thirdparty/busybox/rootfs build/sw/linux/sdcard.img

cd build/hw/model
./sim_top --boot-prog --boot sdcard \
    --sd-image ../../sw/linux/sdcard.img
```

The generated DOS-partitioned image stores `linux.bin` in bootable raw
partition 1 (type `0xda`) and an ext4 root filesystem in partition 2. The Boot
ROM samples GPIO 21 to select QSPI or SD as the image source. The simulator's
`--boot` option drives that pin; QSPI remains the default. QSPI and SD images
may be supplied together because boot selection is independent of media
attachment. Linux starts from the initramfs, mounts `/dev/mmcblk0p2`, and
switches to it as the root filesystem.

The SD-SPI design is split at a packet interface. The `u_io` subsystem owns
DMA-capable devices separately from the APB-only `u_peri` subsystem. Its
SD-SPI controller owns APB registers and an AXI DMA master. The C-model VIP
under `sim/model/vip` and the shared VCS/Verilator DPI backend under
`sim/rtl/common` provide persistent file-backed cards. FPGA builds terminate the
same packet interfaces in `design/rtl/io/sdspi_phy.sv`. `sdspi_cmd` and
`sdspi_data` are
backpressured generated interfaces with matching C-model and SystemVerilog
definitions. The controller keeps one command outstanding but overlaps AXI DMA
and protocol traffic through a 256-byte FIFO; write transfers start after a
64-byte prefetch. The command stream also carries controller enable and SPI
clock-divider configuration. See `design/model/spec/io/sdspi_itf.md` for packet
ordering and byte-order rules.

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
./sim_top +program=../../sw/sim/<case>/<case>.hex

# Boot from SD; +boot_prog independently enables bootloader progress output.
./sim_top +boot=sdcard +boot_prog \
    +sd_image=../../sw/linux/sdcard.img
```

**RTL (Verilator):**
```bash
cd build/hw/verilator
./obj_dir/Vsim_top +program=../../sw/sim/<case>/<case>.hex

# QSPI boot with an attached SD data/root filesystem.
./obj_dir/Vsim_top +boot=qspi \
    +program=../../sw/linux/linux.hex \
    +sd_image=../../sw/linux/sdcard.img
```

RTL uses the same boot semantics as ASM: `qspi` is the default and requires
`+program`; `sdcard` requires `+sd_image`; `+fast_load_linux` is valid only for
QSPI boot. `+boot_prog` controls GPIO 20 and `+boot=sdcard` controls GPIO 21.

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
- [x] ASM/RTL SD-SPI host with AXI DMA, SD cold boot, and Linux ext4 rootfs
- [x] ASM Web UI Dashboard (xterm.js, GPIO LEDs/buttons/switches, reset, theme toggle)
- [x] ASM Terminal UI (vim-style cmd mode, ESC-key reset)
- [x] ASM module-level unit test framework
- [x] ASM regression suite (run.sh)
- [x] Linux RVBucket GPIO driver (gpio-rvbucket, sysfs interface)
- [x] RTL RV32G instruction set
- [x] RTL Sv32 MMU with direct page-table walk and TLB optimization
- [x] RTL reaches Linux smoke stage
- [x] RTL TLB optimization
- [ ] RTL L1 cache optimization
- [x] FPGA boots Linux kernel
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

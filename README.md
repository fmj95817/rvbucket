# RVBucket: ASM and RTL Implementation of a 32-bit RISC-V Architecture SoC

[简体中文](README.zh-CN.md)

## Overview

RVBucket is an open-source 32-bit RISC-V processor and MCU-class SoC with both
a cycle-accurate ASM (Architecture Simulation Model) and synthesizable
SystemVerilog RTL. Both implementations boot Linux, while the RTL also targets
VCS, Verilator, ASIC synthesis, and Xilinx FPGA platforms.

## System Specifications

- **CPU Core**: In-order RV32IMA_Zicsr_Zifencei_Zicbom CPU with Machine,
  Supervisor, and User modes; ASM and RTL both include an Sv32 MMU and TLB
- **L1 Cache**: 16 KiB, 2-way, 64-byte-line write-back L1 I-Cache and D-Cache
  in ASM and RTL, with bypass and Zicbom cache-block operations
- **Boot ROM**: 4 KB
- **Flash**: 32 MB
- **DDR**: Configurable size, supports preloading Linux payloads
- **UART**: TX/RX, interrupt, and Linux console support
- **GPIO**: 24-bit bidirectional GPIO with output/input/interrupt modes, Web UI buttons & switches
- **GTimer**: General-purpose countdown timer with IRQ
- **SD-SPI Host**: Protocol-level controller with AXI DMA in ASM/RTL and an FPGA SPI PHY
- **ACLINT**: Core-local interruptor (mtime, mtimecmp, MSWI, SSWI)
- **PLIC**: Platform-level interrupt controller for UART, GPIO, GTimer, and SD-SPI IRQs
- **Bus Fabric**: BTI/AXI4/APB protocol bridges, mux, and demux

## Features

- **RISC-V ISA**: RV32I + M + A + Zicsr + Zifencei + Zicbom.
- **CPU Microarchitecture**: In-order pipeline with BHT branch prediction.
- **MMU with Sv32 Paging**: ASM and RTL both support TLBs and hardware page-table walking.
- **L1 Cache**: Configurable set-associative write-back L1 I-Cache and D-Cache
  with cached, bypass, flush, and cache-block-management paths.
- **Non-coherent DMA**: Standard Zicbom cache maintenance is used by Linux for
  DMA between cached memory and devices such as SD-SPI.
- **Trap Handling**: Machine/Supervisor mode traps, interrupts, delegations.
- **SoC System**: Boot ROM, Flash, DDR, UART, GPIO, GTimer, SD-SPI, ACLINT,
  PLIC, and a dedicated DMA-capable I/O subsystem.
- **ASM (Architecture Simulation Model)**: Cycle-accurate simulation with VCD waveform dumps and interface transaction traces.
- **Dual UI**: Web UI Dashboard (xterm.js terminal + GPIO panel + reset) and Terminal UI (vim-style cmd mode).
- **RTL**: SystemVerilog implementation with auto-generated interfaces (`itfgen.py`), supports VCS/Verilator/FPGA.
- **Unit Test Framework**: Automated ASM and RTL module UTs with scoreboards,
  timeouts, colored summaries, and interface protocol checking.
- **Linux Support**: OpenSBI boot, Sv32 virtual memory, UART console, GPIO,
  MMC block storage, and userspace regression tests in ASM, VCS, and FPGA.
- **Linux Storage**: Custom MMC host driver and ext4 root filesystem on SD.
- **Boot Media**: Bootloader-selectable QSPI or SD kernel image; an attached SD
  ext4 root filesystem can be used with either boot source.
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
| `./build.sh hw fpga xilinx <project\|ip\|syn\|impl\|bit> [config]` | Run the selected Vivado FPGA flow |
| `./build.sh hw asic syn <config>` | Run ASIC synthesis with the selected configuration |
| `./build.sh sw` | Build all simulation bare-metal C test cases under `build/sw/sim` |
| `./build.sh sw sim <name>` | Build a single simulation C test case |
| `./build.sh sw board` | Build all board/real-hardware bare-metal C test cases under `build/sw/board` |
| `./build.sh sw board <name>` | Build a single board/real-hardware C test case |
| `./build.sh sw sim opensbi` / `./build.sh sw board opensbi` | Build the standalone OpenSBI case; currently both profiles share one output |
| `./build.sh sw sim linux` / `./build.sh sw board linux` | Build Linux kernel + OpenSBI; currently both profiles share one output |
| `./build.sh sw sim linux clean` / `./build.sh sw board linux clean` | Clean Linux kernel + OpenSBI build |
| `./build.sh ut model` | Build all ASM unit tests |
| `./build.sh ut model <name>` | Build a single UT or UT subdirectory |
| `./build.sh ut rtl` | Build all RTL unit tests with VCS |
| `./build.sh ut rtl <name>` | Build a single RTL UT or UT subdirectory |

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
./tools/mksdcard.sh build/sw/linux/linux.bin \
    thirdparty/busybox/rootfs build/sw/linux/sdcard.img

cd build/hw/model
./sim_top --boot-prog --boot sdcard \
    --sd-image ../../sw/linux/sdcard.img
```

The generated DOS-partitioned image stores `linux.bin` in bootable raw
partition 1 (type `0xda`) and an ext4 root filesystem in partition 2. The Boot
ROM samples GPIO 21 to select QSPI or SD as the image source. The simulator's
`--boot` option drives that pin. QSPI is the default for ASM, RTL simulation,
and the FPGA project. QSPI and SD images may be supplied together because boot
selection is independent of media attachment. Linux starts from the initramfs,
mounts `/dev/mmcblk0p2`, and switches to it as the root filesystem.

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
- Press `Ctrl-\` twice to enter vim-style cmd mode: `p1`–`p4` toggle buttons,
  `s1`–`s4` toggle switches, `r` reset, `i` back to interactive

**Regression Suite:**
```bash
./run.sh model sw              # all SW cases (batch)
./run.sh model sw <name>       # single SW case
./run.sh model ut              # all UTs (batch)
./run.sh model ut <name>       # single UT
./run.sh model                 # full SW + UT regression
./run.sh rtl ut                # all RTL module UTs
./run.sh rtl ut <name>         # single RTL module UT
./run.sh rtl vcs sw            # all RTL bare-metal cases with VCS
./run.sh rtl verilator sw      # all RTL bare-metal cases with Verilator
```

**Unit Tests:**
```bash
./build.sh ut model            # build all UTs
./run.sh model ut axi2bti      # run one ASM UT
./build.sh ut rtl              # build all RTL UTs
./run.sh rtl ut io/sdspi_phy   # run one RTL UT
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
│   │   ├── core/            CPU core and platform controllers
│   │   ├── io/              DMA-capable I/O controllers
│   │   ├── itf/             interface definitions
│   │   ├── mem/             memory models
│   │   ├── peri/            uart, gpio, gtimer
│   │   └── spec/            ISA, CSR, SoC config
│   └── rtl/                 synthesizable SystemVerilog, mirroring ASM hierarchy
├── sim/
│   ├── model/               ASM simulation top, UI, and device VIPs
│   └── rtl/                 VCS/Verilator tops, models, VIPs, and DPI backends
├── ut/
│   ├── model/               ASM module unit tests
│   └── rtl/                 RTL module unit tests
├── cases/
│   ├── bare/                bare-metal software tests and bootloader
│   └── user/                Linux userspace tests and init scripts
├── sdk/                     Software SDK (CRT, drivers)
├── fpga/xilinx/             board descriptions, FPGA RTL, and Vivado configuration
├── tools/                   build/image tools, Web UI, interface generator
├── thirdparty/              Linux kernel, OpenSBI, and BusyBox
└── build/                   Build outputs
```

## Development Status

- [x] ASM and RTL RV32IMA_Zicsr_Zifencei_Zicbom instruction set
- [x] ASM and RTL Sv32 MMU with TLB and hardware page-table walker
- [x] ASM and RTL write-back L1 I-Cache/D-Cache with bypass and Zicbom CMO
- [x] ASM and RTL BTI/AXI4/APB bus fabric and DMA-capable I/O subsystem
- [x] UART, GPIO, GTimer, ACLINT, PLIC, and their interrupt paths
- [x] SD-SPI host with AXI DMA, protocol VIPs, synthesizable SPI PHY, and Linux MMC driver
- [x] QSPI/SD boot selection and SD ext4 root filesystem switching
- [x] Linux boot and userspace regression in ASM, VCS, and Xilinx FPGA
- [x] ASM Web UI Dashboard and interactive Terminal UI
- [x] Automated ASM/RTL module UT and bare-metal regression frameworks
- [x] Linux RVBucket UART, GPIO, and MMC host drivers
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

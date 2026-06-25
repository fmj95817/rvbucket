# RVBucket: C-Model and RTL Implementation of a 32-bit RISC-V Architecture SoC

## Overview

This is an open-source 32-bit RISC-V processor, featuring a 2-stage pipelined RV32G instruction set CPU and a simple MCU-class SoC, implemented using both C-Model and RTL approaches.

## System Specifications

- **CPU Core**: 2-stage pipelined RV32G (RV32IMACZicsr) CPU with MMU and TLB
- **Boot ROM**: 1 KB
- **Flash**: 1 MB
- **ITCM (Instruction Tightly Coupled Memory)**: 128 KB
- **DTCM (Data Tightly Coupled Memory)**: 64 KB
- **DDR**: Configurable size, supports preloading Linux payloads
- **UART**: 16550-compatible serial interface
- **ACLINT**: Core-local interruptor (mtime, mtimecmp, MSWI, SSWI)
- **PLIC**: Platform-level interrupt controller
- **Bus Fabric**: BTI/AXI4/APB protocol bridges and crossbars

## Features

- **RV32G Instruction Set**: Supports RV32I + M + A + Zicsr + Zifencei.
- **2-Stage Pipeline**: IF/EX pipeline with branch prediction (BHT).
- **MMU with Sv32 Paging**: TLB support, page table walker, boots Linux kernel.
- **Trap Handling**: Machine/Supervisor mode traps, interrupts, delegations.
- **Simple SoC System**: Boot ROM, Flash, ITCM/DTCM, DDR, UART, ACLINT, PLIC.
- **C-Model**: Cycle-accurate simulation with VCD waveform dumps and interface transaction dumps.
- **RTL**: SystemVerilog implementation, supports VCS and Verilator simulation, FPGA synthesis.
- **Unit Test Framework**: Modular UT framework with scoreboard, colored output, and interface dump verification.

## Quick Start

### Dependencies

- **Simulation Tools**: VCS or Verilator simulator.
- **RISC-V Toolchain**: RISC-V GCC toolchain for compiling test programs. See [RISC-V Toolchain GitHub Repository](https://github.com/riscv/riscv-gnu-toolchain).
- **FPGA Tools**: Xilinx Vivado (for FPGA target).

### Build Commands

| Command | Description |
|---------|-------------|
| `./build.sh hw model` | Build C-Model simulation |
| `./build.sh hw model ut` | Build C-Model + all unit tests |
| `./build.sh hw rtl vcs` | Build RTL simulation (VCS) |
| `./build.sh hw rtl verilator` | Build RTL simulation (Verilator) |
| `./build.sh hw fpga xilinx` | Generate Vivado FPGA project |
| `./build.sh sw` | Build all software test cases |
| `./build.sh linux` | Build Linux kernel + OpenSBI |

*Note: For FPGA, add board description files under `fpga/xilinx/boards` and update `board` in `fpga/xilinx/proj.json`.*

### Running Simulations

**C-Model:**
```bash
cd build/hw/model
./sim_top ../../sw/<test_case>/<test_case>.bin

# Fast-load Linux (preloads kernel/initrd/dtb to DDR):
./sim_top --fast-load-linux ../../sw/linux/linux.bin
```

**C-Model Unit Tests:**
```bash
# Build all UTs:
./build.sh hw model ut

# Run a UT (cd to its build dir to keep artifacts local):
cd build/hw/model/ut/<module_path>
./<module_name>_ut

# Example: run the axi2bti bridge UT with interface dump
cd build/hw/model/ut/bus/bridge
ITF_DUMP=1 ./axi2bti_ut
cat itf_dump/*.txt
```

**RTL (VCS):**
```bash
cd build/hw/vcs
./sim_top +program=../../sw/<test_case>/<test_case>.hex
```

**RTL (Verilator):**
```bash
cd build/hw/verilator
./obj_dir/Vsim_top +program=../../sw/<test_case>/<test_case>.hex
```

## Project Structure

```
model/            C-Model source
├── base/         Base types, ITF, FIFO
├── bus/          Bus fabric (bridge, mux, demux)
│   ├── bridge/   bti2apb, bti2axi, axi2bti
│   ├── mux/      BTI mux
│   └── demux/    BTI/APB demux
├── core/         CPU core + peripherals
│   └── hart/     Hart sub-modules (ifu, exu, mmu, csr, trap)
├── dbg/          Debug infrastructure (VCD, log, PCM)
├── io/           I/O peripherals (UART)
├── itf/          Interface definitions (BTI, AXI4, APB, etc.)
├── mem/          Memory models (RAM, ROM)
└── spec/         Architecture specifications (CSR, ISA, SoC config)
sim/              Simulation environment
├── model/        C-Model simulation top
└── rtl/          RTL simulation top
ut/model/         Unit tests (mirrors model/ structure)
├── utils.h/c     UT framework (scoreboard, macros)
├── bus/bridge/   Bridge UTs (bti2axi, axi2bti)
├── core/         Core UTs (aclint, hart/ifu)
└── io/           I/O UTs (uart)
rtl/              RTL source (SystemVerilog)
cases/            Software test cases
sdk/              Software SDK (CRT, drivers)
tools/            Build tools (bin2x, mkbin)
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
- [x] C-Model boots Linux kernel
- [x] C-Model bus fabric with BTI/AXI4/APB protocol bridges
- [x] C-Model unit test framework with 5 module UTs
- [x] RTL RV32G instruction set
- [ ] RTL MMU with paging
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

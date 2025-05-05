# RVBucket: C-Model and RTL Implementation of a 32-bit RISC-V Architecture SoC
## Overview

This is an open-source 32-bit RISC-V processor, featuring a 2-stage pipelined RV32I instruction set CPU and a simple MCU-class SoC, implemented using both C-Model and RTL approaches.

## System Specifications

- **CPU Core**: 2-stage pipelined RV32I instruction set CPU
- **Boot ROM**: 1 KB
- **Flash**: 1 MB
- **ITCM (Instruction Tightly Coupled Memory)**: 128 KB
- **DTCM (Data Tightly Coupled Memory)**: 64 KB
- **UART Interface**: For serial communication

## Features

- **RV32I Instruction Set Support**: Supports the RISC-V base integer instruction set (RV32I).
- **2-Stage Pipeline**: Utilizes an IF/EX 2-stage pipeline design.
- **Simple SoC System**: Includes Boot ROM, Flash, ITCM/DTCM, and UART interface, suitable for embedded applications.
- **C-Model Implementation**: Provides a cycle-accurate simulation model for performance analysis and verification.
- **RTL Implementation**: Implemented in SystemVerilog, supports simulation on VCS and Verilator, and can run on FPGA.

## Quick Start

### Dependencies

- **Simulation Tools**: Requires installation of VCS or Verilator simulator.
- **RISC-V Toolchain**: Requires installation of the RISC-V GCC toolchain to compile and generate test programs. Can be obtained from the RISC-V Toolchain GitHub Repository.
- **FPGA Tools**: Currently supports Xilinx platforms; requires installation of Vivado.

### Compilation and Execution

1. **Compile the C-Model**:
   ```bash
   ./build.sh hw model
   ```

2. **Compile RTL Simulation**:
    For VCS simulator:
    ```bash
   ./build.sh hw rtl vcs
   ```
    For Verilator simulator:
   ```bash
   ./build.sh hw rtl verilator
   ```

3. **Generate FPGA Project**:
    For Xilinx FPGA:
    ```bash
   ./build.sh hw fpga xilinx
   ```
    After execution, the Vivado project will be generated under `build/hw/vivado`.

    *Note: FPGA board description files need to be added under `fpga/xilinx/boards`, and the `board` item in `fpga/xilinx/proj.json` should be updated.*

4. **Compile Test Cases**:
    Ensure the bin directory of the RISC-V toolchain is added to the PATH environment variable.
   ```bash
   ./build.sh sw
   ```

5. **Run the C-Model**:
   ```bash
   cd build/hw/model
   ./sim_top ../../sw/<test_case_name>/<test_case_name>.bin
   ```
6. **Run RTL VCS Simulation**:
   ```bash
   cd build/hw/vcs
   ./sim_top +program=../../sw/<test_case_name>/<test_case_name>.hex
   ```

7. **Run RTL Verilator Simulation**:
    ```bash
   cd build/hw/verilator
   ./obj_dir/Vsim_top +program=../../sw/<test_case_name>/<test_case_name>.hex
   ```

## Development Plan
* C-Model supports M/A/Zicsr/Zifencei instruction sets
* C-Model supports paging and boots the Linux kernel
* RTL supports M/A/Zicsr/Zifencei instruction sets
* RTL supports paging and boots the Linux kernel
* FPGA boots the Linux kernel
* C-Model/RTL support F/D instruction sets

## Contributions

Contributions in the form of code, bug reports, and suggestions are welcome! Please follow these steps:

1. Fork this project.
2. Create your feature branch (git checkout -b feature/YourFeature).
3. Commit your changes (git commit -am 'Add some feature').
4. Push to the branch (git push origin feature/YourFeature).
5. Create a Pull Request.

## License

This project is licensed under the [GPLv3 License](LICENSE).

## Contact

For any questions or suggestions, please contact azfmj22@hotmail.com.
# RVBucket: 一个32位RISC-V架构SoC的C-Model及RTL实现

## 概述

这是一个开源的32位RISC-V处理器，包含一个2级流水线的RV32I指令集CPU、一个简单的MCU类SoC，采用C-Model方式和RTL方式实现。

## 系统规格

- **CPU核心**: 2级流水线的RV32I指令集CPU
- **Boot ROM**: 1 KB
- **Flash**: 1 MB
- **ITCM (Instruction Tightly Coupled Memory)**: 128 KB
- **DTCM (Data Tightly Coupled Memory)**: 64 KB
- **UART接口**: 用于串口通信

## 功能特性

- **RV32I指令集支持**: 支持RISC-V基础整数指令集（RV32I）。
- **2级流水线**: 采用if/ex 2级流水线设计。
- **简单SoC系统**: 包含Boot ROM、Flash、ITCM/DTCM和UART接口，适合嵌入式应用。
- **C模型实现**: 提供精确到时钟周期的仿真模型，便于性能分析和验证。
- **RTL实现**: 使用SystemVerilog实现，支持在VCS和Verilator仿真器上仿真，支持在FPGA上运行。

## 快速开始

### 依赖项

- **仿真工具**: 需要安装VCS或Verilator仿真器。
- **RISC-V工具链**: 需要安装RISC-V GCC工具链以编译和生成测试程序。可以从[RISC-V工具链GitHub仓库](https://github.com/riscv/riscv-gnu-toolchain)获取。
- **FPGA工具**: 目前仅支持Xilinx平台，需要安装Vivado。

### 编译和运行

1. **编译CA模型**:
   ```bash
   ./build.sh hw model
   ```

2. **编译RTL仿真**:
   对于VCS仿真器:
   ```bash
   ./build.sh hw rtl vcs
   ```
   对于Verilator仿真器:
   ```bash
   ./build.sh hw rtl verilator
   ```

3. **生成FPGA工程**:
   对于Xilinx FPGA:
   ```bash
   ./build.sh hw fpga xilinx
   ```
   运行后会在`build/hw/vivado`下生成Vivado工程。

   *备注：需要在`fpga/xilinx/boards`下添加FPGA开发板描述文件，并在`fpga/xilinx/proj.json`中更新`board`项。*

4. **编译测试用例**:
   确保RISC-V工具链的`bin`目录已添加到`PATH`环境变量中。
   ```bash
   ./build.sh sw
   ```

5. **运行C模型**:
   ```bash
   cd build/hw/model
   ./sim_top ../../sw/<用例名称>/<用例名称>.bin
   ```

7. **运行RTL VCS仿真**:
   ```bash
   cd build/hw/vcs
   ./sim_top +program=../../sw/<用例名称>/<用例名称>.hex
   ```

7. **运行RTL Verilator仿真**:
   ```bash
   cd build/hw/verilator
   ./obj_dir/Vsim_top +program=../../sw/<用例名称>/<用例名称>.hex
   ```

## 贡献

欢迎贡献代码、报告问题和提出建议！请遵循以下步骤：

1. Fork 本项目
2. 创建您的特性分支 (`git checkout -b feature/YourFeature`)
3. 提交您的更改 (`git commit -am 'Add some feature'`)
4. 推送到分支 (`git push origin feature/YourFeature`)
5. 创建一个 Pull Request

## 许可证

本项目采用 [GPLv3 许可证](LICENSE)。

## 联系方式

如有任何问题或建议，请联系 [azfmj22@hotmail.com](mailto:azfmj22@hotmail.com)。

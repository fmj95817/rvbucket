# RVBucket: RV32I CPU + SoC的CA model与RTL实现

## 概述

本项目是一个开源的RISC-V CPU实现，包含一个2级流水线的RV32I指令集CPU、一个简单的SoC系统以及对应的Cycle-Accurate (CA)模型和RTL实现。

## SoC系统规格

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
- **CA模型**: 提供精确到时钟周期的仿真模型，便于性能分析和验证。
- **RTL实现**: 使用SystemVerilog实现，支持在VCS和Verilator仿真器上仿真，支持在FPGA上运行。

## 快速开始

### 依赖项

- **仿真工具**: 需要安装VCS或Verilator仿真器。
- **RISC-V工具链**: 需要安装RISC-V GCC工具链以编译和生成测试程序。可以从[RISC-V工具链GitHub仓库](https://github.com/riscv/riscv-gnu-toolchain)获取。
- **FPGA工具**: 目前仅支持Xilinx平台，需要安装Vivado。

### 编译和运行

1. **克隆仓库**:
   ```bash
   git clone https://gitee.com/bmyds/rvbucket
   cd rvbucket
   ```

2. **编译CA模型**:
   ```bash
   ./build.sh hw model
   ```

3. **编译RTL仿真**:
   对于VCS仿真器:
   ```bash
   ./build.sh hw rtl vcs
   ```
   对于Verilator仿真器:
   ```bash
   ./build.sh hw rtl verilator
   ```

4. **生成FPGA工程**:
   对于Xilinx FPGA，需要在`fpga/xilinx/boards`下添加FPGA开发板描述文件，并在`fpga/xilinx/proj.json`中更新`board`项，然后运行：
   ```bash
   ./build.sh hw fpga xilinx
   ```
   运行后会在`build/hw/vivado`下生成Vivado工程。

5. **编译测试用例**:
   确保RISC-V工具链的`bin`目录已添加到`PATH`环境变量中。
   ```bash
   ./build.sh sw
   ```

6. **运行CA模型**:
   ```bash
   cd build/hw/model
   ./sim_top ../../sw/<用例名称>/<用例名称>.bin
   ```

7. **运行RTL VCS仿真**:
   ```bash
   cd build/hw/vcs
   ./sim_top +program=../../sw/<用例名称>/<用例名称>.hex
   ```

8. **运行RTL Verilator仿真**:
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

本项目采用 [MIT 许可证](LICENSE)。

## 联系方式

如有任何问题或建议，请联系 [azfmj22@hotmail.com](mailto:azfmj22@hotmail.com)。

---

感谢您对本项目的关注和支持！希望这个RISC-V CPU项目能为您的学习和开发带来帮助。
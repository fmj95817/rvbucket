# RVBucket: 一个32位RISC-V架构SoC的C-Model及RTL实现

## 概述

这是一个开源的32位RISC-V处理器，包含一个2级流水线的RV32G指令集CPU、一个简单的MCU类SoC，采用C-Model方式和RTL方式实现。

## 系统规格

- **CPU核心**: 2级流水线的RV32G (RV32IMACZicsr) CPU，含MMU和TLB
- **Boot ROM**: 1 KB
- **Flash**: 1 MB
- **ITCM (Instruction Tightly Coupled Memory)**: 128 KB
- **DTCM (Data Tightly Coupled Memory)**: 64 KB
- **DDR**: 可配置大小，支持预加载Linux镜像
- **UART**: 16550兼容串口
- **ACLINT**: 核局部中断控制器（mtime, mtimecmp, MSWI, SSWI）
- **PLIC**: 平台级中断控制器
- **总线架构**: BTI/AXI4/APB协议桥接与交叉开关

## 功能特性

- **RV32G指令集**: 支持 RV32I + M + A + Zicsr + Zifencei。
- **2级流水线**: IF/EX流水线，含分支预测（BHT）。
- **MMU与Sv32分页**: TLB支持，页表遍历器，可启动Linux内核。
- **陷阱处理**: 机器态/监管态陷阱、中断、委托机制。
- **简单SoC系统**: Boot ROM、Flash、ITCM/DTCM、DDR、UART、ACLINT、PLIC。
- **C模型**: 精确到时钟周期的仿真，支持VCD波形和接口事务dump。
- **RTL实现**: SystemVerilog实现，支持VCS和Verilator仿真及FPGA综合。
- **单元测试框架**: 模块化UT框架，彩色输出，计分板，接口dump验证。

## 快速开始

### 依赖项

- **仿真工具**: VCS 或 Verilator 仿真器。
- **RISC-V工具链**: RISC-V GCC 工具链，用于编译测试程序。详见 [RISC-V工具链GitHub仓库](https://github.com/riscv/riscv-gnu-toolchain)。
- **FPGA工具**: Xilinx Vivado（FPGA目标）。

### 编译命令

| 命令 | 说明 |
|------|------|
| `./build.sh hw model` | 编译C模型仿真 |
| `./build.sh hw model ut` | 编译C模型 + 全部单元测试 |
| `./build.sh hw rtl vcs` | 编译RTL仿真（VCS） |
| `./build.sh hw rtl verilator` | 编译RTL仿真（Verilator） |
| `./build.sh hw fpga xilinx` | 生成Vivado FPGA工程 |
| `./build.sh sw` | 编译全部软件测试用例 |
| `./build.sh linux` | 编译Linux内核 + OpenSBI |

*备注：FPGA需要在 `fpga/xilinx/boards` 下添加开发板描述文件，并在 `fpga/xilinx/proj.json` 中更新 `board` 项。*

### 运行仿真

**C模型:**
```bash
cd build/hw/model
./sim_top ../../sw/<用例名称>/<用例名称>.bin

# Linux快速加载（预加载kernel/initrd/dtb至DDR）:
./sim_top --fast-load-linux ../../sw/linux/linux.bin
```

**C模型单元测试:**
```bash
# 编译全部UT:
./build.sh hw model ut

# 运行UT（cd到对应模块的build目录，产物落在本地）:
cd build/hw/model/ut/<模块路径>
./<模块名>_ut

# 示例：运行axi2bti桥接UT并开启接口dump
cd build/hw/model/ut/bus/bridge
ITF_DUMP=1 ./axi2bti_ut
cat itf_dump/*.txt
```

**RTL (VCS):**
```bash
cd build/hw/vcs
./sim_top +program=../../sw/<用例名称>/<用例名称>.hex
```

**RTL (Verilator):**
```bash
cd build/hw/verilator
./obj_dir/Vsim_top +program=../../sw/<用例名称>/<用例名称>.hex
```

## 项目结构

```
model/            C模型源码
├── base/         基础类型、ITF接口、FIFO
├── bus/          总线架构（bridge, mux, demux）
│   ├── bridge/   bti2apb, bti2axi, axi2bti
│   ├── mux/      BTI多路选择
│   └── demux/    BTI/APB地址路由
├── core/         CPU核心与外设
│   └── hart/     Hart子模块（ifu, exu, mmu, csr, trap）
├── dbg/          调试基础设施（VCD, log, PCM）
├── io/           I/O外设（UART）
├── itf/          接口定义（BTI, AXI4, APB等）
├── mem/          存储模型（RAM, ROM）
└── spec/         架构规格（CSR, ISA, SoC配置）
sim/              仿真环境
├── model/        C模型仿真顶层
└── rtl/          RTL仿真顶层
ut/model/         单元测试（镜像 model/ 目录结构）
├── utils.h/c     UT框架（计分板、宏）
├── bus/bridge/   桥接模块UT（bti2axi, axi2bti）
├── core/         核心模块UT（aclint, hart/ifu）
└── io/           I/O模块UT（uart）
rtl/              RTL源码（SystemVerilog）
cases/            软件测试用例
sdk/              软件SDK（CRT、驱动）
tools/            构建工具（bin2x, mkbin）
build/            构建产物
├── hw/model/     C模型可执行文件 + UT可执行文件
├── hw/vcs/       VCS仿真
├── hw/verilator/ Verilator仿真
├── hw/vivado/    FPGA工程
└── sw/           软件构建产物
```

## 开发状态

- [x] C-Model RV32G (I/M/A/Zicsr/Zifencei) 指令集
- [x] C-Model MMU 含 Sv32 分页和 TLB
- [x] C-Model 启动 Linux 内核
- [x] C-Model 总线架构含 BTI/AXI4/APB 协议桥接
- [x] C-Model 单元测试框架，覆盖5个模块
- [x] RTL RV32G 指令集
- [ ] RTL MMU 分页
- [ ] RTL 启动 Linux 内核
- [ ] FPGA 启动 Linux 内核
- [ ] C-Model/RTL F/D 指令集扩展

## 贡献

欢迎贡献代码、报告问题和提出建议！

1. Fork 本项目
2. 创建特性分支 (`git checkout -b feature/YourFeature`)
3. 提交更改 (`git commit -am 'Add some feature'`)
4. 推送到分支 (`git push origin feature/YourFeature`)
5. 创建 Pull Request

## 许可证

本项目采用 [GPLv3 许可证](LICENSE)。

## 联系方式

如有任何问题或建议，请联系 [azfmj22@hotmail.com](mailto:azfmj22@hotmail.com)。

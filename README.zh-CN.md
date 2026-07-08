# RVBucket: 一个32位RISC-V架构SoC的C-Model及RTL实现

## 概述

这是一个开源的32位RISC-V处理器，包含一个2级流水线的RV32G指令集CPU、一个简单的MCU类SoC，采用C-Model方式和RTL方式实现。

## 系统规格

- **CPU核心**: 2级流水线的RV32G (RV32IMACZicsr) CPU，含MMU和TLB
- **L1 Cache**: 可配置组相联L1指令缓存和数据缓存，支持写回
- **Boot ROM**: 2 KB
- **Flash**: 32 MB
- **ITCM (Instruction Tightly Coupled Memory)**: 128 KB
- **DTCM (Data Tightly Coupled Memory)**: 64 KB
- **DDR**: 可配置大小，支持预加载Linux镜像
- **UART**: 基本TX/RX功能
- **GPIO**: 24位双向GPIO，支持输出/输入/中断三种模式，Web UI模拟按钮和开关
- **GTimer**: 通用递减计时器，支持IRQ
- **ACLINT**: 核局部中断控制器（mtime, mtimecmp, MSWI, SSWI）
- **PLIC**: 平台级中断控制器（UART/GPIO/GTimer 共3个中断源）
- **总线架构**: BTI/AXI4/APB协议桥接、多路选择及地址路由

## 功能特性

- **RV32G指令集**: 支持 RV32I + M + A + Zicsr + Zifencei。
- **2级流水线**: IF/EX流水线，含分支预测（BHT）。
- **MMU与Sv32分页**: TLB支持，页表遍历器，可启动Linux内核。
- **L1缓存**: 可配置组相联L1指令/数据缓存，支持bypass模式。
- **陷阱处理**: 机器态/监管态陷阱、中断、委托机制。
- **简单SoC系统**: Boot ROM、Flash、ITCM/DTCM、DDR、UART、GPIO、GTimer、ACLINT、PLIC。
- **C模型**: 精确到时钟周期的仿真，支持VCD波形和接口事务trace。
- **双UI**: Web UI Dashboard（xterm.js终端 + GPIO面板）和 Terminal UI（vim风格命令模式）。
- **RTL实现**: SystemVerilog实现，接口由 `itfgen.py` 自动生成，支持VCS/Verilator/FPGA。
- **单元测试框架**: 模块化UT框架，彩色输出，计分板，接口trace验证。
- **Linux支持**: UART控制台、sysfs GPIO接口。
- **回归套件**: `run.sh` 全量SW用例 + UT回归，支持单用例运行。

## 快速开始

### 依赖项

- **仿真工具**: VCS 或 Verilator 仿真器。
- **RISC-V工具链**: RISC-V GCC 工具链，用于编译测试程序。详见 [RISC-V工具链GitHub仓库](https://github.com/riscv/riscv-gnu-toolchain)。
- **FPGA工具**: Xilinx Vivado（FPGA目标）。
- **Python（Web UI）**: Python 3 + aiohttp (`pip3 install aiohttp`)。

### 编译命令

| 命令 | 说明 |
|------|------|
| `./build.sh hw model` | 编译C模型仿真 |
| `./build.sh hw rtl vcs` | 编译RTL仿真（VCS） |
| `./build.sh hw rtl verilator` | 编译RTL仿真（Verilator） |
| `./build.sh hw fpga xilinx` | 生成Vivado FPGA工程 |
| `./build.sh sw` | 编译全部裸机C测试用例 |
| `./build.sh sw <名称>` | 编译单个C测试用例 |
| `./build.sh sw linux` | 编译Linux内核 + OpenSBI |
| `./build.sh sw linux clean` | 清理Linux内核 + OpenSBI编译 |
| `./build.sh ut model` | 编译全部C模型单元测试 |
| `./build.sh ut model <名称>` | 编译单个UT或UT子目录 |

*备注：FPGA需要在 `fpga/xilinx/boards` 下添加开发板描述文件，并在 `fpga/xilinx/proj.json` 中更新 `board` 项。*

### 运行仿真

**C模型:**
```bash
cd build/hw/model

# 终端模式:
./sim_top ../../sw/<用例>/<用例>.bin

# Web UI Dashboard（浏览器打开 http://localhost:5000）:
./sim_top --web-ui ../../sw/<用例>/<用例>.bin

# Linux 快速加载（预加载kernel/initrd/dtb至DDR）:
./sim_top --fast-load-linux --web-ui ../../sw/linux/linux.bin

# 禁用仿真结束检测（交互式使用）:
./sim_top --no-end-detect --web-ui ../../sw/<用例>/<用例>.bin
```

**Web UI Dashboard 功能:**
- xterm.js 完整终端仿真（ANSI彩色、方向键、粘贴、退格）
- 16 LED GPIO 输出面板 + 4 按钮 + 4 拨码开关
- Reset 按钮，服务端重启自动重连及历史回放
- 浅色/深色主题切换，偏好设置持久化

**Terminal UI 功能:**
- 原始模式交互式UART控制台
- `ESC` 进入vim风格命令模式：`p1`–`p4` 翻转按钮，`s1`–`s4` 翻转开关，`r` 复位，`i` 返回交互

**回归套件:**
```bash
./run.sh model sw              # 全部SW用例（批量）
./run.sh model sw <名称>       # 单个SW用例
./run.sh model ut              # 全部UT（批量）
./run.sh model ut <名称>       # 单个UT
./run.sh model                 # 全量 SW + UT 回归
```

**C模型单元测试（独立编译）:**
```bash
./build.sh ut model            # 编译全部UT
./run.sh model ut axi2bti      # 运行单个UT（自动cd到对应目录）
```

**RTL (VCS):**
```bash
cd build/hw/vcs
./sim_top +program=../../sw/<用例>/<用例>.hex
```

**RTL (Verilator):**
```bash
cd build/hw/verilator
./obj_dir/Vsim_top +program=../../sw/<用例>/<用例>.hex
```

## 项目结构

```
.
├── base/                    共享基础库（C模型 + RTL）
│   ├── model/base/          types, itf, fifo, 仲裁器
│   ├── model/dbg/           调试: vcd, log, pcm, chk
│   └── rtl/                 RTL基础
├── design/
│   ├── model/               C模型源码
│   │   ├── bus/             总线架构: bridge, mux, demux
│   │   ├── core/            CPU与外设
│   │   ├── itf/             接口定义
│   │   ├── mem/             存储: ram, rom, L1
│   │   ├── peri/            外设: uart, gpio, gtimer
│   │   └── spec/            ISA, CSR, SoC配置
│   └── rtl/                 RTL源码（SystemVerilog）
├── sim/
│   ├── model/               C仿真顶层及UI
│   └── rtl/                 RTL仿真顶层
├── ut/model/                单元测试（镜像 design/model/）
├── cases/                   软件测试用例
├── sdk/                     软件SDK（CRT, 驱动）
├── tools/                   构建工具, Web UI, itfgen
├── thirdparty/              Linux内核, OpenSBI
└── build/                   构建产物
```

## 开发状态

- [x] C-Model RV32G (I/M/A/Zicsr/Zifencei) 指令集
- [x] C-Model MMU 含 Sv32 分页和 TLB
- [x] C-Model L1 指令/数据缓存，支持bypass
- [x] C-Model 启动 Linux 内核
- [x] C-Model 总线架构含 BTI/AXI4/APB 协议桥接、多路选择及地址路由
- [x] C-Model GPIO（24位双向，输出/输入/中断三种模式）
- [x] C-Model GTimer（通用递减计时器）
- [x] C-Model Web UI Dashboard（xterm.js, GPIO LED/按钮/开关, reset, 主题切换）
- [x] C-Model Terminal UI（vim风格cmd模式, ESC键复位）
- [x] C-Model 模块级单元测试框架
- [x] C-Model 回归套件（run.sh）
- [x] Linux RVBucket GPIO 驱动（gpio-rvbucket, sysfs接口）
- [ ] RTL RV32G 指令集
- [ ] RTL MMU 分页
- [ ] RTL L1 缓存
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

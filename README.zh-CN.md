# RVBucket: 一个32位RISC-V架构SoC的ASM及RTL实现

[English](README.md)

## 概述

RVBucket是一个开源的32位RISC-V处理器和MCU类SoC，同时提供精确到时钟周期的
ASM（Architecture Simulation Model，架构仿真模型）和可综合SystemVerilog RTL。
两种实现均可启动Linux，RTL还支持VCS、Verilator、ASIC综合和Xilinx FPGA平台。

## 系统规格

- **CPU核心**: 顺序执行的RV32IMA_Zicsr_Zifencei_Zicbom CPU，支持机器态、
  监管态和用户态；ASM和RTL均包含Sv32 MMU与TLB
- **L1 Cache**: ASM和RTL均实现16 KiB、2路组相联、64字节cache line的写回式
  L1指令缓存和数据缓存，支持bypass与Zicbom cache block操作
- **Boot ROM**: 4 KB
- **Flash**: 32 MB
- **DDR**: 可配置大小，支持预加载Linux镜像
- **UART**: 支持TX/RX、中断和Linux控制台
- **GPIO**: 24位双向GPIO，支持输出/输入/中断三种模式，Web UI模拟按钮和开关
- **GTimer**: 通用递减计时器，支持IRQ
- **SD-SPI Host**: ASM/RTL协议层控制器与AXI DMA，以及用于FPGA的SPI PHY
- **ACLINT**: 核局部中断控制器（mtime, mtimecmp, MSWI, SSWI）
- **PLIC**: 平台级中断控制器，接入UART、GPIO、GTimer和SD-SPI中断
- **总线架构**: BTI/AXI4/APB协议桥接、多路选择及地址路由

## 功能特性

- **RISC-V指令集**: 支持RV32I + M + A + Zicsr + Zifencei + Zicbom。
- **CPU微架构**: 顺序执行流水线，支持BHT分支预测。
- **MMU与Sv32分页**: ASM和RTL均支持TLB与硬件页表遍历。
- **L1缓存**: ASM和RTL均支持可配置组相联写回式L1指令/数据缓存，以及cached、
  bypass、flush和cache block管理路径。
- **非一致性DMA**: Linux通过标准Zicbom cache维护机制支持SD-SPI等外设与
  cacheable内存之间的DMA。
- **陷阱处理**: 机器态/监管态陷阱、中断、委托机制。
- **SoC系统**: Boot ROM、Flash、DDR、UART、GPIO、GTimer、SD-SPI、ACLINT、
  PLIC，以及独立的DMA I/O子系统。
- **ASM（架构仿真模型）**: 精确到时钟周期的仿真，支持VCD波形和接口事务trace。
- **双UI**: Web UI Dashboard（xterm.js终端 + GPIO面板）和 Terminal UI（vim风格命令模式）。
- **RTL实现**: SystemVerilog实现，接口由 `itfgen.py` 自动生成，支持VCS/Verilator/FPGA。
- **单元测试框架**: ASM和RTL模块级自动化UT，支持计分板、超时、彩色汇总和接口协议检查。
- **Linux支持**: OpenSBI启动、Sv32虚拟内存、UART控制台、GPIO、MMC块设备，
  以及已在ASM、VCS和FPGA验证的用户态测试集。
- **Linux存储**: 自研MMC host驱动，通过SD卡上的ext4文件系统提供rootfs。
- **启动介质**: bootloader可选择从QSPI或SD加载内核；无论从哪种介质启动，
  均可挂载SD卡上的ext4 rootfs。
- **回归套件**: `run.sh` 全量SW用例 + UT回归，支持单用例运行。

## 快速开始

### 依赖项

- **仿真工具**: VCS 或 Verilator 仿真器。
- **RISC-V工具链**: RISC-V GCC 工具链，用于编译测试程序。详见 [RISC-V工具链GitHub仓库](https://github.com/riscv/riscv-gnu-toolchain)。
- **FPGA工具**: Xilinx Vivado（FPGA目标）。
- **Python（Web UI）**: Python 3 + aiohttp (`pip3 install aiohttp`)。
- **SD镜像工具**: util-linux中的`sfdisk`和e2fsprogs中的`mke2fs`。

### 编译命令

| 命令 | 说明 |
|------|------|
| `./build.sh hw model` | 编译ASM仿真 |
| `./build.sh hw rtl vcs` | 编译RTL仿真（VCS） |
| `./build.sh hw rtl vcs debug` | 编译RTL仿真（VCS debug + FSDB） |
| `./build.sh hw rtl verilator` | 编译RTL仿真（Verilator） |
| `./build.sh hw fpga xilinx <project\|ip\|syn\|impl\|bit> [配置]` | 执行指定的Vivado FPGA流程 |
| `./build.sh hw asic syn <配置>` | 使用指定配置执行ASIC综合 |
| `./build.sh sw` | 编译全部仿真裸机C测试用例，输出到 `build/sw/sim` |
| `./build.sh sw sim <名称>` | 编译单个仿真C测试用例 |
| `./build.sh sw board` | 编译全部真机裸机C测试用例，输出到 `build/sw/board` |
| `./build.sh sw board <名称>` | 编译单个真机C测试用例 |
| `./build.sh sw sim opensbi` / `./build.sh sw board opensbi` | 编译独立OpenSBI用例；当前两个profile共享同一份产物 |
| `./build.sh sw sim linux` / `./build.sh sw board linux` | 编译Linux内核 + OpenSBI；当前两个profile共享同一份产物 |
| `./build.sh sw sim linux clean` / `./build.sh sw board linux clean` | 清理Linux内核 + OpenSBI编译 |
| `./build.sh ut model` | 编译全部ASM单元测试 |
| `./build.sh ut model <名称>` | 编译单个UT或UT子目录 |
| `./build.sh ut rtl` | 使用VCS编译全部RTL单元测试 |
| `./build.sh ut rtl <名称>` | 编译单个RTL UT或UT子目录 |

*备注：FPGA需要在 `fpga/xilinx/boards` 下添加开发板描述文件，并在 `fpga/xilinx/proj.json` 中更新 `board` 项。*

### 运行仿真

**ASM:**
```bash
cd build/hw/model

# 终端模式:
./sim_top ../../sw/sim/<用例>/<用例>.bin

# Web UI Dashboard（浏览器打开 http://localhost:5000）:
./sim_top --web-ui ../../sw/sim/<用例>/<用例>.bin

# Linux 快速加载（预加载kernel/initrd/dtb至DDR）:
./sim_top --fast-load-linux --web-ui ../../sw/linux/linux.bin

# 从QSPI启动，同时挂载SD卡rootfs:
./sim_top --boot qspi --sd-image <sd.img> <program.bin>

# 从SD卡第1分区加载内核镜像:
./sim_top --boot sdcard --sd-image <sd.img>

# 禁用仿真结束检测（交互式使用）:
./sim_top --no-end-detect --web-ui ../../sw/sim/<用例>/<用例>.bin
```

**SD Linux冷启动:**
```bash
./build.sh sw sim linux
./tools/mksdcard.sh build/sw/linux/linux.bin \
    thirdparty/busybox/rootfs build/sw/linux/sdcard.img

cd build/hw/model
./sim_top --boot-prog --boot sdcard \
    --sd-image ../../sw/linux/sdcard.img
```

生成的DOS分区镜像包含两个分区：第1个可启动裸分区（类型`0xda`）保存
`linux.bin`，第2个分区保存ext4 rootfs。Boot ROM采样GPIO 21选择QSPI或SD
作为内核镜像来源；ASM、RTL仿真和FPGA工程均默认从QSPI启动。Linux先进入
initramfs，再挂载`/dev/mmcblk0p2`并切换到ext4 rootfs。

SD-SPI在协议包接口处分层。`u_io`子系统承载具有DMA能力的设备，独立于仅有
APB外设的`u_peri`。SD-SPI controller提供APB寄存器和AXI DMA master；
`sim/model/vip`中的ASM VIP以及`sim/rtl/common`中的VCS/Verilator共享DPI后端
提供基于真实文件的SD卡。FPGA通过`design/rtl/io/sdspi_phy.sv`将相同的
`sdspi_cmd`/`sdspi_data`包接口转换为SPI物理信号。

**Web UI Dashboard 功能:**
- xterm.js 完整终端仿真（ANSI彩色、方向键、粘贴、退格）
- 16 LED GPIO 输出面板 + 4 按钮 + 4 拨码开关
- Reset 按钮，服务端重启自动重连及历史回放
- 浅色/深色主题切换，偏好设置持久化

**Terminal UI 功能:**
- 原始模式交互式UART控制台
- 连续按两次`Ctrl-\`进入vim风格命令模式：`p1`–`p4`翻转按钮，
  `s1`–`s4`翻转开关，`r`复位，`i`返回交互

**回归套件:**
```bash
./run.sh model sw              # 全部SW用例（批量）
./run.sh model sw <名称>       # 单个SW用例
./run.sh model ut              # 全部UT（批量）
./run.sh model ut <名称>       # 单个UT
./run.sh model                 # 全量 SW + UT 回归
./run.sh rtl ut                # 全部RTL模块UT
./run.sh rtl ut <名称>         # 单个RTL模块UT
./run.sh rtl vcs sw            # 使用VCS运行全部RTL裸机用例
./run.sh rtl verilator sw      # 使用Verilator运行全部RTL裸机用例
```

**单元测试:**
```bash
./build.sh ut model            # 编译全部UT
./run.sh model ut axi2bti      # 运行单个ASM UT
./build.sh ut rtl              # 编译全部RTL UT
./run.sh rtl ut io/sdspi_phy   # 运行单个RTL UT
```

**RTL (VCS):**
```bash
cd build/hw/vcs
./sim_top +program=../../sw/sim/<用例>/<用例>.hex

# 从SD启动；+boot_prog独立控制bootloader进度输出。
./sim_top +boot=sdcard +boot_prog \
    +sd_image=../../sw/linux/sdcard.img
```

**RTL (Verilator):**
```bash
cd build/hw/verilator
./obj_dir/Vsim_top +program=../../sw/sim/<用例>/<用例>.hex

# 从QSPI启动，同时挂载SD数据盘/rootfs。
./obj_dir/Vsim_top +boot=qspi \
    +program=../../sw/linux/linux.hex \
    +sd_image=../../sw/linux/sdcard.img
```

RTL与ASM使用相同的启动语义：`qspi`是默认值并要求提供`+program`；`sdcard`
要求提供`+sd_image`；`+fast_load_linux`仅适用于QSPI启动。`+boot_prog`控制
GPIO 20，`+boot=sdcard`控制GPIO 21。

## 项目结构

```
.
├── base/                    共享基础库（ASM + RTL）
│   ├── model/base/          types, itf, fifo, 仲裁器
│   ├── model/dbg/           调试: vcd, log, pcm, chk
│   └── rtl/                 RTL基础
├── design/
│   ├── model/               ASM源码
│   │   ├── bus/             总线架构: bridge, mux, demux
│   │   ├── core/            CPU核心与平台控制器
│   │   ├── io/              支持DMA的I/O控制器
│   │   ├── itf/             接口定义
│   │   ├── mem/             存储模型
│   │   ├── peri/            外设: uart, gpio, gtimer
│   │   └── spec/            ISA, CSR, SoC配置
│   └── rtl/                 与ASM层次对应的可综合SystemVerilog
├── sim/
│   ├── model/               ASM仿真顶层、UI及设备VIP
│   └── rtl/                 VCS/Verilator顶层、模型、VIP及DPI后端
├── ut/
│   ├── model/               ASM模块级单元测试
│   └── rtl/                 RTL模块级单元测试
├── cases/
│   ├── bare/                裸机测试和bootloader
│   └── user/                Linux用户态测试和init脚本
├── sdk/                     软件SDK（CRT, 驱动）
├── fpga/xilinx/             板卡描述、FPGA RTL及Vivado配置
├── tools/                   构建/镜像工具、Web UI及接口生成器
├── thirdparty/              Linux内核、OpenSBI及BusyBox
└── build/                   构建产物
```

## 开发状态

- [x] ASM和RTL实现RV32IMA_Zicsr_Zifencei_Zicbom指令集
- [x] ASM和RTL实现带TLB及硬件页表遍历的Sv32 MMU
- [x] ASM和RTL实现写回式L1指令/数据缓存、bypass及Zicbom CMO
- [x] ASM和RTL实现BTI/AXI4/APB总线及支持DMA的I/O子系统
- [x] UART、GPIO、GTimer、ACLINT、PLIC及完整中断通路
- [x] SD-SPI AXI DMA host、协议VIP、可综合SPI PHY及Linux MMC驱动
- [x] QSPI/SD启动选择及SD ext4 rootfs切换
- [x] ASM、VCS及Xilinx FPGA均跑通Linux和用户态回归
- [x] ASM Web UI Dashboard和交互式Terminal UI
- [x] ASM/RTL模块级UT及裸机自动化回归框架
- [x] Linux RVBucket UART、GPIO及MMC host驱动
- [ ] ASM/RTL F/D 指令集扩展

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

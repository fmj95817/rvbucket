# RVBucket FPGA Flow

The FPGA flow is configuration driven and currently targets the custom
Kintex-7 board described by `fpga/xilinx/boards/k7ecor2p.json`.

## Commands

Generate the Vivado project and available IP output products:

```sh
./build.sh hw fpga xilinx ip
```

Run synthesis, implementation, or bitstream generation:

```sh
./build.sh hw fpga xilinx syn
./build.sh hw fpga xilinx impl
./build.sh hw fpga xilinx bit
```

Generated files are placed under `build/hw/fpga/xilinx`.

Generate a QSPI flash image for Linux:

```sh
./build.sh sw board linux
./build.sh hw fpga xilinx qspi
```

This writes `build/hw/fpga/xilinx/linux_qspi.mcs` from
`build/sw/linux/linux.bin`. The `.hex` image is for simulation `$readmemh`
flows, not for programming the board QSPI flash. To program the QSPI flash
through Vivado hardware manager:

```sh
./build.sh hw fpga xilinx qspi-program
```

## Architecture

The reusable RTL top is `design/rtl/soc.sv`. FPGA-only integration lives under
`fpga/xilinx/rtl`:

- `fpga_top.sv` instantiates `soc`, the DDR wrapper, and the QSPI wrapper.
- `xilinx_axi_ddr.sv` adapts the `soc` DDR AXI port to a Xilinx 7-series MIG
  AXI DDR3 controller.
- `xilinx_axi_qspi.sv` adapts the `soc` flash AXI port to Xilinx AXI Quad SPI
  in XIP mode.

The Linux image is expected to live in QSPI flash. The bitstream is expected to
be loaded through JTAG.

The SoC UART baud divider is not a hardware parameter in the FPGA top. The
FPGA bootloader programs the UART BC register in software. `RVB_SIM` builds
use a fast divider for simulation; non-simulation builds use `867` as the
default for the current MIG `ui_clk` domain.

## IP Configuration

`fpga/xilinx/proj.json` controls Vivado project generation and IP parameters.

QSPI is generated automatically as `rvbucket_qspi` with:

- AXI4 full interface
- XIP mode enabled
- quad SPI mode
- 32-bit XIP flash address for the 32MiB W25Q256FV device
- STARTUPE2 path enabled for the configuration flash CCLK pin

DDR uses `mig_7series` as `rvbucket_ddr`. The flow generates the MIG XML/PRJ
file into the build directory from `fpga/xilinx/proj.json` and the DDR3 pin map
in `fpga/xilinx/boards/k7ecor2p.json`. The default configuration is DDR3 x32
using two `MT41J256M16` devices and a 32-bit AXI slave port.

The MIG clocking and DDR pins still need board-level review against the final
schematic before loading hardware.

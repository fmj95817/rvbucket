#!/usr/bin/env python3

import argparse
import os
import subprocess
import sys
import tempfile


def parse_int(value):
    return int(value, 0)


def format_cell(value):
    return f"0x{value:08x}"


def render_dts(args):
    initrd_end = args.initrd_load + args.initrd_size
    reserved_memory = ""
    ufs_node = ""

    if args.ufs:
        reserved_memory = """
\treserved-memory {
\t\t#address-cells = <1>;
\t\t#size-cells = <1>;
\t\tranges;

\t\tufs_dma_pool: ufs-dma-pool@4f000000 {
\t\t\tcompatible = "shared-dma-pool";
\t\t\tno-map;
\t\t\treg = <0x4f000000 0x01000000>;
\t\t};
\t};
"""
        ufs_node = """

\t\tufshc0: ufs@30003000 {
\t\t\tcompatible = "rvbucket,ufshc";
\t\t\treg = <0x30003000 0x2000>;
\t\t\tinterrupts = <4>;
\t\t\tlanes-per-direction = <1>;
\t\t\tdma-noncoherent;
\t\t\tmemory-region = <&ufs_dma_pool>;
\t\t};
"""

    return f"""// SPDX-License-Identifier: GPL-2.0
/dts-v1/;

/ {{
\t#address-cells = <1>;
\t#size-cells = <1>;
\tcompatible = "rvbucket,rvbucket";
\tmodel = "RVBucket";

\tchosen {{
\t\tbootargs = "root=/dev/ram rw console=ttyRVB0 earlycon=sbi loglevel=8";
\t\tstdout-path = "serial0:115200n8";
\t\tlinux,initrd-start = <{format_cell(args.initrd_load)}>;
\t\tlinux,initrd-end = <{format_cell(initrd_end)}>;
\t}};

\taliases {{
\t\tserial0 = &uart0;
\t}};

\tcpus {{
\t\t#address-cells = <1>;
\t\t#size-cells = <0>;
\t\ttimebase-frequency = <10000>;

\t\tcpu0: cpu@0 {{
\t\t\tdevice_type = "cpu";
\t\t\treg = <0>;
\t\t\tcompatible = "riscv";
\t\t\triscv,isa = "rv32ima_zicsr_zifencei_zicbom";
\t\t\triscv,isa-base = "rv32i";
\t\t\triscv,isa-extensions = "i", "m", "a", "zicsr", "zifencei", "zicbom";
\t\t\triscv,cbom-block-size = <64>;
\t\t\tmmu-type = "riscv,sv32";

\t\t\tcpu0_intc: interrupt-controller {{
\t\t\t\t#interrupt-cells = <1>;
\t\t\t\tcompatible = "riscv,cpu-intc";
\t\t\t\tinterrupt-controller;
\t\t\t}};
\t\t}};
\t}};

\tmemory@40000000 {{
\t\tdevice_type = "memory";
\t\treg = <0x40000000 0x10000000>;
\t}};
{reserved_memory}
\tsoc {{
\t\t#address-cells = <1>;
\t\t#size-cells = <1>;
\t\tcompatible = "simple-bus";
\t\tranges;
\t\tinterrupt-parent = <&plic>;

\t\tuart0: serial@30000000 {{
\t\t\tcompatible = "rvbucket,uart";
\t\t\treg = <0x30000000 0x10>;
\t\t\tinterrupts = <1>;
\t\t\tcurrent-speed = <115200>;
\t\t}};

\t\tgpio0: gpio@30001000 {{
\t\t\tcompatible = "rvbucket,gpio";
\t\t\treg = <0x30001000 0x10>;
\t\t\tinterrupts = <2>;
\t\t\tgpio-controller;
\t\t\t#gpio-cells = <2>;
\t\t\tinterrupt-controller;
\t\t\t#interrupt-cells = <2>;
\t\t}};{ufs_node}

\t\tplic: interrupt-controller@31100000 {{
\t\t\tcompatible = "sifive,plic-1.0.0";
\t\t\treg = <0x31100000 0x400000>;
\t\t\t#interrupt-cells = <1>;
\t\t\tinterrupt-controller;
\t\t\triscv,ndev = <32>;
\t\t\tinterrupts-extended = <&cpu0_intc 9>;
\t\t}};
\t}};
}};
"""


def main():
    parser = argparse.ArgumentParser(description="Generate RVBucket DTS/DTB")
    parser.add_argument("--out-dtb", required=True)
    parser.add_argument("--out-dts")
    parser.add_argument("--initrd-load", type=parse_int, default=0x45000000)
    parser.add_argument("--initrd-size", type=parse_int)
    parser.add_argument("--initrd")
    parser.add_argument("--ufs", action="store_true")
    parser.add_argument("--dtc", default="dtc")
    args = parser.parse_args()

    if args.initrd_size is None:
        if args.initrd is None:
            parser.error("one of --initrd-size or --initrd is required")
        args.initrd_size = os.path.getsize(args.initrd)

    out_dts = args.out_dts
    remove_dts = False
    if out_dts is None:
        fd, out_dts = tempfile.mkstemp(prefix="rvbucket-", suffix=".dts")
        os.close(fd)
        remove_dts = True

    os.makedirs(os.path.dirname(os.path.abspath(args.out_dtb)), exist_ok=True)
    os.makedirs(os.path.dirname(os.path.abspath(out_dts)), exist_ok=True)

    with open(out_dts, "w", encoding="ascii") as fp:
        fp.write(render_dts(args))

    cmd = [
        args.dtc,
        "-I", "dts",
        "-O", "dtb",
        "-o", args.out_dtb,
        "-Wno-unique_unit_address",
        "-Wno-unit_address_vs_reg",
        "-Wno-avoid_unnecessary_addr_size",
        "-Wno-alias_paths",
        "-Wno-interrupt_provider",
        out_dts,
    ]
    try:
        subprocess.run(cmd, check=True)
    finally:
        if remove_dts:
            try:
                os.unlink(out_dts)
            except FileNotFoundError:
                pass

    return 0


if __name__ == "__main__":
    sys.exit(main())

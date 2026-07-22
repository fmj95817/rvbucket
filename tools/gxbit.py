#!/usr/bin/env python3

import argparse
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess


def read_json(path):
    with open(path, "r", encoding="utf-8") as fp:
        return json.load(fp)


def tcl_quote(value):
    return "{" + str(value).replace("}", "\\}") + "}"


def normalize_hw_server(host):
    if host.startswith("TCP:"):
        host = host[4:]
    if ":" in host:
        return host
    return f"{host}:3121"


def default_hw_filter(conf, board):
    qspi_filter = board.get("qspi_flash", {}).get("hw_device_filter", "")
    if qspi_filter:
        return qspi_filter

    device = board.get("device", "")
    if device.startswith("xc"):
        for sep in ("ff", "fg", "fb", "sf", "cs", "cl"):
            pos = device.find(sep)
            if pos > 0:
                return device[:pos] + "*"

    return conf.get("hw_device_filter", "*")


def gen_program_tcl(bit, hw_device_filter, hw_server_url):
    return f"""set bit_file {tcl_quote(bit)}

if {{![file exists $bit_file]}} {{
    error "FPGA bitstream not found: $bit_file"
}}

open_hw_manager
connect_hw_server -url {hw_server_url}
open_hw_target

set hw_devices [get_hw_devices {hw_device_filter}]
if {{[llength $hw_devices] == 0}} {{
    set hw_devices [get_hw_devices]
}}
if {{[llength $hw_devices] == 0}} {{
    error "No FPGA hardware device found"
}}

set dev [lindex $hw_devices 0]
current_hw_device $dev
refresh_hw_device $dev
set_property PROGRAM.FILE $bit_file $dev
program_hw_devices $dev
refresh_hw_device $dev
close_hw_manager
exit
"""


def run_vivado(vivado, tcl, cwd):
    if shutil.which(vivado):
        subprocess.run(
            [vivado, "-mode", "batch", "-source", str(tcl)],
            cwd=str(cwd),
            check=True,
        )
        return

    edaenv = "/etc/profile.d/edaenv.sh"
    if os.path.exists(edaenv):
        cmd = (
            f"source {shlex.quote(edaenv)}; "
            f"{shlex.quote(vivado)} -mode batch -source {shlex.quote(str(tcl))}"
        )
        subprocess.run(["bash", "-lc", cmd], cwd=str(cwd), check=True)
        return

    raise RuntimeError(f"Vivado not found: {vivado}")


def main():
    parser = argparse.ArgumentParser(description="Program a Xilinx FPGA bitstream.")
    parser.add_argument(
        "host",
        nargs="?",
        default="localhost",
        help="Vivado hw_server host or host:port, default: localhost",
    )
    parser.add_argument("--conf", default="fpga/xilinx/proj.json")
    parser.add_argument(
        "--bit",
        default="build/hw/fpga/xilinx/rvbucket_k7ecor2p.runs/impl_1/fpga_top.bit",
    )
    parser.add_argument("--out-dir", default="build/hw/fpga/xilinx")
    parser.add_argument("--hw-device-filter", default="")
    parser.add_argument("--vivado", default="vivado")
    parser.add_argument("--no-run", action="store_true")
    args = parser.parse_args()

    root = Path.cwd().resolve()
    conf = read_json(root / args.conf)
    board = read_json(root / conf["board"])

    bit = (root / args.bit).resolve()
    out_dir = (root / args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    tcl = out_dir / "program_bit.tcl"

    if not bit.exists():
        raise FileNotFoundError(f"FPGA bitstream not found: {bit}")

    hw_device_filter = args.hw_device_filter or default_hw_filter(conf, board)
    hw_server_url = normalize_hw_server(args.host)

    tcl.write_text(
        gen_program_tcl(bit, hw_device_filter, hw_server_url),
        encoding="utf-8",
    )

    print(f"Bitstream  : {bit}")
    print(f"Program Tcl: {tcl}")
    print(f"HW server  : {hw_server_url}")
    print(f"HW device  : {hw_device_filter}")

    if args.no_run:
        return

    run_vivado(args.vivado, tcl, out_dir)


if __name__ == "__main__":
    main()

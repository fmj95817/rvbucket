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


def parse_int(value):
    if isinstance(value, int):
        return value
    return int(str(value), 0)


def tcl_quote(path):
    return "{" + str(path).replace("}", "\\}") + "}"


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


def gen_mcs_tcl(image, mcs, interface, size_mib, offset):
    offset_hex = f"0x{offset:08x}"
    return f"""set image_file {tcl_quote(image)}
set mcs_file {tcl_quote(mcs)}

if {{![file exists $image_file]}} {{
    error "QSPI image not found: $image_file"
}}

set load_data [list up {offset_hex} $image_file]
write_cfgmem -force \\
    -format mcs \\
    -interface {interface} \\
    -size {size_mib} \\
    -loaddata $load_data \\
    -file $mcs_file
"""


def gen_program_tcl(mcs, cfgmem_filter, hw_device_filter, hw_server_url):
    connect_cmd = "connect_hw_server"
    if hw_server_url:
        connect_cmd += f" -url {hw_server_url}"

    return f"""set mcs_file {tcl_quote(mcs)}

if {{![file exists $mcs_file]}} {{
    error "QSPI MCS not found: $mcs_file"
}}

open_hw_manager
{connect_cmd}
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

set cfgmem_parts [get_cfgmem_parts {cfgmem_filter}]
if {{[llength $cfgmem_parts] == 0}} {{
    error "No cfgmem part matches filter: {cfgmem_filter}"
}}

create_hw_cfgmem -hw_device $dev [lindex $cfgmem_parts 0]
set cfgmem [current_hw_cfgmem]

set_property PROGRAM.FILES $mcs_file $cfgmem
set_property PROGRAM.ADDRESS_RANGE {{use_file}} $cfgmem
set_property PROGRAM.ERASE 1 $cfgmem
set_property PROGRAM.CFG_PROGRAM 1 $cfgmem
set_property PROGRAM.VERIFY 1 $cfgmem

program_hw_cfgmem $cfgmem
"""


def main():
    parser = argparse.ArgumentParser(
        description="Generate and optionally program a Xilinx QSPI Linux image."
    )
    parser.add_argument("--conf", default="fpga/xilinx/proj.json")
    parser.add_argument("--image", default="build/sw/linux/linux.bin")
    parser.add_argument("--out-dir", default="build/hw/fpga/xilinx")
    parser.add_argument("--mcs", default="")
    parser.add_argument("--offset", default="")
    parser.add_argument("--cfgmem-filter", default="")
    parser.add_argument("--hw-device-filter", default="")
    parser.add_argument("--hw-server-url", default="")
    parser.add_argument("--vivado", default="vivado")
    parser.add_argument("--no-run", action="store_true")
    parser.add_argument("--program", action="store_true")
    args = parser.parse_args()

    root = Path.cwd().resolve()
    conf = read_json(root / args.conf)
    board = read_json(root / conf["board"])
    qspi = conf.get("ip", {}).get("qspi", {})
    board_qspi = board.get("qspi_flash", {})

    image = (root / args.image).resolve()
    out_dir = (root / args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    mcs = (root / args.mcs).resolve() if args.mcs else out_dir / "linux_qspi.mcs"
    make_tcl = out_dir / "make_qspi_mcs.tcl"
    program_tcl = out_dir / "program_qspi.tcl"

    size_mib = parse_int(board_qspi.get("size_mib", 32))
    interface = board_qspi.get("interface", "SPIx4")
    offset = parse_int(args.offset) if args.offset else parse_int(board_qspi.get("offset", 0))
    cfgmem_filter = args.cfgmem_filter or board_qspi.get(
        "vivado_part_filter", "*w25q256*"
    )
    hw_device_filter = args.hw_device_filter or board_qspi.get(
        "hw_device_filter", "xc7k325t*"
    )
    hw_server_url = args.hw_server_url or board_qspi.get("hw_server_url", "")

    if not image.exists():
        raise FileNotFoundError(
            f"QSPI image not found: {image}. Run './build.sh sw linux' first."
        )

    image_size = image.stat().st_size
    flash_size = size_mib * 1024 * 1024
    if offset + image_size > flash_size:
        raise RuntimeError(
            f"image does not fit QSPI flash: image={image_size} bytes, "
            f"offset={offset}, flash={flash_size} bytes"
        )

    addr_bits = parse_int(qspi.get("addr_bits", 0))
    flash_aw = parse_int(conf.get("params", {}).get("FLASH_AW", addr_bits))
    for name, bits in (("ip.qspi.addr_bits", addr_bits), ("params.FLASH_AW", flash_aw)):
        if bits and offset + image_size > (1 << bits):
            raise RuntimeError(
                f"image exceeds {name} capacity: required={offset + image_size} "
                f"bytes, capacity={1 << bits} bytes"
            )

    make_tcl.write_text(gen_mcs_tcl(image, mcs, interface, size_mib, offset), encoding="utf-8")
    program_tcl.write_text(
        gen_program_tcl(mcs, cfgmem_filter, hw_device_filter, hw_server_url),
        encoding="utf-8"
    )

    print(f"QSPI image : {image}")
    print(f"Image size : {image_size} bytes")
    print(f"Flash size : {flash_size} bytes")
    print(f"Offset     : 0x{offset:08x}")
    print(f"MCS        : {mcs}")
    print(f"Make Tcl   : {make_tcl}")
    print(f"Program Tcl: {program_tcl}")
    if hw_server_url:
        print(f"HW server  : {hw_server_url}")

    if args.no_run:
        return

    run_vivado(args.vivado, make_tcl, out_dir)
    if args.program:
        run_vivado(args.vivado, program_tcl, out_dir)


if __name__ == "__main__":
    main()

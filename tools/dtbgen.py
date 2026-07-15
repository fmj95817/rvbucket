#!/usr/bin/env python3

import argparse
import json
import os
import re
import subprocess
import sys
import tempfile


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_ARCH = os.path.normpath(os.path.join(SCRIPT_DIR, "..", "desc", "arch.json"))
TARGETS = ("model", "rtl", "fpga", "asic")
DEVICE_KINDS = ("uart", "gpio", "ufshc")


def parse_int(value):
    return int(value, 0)


def parse_u32(value, field):
    if isinstance(value, bool):
        raise ValueError(f"{field} must be an integer")
    try:
        number = int(value, 0) if isinstance(value, str) else int(value)
    except (TypeError, ValueError) as exc:
        raise ValueError(f"{field} must be an integer") from exc
    if number < 0 or number > 0xFFFFFFFF:
        raise ValueError(f"{field} is outside the 32-bit range")
    return number


def require(obj, key, context):
    if not isinstance(obj, dict):
        raise ValueError(f"{context} must be an object")
    if key not in obj:
        raise ValueError(f"{context} is missing {key}")
    return obj[key]


def require_string(obj, key, context):
    value = require(obj, key, context)
    if not isinstance(value, str) or not value:
        raise ValueError(f"{context}.{key} must be a non-empty string")
    return value


def require_bool(obj, key, context, default=None):
    value = obj.get(key, default)
    if not isinstance(value, bool):
        raise ValueError(f"{context}.{key} must be a boolean")
    return value


def format_cell(value):
    if value < 0 or value > 0xFFFFFFFF:
        raise ValueError(f"cell value is outside the 32-bit range: {value}")
    return f"0x{value:08x}"


def quote_string(value):
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def load_arch(path):
    with open(path, "r", encoding="utf-8") as fp:
        arch = json.load(fp)
    if arch.get("schema_version") != 1:
        raise ValueError("unsupported or missing arch schema_version")
    return arch


def validate_name(name, field):
    if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name) is None:
        raise ValueError(f"{field} must be an identifier")


def validate_arch(arch):
    system = require(arch, "system", "arch")
    vendor = require_string(system, "vendor", "system")
    validate_name(vendor, "system.vendor")
    system_name = require_string(system, "name", "system")
    validate_name(system_name, "system.name")
    require_string(system, "model", "system")
    if parse_u32(require(system, "address_bits", "system"), "system.address_bits") != 32:
        raise ValueError("only 32-bit architectures are currently supported")

    boot = require(arch, "boot", "arch")
    require_string(boot, "args", "boot")
    console = require_string(boot, "console", "boot")
    require_string(boot, "console_options", "boot")

    cpu = require(arch, "cpu", "arch")
    hart_count = parse_u32(require(cpu, "hart_count", "cpu"), "cpu.hart_count")
    if hart_count == 0:
        raise ValueError("cpu.hart_count must be greater than zero")
    parse_u32(require(cpu, "timebase_frequency", "cpu"), "cpu.timebase_frequency")
    isa = require_string(cpu, "isa", "cpu")
    if re.fullmatch(r"rv32i[A-Za-z0-9_]*", isa) is None:
        raise ValueError("cpu.isa is not a valid RISC-V ISA string")
    extensions = require(cpu, "extensions", "cpu")
    if not isinstance(extensions, list) or not extensions or not all(
        isinstance(item, str) and item for item in extensions
    ):
        raise ValueError("cpu.extensions must be a non-empty string list")
    require_string(cpu, "mmu", "cpu")
    parse_u32(require(cpu, "cbom_block_size", "cpu"), "cpu.cbom_block_size")

    memory = require(arch, "memory", "arch")
    if not isinstance(memory, list) or not memory:
        raise ValueError("arch.memory must be a non-empty list")
    memory_names = set()
    for index, region in enumerate(memory):
        context = f"memory[{index}]"
        name = require_string(region, "name", context)
        validate_name(name, f"{context}.name")
        if name in memory_names:
            raise ValueError(f"duplicate memory name: {name}")
        memory_names.add(name)
        parse_u32(require(region, "base", context), f"{context}.base")
        size = parse_u32(require(region, "size", context), f"{context}.size")
        if size == 0:
            raise ValueError(f"{context}.size must be greater than zero")

    intc = require(arch, "interrupt_controller", "arch")
    if require_string(intc, "kind", "interrupt_controller") != "plic":
        raise ValueError("only a PLIC interrupt controller is currently supported")
    parse_u32(require(intc, "base", "interrupt_controller"), "interrupt_controller.base")
    intc_size = parse_u32(
        require(intc, "size", "interrupt_controller"), "interrupt_controller.size"
    )
    if intc_size == 0:
        raise ValueError("interrupt_controller.size must be greater than zero")
    source_count = parse_u32(
        require(intc, "source_count", "interrupt_controller"),
        "interrupt_controller.source_count",
    )
    if source_count == 0:
        raise ValueError("interrupt_controller.source_count must be greater than zero")
    parse_u32(
        require(intc, "supervisor_irq", "interrupt_controller"),
        "interrupt_controller.supervisor_irq",
    )

    devices = require(arch, "devices", "arch")
    if not isinstance(devices, list):
        raise ValueError("arch.devices must be a list")
    device_names = set()
    features = set()
    for index, device in enumerate(devices):
        context = f"devices[{index}]"
        name = require_string(device, "name", context)
        validate_name(name, f"{context}.name")
        if name in device_names:
            raise ValueError(f"duplicate device name: {name}")
        device_names.add(name)
        kind = require_string(device, "kind", context)
        if kind not in DEVICE_KINDS:
            raise ValueError(f"{context}.kind is unsupported: {kind}")
        parse_u32(require(device, "base", context), f"{context}.base")
        size = parse_u32(require(device, "size", context), f"{context}.size")
        if size == 0:
            raise ValueError(f"{context}.size must be greater than zero")
        irq = parse_u32(require(device, "irq", context), f"{context}.irq")
        if irq == 0 or irq > source_count:
            raise ValueError(f"{context}.irq must select a valid PLIC source")
        require_bool(device, "model_only", context, False)

        feature = device.get("feature")
        if feature is not None:
            if not isinstance(feature, str) or not feature:
                raise ValueError(f"{context}.feature must be a non-empty string")
            validate_name(feature, f"{context}.feature")
            features.add(feature)

        if kind == "uart":
            parse_u32(require(device, "baud_rate", context), f"{context}.baud_rate")
        elif kind == "gpio":
            parse_u32(require(device, "width", context), f"{context}.width")
        elif kind == "ufshc":
            parse_u32(
                require(device, "lanes_per_direction", context),
                f"{context}.lanes_per_direction",
            )
            require_bool(device, "dma_coherent", context)
            pool = require(device, "dma_pool", context)
            parse_u32(require(pool, "base", f"{context}.dma_pool"), f"{context}.dma_pool.base")
            pool_size = parse_u32(
                require(pool, "size", f"{context}.dma_pool"),
                f"{context}.dma_pool.size",
            )
            if pool_size == 0:
                raise ValueError(f"{context}.dma_pool.size must be greater than zero")

    if console not in device_names:
        raise ValueError(f"boot.console references unknown device: {console}")
    console_device = next(device for device in devices if device["name"] == console)
    if console_device["kind"] != "uart" or console_device.get("feature") is not None:
        raise ValueError("boot.console must reference an always-present UART")
    return features


def device_enabled(device, target, enabled_features):
    if device.get("model_only", False) and target != "model":
        return False
    feature = device.get("feature")
    return feature is None or feature in enabled_features


def render_cpu(lines, cpu):
    hart_count = parse_u32(cpu["hart_count"], "cpu.hart_count")
    isa = cpu["isa"]
    isa_base = isa[:5]
    extensions = ", ".join(quote_string(item) for item in cpu["extensions"])

    lines.extend([
        "\tcpus {",
        "\t\t#address-cells = <0x00000001>;",
        "\t\t#size-cells = <0x00000000>;",
        f"\t\ttimebase-frequency = <{format_cell(parse_u32(cpu['timebase_frequency'], 'cpu.timebase_frequency'))}>;",
    ])
    for hart_id in range(hart_count):
        lines.extend([
            "",
            f"\t\tcpu{hart_id}: cpu@{hart_id:x} {{",
            "\t\t\tdevice_type = \"cpu\";",
            f"\t\t\treg = <{format_cell(hart_id)}>;",
            "\t\t\tcompatible = \"riscv\";",
            f"\t\t\triscv,isa = {quote_string(isa)};",
            f"\t\t\triscv,isa-base = {quote_string(isa_base)};",
            f"\t\t\triscv,isa-extensions = {extensions};",
            f"\t\t\triscv,cbom-block-size = <{format_cell(parse_u32(cpu['cbom_block_size'], 'cpu.cbom_block_size'))}>;",
            f"\t\t\tmmu-type = {quote_string('riscv,' + cpu['mmu'])};",
            "",
            f"\t\t\tcpu{hart_id}_intc: interrupt-controller {{",
            "\t\t\t\t#interrupt-cells = <0x00000001>;",
            "\t\t\t\tcompatible = \"riscv,cpu-intc\";",
            "\t\t\t\tinterrupt-controller;",
            "\t\t\t};",
            "\t\t};",
        ])
    lines.append("\t};")


def render_memory(lines, regions):
    for region in regions:
        base = parse_u32(region["base"], f"memory.{region['name']}.base")
        size = parse_u32(region["size"], f"memory.{region['name']}.size")
        lines.extend([
            "",
            f"\tmemory@{base:x} {{",
            "\t\tdevice_type = \"memory\";",
            f"\t\treg = <{format_cell(base)} {format_cell(size)}>;",
            "\t};",
        ])


def dma_pool_label(device):
    return f"{device['name']}_dma_pool"


def render_dma_pools(lines, devices):
    dma_devices = [device for device in devices if device["kind"] == "ufshc"]
    if not dma_devices:
        return
    lines.extend([
        "",
        "\treserved-memory {",
        "\t\t#address-cells = <0x00000001>;",
        "\t\t#size-cells = <0x00000001>;",
        "\t\tranges;",
    ])
    for device in dma_devices:
        pool = device["dma_pool"]
        base = parse_u32(pool["base"], f"{device['name']}.dma_pool.base")
        size = parse_u32(pool["size"], f"{device['name']}.dma_pool.size")
        lines.extend([
            "",
            f"\t\t{dma_pool_label(device)}: ufs-dma-pool@{base:x} {{",
            "\t\t\tcompatible = \"shared-dma-pool\";",
            "\t\t\tno-map;",
            f"\t\t\treg = <{format_cell(base)} {format_cell(size)}>;",
            "\t\t};",
        ])
    lines.append("\t};")


def render_uart(lines, device):
    base = parse_u32(device["base"], f"{device['name']}.base")
    size = parse_u32(device["size"], f"{device['name']}.size")
    irq = parse_u32(device["irq"], f"{device['name']}.irq")
    baud = parse_u32(device["baud_rate"], f"{device['name']}.baud_rate")
    lines.extend([
        f"\t\t{device['name']}: serial@{base:x} {{",
        "\t\t\tcompatible = \"rvbucket,uart\";",
        f"\t\t\treg = <{format_cell(base)} {format_cell(size)}>;",
        f"\t\t\tinterrupts = <{format_cell(irq)}>;",
        f"\t\t\tcurrent-speed = <{format_cell(baud)}>;",
        "\t\t};",
    ])


def render_gpio(lines, device):
    base = parse_u32(device["base"], f"{device['name']}.base")
    size = parse_u32(device["size"], f"{device['name']}.size")
    irq = parse_u32(device["irq"], f"{device['name']}.irq")
    lines.extend([
        f"\t\t{device['name']}: gpio@{base:x} {{",
        "\t\t\tcompatible = \"rvbucket,gpio\";",
        f"\t\t\treg = <{format_cell(base)} {format_cell(size)}>;",
        f"\t\t\tinterrupts = <{format_cell(irq)}>;",
        "\t\t\tgpio-controller;",
        "\t\t\t#gpio-cells = <0x00000002>;",
        "\t\t\tinterrupt-controller;",
        "\t\t\t#interrupt-cells = <0x00000002>;",
        "\t\t};",
    ])


def render_ufshc(lines, device):
    base = parse_u32(device["base"], f"{device['name']}.base")
    size = parse_u32(device["size"], f"{device['name']}.size")
    irq = parse_u32(device["irq"], f"{device['name']}.irq")
    lanes = parse_u32(
        device["lanes_per_direction"], f"{device['name']}.lanes_per_direction"
    )
    lines.extend([
        f"\t\t{device['name']}: ufs@{base:x} {{",
        "\t\t\tcompatible = \"rvbucket,ufshc\";",
        f"\t\t\treg = <{format_cell(base)} {format_cell(size)}>;",
        f"\t\t\tinterrupts = <{format_cell(irq)}>;",
        f"\t\t\tlanes-per-direction = <{format_cell(lanes)}>;",
    ])
    if not device["dma_coherent"]:
        lines.append("\t\t\tdma-noncoherent;")
    lines.extend([
        f"\t\t\tmemory-region = <&{dma_pool_label(device)}>;",
        "\t\t};",
    ])


def render_plic(lines, intc, hart_count):
    base = parse_u32(intc["base"], "interrupt_controller.base")
    size = parse_u32(intc["size"], "interrupt_controller.size")
    source_count = parse_u32(intc["source_count"], "interrupt_controller.source_count")
    supervisor_irq = parse_u32(
        intc["supervisor_irq"], "interrupt_controller.supervisor_irq"
    )
    contexts = " ".join(
        f"&cpu{hart_id}_intc {format_cell(supervisor_irq)}"
        for hart_id in range(hart_count)
    )
    lines.extend([
        f"\t\tplic: interrupt-controller@{base:x} {{",
        "\t\t\tcompatible = \"sifive,plic-1.0.0\";",
        f"\t\t\treg = <{format_cell(base)} {format_cell(size)}>;",
        "\t\t\t#interrupt-cells = <0x00000001>;",
        "\t\t\tinterrupt-controller;",
        f"\t\t\triscv,ndev = <{format_cell(source_count)}>;",
        f"\t\t\tinterrupts-extended = <{contexts}>;",
        "\t\t};",
    ])


def render_soc(lines, arch, devices):
    lines.extend([
        "",
        "\tsoc {",
        "\t\t#address-cells = <0x00000001>;",
        "\t\t#size-cells = <0x00000001>;",
        "\t\tcompatible = \"simple-bus\";",
        "\t\tranges;",
        "\t\tinterrupt-parent = <&plic>;",
    ])
    renderers = {
        "uart": render_uart,
        "gpio": render_gpio,
        "ufshc": render_ufshc,
    }
    for device in devices:
        lines.append("")
        renderers[device["kind"]](lines, device)
    lines.append("")
    hart_count = parse_u32(arch["cpu"]["hart_count"], "cpu.hart_count")
    render_plic(lines, arch["interrupt_controller"], hart_count)
    lines.append("\t};")


def render_dts(arch, target, enabled_features, runtime, arch_path):
    available_features = validate_arch(arch)
    unknown = enabled_features.difference(available_features)
    if unknown:
        raise ValueError(f"unknown architecture feature(s): {', '.join(sorted(unknown))}")

    devices = [
        device
        for device in arch["devices"]
        if device_enabled(device, target, enabled_features)
    ]
    boot = arch["boot"]
    system = arch["system"]
    source = os.path.relpath(os.path.abspath(arch_path), SCRIPT_DIR)
    lines = [
        "// SPDX-License-Identifier: GPL-2.0",
        f"// Generated from {source}. Do not edit.",
        "/dts-v1/;",
        "",
        "/ {",
        "\t#address-cells = <0x00000001>;",
        "\t#size-cells = <0x00000001>;",
        f"\tcompatible = {quote_string(system['vendor'] + ',' + system['name'])};",
        f"\tmodel = {quote_string(system['model'])};",
        "",
        "\tchosen {",
        f"\t\tbootargs = {quote_string(boot['args'])};",
        f"\t\tstdout-path = {quote_string('serial0:' + boot['console_options'])};",
        f"\t\tlinux,initrd-start = <{format_cell(runtime['initrd_load'])}>;",
        f"\t\tlinux,initrd-end = <{format_cell(runtime['initrd_end'])}>;",
        "\t};",
        "",
        "\taliases {",
        f"\t\tserial0 = &{boot['console']};",
        "\t};",
        "",
    ]
    render_cpu(lines, arch["cpu"])
    render_memory(lines, arch["memory"])
    render_dma_pools(lines, devices)
    render_soc(lines, arch, devices)
    lines.append("};")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description="Generate RVBucket DTS/DTB")
    parser.add_argument("--arch", default=DEFAULT_ARCH)
    parser.add_argument("--target", choices=TARGETS, default="model")
    parser.add_argument("--enable", action="append", default=[], metavar="FEATURE")
    parser.add_argument("--out-dtb", required=True)
    parser.add_argument("--out-dts")
    parser.add_argument("--initrd-load", type=parse_int, default=0x45000000)
    parser.add_argument("--initrd-size", type=parse_int)
    parser.add_argument("--initrd")
    parser.add_argument("--ufs", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--dtc", default="dtc")
    args = parser.parse_args()

    if args.initrd_size is None:
        if args.initrd is None:
            parser.error("one of --initrd-size or --initrd is required")
        args.initrd_size = os.path.getsize(args.initrd)

    features = set(args.enable)
    if args.ufs:
        features.add("ufshc")
    runtime = {
        "initrd_load": args.initrd_load,
        "initrd_end": args.initrd_load + args.initrd_size,
    }
    try:
        arch = load_arch(args.arch)
        dts = render_dts(arch, args.target, features, runtime, args.arch)
    except (OSError, json.JSONDecodeError, ValueError) as exc:
        parser.error(str(exc))

    out_dts = args.out_dts
    remove_dts = False
    if out_dts is None:
        fd, out_dts = tempfile.mkstemp(prefix="rvbucket-", suffix=".dts")
        os.close(fd)
        remove_dts = True

    os.makedirs(os.path.dirname(os.path.abspath(args.out_dtb)), exist_ok=True)
    os.makedirs(os.path.dirname(os.path.abspath(out_dts)), exist_ok=True)
    with open(out_dts, "w", encoding="ascii") as fp:
        fp.write(dts)

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

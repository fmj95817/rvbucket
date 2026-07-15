#!/usr/bin/env python3

import argparse
import json
import os
from pathlib import Path


def q(s):
    return "{" + str(s) + "}"


def bool_int(v):
    return "1" if v else "0"


def read_json(path):
    with open(path, "r", encoding="utf-8") as fp:
        return json.load(fp)


def find_rtl_src(root, dirs, excludes=None):
    exclude_paths = set()
    for item in excludes or []:
        path = Path(item)
        if not path.is_absolute():
            path = root / path
        exclude_paths.add(path.resolve())

    sv_files = []
    for src_dir in dirs:
        path = root / src_dir
        if not path.exists():
            continue
        for file in path.rglob("*.sv"):
            if file.resolve() not in exclude_paths:
                sv_files.append(file)
    return sorted(sv_files)


def mig_pin_iostd(name):
    if "dq[" in name:
        return "SSTL15_T_DCI"
    if "dqs_" in name:
        return "DIFF_SSTL15_T_DCI"
    if "ck_" in name:
        return "DIFF_SSTL15"
    if "reset" in name:
        return "LVCMOS15"
    return "SSTL15"


def mig_pin_name(name):
    scalar_to_vector = {
        "ddr3_ck_p": "ddr3_ck_p[0]",
        "ddr3_ck_n": "ddr3_ck_n[0]",
        "ddr3_cke": "ddr3_cke[0]",
        "ddr3_cs_n": "ddr3_cs_n[0]",
        "ddr3_odt": "ddr3_odt[0]",
    }
    return scalar_to_vector.get(name, name)


def gen_mig_prj(proj, board, out_dir):
    ddr_ip = proj.get("ip", {}).get("ddr", {})
    ddr = board.get("ddr3", {})
    pins = ddr.get("pins", {})
    if not ddr_ip.get("generate_config", False):
        return None
    if not pins:
        raise RuntimeError("DDR MIG config generation requested but board.ddr3.pins is empty")

    data_width = int(ddr_ip.get("data_width", ddr.get("data_width", 32)))
    dqs_width = int(ddr_ip.get("dqs_width", data_width // 8))
    dm_width = int(ddr_ip.get("dm_width", data_width // 8))
    addr_width = int(ddr_ip.get("addr_width", 15))
    ba_width = int(ddr_ip.get("ba_width", 3))

    pin_order = []
    for prefix, count in (
        ("ddr3_addr", addr_width),
        ("ddr3_ba", ba_width),
        ("ddr3_dm", dm_width),
        ("ddr3_dq", data_width),
        ("ddr3_dqs_n", dqs_width),
        ("ddr3_dqs_p", dqs_width),
    ):
        pin_order += [f"{prefix}[{i}]" for i in range(count)]
    pin_order += [
        "ddr3_ras_n",
        "ddr3_cas_n",
        "ddr3_we_n",
        "ddr3_reset_n",
        "ddr3_ck_p",
        "ddr3_ck_n",
        "ddr3_cke",
        "ddr3_cs_n",
        "ddr3_odt",
    ]

    pin_lines = []
    missing = []
    for name in pin_order:
        if name not in pins:
            missing.append(name)
            continue
        mig_name = mig_pin_name(name)
        pin_lines.append(
            f'      <Pin IN_TERM="" IOSTANDARD="{mig_pin_iostd(mig_name)}" '
            f'PADName="{pins[name]}" SLEW="FAST" VCCAUX_IO="NORMAL" '
            f'name="{mig_name}"/>'
        )
    if missing:
        raise RuntimeError("DDR MIG pin map is missing: " + ", ".join(missing))

    timing = ddr_ip.get("timing", {})
    timing_defaults = {
        "tcke": "5",
        "tfaw": "40",
        "tras": "35",
        "trcd": "13.75",
        "trefi": "7.8",
        "trfc": "260",
        "trp": "13.75",
        "trrd": "7.5",
        "trtp": "7.5",
        "twtr": "7.5",
    }
    timing_defaults.update({k: str(v) for k, v in timing.items()})
    timing_attrs = " ".join(f'{k}="{v}"' for k, v in timing_defaults.items())

    module_name = ddr_ip.get("module_name", "rvbucket_ddr")
    path = out_dir / f"{module_name}.prj"
    text = f'''<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
<Project NoOfControllers="1">
  <ModuleName>{module_name}</ModuleName>
  <dci_inouts_inputs>1</dci_inouts_inputs>
  <dci_inputs>1</dci_inputs>
  <Debug_En>{ddr_ip.get("debug", "OFF")}</Debug_En>
  <DataDepth_En>{ddr_ip.get("data_depth", 1024)}</DataDepth_En>
  <LowPower_En>{ddr_ip.get("low_power", "ON")}</LowPower_En>
  <XADC_En>{ddr_ip.get("xadc", "Enabled")}</XADC_En>
  <TargetFPGA>{ddr_ip.get("target_fpga", ddr.get("target_fpga", "xc7k325t-ffg676/-2"))}</TargetFPGA>
  <Version>{ddr_ip.get("version", "4.2")}</Version>
  <SystemClock>{ddr_ip.get("system_clock", "No Buffer")}</SystemClock>
  <ReferenceClock>{ddr_ip.get("reference_clock", "Use System Clock")}</ReferenceClock>
  <SysResetPolarity>{ddr_ip.get("sys_reset_polarity", "ACTIVE LOW")}</SysResetPolarity>
  <BankSelectionFlag>FALSE</BankSelectionFlag>
  <InternalVref>0</InternalVref>
  <dci_hr_inouts_inputs>50 Ohms</dci_hr_inouts_inputs>
  <dci_cascade>1</dci_cascade>
  <Controller number="0">
    <MemoryDevice>{ddr_ip.get("memory_device", ddr.get("memory_device", "DDR3_SDRAM/Components/MT41J256M16XX-125"))}</MemoryDevice>
    <TimePeriod>{ddr_ip.get("time_period_ps", 2500)}</TimePeriod>
    <VccAuxIO>{ddr_ip.get("vcc_aux_io", "1.8V")}</VccAuxIO>
    <PHYRatio>{ddr_ip.get("phy_ratio", "4:1")}</PHYRatio>
    <InputClkFreq>{ddr_ip.get("input_clk_freq_mhz", 50)}</InputClkFreq>
    <UIExtraClocks>{ddr_ip.get("ui_extra_clocks", 1)}</UIExtraClocks>
    <MMCM_VCO>{ddr_ip.get("mmcm_vco", 800)}</MMCM_VCO>
    <MMCMClkOut0>{ddr_ip.get("mmcm_clkout0", "10.000")}</MMCMClkOut0>
    <MMCMClkOut1>1</MMCMClkOut1>
    <MMCMClkOut2>1</MMCMClkOut2>
    <MMCMClkOut3>1</MMCMClkOut3>
    <MMCMClkOut4>1</MMCMClkOut4>
    <DataWidth>{data_width}</DataWidth>
    <DeepMemory>1</DeepMemory>
    <DataMask>1</DataMask>
    <ECC>Disabled</ECC>
    <Ordering>Normal</Ordering>
    <BankMachineCnt>{ddr_ip.get("bank_machine_count", 4)}</BankMachineCnt>
    <CustomPart>FALSE</CustomPart>
    <NewPartName></NewPartName>
    <RowAddress>{addr_width}</RowAddress>
    <ColAddress>{ddr_ip.get("col_address", 10)}</ColAddress>
    <BankAddress>{ba_width}</BankAddress>
    <MemoryVoltage>{ddr_ip.get("memory_voltage", "1.5V")}</MemoryVoltage>
    <C0_MEM_SIZE>{ddr_ip.get("memory_size", 1073741824)}</C0_MEM_SIZE>
    <UserMemoryAddressMap>BANK_ROW_COLUMN</UserMemoryAddressMap>
    <PinSelection>
{chr(10).join(pin_lines)}
    </PinSelection>
    <System_Clock>
      <Pin Bank="Select Bank" PADName="No connect" name="sys_clk_i"/>
    </System_Clock>
    <System_Control>
      <Pin Bank="Select Bank" PADName="No connect" name="sys_rst"/>
      <Pin Bank="Select Bank" PADName="No connect" name="init_calib_complete"/>
      <Pin Bank="Select Bank" PADName="No connect" name="tg_compare_error"/>
    </System_Control>
    <TimingParameters>
      <Parameters {timing_attrs}/>
    </TimingParameters>
    <mrBurstLength name="Burst Length">8 - Fixed</mrBurstLength>
    <mrBurstType name="Read Burst Type and Length">Sequential</mrBurstType>
    <mrCasLatency name="CAS Latency">{ddr_ip.get("cas_latency", 6)}</mrCasLatency>
    <mrMode name="Mode">Normal</mrMode>
    <mrDllReset name="DLL Reset">No</mrDllReset>
    <mrPdMode name="DLL control for precharge PD">Slow Exit</mrPdMode>
    <emrDllEnable name="DLL Enable">Enable</emrDllEnable>
    <emrOutputDriveStrength name="Output Driver Impedance Control">RZQ/7</emrOutputDriveStrength>
    <emrMirrorSelection name="Address Mirroring">Disable</emrMirrorSelection>
    <emrCSSelection name="Controller Chip Select Pin">Enable</emrCSSelection>
    <emrRTT name="RTT (nominal) - On Die Termination (ODT)">RZQ/6</emrRTT>
    <emrPosted name="Additive Latency (AL)">0</emrPosted>
    <emrOCD name="Write Leveling Enable">Disabled</emrOCD>
    <emrDQS name="TDQS enable">Enabled</emrDQS>
    <emrRDQS name="Qoff">Output Buffer Enabled</emrRDQS>
    <mr2PartialArraySelfRefresh name="Partial-Array Self Refresh">Full Array</mr2PartialArraySelfRefresh>
    <mr2CasWriteLatency name="CAS write latency">{ddr_ip.get("cas_write_latency", 5)}</mr2CasWriteLatency>
    <mr2AutoSelfRefresh name="Auto Self Refresh">Enabled</mr2AutoSelfRefresh>
    <mr2SelfRefreshTempRange name="High Temparature Self Refresh Rate">Normal</mr2SelfRefreshTempRange>
    <mr2RTTWR name="RTT_WR - Dynamic On Die Termination (ODT)">Dynamic ODT off</mr2RTTWR>
    <PortInterface>AXI</PortInterface>
    <AXIParameters>
      <C0_C_RD_WR_ARB_ALGORITHM>RD_PRI_REG</C0_C_RD_WR_ARB_ALGORITHM>
      <C0_S_AXI_ADDR_WIDTH>{ddr_ip.get("axi_addr_width", 30)}</C0_S_AXI_ADDR_WIDTH>
      <C0_S_AXI_DATA_WIDTH>{ddr_ip.get("axi_data_width", 32)}</C0_S_AXI_DATA_WIDTH>
      <C0_S_AXI_ID_WIDTH>{ddr_ip.get("axi_id_width", 8)}</C0_S_AXI_ID_WIDTH>
      <C0_S_AXI_SUPPORTS_NARROW_BURST>1</C0_S_AXI_SUPPORTS_NARROW_BURST>
    </AXIParameters>
  </Controller>
</Project>
'''
    path.write_text(text, encoding="utf-8")
    return path


def port_pin(pin_cfg):
    if isinstance(pin_cfg, str):
        return pin_cfg
    return pin_cfg["pin"]


def port_iostd(pin_cfg, default_iostd):
    if isinstance(pin_cfg, str):
        return default_iostd
    return pin_cfg.get("iostandard", default_iostd)


def port_props(pin_cfg):
    if isinstance(pin_cfg, str):
        return {}
    return pin_cfg.get("props", {})


def gen_xdc(proj, board):
    lines = []
    default_iostd = board.get("iostd")
    pinout = board.get("pinout", {})

    for port, cfg in pinout.items():
        lines.append(f"set_property PACKAGE_PIN {port_pin(cfg)} [get_ports {q(port)}]")
        iostd = port_iostd(cfg, default_iostd)
        if iostd:
            lines.append(f"set_property IOSTANDARD {iostd} [get_ports {q(port)}]")
        for prop, val in port_props(cfg).items():
            lines.append(f"set_property {prop} {val} [get_ports {q(port)}]")

    for port, props in proj.get("props", {}).items():
        if isinstance(props, list):
            for prop in props:
                lines.append(f"set_property {prop} true [get_ports {q(port)}]")
        else:
            for prop, val in props.items():
                lines.append(f"set_property {prop} {val} [get_ports {q(port)}]")

    for clk in proj.get("clocks", []):
        if not clk.get("genclk", True):
            continue
        period = 1e9 / float(clk["freq"])
        lines.append(
            f"create_clock -period {period:.3f} -name {clk['name']} "
            f"[get_ports {q(clk['port'])}]"
        )

    for xdc in board.get("misc_xdc", []):
        lines.append(str(xdc))

    return "\n".join(lines) + "\n"


def gen_tcl(proj, board, root, out_dir, flow, mig_prj_path):
    name = proj["name"]
    xdc_name = f"{name}.xdc"
    top = proj["top_module"]
    part = board["device"]
    inc_dirs = [root / p for p in proj.get("inc_dir", [])]
    rtl_files = find_rtl_src(root, proj.get("rtl_dir", []),
                             proj.get("rtl_exclude", []))
    params = proj.get("params", {})
    ip = proj.get("ip", {})
    defines = ["SYNTHESIS"] + proj.get("defines", [])
    qspi_cfg = ip.get("qspi", {})
    if qspi_cfg.get("enabled", False) and int(qspi_cfg.get("spi_mode", 2)) != 0:
        defines.append("RVB_QSPI_HAS_IO23")
    jobs = int(proj.get("jobs", 8))
    max_threads = int(proj.get("max_threads", jobs))

    lines = []
    lines.append(f"set_param general.maxThreads {max_threads}")
    lines.append(f"set RVB_FLOW {flow}")
    lines.append(f"set RVB_ROOT {q(root)}")
    lines.append(f"create_project -force -part {part} {name} .")
    lines.append("set_property target_language Verilog [current_project]")
    lines.append("set_property simulator_language Mixed [current_project]")
    lines.append("")

    if rtl_files:
        lines.append("add_files -fileset sources_1 {")
        for path in rtl_files:
            lines.append(f"    {path}")
        lines.append("}")
        lines.append("")

    if inc_dirs:
        lines.append("set_property include_dirs {")
        for path in inc_dirs:
            lines.append(f"    {path}")
        lines.append("} [get_filesets sources_1]")
        lines.append("")

    if defines:
        lines.append("set_property verilog_define {")
        for define in defines:
            lines.append(f"    {define}")
        lines.append("} [get_filesets sources_1]")
        lines.append("")

    if params:
        lines.append("set_property generic {")
        for key, val in params.items():
            val = normalize_param_value(root, key, val)
            lines.append(f"    {key}={val}")
        lines.append("} [get_filesets sources_1]")
        lines.append("")

    lines.append(f"add_files -fileset constrs_1 {out_dir / xdc_name}")
    lines.append(f"set_property top {top} [get_filesets sources_1]")
    lines.append("")

    clk_wiz = ip.get("clk_wiz", {})
    if clk_wiz.get("enabled", False):
        mod = clk_wiz.get("module_name", "rvbucket_clk_wiz")
        input_freq = float(clk_wiz.get("input_freq_mhz", 50.0))
        reset_type = clk_wiz.get("reset_type", "ACTIVE_LOW")
        outputs = clk_wiz.get("outputs", [{"freq_mhz": 200.0}])

        lines.append(f"create_ip -name clk_wiz -vendor xilinx.com -library ip -module_name {mod}")
        lines.append("set_property -dict [list \\")
        lines.append(f"    CONFIG.PRIM_IN_FREQ {{{input_freq:.3f}}} \\")
        lines.append(f"    CONFIG.RESET_TYPE {{{reset_type}}} \\")
        lines.append("    CONFIG.USE_RESET {true} \\")
        lines.append("    CONFIG.USE_LOCKED {true} \\")
        for idx, output in enumerate(outputs, start=1):
            freq = float(output.get("freq_mhz", output.get("freq", 100.0)))
            if idx > 1:
                lines.append(f"    CONFIG.CLKOUT{idx}_USED {{true}} \\")
            lines.append(f"    CONFIG.CLKOUT{idx}_REQUESTED_OUT_FREQ {{{freq:.3f}}} \\")
        lines.append(f"] [get_ips {mod}]")
        lines.append(f"generate_target all [get_files {mod}.xci]")
        lines.append("")

    qspi = ip.get("qspi", {})
    if qspi.get("enabled", False):
        mod = qspi.get("module_name", "rvbucket_qspi")
        spi_mode = int(qspi.get("spi_mode", 2))
        spi_memory = int(qspi.get("spi_memory", 2))
        addr_bits = int(qspi.get("addr_bits", 24))
        id_width = int(qspi.get("axi_id_width", 8))
        ss_bits = int(qspi.get("num_ss_bits", 1))
        sck_ratio = int(qspi.get("sck_ratio", 8))
        use_startup = bool_int(qspi.get("use_startup", True))
        use_startup_int = bool_int(qspi.get("use_startup_int", True))
        xip_mode = bool_int(qspi.get("xip_mode", True))
        xip_perf_mode = bool_int(qspi.get("xip_perf_mode", True))

        lines.append(f"create_ip -name axi_quad_spi -vendor xilinx.com -library ip -module_name {mod}")
        lines.append("set_property -dict [list \\")
        lines.append(f"    CONFIG.C_USE_STARTUP {{{use_startup}}} \\")
        lines.append(f"    CONFIG.C_USE_STARTUP_INT {{{use_startup_int}}} \\")
        lines.append(f"    CONFIG.C_SPI_MODE {{{spi_mode}}} \\")
        lines.append(f"    CONFIG.C_SPI_MEMORY {{{spi_memory}}} \\")
        lines.append(f"    CONFIG.C_XIP_MODE {{{xip_mode}}} \\")
        lines.append(f"    CONFIG.C_XIP_PERF_MODE {{{xip_perf_mode}}} \\")
        lines.append("    CONFIG.C_TYPE_OF_AXI4_INTERFACE {1} \\")
        lines.append(f"    CONFIG.C_S_AXI4_ID_WIDTH {{{id_width}}} \\")
        lines.append(f"    CONFIG.C_SPI_MEM_ADDR_BITS {{{addr_bits}}} \\")
        lines.append(f"    CONFIG.C_NUM_SS_BITS {{{ss_bits}}} \\")
        lines.append(f"    CONFIG.C_SCK_RATIO1 {{{sck_ratio}}} \\")
        lines.append(f"] [get_ips {mod}]")
        lines.append(f"generate_target all [get_files {mod}.xci]")
        lines.append("")

    ddr = ip.get("ddr", {})
    ddr_required = "false"
    if ddr.get("enabled", False):
        mod = ddr.get("module_name", "rvbucket_ddr")
        cfg = ddr.get("config_file", "")
        cfg_path = mig_prj_path if mig_prj_path else (root / cfg if cfg else None)
        ddr_required = "true" if ddr.get("required", True) else "false"
        lines.append(f"set ddr_mig_config {q(cfg_path) if cfg_path else q('')}")
        lines.append("set ddr_mig_ready [expr {[string length $ddr_mig_config] != 0 && [file exists $ddr_mig_config]}]")
        lines.append("if {$ddr_mig_ready} {")
        lines.append(f"    create_ip -name mig_7series -vendor xilinx.com -library ip -module_name {mod}")
        lines.append(f"    set_property CONFIG.XML_INPUT_FILE $ddr_mig_config [get_ips {mod}]")
        lines.append(f"    generate_target all [get_files {mod}.xci]")
        lines.append("} else {")
        lines.append('    puts "WARNING: DDR MIG config is missing; DDR IP is not generated."')
        lines.append("}")
        lines.append("")

    lines.append("update_compile_order -fileset sources_1")
    lines.append("")
    lines.append('if {$RVB_FLOW eq "project" || $RVB_FLOW eq "ip"} {')
    lines.append("    exit")
    lines.append("}")
    if ddr.get("enabled", False):
        lines.append(f"if {{{ddr_required} && !$ddr_mig_ready}} {{")
        lines.append('    error "DDR MIG config is required for syn/impl/bit flow."')
        lines.append("}")
    lines.append("")
    lines.append(f"launch_runs synth_1 -jobs {jobs}")
    lines.append("wait_on_run synth_1")
    lines.append("open_run synth_1")
    lines.append("file mkdir reports")
    lines.append("report_timing_summary -file reports/post_synth_timing.rpt")
    lines.append("report_utilization -file reports/post_synth_util.rpt")
    lines.append('if {$RVB_FLOW eq "syn"} { exit }')
    lines.append("")
    lines.append(f"launch_runs impl_1 -jobs {jobs}")
    lines.append("wait_on_run impl_1")
    lines.append("open_run impl_1")
    lines.append("report_timing_summary -file reports/post_impl_timing.rpt")
    lines.append("report_utilization -file reports/post_impl_util.rpt")
    lines.append('if {$RVB_FLOW eq "impl"} { exit }')
    lines.append("")
    lines.append(f"launch_runs impl_1 -to_step write_bitstream -jobs {jobs}")
    lines.append("wait_on_run impl_1")
    lines.append("")
    return "\n".join(lines) + "\n"


def normalize_param_value(root, key, val):
    if not isinstance(val, str):
        return val
    if val == "":
        return val
    if not (key.endswith("_FILE") or key.endswith("_PATH")):
        return val

    path = Path(val)
    if path.is_absolute():
        return str(path)
    return str((root / path).resolve())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("conf")
    parser.add_argument("out_dir")
    parser.add_argument("--flow", default="ip", choices=["project", "ip", "syn", "impl", "bit"])
    args = parser.parse_args()

    root = Path.cwd().resolve()
    conf_path = (root / args.conf).resolve()
    out_dir = (root / args.out_dir).resolve()
    proj = read_json(conf_path)
    board = read_json(root / proj["board"])

    out_dir.mkdir(parents=True, exist_ok=True)
    rtl_files = find_rtl_src(root, proj.get("rtl_dir", []),
                             proj.get("rtl_exclude", []))

    mig_prj_path = gen_mig_prj(proj, board, out_dir)

    with open(out_dir / "rtl.f", "w", encoding="utf-8") as fp:
        for path in rtl_files:
            fp.write(f"{path}\n")

    with open(out_dir / f"{proj['name']}.xdc", "w", encoding="utf-8") as fp:
        fp.write(gen_xdc(proj, board))

    with open(out_dir / "mkprj.tcl", "w", encoding="utf-8") as fp:
        fp.write(gen_tcl(proj, board, root, out_dir, args.flow, mig_prj_path))


if __name__ == "__main__":
    main()

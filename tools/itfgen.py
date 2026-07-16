#!/usr/bin/env python3

import json
import sys
import math

def gen_c_itf(itf_name, desc):
    if desc.get("rtl_only", False):
        return

    c_path = "design/model/itf/{}_if.h".format(itf_name)
    f = open(c_path, "w")
    f.write("#ifndef {}_IF_H\n".format(itf_name.upper()))
    f.write("#define {}_IF_H\n\n".format(itf_name.upper()))

    f.write("#include <stdio.h>\n")
    f.write("#include \"base/types.h\"\n")
    f.write("#include \"base/def.h\"\n")
    f.write("#include \"dbg/vcd.h\"\n")
    if "include" in desc and len(desc["include"]) > 0:
        for inc in desc["include"]:
            f.write("#include \"{}\"\n".format(inc))
    f.write("\n")

    f.write("#define {}_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, ext_src) \\\n".format(itf_name.upper()))
    f.write("    {}_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, false)\n\n".format(itf_name.upper()))

    f.write("#define {}_SIM_PROT_SIGNAL_IF_CONSTRUCT(module, itf, dis_trace, sim_prot) \\\n".format(itf_name.upper()))
    f.write("    {}_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, false, sim_prot)\n\n".format(itf_name.upper()))

    f.write("#define {}_SIGNAL_IF_CONSTRUCT_INNER(module, itf, dis_trace, ext_src, sim_prot_) do {{ \\\n".format(itf_name.upper()))
    f.write("    itf_conf_t conf = { \\\n")
    f.write("        .cycle = module->mod.cycle, \\\n")
    f.write("        .hier_name = module->mod.hier_name, \\\n")
    f.write("        .mode = ITF_MODE_SIGNAL, \\\n")
    f.write("        .pkt_size = sizeof({}_if_t), \\\n".format(itf_name))
    f.write("        .pkt2str = &{}_if_to_str, \\\n".format(itf_name))
    f.write("        .reg_vcd = &{}_if_reg_vcd, \\\n".format(itf_name))
    f.write("        .force_disable_trace = dis_trace, \\\n")
    f.write("        .ext_sig_src = ext_src, \\\n")
    f.write("        .sim_prot = sim_prot_ \\\n")
    f.write("    }; \\\n")
    f.write("    itf_construct(&module->itf, #itf, &conf); \\\n")
    f.write("} while (0)\n\n")

    f.write("#define {}_IF_CONSTRUCT(module, itf, depth) \\\n".format(itf_name.upper()))
    f.write("    {}_IF_CONSTRUCT_INNER(module, itf, depth, false)\n\n".format(itf_name.upper()))

    f.write("#define {}_SIM_PROT_IF_CONSTRUCT(module, itf, depth, sim_prot) \\\n".format(itf_name.upper()))
    f.write("    {}_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot)\n\n".format(itf_name.upper()))

    f.write("#define {}_IF_CONSTRUCT_INNER(module, itf, depth, sim_prot_) do {{ \\\n".format(itf_name.upper()))
    f.write("    itf_conf_t conf = { \\\n")
    f.write("        .cycle = module->mod.cycle, \\\n")
    f.write("        .hier_name = module->mod.hier_name, \\\n")
    f.write("        .mode = ITF_MODE_FIFO, \\\n")
    f.write("        .pkt_size = sizeof({}_if_t), \\\n".format(itf_name))
    f.write("        .pkt2str = &{}_if_to_str, \\\n".format(itf_name))
    f.write("        .reg_vcd = &{}_if_reg_vcd, \\\n".format(itf_name))
    f.write("        .force_disable_trace = false, \\\n")
    f.write("        .sim_prot = sim_prot_, \\\n")
    f.write("        .fifo_depth = depth \\\n")
    f.write("    }; \\\n")
    f.write("    itf_construct(&module->itf, #itf, &conf); \\\n")
    f.write("} while (0)\n\n")

    enums_bw = {}
    if "enums" in desc:
        for enum_name, enum_list in desc["enums"].items():
            max_bw = 0
            enum_full_name = "{}_{}".format(itf_name, enum_name)
            f.write("typedef enum {} {{\n".format(enum_full_name))
            items = list(enum_list.items())
            for idx, (item_name, item_val) in enumerate(items):
                comma = "" if idx == len(items) - 1 else ","
                f.write("    {}_{} = {}{}\n".format(enum_full_name.upper(), item_name.upper(), item_val, comma))
                max_bw = max(math.ceil(math.log2(item_val + 1)), max_bw)
            f.write("}} {}_t;\n\n".format(enum_full_name))
            enums_bw[enum_name] = max_bw

    payload_num = len(desc["payloads"])
    f.write("typedef struct {}_if {{\n".format(itf_name))
    for p in desc["payloads"]:
        field_name = p["name"]
        if "type" in p:
            f.write("    {}_{}_t {};\n".format(itf_name, p["type"], field_name))
        elif "ext_type" in p:
            f.write("    {} {};\n".format(p["ext_type"], field_name))
        else:
            bw = p["width"]
            if bw == 1:
                f.write("    bool {};\n".format(field_name))
            elif bw > 1 and bw < 8:
                f.write("    u8 {}; // {}-bit\n".format(field_name, bw))
            elif bw == 8:
                f.write("    u8 {};\n".format(field_name))
            elif bw > 8 and bw < 16:
                f.write("    u16 {}; // {}-bit\n".format(field_name, bw))
            elif bw == 16:
                f.write("    u16 {};\n".format(field_name))
            elif bw > 16 and bw < 32:
                f.write("    u32 {}; // {}-bit\n".format(field_name, bw))
            elif bw == 32:
                f.write("    u32 {};\n".format(field_name))
            elif bw > 32 and bw < 64:
                f.write("    u64 {}; // {}-bit\n".format(field_name, bw))
            elif bw == 64:
                f.write("    u64 {};\n".format(field_name))
            else:
                raise Exception("bit width not support")
    if payload_num == 0:
        f.write("    u32 dummy;\n")
    f.write("}} {}_if_t;\n\n".format(itf_name))

    f.write("static inline void {}_if_to_str(const void *pkt, char *str)\n".format(itf_name))
    f.write("{\n")
    f.write("    const {}_if_t *{} = (const {}_if_t *)pkt;\n".format(itf_name, itf_name, itf_name))
    f.write("    sprintf(str, \"")
    payload_idx = 0
    for p in desc["payloads"]:
        field_name = p["name"]
        bw = enums_bw[p["type"]] if "type" in p else p["width"]
        hex_w = math.ceil(bw / 4)
        if bw > 32:
            f.write("%0{}\"U64_HEX_FMT\"".format(hex_w))
        else:
            f.write("%0{}x".format(hex_w))
        if payload_idx < (payload_num - 1):
            f.write(" ")
        else:
            f.write("\\n\", ")
        payload_idx += 1
    payload_idx = 0
    for p in desc["payloads"]:
        field_name = p["name"]
        if "type" in p:
            f.write("(u32)")
        f.write("{}->{}".format(itf_name, field_name))
        if "ext_type" in p:
            f.write(".{}".format(p["raw"]))
        if payload_idx < (payload_num - 1):
            f.write(", ")
        else:
            f.write(");\n")
        payload_idx += 1
    if payload_num == 0:
        f.write("%08x\\n\", {}->dummy);\n".format(itf_name))
    f.write("}\n\n")

    f.write("static inline void {}_if_reg_vcd(const void *pkt, dbg_sig_type_t type)\n".format(itf_name))
    f.write("{\n")
    f.write("    const {}_if_t *{} = (const {}_if_t *)pkt;\n".format(itf_name, itf_name, itf_name))
    for p in desc["payloads"]:
        field_name = p["name"]
        if "ext_type" in p:
            field_name = "{}.{}".format(field_name, p["raw"])
        bw = enums_bw[p["type"]] if "type" in p else p["width"]
        f.write("    dbg_vcd_add_sig(\"{}\", type, {}, &{}->{});\n".format(p["name"], bw, itf_name, field_name))
    if payload_num == 0:
        f.write("    dbg_vcd_add_sig(\"dummy\", type, 32, &{}->dummy);\n".format(itf_name))
    f.write("}\n\n")

    f.write("#endif\n")
    f.close()


def _rtl_field_width(p):
    """return (width_bits, is_enum) for RTL logic declaration."""
    if "type" in p:
        return (None, True)   # enum — width derived from enum name
    if "ext_type" in p:
        return (p["width"], False)  # external type
    return (p["width"], False)


def _enum_width(enum_list):
    max_val = max(enum_list.values())
    return max(1, math.ceil(math.log2(max_val + 1)))


def _rtl_trace_expr(itf_name, p):
    name = p.get("rtl_alias", p["name"])
    if "type" in p:
        return "pkt.{}".format(name)
    if "ext_type" in p:
        return "pkt.{}.{}".format(name, p["raw"])
    return "pkt.{}".format(name)


def _rtl_trace_width(desc, p):
    if "type" in p:
        return _enum_width(desc["enums"][p["type"]])
    return p["width"]


def _rtl_trace_when(desc, itf_type, payloads, ctrl_sigs):
    if itf_type == "vld-rdy":
        return "vld && rdy"
    if itf_type == "vld":
        return "vld"
    if itf_type == "none":
        if "trace_when" in desc:
            return desc["trace_when"]
        if ctrl_sigs:
            return "__itf_trace_ctrl_changed()"
        if payloads:
            return "__itf_trace_pkt_changed()"
    if "trace_when" in desc:
        raise Exception("trace_when is only supported for type=none interfaces")
    return None


def _rtl_trace_suffixes(desc, itf_type, ctrl_sigs):
    if "trace_suffixes" in desc:
        suffixes = desc["trace_suffixes"]
        if len(suffixes) == 1:
            return (suffixes[0], "")
        if len(suffixes) == 2:
            return (suffixes[0], suffixes[1])
        raise Exception("trace_suffixes expects one or two suffixes")
    if itf_type == "vld-rdy" or ctrl_sigs:
        return ("mst", "slv")
    return ("drv", "")


def _rtl_ctrl_width(ctrl_sigs):
    return sum(c["width"] for c in ctrl_sigs)


def gen_rtl_itf(itf_name, desc):
    """Generate RTL .sv interface and .svh header for one interface."""
    if desc.get("model_only", False):
        return

    itf_type = desc.get("type", "vld-rdy")
    payloads = desc.get("payloads", [])
    ctrl_sigs = desc.get("ctrl", [])
    enums = desc.get("enums", {})
    inc_rtl = desc.get("include_rtl", [])

    sv_path = "design/rtl/itf/{}_if.sv".format(itf_name)

    if enums:
        svh_path = "design/rtl/itf/{}_if.svh".format(itf_name)
        guard = "{}_IF_SVH".format(itf_name.upper())
        with open(svh_path, "w") as f:
            f.write("`ifndef {}\n".format(guard))
            f.write("`define {}\n".format(guard))

            for inc in inc_rtl:
                f.write("`include \"{}\"\n".format(inc))

            for enum_name, enum_list in enums.items():
                max_val = max(enum_list.values())
                enum_bw = max(1, math.ceil(math.log2(max_val + 1)))
                f.write("\ntypedef enum logic [{}:0] {{\n".format(enum_bw - 1))
                full = "{}_{}".format(itf_name, enum_name)
                items = list(enum_list.items())
                for idx, (item_name, item_val) in enumerate(items):
                    comma = "" if idx == len(items) - 1 else ","
                    f.write("    {}_{} = {}'d{}{}\n".format(full.upper(), item_name.upper(), enum_bw, item_val, comma))
                f.write("}} {}_{}_t;\n".format(itf_name, enum_name))

            f.write("\n`endif\n")

    with open(sv_path, "w") as f:
        needs_blank = False
        # include own .svh if enums exist
        if enums:
            f.write("`include \"itf/{}_if.svh\"\n".format(itf_name))
            needs_blank = True
        for inc in inc_rtl:
            f.write("`include \"{}\"\n".format(inc))
            needs_blank = True
        f.write("`include \"dbg/itf_trace.svh\"\n")
        needs_blank = True

        f.write("{}interface {}_if_t;\n".format("\n" if needs_blank else "", itf_name))

        has_vld  = itf_type in ("vld", "vld-rdy")
        has_rdy  = itf_type == "vld-rdy"
        has_pkt  = len(payloads) > 0
        has_ctrl = len(ctrl_sigs) > 0
        trace_needs_change = itf_type == "none"
        trace_has_state = trace_needs_change and (has_pkt or has_ctrl)

        if has_vld:
            f.write("    logic vld;\n")
        if has_rdy:
            f.write("    logic rdy;\n")

        if has_ctrl:
            for c in ctrl_sigs:
                cw = c["width"]
                if cw == 1:
                    f.write("    logic {};\n".format(c["name"]))
                else:
                    f.write("    logic [{}:0] {};\n".format(cw - 1, c["name"]))

        if has_pkt:
            f.write("\n    struct packed {\n")
            for p in payloads:
                name = p.get("rtl_alias", p["name"])
                if "type" in p:
                    f.write("        {}_{}_t {};\n".format(itf_name, p["type"], name))
                elif "ext_type" in p:
                    f.write("        {} {};\n".format(p["ext_type"], name))
                else:
                    w = p["width"]
                    if w == 1:
                        f.write("        logic {};\n".format(name))
                    else:
                        f.write("        logic [{}:0] {};\n".format(w - 1, name))
            f.write("    } pkt;\n")

        f.write("\n")
        f.write("`ifdef RVB_ITF_TRACE_ENABLED\n")
        if has_pkt and trace_has_state:
            f.write("    logic [$bits(pkt)-1:0] __itf_trace_old_pkt;\n")
            f.write("    bit __itf_trace_pkt_seen;\n")
        if has_ctrl and trace_has_state:
            ctrl_width = _rtl_ctrl_width(ctrl_sigs)
            f.write("    localparam int __ITF_TRACE_CTRL_WIDTH = {};\n".format(ctrl_width))
            f.write("    logic [__ITF_TRACE_CTRL_WIDTH-1:0] __itf_trace_old_ctrl;\n")
            f.write("    bit __itf_trace_ctrl_seen;\n")

        if trace_has_state:
            f.write("\n")
            f.write("    initial begin\n")
            if has_pkt:
                f.write("        __itf_trace_old_pkt = '0;\n")
                f.write("        __itf_trace_pkt_seen = 1'b0;\n")
            if has_ctrl:
                f.write("        __itf_trace_old_ctrl = '0;\n")
                f.write("        __itf_trace_ctrl_seen = 1'b0;\n")
            f.write("    end\n")

        if has_pkt and trace_has_state:
            f.write("\n")
            f.write("    function automatic bit __itf_trace_pkt_changed;\n")
            f.write("        if (!__itf_trace_pkt_seen)\n")
            f.write("            __itf_trace_pkt_changed = 1'b1;\n")
            f.write("        else\n")
            f.write("            __itf_trace_pkt_changed = __itf_trace_old_pkt !== $bits(pkt)'(pkt);\n")
            f.write("    endfunction\n")

        if has_ctrl and trace_has_state:
            f.write("\n")
            f.write("    function automatic logic [__ITF_TRACE_CTRL_WIDTH-1:0] __itf_trace_ctrl_pack;\n")
            f.write("        __itf_trace_ctrl_pack = {")
            f.write(", ".join(c["name"] for c in ctrl_sigs))
            f.write("};\n")
            f.write("    endfunction\n\n")
            f.write("    function automatic bit __itf_trace_ctrl_changed;\n")
            f.write("        if (!__itf_trace_ctrl_seen)\n")
            f.write("            __itf_trace_ctrl_changed = 1'b0;\n")
            f.write("        else\n")
            f.write("            __itf_trace_ctrl_changed = __itf_trace_old_ctrl !==\n")
            f.write("                __itf_trace_ctrl_pack();\n")
            f.write("    endfunction\n")

        if trace_has_state:
            f.write("\n")
            f.write("    task automatic __itf_trace_update_state;\n")
            f.write("        begin\n")
            if has_pkt:
                f.write("            __itf_trace_old_pkt = $bits(pkt)'(pkt);\n")
                f.write("            __itf_trace_pkt_seen = 1'b1;\n")
            if has_ctrl:
                f.write("            __itf_trace_old_ctrl = __itf_trace_ctrl_pack();\n")
                f.write("            __itf_trace_ctrl_seen = 1'b1;\n")
            f.write("        end\n")
            f.write("    endtask\n")

        f.write("\n")
        f.write("    function automatic string __itf_trace_pkt_str;\n")
        if has_pkt:
            fmt_parts = []
            args = []
            for p in payloads:
                width = _rtl_trace_width(desc, p)
                fmt_parts.append("%0{}x".format(math.ceil(width / 4)))
                args.append(_rtl_trace_expr(itf_name, p))
            f.write("        __itf_trace_pkt_str = $sformatf(\n")
            f.write("            \"{}\"".format(" ".join(fmt_parts)))
            for idx, arg in enumerate(args):
                suffix = "," if idx != len(args) - 1 else ""
                if idx == 0:
                    f.write(",\n")
                f.write("            {}{}\n".format(arg, suffix))
            f.write("        );\n")
        else:
            f.write("        __itf_trace_pkt_str = \"00000000\";\n")
        f.write("    endfunction\n")
        f.write("`endif\n")

        # modports
        mst_outs = []
        mst_ins  = []
        slv_outs = []
        slv_ins  = []

        if has_vld:
            mst_outs.append("vld")
            slv_ins.append("vld")
        if has_rdy:
            mst_ins.append("rdy")
            slv_outs.append("rdy")
        if has_pkt:
            mst_outs.append("pkt")
            slv_ins.append("pkt")
        if has_ctrl:
            for c in ctrl_sigs:
                mst_outs.append(c["name"])
                slv_ins.append(c["name"])

        def _fmt_port_list(items, dir_kw):
            if not items:
                return ""
            return "{} {}".format(dir_kw, ", ".join(items))

        mst_out_str = _fmt_port_list(mst_outs, "output")
        mst_in_str  = _fmt_port_list(mst_ins,  "input")
        slv_out_str = _fmt_port_list(slv_outs, "output")
        slv_in_str  = _fmt_port_list(slv_ins,  "input")

        mst_ports = ", ".join(filter(None, [mst_out_str, mst_in_str]))
        slv_ports = ", ".join(filter(None, [slv_in_str,  slv_out_str]))

        if mst_ports:
            f.write("    modport mst ({});\n".format(mst_ports))
        if slv_ports:
            f.write("    modport slv ({});\n".format(slv_ports))

        trace_when = _rtl_trace_when(desc, itf_type, payloads, ctrl_sigs)
        if trace_when is not None:
            suffix0, suffix1 = _rtl_trace_suffixes(desc, itf_type, ctrl_sigs)
            macro = "RVB_ITF_TRACE_WHEN_UPDATE" if trace_has_state else "RVB_ITF_TRACE_WHEN"
            if not has_pkt:
                macro += "_NOPKT"
            f.write("\n")
            f.write("    `{}(\"{}\", \"{}\", {})\n".format(
                macro, suffix0, suffix1, trace_when))

        f.write("endinterface\n")


if len(sys.argv) < 2:
    print("usage: {} <itf.json> [c|rtl]".format(sys.argv[0]))
    sys.exit(1)

conf = open(sys.argv[1], "r")
desc = json.load(conf)["itfs_desc"]
conf.close()

mode = sys.argv[2] if len(sys.argv) >= 3 else "all"

for itf_name, itf_desc in desc.items():
    if mode in ("c", "all"):
        gen_c_itf(itf_name, itf_desc)
    if mode in ("rtl", "all"):
        gen_rtl_itf(itf_name, itf_desc)

#!/usr/bin/env python3

import sys
import json
from pathlib import Path

def gen_c_header(csr_desc, fp):
    fp.write("#ifndef RV32G_CSR_H\n")
    fp.write("#define RV32G_CSR_H\n\n")
    fp.write("#include \"base/types.h\"\n")
    fp.write("#include \"isa.h\"\n\n")

    for dg in csr_desc["def"]:
        fp.write("typedef enum {} {{\n".format(dg))
        for d in csr_desc["def"][dg]:
            fp.write("    {}_{} = {}u,\n".format(dg.upper(), d.upper(), csr_desc["def"][dg][d]))
        fp.write("}} {}_t;\n\n".format(dg))

    for csr in csr_desc["csr"]:
        if "fields" in csr:
            has_bit = False
            for f, d in csr["fields"].items():
                if d[1] == 1:
                    fp.write("#define RV32G_CSR_{}_{}_BIT (1u << {})\n".format(csr["name"].upper(), f.upper(), d[0]))
                    has_bit = True
            if has_bit:
                fp.write("\n")

    fp.write("#pragma pack(1)\n")
    csr_idx = 0
    for csr in csr_desc["csr"]:
        if "fields" in csr:
            if csr_idx > 0:
                fp.write("\n")
            fp.write("typedef union rv32g_csr_{} {{\n".format(csr["name"]))
            fp.write("    struct {\n")
            rsvd_idx, last_bit = 0, 0
            for f, d in csr["fields"].items():
                if d[0] > last_bit:
                    fp.write("        u32 rsvd_{} : {};\n".format(rsvd_idx, d[0] - last_bit))
                    rsvd_idx += 1
                fp.write("        u32 {} : {};\n".format(f.lower(), d[1]))
                last_bit = d[0] + d[1]
            if last_bit < 32:
                fp.write("        u32 rsvd_{} : {};\n".format(rsvd_idx, 32 - last_bit))
                rsvd_idx += 1
            fp.write("    } reg;\n")
            fp.write("    u32 raw;\n")
            fp.write("}} rv32g_csr_{}_t;\n".format(csr["name"]))
            csr_idx += 1
    fp.write("#pragma pack()\n\n")

    for csr in csr_desc["csr"]:
        if "fields" not in csr:
            fp.write("typedef u32 rv32g_csr_{}_t;\n".format(csr["name"]))
    fp.write("\n")

    fp.write("typedef struct rv32g_csr {\n")
    for csr in csr_desc["csr"]:
        fp.write("    rv32g_csr_{}_t {};\n".format(csr["name"], csr["name"]))
    fp.write("} rv32g_csr_t;\n\n")

    fp.write("extern void rv32g_csr_reset(rv32g_csr_t *csr);\n")
    fp.write("extern bool rv32g_csr_read(rv32g_csr_t *csr, rv32g_priv_t priv, u32 addr, u32 *data);\n")
    fp.write("extern bool rv32g_csr_write(rv32g_csr_t *csr, rv32g_priv_t priv, u32 addr, u32 data);\n")
    fp.write("extern const char *rv32g_csr_name(u32 addr);\n\n")
    fp.write("#endif")

def gen_c_src(csr_desc, fp):
    priv_mode_map = {
        'u': "RV32G_PRIV_USER",
        's': "RV32G_PRIV_SUPERVISOR",
        'm': "RV32G_PRIV_MACHINE"
    }
    fp.write("#include \"csr.h\"\n\n")
    fp.write("#define CSR_CHECK_PRIV(p) if (priv < p) { return false; }\n\n")

    fp.write("void rv32g_csr_reset(rv32g_csr_t *csr)\n")
    fp.write("{\n")
    for csr in csr_desc["csr"]:
        reset_val = "0"
        if "reset" in csr:
            reset_val = csr["reset"]
            if csr["name"] == "misa":
                reset_bits = ["RV32G_CSR_MISA_{}_BIT".format(c.upper()) for c in reset_val]
                reset_bits.append("(1u << 30)")
                reset_val = ' | '.join(reset_bits)
        if "fields" in csr:
            fp.write("    csr->{}.raw = {};\n".format(csr["name"], reset_val))
        else:
            fp.write("    csr->{} = {};\n".format(csr["name"], reset_val))
    fp.write("}\n\n")

    fp.write("bool rv32g_csr_read(rv32g_csr_t *csr, rv32g_priv_t priv, u32 addr, u32 *data)\n")
    fp.write("{\n")
    fp.write("    switch (addr) {\n")
    for csr in csr_desc["csr"]:
        fp.write("        case {}: {{\n".format(csr["addr"]))
        fp.write("            CSR_CHECK_PRIV({});\n".format(priv_mode_map[csr["prop"][0]]))
        if "fields" in csr:
            fp.write("            *data = csr->{}.raw;\n".format(csr["name"]))
        else:
            fp.write("            *data = csr->{};\n".format(csr["name"]))
        fp.write("            return true;\n")
        fp.write("        }\n")
    fp.write("        default:\n")
    fp.write("            return false;\n")
    fp.write("    }\n\n")
    fp.write("    return false;\n")
    fp.write("}\n\n")

    fp.write("bool rv32g_csr_write(rv32g_csr_t *csr, rv32g_priv_t priv, u32 addr, u32 data)\n")
    fp.write("{\n")
    fp.write("    switch (addr) {\n")
    for csr in csr_desc["csr"]:
        if csr["prop"].find("w") < 0:
            continue
        fp.write("        case {}: {{\n".format(csr["addr"]))
        fp.write("            CSR_CHECK_PRIV({});\n".format(priv_mode_map[csr["prop"][0]]))
        if "fields" in csr:
            fp.write("            csr->{}.raw = data;\n".format(csr["name"]))
        else:
            fp.write("            csr->{} = data;\n".format(csr["name"]))
        fp.write("            return true;\n")
        fp.write("        }\n")
    fp.write("        default:\n")
    fp.write("            return false;\n")
    fp.write("    }\n\n")
    fp.write("    return false;\n")
    fp.write("}\n\n")

    fp.write("const char *rv32g_csr_name(u32 addr)\n")
    fp.write("{\n")
    fp.write("    switch (addr) {\n")
    for csr in csr_desc["csr"]:
        fp.write("        case {}:\n".format(csr["addr"]))
        fp.write("            return \"{}\";\n".format(csr["name"]))
    fp.write("        default:\n")
    fp.write("            return \"unknown\";\n")
    fp.write("    }\n\n")
    fp.write("    return \"unknown\";\n")
    fp.write("}\n")


def rtl_reset_value(csr):
    reset = csr.get("reset", 0)
    if csr["name"] == "misa" and isinstance(reset, str):
        value = 1 << 30
        for ext in reset:
            value |= 1 << (ord(ext.lower()) - ord('a'))
        return f"32'h{value:08x}"
    if isinstance(reset, str):
        return reset
    return f"32'h{reset:08x}"


def gen_rtl(csr_desc, fp):
    priv_map = {'u': 0, 's': 1, 'm': 3}
    fp.write("module csr(\n")
    fp.write("    input logic                    clk,\n")
    fp.write("    input logic                    rst_n,\n")
    fp.write("    exu_csr_read_req_if_t.slv      exu_csr_read_req_slv,\n")
    fp.write("    csr_exu_read_rsp_if_t.mst      csr_exu_read_rsp_mst,\n")
    fp.write("    exu_csr_write_req_if_t.slv     exu_csr_write_req_slv,\n")
    fp.write("    csr_exu_write_rsp_if_t.mst     csr_exu_write_rsp_mst,\n")
    fp.write("    core_timer_if_t.slv             core_timer_slv,\n")
    fp.write("    core_m_irq_if_t.slv             core_m_irq_slv,\n")
    fp.write("    ext_irq_if_t.slv                ext_irq_slv,\n")
    fp.write("    trap_csr_write_req_if_t.slv     trap_csr_write_req_slv,\n")
    fp.write("    csr_trap_write_rsp_if_t.mst     csr_trap_write_rsp_mst,\n")
    fp.write("    csr_trap_state_if_t.mst         csr_trap_state_mst\n")
    fp.write(");\n")
    for csr in csr_desc["csr"]:
        fp.write(f"    logic [31:0] csr_{csr['name']};\n")
    fp.write("\n")
    fp.write("    always_ff @(posedge clk or negedge rst_n) begin\n")
    fp.write("        if (!rst_n) begin\n")
    for csr in csr_desc["csr"]:
        fp.write(f"            csr_{csr['name']} <= {rtl_reset_value(csr)};\n")
    fp.write("        end else begin\n")
    fp.write("            csr_cycle <= csr_cycle + 1'b1;\n")
    fp.write("            csr_time <= core_timer_slv.pkt.mtime[31:0];\n")
    fp.write("            csr_timeh <= core_timer_slv.pkt.mtime[63:32];\n")
    fp.write("            csr_mip[7] <= core_m_irq_slv.pkt.mtimer;\n")
    fp.write("            csr_mip[3] <= core_m_irq_slv.pkt.msw;\n")
    fp.write("            csr_mip[11] <= ext_irq_slv.pkt.irq;\n")
    fp.write("            if (trap_csr_write_req_slv.vld && csr_trap_write_rsp_mst.pkt.ok) begin\n")
    fp.write("                case (trap_csr_write_req_slv.pkt.addr)\n")
    for csr in csr_desc["csr"]:
        if 'w' in csr["prop"]:
            fp.write(f"                    12'h{int(csr['addr'], 0):03x}: csr_{csr['name']} <= trap_csr_write_req_slv.pkt.val;\n")
    fp.write("                    default: ;\n")
    fp.write("                endcase\n")
    fp.write("            end else if (exu_csr_write_req_slv.vld && csr_exu_write_rsp_mst.pkt.ok) begin\n")
    fp.write("                case (exu_csr_write_req_slv.pkt.addr)\n")
    for csr in csr_desc["csr"]:
        if 'w' in csr["prop"]:
            fp.write(f"                    12'h{int(csr['addr'], 0):03x}: csr_{csr['name']} <= exu_csr_write_req_slv.pkt.val;\n")
    fp.write("                    default: ;\n")
    fp.write("                endcase\n")
    fp.write("            end\n")
    fp.write("        end\n")
    fp.write("    end\n\n")
    fp.write("    always_comb begin\n")
    fp.write("        csr_exu_read_rsp_mst.pkt.val = '0;\n")
    fp.write("        csr_exu_read_rsp_mst.pkt.ok = 1'b0;\n")
    fp.write("        case (exu_csr_read_req_slv.pkt.addr)\n")
    for csr in csr_desc["csr"]:
        addr = int(csr["addr"], 0)
        priv = priv_map[csr["prop"][0]]
        fp.write(f"            12'h{addr:03x}: begin\n")
        if priv == 0:
            fp.write("                csr_exu_read_rsp_mst.pkt.ok = 1'b1;\n")
        else:
            fp.write(f"                csr_exu_read_rsp_mst.pkt.ok = exu_csr_read_req_slv.pkt.priv >= 2'd{priv};\n")
        fp.write(f"                csr_exu_read_rsp_mst.pkt.val = csr_{csr['name']};\n")
        fp.write("            end\n")
    fp.write("            default: ;\n")
    fp.write("        endcase\n")
    fp.write("    end\n\n")
    fp.write("    always_comb begin\n")
    fp.write("        csr_exu_write_rsp_mst.vld = exu_csr_write_req_slv.vld;\n")
    fp.write("        csr_exu_write_rsp_mst.pkt.ok = 1'b0;\n")
    fp.write("        case (exu_csr_write_req_slv.pkt.addr)\n")
    for csr in csr_desc["csr"]:
        if 'w' not in csr["prop"]:
            continue
        addr = int(csr["addr"], 0)
        priv = priv_map[csr["prop"][0]]
        if priv == 0:
            fp.write(f"            12'h{addr:03x}: csr_exu_write_rsp_mst.pkt.ok = 1'b1;\n")
        else:
            fp.write(f"            12'h{addr:03x}: csr_exu_write_rsp_mst.pkt.ok = exu_csr_write_req_slv.pkt.priv >= 2'd{priv};\n")
    fp.write("            default: ;\n")
    fp.write("        endcase\n")
    fp.write("    end\n")
    fp.write("\n    assign csr_trap_write_rsp_mst.vld = trap_csr_write_req_slv.vld;\n")
    fp.write("    assign csr_trap_write_rsp_mst.pkt.ok = 1'b1;\n")
    for name in ["mstatus", "mip", "mie", "mtvec", "mepc", "medeleg", "mideleg", "sstatus", "sip", "sie", "stvec", "sepc"]:
        fp.write(f"    assign csr_trap_state_mst.pkt.{name} = csr_{name};\n")
    fp.write("endmodule\n")


repo_root = Path(__file__).resolve().parent.parent
csr_desc_fp = open(sys.argv[1], "r")
csr_desc = json.load(csr_desc_fp)
csr_desc_fp.close()

c_header_fp = open(repo_root / "design/model/spec/core/csr.h", "w")
gen_c_header(csr_desc, c_header_fp)
c_header_fp.close()

c_src_fp = open(repo_root / "design/model/spec/core/csr.c", "w")
gen_c_src(csr_desc, c_src_fp)
c_src_fp.close()

rtl_fp = open(repo_root / "design/rtl/core/hart/csr.sv", "w")
gen_rtl(csr_desc, rtl_fp)
rtl_fp.close()

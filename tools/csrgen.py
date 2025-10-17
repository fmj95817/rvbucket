#!/usr/bin/env python3

import sys
import json

def gen_c_header(csr_desc, fp):
    fp.write("#ifndef CSR_H\n")
    fp.write("#define CSR_H\n\n")
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

    fp.write("typedef struct csr {\n")
    for csr in csr_desc["csr"]:
        fp.write("    rv32g_csr_{}_t {};\n".format(csr["name"], csr["name"]))
    fp.write("} csr_t;\n\n")

    fp.write("extern void csr_reset(csr_t *csr);\n")
    fp.write("extern bool csr_read(csr_t *csr, rv32g_priv_t priv, u32 addr, u32 *data);\n")
    fp.write("extern bool csr_write(csr_t *csr, rv32g_priv_t priv, u32 addr, u32 data);\n")
    fp.write("extern const char *csr_name(u32 addr);\n\n")
    fp.write("#endif")

def gen_c_src(csr_desc, fp):
    priv_mode_map = {
        'u': "RV32G_PRIV_USER",
        's': "RV32G_PRIV_SUPERVISOR",
        'm': "RV32G_PRIV_MACHINE"
    }
    fp.write("#include \"csr.h\"\n\n")
    fp.write("#define CSR_CHECK_PRIV(p) if (priv < p) { return false; }\n\n")

    fp.write("void csr_reset(csr_t *csr)\n")
    fp.write("{\n")
    for csr in csr_desc["csr"]:
        reset_val = "0"
        if "reset" in csr:
            reset_val = csr["reset"]
            if csr["name"] == "misa":
                reset_val = ' | '.join(["RV32G_CSR_MISA_{}_BIT".format(c) for c in reset_val])
        if "fields" in csr:
            fp.write("    csr->{}.raw = {};\n".format(csr["name"], reset_val))
        else:
            fp.write("    csr->{} = {};\n".format(csr["name"], reset_val))
    fp.write("}\n\n")

    fp.write("bool csr_read(csr_t *csr, rv32g_priv_t priv, u32 addr, u32 *data)\n")
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

    fp.write("bool csr_write(csr_t *csr, rv32g_priv_t priv, u32 addr, u32 data)\n")
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

    fp.write("const char *csr_name(u32 addr)\n")
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


csr_desc_fp = open(sys.argv[1], "r")
csr_desc = json.load(csr_desc_fp)
csr_desc_fp.close()

c_header_fp = open("model/core/hart/csr.h", "w")
gen_c_header(csr_desc, c_header_fp)
c_header_fp.close()

c_src_fp = open("model/core/hart/csr.c", "w")
gen_c_src(csr_desc, c_src_fp)
c_src_fp.close()
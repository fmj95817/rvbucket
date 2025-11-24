#!/usr/bin/env python3

import json
import sys
import math

def gen_c_itf(itf_name, desc):
    c_path = "model/itf/{}_if.h".format(itf_name)
    f = open(c_path, "w")
    f.write("#ifndef {}_IF_H\n".format(itf_name.upper()))
    f.write("#define {}_IF_H\n\n".format(itf_name.upper()))

    f.write("#include <stdio.h>\n")
    f.write("#include \"base/types.h\"\n")
    f.write("#include \"dbg/vcd.h\"\n")
    if "include" in desc and len(desc["include"]) > 0:
        for inc in desc["include"]:
            f.write("#include \"{}\"\n".format(inc))
    f.write("\n")

    f.write("#define {}_IF_CONSTRUCT(module, itf, depth) do {{ \\\n".format(itf_name.upper()))
    f.write("    itf_conf_t conf = { \\\n")
    f.write("        .cycle = module->cycle, \\\n")
    f.write("        .pkt2str = &{}_if_to_str, \\\n".format(itf_name))
    f.write("        .reg_vcd = &{}_if_reg_vcd, \\\n".format(itf_name))
    f.write("        .pkt_size = sizeof({}_if_t), \\\n".format(itf_name))
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
            for item_name, item_val in enum_list.items():
                f.write("    {}_{} = {},\n".format(enum_full_name.upper(), item_name.upper(), item_val))
                max_bw = max(math.ceil(math.log2(item_val + 1)), max_bw)
            f.write("}} {}_t;\n".format(enum_full_name))
            enums_bw[enum_name] = max_bw
        f.write("\n")

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

    f.write("static inline void {}_if_reg_vcd(const void *pkt)\n".format(itf_name))
    f.write("{\n")
    f.write("    const {}_if_t *{} = (const {}_if_t *)pkt;\n".format(itf_name, itf_name, itf_name))
    for p in desc["payloads"]:
        field_name = p["name"]
        if "ext_type" in p:
            field_name = "{}.{}".format(field_name, p["raw"])
        bw = enums_bw[p["type"]] if "type" in p else p["width"]
        f.write("    dbg_vcd_add_sig(\"{}\", DBG_SIG_TYPE_REG, {}, &{}->{});\n".format(p["name"], bw, itf_name, field_name))
    if payload_num == 0:
        f.write("    dbg_vcd_add_sig(\"dummy\", DBG_SIG_TYPE_REG, 32, &{}->dummy);\n".format(itf_name))
    f.write("}\n\n")

    f.write("#endif\n")
    f.close()


conf = open(sys.argv[1], "r")
desc = json.load(conf)["itfs_desc"]
conf.close()

for itf_name, itf_desc in desc.items():
    gen_c_itf(itf_name, itf_desc)

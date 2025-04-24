#!/usr/bin/env python3

import json
import sys
import os

def gen_iostd(board):
    return f"set_property IOSTANDARD {board['iostd']} [get_ports]\n\n"

def gen_clk(proj):
    code = ""
    for clk in proj["clocks"]:
        period = 1e9 / clk["freq"]
        code += f"create_clock -period {period} -name {clk['name']} [get_ports {clk['port']}]\n"
    code += "\n"
    return code

def gen_pin_assign(board):
    code = ""
    for port, pin in board["pinout"].items():
        code += f"set_property PACKAGE_PIN {pin} [get_ports {{{port}}}]\n"
    code += "\n"
    return code

def gen_port_prop(proj):
    code = ""
    for port, props in proj["props"].items():
        for prop in props:
            code += f"set_property {prop} true [get_ports {port}]\n"
    code += "\n"
    return code

def gen_extra_xdc(board):
    code = ""
    for xdc in board["misc_xdc"]:
        code += f"{xdc}\n"
    if code != "":
        code += "\n"
    return code

def find_rtl_src(dir):
    sv_files = []
    for root, dirs, files in os.walk(dir, followlinks=True):
        for file in files:
            if file.endswith('.sv'):
                full_path = os.path.join(root, file)
                sv_files.append(full_path)
    return sv_files

def gen_proj_create(proj, board):
    code = f"create_project -part {board['device']} {proj['name']} .\n\n"
    return code

def gen_param(proj):
    if not proj["params"]:
        return ""

    code = "set_property generic {\n"
    for k, val in proj["params"].items():
        code += f"    {k}={val}\n"
    code += "} [get_filesets sources_1]\n\n"

    return code

def gen_include(proj):
    code = "set_property include_dirs {\n"
    root_dir = os.getcwd()
    for dir in proj["inc_dir"]:
        code += f"    {root_dir}/{dir}\n"
    code += "} [current_fileset]\n\n"
    return code

def gen_file_list(proj):
    code = ""
    root_dir = os.getcwd()

    rtl_src = []
    for dir in proj["rtl_dir"]:
        rtl_src += find_rtl_src(os.path.join(root_dir, dir))

    for rtl_path in rtl_src:
        code += f"add_files -fileset sources_1 {rtl_path}\n"
    code += f"add_files -fileset constrs_1 {proj['name']}.xdc\n\n"
    code += f"set_property top {proj['top_module']} [get_filesets sources_1]\n\n"

    return code

def gen_xdc(proj, board):
    code = ""
    code += gen_iostd(board)
    code += gen_clk(proj)
    code += gen_pin_assign(board)
    code += gen_port_prop(proj)
    code += gen_extra_xdc(board)
    return code

def gen_proj(proj, board):
    code = ""
    code += gen_proj_create(proj, board)
    code += gen_param(proj)
    code += gen_include(proj)
    code += gen_file_list(proj)
    return code

def read_conf(path):
    fp = open(path, 'r')
    proj = json.load(fp)
    fp.close()
    fp = open(f"{os.getcwd()}/{proj['board']}", 'r')
    board = json.load(fp)
    fp.close()
    return proj, board

def write_file(path, code):
    fp = open(path, "w")
    fp.write(code)
    fp.close()

if len(sys.argv) < 3:
    print("Usage: {} <project config json> <out dir>".format(sys.argv[0]))
    sys.exit(0)

conf_path = sys.argv[1]
out_dir = sys.argv[2]

proj, board = read_conf(conf_path)
write_file(f"{out_dir}/mkprj.tcl", gen_proj(proj, board))
write_file(f"{out_dir}/{proj['name']}.xdc", gen_xdc(proj, board))

#!/bin/bash

set -e

if [ "${1}" = "rtl" ]; then
    mkdir -p build/rtl
    cd build/rtl;
    vcs \
        -full64 \
        -sverilog \
        +v2k \
        -debug_acc+all \
        -lca -kdb \
        -top sim_top \
        -o sim_top \
        ../../sim/rtl/model/clk_rst.sv \
        ../../rtl/biu.sv \
        ../../rtl/exu.sv \
        ../../rtl/ifu.sv \
        ../../rtl/rv32i.sv \
        ../../sim/rtl/model/sram.sv \
        ../../sim/rtl/sim_top.sv;
    cd ../..
elif [ "${1}" = "model" ]; then
    mkdir -p build/model
    gcc \
        -I./model \
        -o build/model/sim_top \
        $(find model -name *.c) \
        $(find sim/model -name *.c)
fi
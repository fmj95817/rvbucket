#!/bin/bash

set -e

function build_sw_case {
    local TC="/home/mfeng/opt/rv32i-tc/bin/riscv32-unknown-elf"
    local CC="${TC}-gcc"
    local LD="${TC}-gcc"
    local OBJCOPY="${TC}-objcopy"
    local OBJDUMP="${TC}-objdump"
    local CRT="./sdk/crt"
    local COMPILE_OPTIONS="-Wall -O2 -fPIC -march=rv32i -I./sdk"

    local case_name="${1}"
    local case_dir="cases/${case_name}"
    local output_dir="build/sw/${case_name}"
    local drivers_dir="sdk/drivers"

    mkdir -p "${output_dir}"

    local elf="${output_dir}/${case_name}.elf"
    local bin="${output_dir}/${case_name}.bin"
    local dis="${output_dir}/${case_name}.S"

    local srcs=("$(find ${case_dir} ${drivers_dir} -name *.c)")
    local objs=()

    for src in ${srcs[@]}; do
        local obj="${output_dir}/$(basename ${src} .c).o"
        ${CC} ${COMPILE_OPTIONS} -c -o "${obj}" "${src}"
        objs+=("${obj}")
    done

    ${CC} ${COMPILE_OPTIONS} -c -o "${output_dir}/start.o" "${CRT}/start.S"
    objs+=("${output_dir}/start.o")

    ${LD} -T "${CRT}/soc.lds" -nostartfiles -o "${elf}" "${objs[@]}"
    ${OBJCOPY} -S "${elf}" -O binary "${bin}"
    ${OBJDUMP} -D "${elf}" > "${dis}"
}

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
elif [ "${1}" = "sw" ]; then
    build_sw_case test
fi



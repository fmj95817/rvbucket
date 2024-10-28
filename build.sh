#!/bin/bash

set -e

TC="/home/mfeng/opt/rv32i-tc/bin/riscv32-unknown-elf"
CC="${TC}-gcc"
OBJCOPY="${TC}-objcopy"
OBJDUMP="${TC}-objdump"

function build_tools {
    mkdir -p build/tools
    gcc -Wall -O3 -o build/tools/bin2x tools/bin2x.c -lm
}

function build_asm_case {
    local LD="${TC}-ld"

    local case_dir="${1}"
    local case_name="$(basename ${case_dir})"
    local output_dir="build/sw/${case_name}"

    mkdir -p "${output_dir}"

    local lds="$(find ${case_dir} -name *.lds)"
    local elf="${output_dir}/${case_name}.elf"
    local bin="${output_dir}/${case_name}.bin"
    local dis="${output_dir}/${case_name}.S"
    local hex="${output_dir}/${case_name}.hex"

    local srcs=("$(find ${case_dir} -name *.S)")
    local objs=()

    for src in ${srcs[@]}; do
        local obj="${output_dir}/$(basename ${src} .S).o"
        ${CC} -march=rv32i -c -o "${obj}" "${src}"
        objs+=("${obj}")
    done

    ${LD} -T "${lds}" -o "${elf}" "${objs[@]}"
    ${OBJCOPY} -S "${elf}" -O binary "${bin}"
    ${OBJDUMP} -D "${elf}" > "${dis}"
    ./build/tools/bin2x "${bin}" hex > "${hex}"

    if [ "${2}" = "gen_model_boot" ]; then
        ./build/tools/bin2x "${bin}" c_array > model/boot.c
    elif [ "${2}" = "gen_rtl_boot" ]; then
        ./build/tools/bin2x "${bin}" sv_rom_header > rtl/boot.svh
        ./build/tools/bin2x "${bin}" sv_rom_src > rtl/boot.sv
    fi
}

function build_c_case {
    local LD="${TC}-gcc"

    local CRT="./sdk/crt"
    local COMPILE_OPTIONS=(-Wall -O2 -fPIC -march=rv32i -I./sdk)

    local case_name="${1}"
    local case_dir="cases/${case_name}"
    local output_dir="build/sw/${case_name}"
    local drivers_dir="sdk/drivers"

    mkdir -p "${output_dir}"

    local elf="${output_dir}/${case_name}.elf"
    local bin="${output_dir}/${case_name}.bin"
    local hex="${output_dir}/${case_name}.hex"
    local dis="${output_dir}/${case_name}.S"

    local srcs=("$(find ${case_dir} ${drivers_dir} -name *.c)")
    local objs=()

    ${CC} ${COMPILE_OPTIONS[@]} -c -o "${output_dir}/start.o" "${CRT}/start.S"
    objs+=("${output_dir}/start.o")

    for src in ${srcs[@]}; do
        local obj="${output_dir}/$(basename ${src} .c).o"
        ${CC} ${COMPILE_OPTIONS[@]} -c -o "${obj}" "${src}"
        objs+=("${obj}")
    done

    ${LD} -T "${CRT}/soc.lds" -nostartfiles -o "${elf}" "${objs[@]}"
    ${OBJCOPY} -S "${elf}" -O binary "${bin}"
    ${OBJDUMP} -D "${elf}" > "${dis}"
    ./build/tools/bin2x "${bin}" hex > "${hex}"
}

function build_model {
    mkdir -p build/hw/model
    gcc \
        -O3 \
        -I./model \
        -o build/hw/model/sim_top \
        $(find model -name *.c) \
        $(find sim/model -name *.c)
}

function build_vcs {
    mkdir -p build/hw/vcs
    cd build/hw/vcs;
    vcs \
        -full64 \
        -sverilog \
        +v2k \
        -debug_acc+all \
        -kdb \
        -top sim_top \
        -o sim_top \
        -timescale=1ns/1ps \
        +incdir+../../../rtl \
        +incdir+../../../rtl/core \
        $(find ../../../rtl -name *.sv) \
        $(find ../../../sim/rtl/model -name *.sv) \
        $(find ../../../sim/rtl/vcs -name *.sv);
    cd ../../..
}

function build_verilator {
    mkdir -p build/hw/verilator
    cd build/hw/verilator;
    verilator \
        --sc --exe --build --trace \
        --top sim_top \
        +incdir+../../../rtl \
        +incdir+../../../rtl/core \
        $(find ../../../rtl -name *.sv) \
        $(find ../../../sim/rtl/model -name *.sv) \
        $(find ../../../sim/rtl/verilator -name *.sv) \
        $(find ../../../sim/rtl/verilator -name *.cc);
    cd ../../..
}

function build_hw {
    local bootloader="./sdk/bootloader"

    if [ "${1}" = "model" ]; then
        build_asm_case "${bootloader}" gen_model_boot
        build_model
    elif [ "${1}" = "vcs" ]; then
        build_asm_case "${bootloader}" gen_rtl_boot
        build_vcs
    elif [ "${1}" = "verilator" ]; then
        build_verilator
    fi
}

build_tools

if [ "${1}" = "hw" ]; then
    build_hw "${2}"
elif [ "${1}" = "sw" ]; then
    if [ "${2}" = "model" ]; then
        build_c_case test
    elif [ "${2}" = "rtl" ]; then
        build_asm_case ./cases/asm_test
    fi
fi
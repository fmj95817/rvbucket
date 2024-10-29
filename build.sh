#!/bin/bash

set -e

HOST_CC="gcc"
CROSS_PREFIX="riscv32-unknown-elf"
CROSS_CC="${CROSS_PREFIX}-gcc"
CROSS_OBJCOPY="${CROSS_PREFIX}-objcopy"
CROSS_OBJDUMP="${CROSS_PREFIX}-objdump"
BIN2X="./build/tools/bin2x"

function build_tools {
    mkdir -p build/tools
    ${HOST_CC} -Wall -O3 -o ${BIN2X} tools/bin2x.c -lm
}

function build_asm_case {
    local CROSS_LD="${CROSS_PREFIX}-ld"

    local case_name="${1}"
    local case_dir="cases/${case_name}"
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
        ${CROSS_CC} -march=rv32i -c -o "${obj}" "${src}"
        objs+=("${obj}")
    done

    ${CROSS_LD} -T "${lds}" -o "${elf}" "${objs[@]}"
    ${CROSS_OBJCOPY} -S "${elf}" -O binary "${bin}"
    ${CROSS_OBJDUMP} -D "${elf}" > "${dis}"
    ${BIN2X} "${bin}" hex > "${hex}"

    if [ "${2}" = "gen_model_boot" ]; then
        ${BIN2X} "${bin}" c_array > model/boot.c
    elif [ "${2}" = "gen_rtl_boot" ]; then
        ${BIN2X} "${bin}" sv_rom_header > rtl/boot.svh
        ${BIN2X} "${bin}" sv_rom_src > rtl/boot.sv
    fi
}

function build_c_case {
    local CROSS_LD="${CROSS_PREFIX}-gcc"

    local CRT="./sdk/crt"
    local CFLAGS=(-Wall -O2 -fPIC -march=rv32i -I./sdk)

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

    ${CROSS_CC} ${CFLAGS[@]} -c -o "${output_dir}/start.o" "${CRT}/start.S"
    objs+=("${output_dir}/start.o")

    for src in ${srcs[@]}; do
        local obj="${output_dir}/$(basename ${src} .c).o"
        ${CROSS_CC} ${CFLAGS[@]} -c -o "${obj}" "${src}"
        objs+=("${obj}")
    done

    ${CROSS_LD} -T "${CRT}/soc.lds" -nostartfiles -o "${elf}" "${objs[@]}"
    ${CROSS_OBJCOPY} -S "${elf}" -O binary "${bin}"
    ${CROSS_OBJDUMP} -D "${elf}" > "${dis}"
    ${BIN2X} "${bin}" hex > "${hex}"
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
        --no-timing \
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
        build_asm_case asm_test
    fi
fi
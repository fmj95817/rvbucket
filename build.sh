#!/bin/bash

set -e

HOST_CC="gcc"

SDK_DIR="./sdk"
CRT_DIR="${SDK_DIR}/crt"
DRIVERS_DIR="${SDK_DIR}/drivers"

CROSS_PREFIX="riscv32-unknown-elf"
CROSS_CC="${CROSS_PREFIX}-gcc"
CROSS_CFLAGS=(-Wall -O2 -fPIC -march=rv32i -I${SDK_DIR})
CROSS_LDFLAGS=(-nostartfiles)
CROSS_LD="${CROSS_PREFIX}-gcc"
CROSS_OBJCOPY="${CROSS_PREFIX}-objcopy"
CROSS_OBJDUMP="${CROSS_PREFIX}-objdump"
BIN2X="./build/tools/bin2x"

function build_tools {
    mkdir -p build/tools
    ${HOST_CC} -Wall -O3 -o ${BIN2X} tools/bin2x.c -lm
}

function build_sw_case {
    local case_name="${1}"
    local case_dir="cases/${case_name}"
    local output_dir="build/sw/${case_name}"

    mkdir -p "${output_dir}"

    local lds="$(find ${case_dir} -name *.lds)"
    local elf="${output_dir}/${case_name}.elf"
    local bin="${output_dir}/${case_name}.bin"
    local dis="${output_dir}/${case_name}.S"
    local hex="${output_dir}/${case_name}.hex"

    local src_dirs=("${case_dir}")
    local ld_flags=()
    if [ -z "${lds}" ]; then
        src_dirs+=(${CRT_DIR} ${DRIVERS_DIR})
        ld_flags+=(-T "${CRT_DIR}/soc.lds")
    else
        ld_flags+=(-T "${lds}")
    fi

    local srcs=("$(find ${src_dirs[@]} -name *.S -o -name *.c)")
    local objs=()
    for src in ${srcs[@]}; do
        local obj="${output_dir}/$(basename ${src}).o"
        ${CROSS_CC} ${CROSS_CFLAGS[@]} -c -o "${obj}" "${src}"
        objs+=("${obj}")
    done

    ${CROSS_LD} ${CROSS_LDFLAGS[@]} ${ld_flags[@]} -o "${elf}" "${objs[@]}"
    ${CROSS_OBJCOPY} -S "${elf}" -O binary "${bin}"
    ${CROSS_OBJDUMP} -D "${elf}" > "${dis}"
    ${BIN2X} "${bin}" hex > "${hex}"

    if [ "${case_name}" = "boot" ]; then
        if [ "${2}" = "model" ]; then
            ${BIN2X} "${bin}" c_array > model/boot.c
        elif [ "${2}" = "rtl" ]; then
            ${BIN2X} "${bin}" sv_rom_header > rtl/boot.svh
            ${BIN2X} "${bin}" sv_rom_src > rtl/boot.sv
        fi
    fi
}

function build_model {
    mkdir -p build/hw/model
    ${HOST_CC} \
        -O3 \
        -I./model \
        -o build/hw/model/sim_top \
        $(find model -name *.c) \
        $(find sim/model -name *.c)
}

function build_rtl {
    local simulator="${1}"
    local wd="$(pwd)"

    local common_args=(
        +incdir+${wd}/rtl \
        +incdir+${wd}/rtl/core \
    )
    local rtl_sim_src=(
        $(find ${wd}/rtl -name *.sv) \
        $(find ${wd}/sim/rtl/model -name *.sv) \
        $(find ${wd}/sim/rtl/${simulator} -name *.sv) \
    )

    mkdir -p "build/hw/${simulator}"
    cd "build/hw/${simulator}";
    if [ "${simulator}" = "vcs" ]; then
        vcs \
            -full64 \
            -sverilog +v2k \
            -debug_access+r -kdb \
            -timescale=1ns/1ps \
            -o sim_top \
            -top sim_top \
            ${common_args[@]} \
            ${rtl_sim_src[@]};
    elif [ "${simulator}" = "verilator" ]; then
        verilator \
            --sc --exe --build \
            --trace --no-timing \
            --top-module sim_top \
            ${common_args[@]} \
            ${rtl_sim_src[@]} \
            $(find ${wd}/sim/rtl/verilator -name *.cc);
    fi
    cd ../../..
}

function build_sw {
    local args=(-mindepth 1 -maxdepth 1 -type d)
    local cases=("$(find cases ${args[@]})")

    for case_dir in ${cases[@]}; do
        local case_name="$(basename ${case_dir})"
        if [ "${case_name}" != "boot" ]; then
            build_sw_case "${case_name}"
        fi
    done
}

function build_hw {
    build_sw_case boot "${1}"
    if [ "${1}" = "model" ]; then
        build_model
    elif [ "${1}" = "rtl" ]; then
        build_rtl "${2}"
    fi
}

build_tools

if [ "${1}" = "hw" ]; then
    build_hw "${2}" "${3}"
elif [ "${1}" = "sw" ]; then
    build_sw "${2}"
fi
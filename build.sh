#!/bin/bash

set -e

UNAME=$(uname -s)
HOST_CC="gcc"

if [[ "${UNAME}" == "Darwin" ]]; then
    MAKE="gmake"
    SED="gsed"
else
    MAKE="make"
    SED="sed"
fi

SDK_DIR="./sdk"
CRT_DIR="${SDK_DIR}/crt"
DRIVERS_DIR="${SDK_DIR}/drivers"

CROSS_PREFIX="riscv32-unknown-elf"
CROSS_CC="${CROSS_PREFIX}-gcc"
CROSS_CFLAGS=(-Wall -O2 -fPIC -march=rv32ima_zicsr -I${SDK_DIR})
CROSS_LDFLAGS=(-nostartfiles)
CROSS_LD="${CROSS_PREFIX}-gcc"
CROSS_OBJCOPY="${CROSS_PREFIX}-objcopy"
CROSS_OBJDUMP="${CROSS_PREFIX}-objdump"

BIN2X="./build/tools/bin2x"
MKBIN="./build/tools/mkbin"
GXPR="./tools/gxpr.py"

OPEN_SBI_DIR="./thirdparty/opensbi"
LINUX_DIR="./thirdparty/linux"
BUSYBOX_DIR="./thirdparty/busybox"
LINUX_CROSS_PREFIX="riscv32-unknown-linux-gnu"
LINUX_KERNEL_LOAD="0x40000000"
LINUX_INITRD_LOAD="0x45000000"
LINUX_DTB_LOAD="0x44f00000"

function build_tools {
    mkdir -p build/tools
    ${HOST_CC} -Wall -O3 -o ${BIN2X} tools/bin2x.c -lm
    ${HOST_CC} -Wall -O3 -o ${MKBIN} tools/mkbin.c -lm
}

function build_opensbi_case {
    local case_name="opensbi"
    local output_dir="build/sw/${case_name}"

    local fw_dir="${OPEN_SBI_DIR}/build/platform/rvbucket/firmware"
    local fw_bin="${fw_dir}/fw_payload.bin"
    local fw_elf="${fw_dir}/fw_payload.elf"
    local empty_dtcm="${output_dir}/${case_name}.dtcm"
    local bin="${output_dir}/${case_name}.bin"
    local hex="${output_dir}/${case_name}.hex"

    mkdir -p "${output_dir}"

    ${MAKE} -C "${OPEN_SBI_DIR}" \
        PLATFORM=rvbucket \
        CROSS_COMPILE="${CROSS_PREFIX}-" \
        FW_PAYLOAD_OFFSET=0x50000 \
        FW_PIC=n

    cp "${fw_elf}" "${output_dir}/${case_name}.elf"
    cp "${fw_bin}" "${output_dir}/${case_name}.itcm"
    : > "${empty_dtcm}"

    ${MKBIN} "${fw_bin}" "${empty_dtcm}" "${bin}"
    ${BIN2X} "${bin}" hex > "${hex}"
}

function build_linux_opensbi {
    local clean="${1}"

    if [ "${clean}" = "clean" ]; then
        ${MAKE} -C "${OPEN_SBI_DIR}" \
            PLATFORM=rvbucket \
            CROSS_COMPILE="${CROSS_PREFIX}-" \
            FW_PIC=n \
            clean
        return
    fi

    ${MAKE} -B -C "${OPEN_SBI_DIR}" \
        PLATFORM=rvbucket \
        CROSS_COMPILE="${CROSS_PREFIX}-" \
        FW_PAYLOAD=n \
        FW_JUMP=y \
        FW_JUMP_ADDR="${LINUX_KERNEL_LOAD}" \
        FW_JUMP_FDT_ADDR="${LINUX_DTB_LOAD}" \
        FW_PIC=n
}

function build_linux_kernel {
    local clean="${1}"

    if [ "${clean}" = "clean" ]; then
        ${MAKE} -C "${LINUX_DIR}" \
            ARCH=riscv \
            CROSS_COMPILE="${LINUX_CROSS_PREFIX}-" \
            clean
        return
    fi

    local initrd="${BUSYBOX_DIR}/rootfs.cpio"
    local initrd_size=0
    if [[ "${UNAME}" == "Darwin" ]]; then
        initrd_size=$(stat -f %z "${initrd}")
    else
        initrd_size=$(stat -c %s "${initrd}")
    fi
    local initrd_end=$(printf "0x%08x" $((${LINUX_INITRD_LOAD} + initrd_size)))
    ${SED} -i "s/linux,initrd-end = <0x[0-9a-fA-F]*>;/linux,initrd-end = <${initrd_end}>;/" \
        "${LINUX_DIR}/arch/riscv/boot/dts/rvbucket/rvbucket.dts"

    if [[ "${UNAME}" == "Darwin" ]]; then
        find "${LINUX_DIR}" -name 'Makefile' -o -name '*.sh' -o -name '*.pl' \
            | xargs grep -l '\<sed\>' 2>/dev/null \
            | xargs ${SED} -i 's/\<sed\>/gsed/g' 2>/dev/null || true
    fi

    ${MAKE} -C "${LINUX_DIR}" \
        ARCH=riscv \
        CROSS_COMPILE="${LINUX_CROSS_PREFIX}-" \
        rvbucket_defconfig

    ${MAKE} -j16 -C "${LINUX_DIR}" \
        ARCH=riscv \
        CROSS_COMPILE="${LINUX_CROSS_PREFIX}-" \
        Image rvbucket/rvbucket.dtb
}

function build_linux_rootfs {
    local clean="${1}"
    local initrd="${BUSYBOX_DIR}/rootfs.cpio"

    if [ "${clean}" = "clean" ]; then
        ${MAKE} -C "${BUSYBOX_DIR}" \
            ARCH=riscv \
            CROSS_COMPILE="${LINUX_CROSS_PREFIX}-" \
            clean
        rm -rf "${BUSYBOX_DIR}/rootfs"
        rm "${initrd}"
        return
    fi

    ${SED} -i '/^config STATIC$/,/^config / { s/^\tdefault n$/\tdefault y/; }' \
        "${BUSYBOX_DIR}/Config.in"

    ${MAKE} -C "${BUSYBOX_DIR}" \
        ARCH=riscv \
        CROSS_COMPILE="${LINUX_CROSS_PREFIX}-" \
        defconfig
    ${MAKE} -j16 -C "${BUSYBOX_DIR}" \
        ARCH=riscv \
        CROSS_COMPILE="${LINUX_CROSS_PREFIX}-"
    ${MAKE} -j16 -C "${BUSYBOX_DIR}" \
        ARCH=riscv \
        CROSS_COMPILE="${LINUX_CROSS_PREFIX}-" \
        install
    mkdir -p ${BUSYBOX_DIR}/rootfs/{bin,sbin,etc,proc,sys,usr/bin,usr/sbin}
    cp -a "${BUSYBOX_DIR}/_install"/* "${BUSYBOX_DIR}/rootfs/"

    echo "#!/bin/sh" > "${BUSYBOX_DIR}/rootfs/init"
    echo "mount -t proc none /proc" >> "${BUSYBOX_DIR}/rootfs/init"
    echo "mount -t sysfs none /sys" >> "${BUSYBOX_DIR}/rootfs/init"
    echo 'echo -e "\nWelcome to RISC-V 32-bit Linux!\n"' >> "${BUSYBOX_DIR}/rootfs/init"
    echo "exec /bin/sh" >> "${BUSYBOX_DIR}/rootfs/init"
    chmod +x "${BUSYBOX_DIR}/rootfs/init"

    cd "${BUSYBOX_DIR}/rootfs"
    find . | cpio -H newc -o > ../rootfs.cpio
    cd -
}

function build_linux_case {
    local clean="${1}"

    if [ "${clean}" = "clean" ]; then
        build_linux_opensbi clean
        build_linux_kernel clean
        build_linux_rootfs clean
        echo "build_sw: linux clean done."
        return
    fi

    local case_name="linux"
    local output_dir="build/sw/${case_name}"

    local fw_dir="${OPEN_SBI_DIR}/build/platform/rvbucket/firmware"
    local fw_bin="${fw_dir}/fw_jump.bin"
    local fw_elf="${fw_dir}/fw_jump.elf"
    local kernel="${LINUX_DIR}/arch/riscv/boot/Image"
    local initrd="${BUSYBOX_DIR}/rootfs.cpio"
    local dtb="${LINUX_DIR}/arch/riscv/boot/dts/rvbucket/rvbucket.dtb"
    local empty_dtcm="${output_dir}/${case_name}.dtcm"
    local bin="${output_dir}/${case_name}.bin"
    local hex="${output_dir}/${case_name}.hex"

    mkdir -p "${output_dir}"

    build_linux_rootfs
    build_linux_kernel
    build_linux_opensbi

    cp "${fw_elf}" "${output_dir}/${case_name}.elf"
    cp "${fw_bin}" "${output_dir}/${case_name}.itcm"
    cp "${dtb}" "${output_dir}/${case_name}.dtb"
    : > "${empty_dtcm}"

    ${MKBIN} --linux "${fw_bin}" "${empty_dtcm}" \
        "${kernel}" "${initrd}" "${dtb}" "${bin}" \
        "${LINUX_KERNEL_LOAD}" "${LINUX_INITRD_LOAD}" "${LINUX_DTB_LOAD}"
    ${BIN2X} "${bin}" hex > "${hex}"
}

function build_sw_case {
    local case_name="${1}"
    if [ "${case_name}" = "opensbi" ]; then
        build_opensbi_case
        return
    fi
    if [ "${case_name}" = "linux" ]; then
        build_linux_case "${2}"
        return
    fi

    echo "  compiling sw/${case_name} ..."

    local case_dir="cases/${case_name}"
    local output_dir="build/sw/${case_name}"

    mkdir -p "${output_dir}"

    local lds="$(find ${case_dir} -name *.lds)"
    local elf="${output_dir}/${case_name}.elf"
    local itcm="${output_dir}/${case_name}.itcm"
    local dtcm="${output_dir}/${case_name}.dtcm"
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
    ${CROSS_OBJCOPY} -S "${elf}" -O binary \
        --only-section='.text*' \
        "${itcm}"
    ${CROSS_OBJCOPY} -S "${elf}" -O binary \
        --only-section='.*data*' \
        --only-section='.*bss*' \
        --only-section='.got*' \
        --only-section='.gnu.linkonce.*' \
        "${dtcm}"
    ${CROSS_OBJDUMP} -D "${elf}" > "${dis}"

    ${MKBIN} "${itcm}" "${dtcm}" "${bin}"
    ${BIN2X} "${bin}" hex > "${hex}"

    if [ "${case_name}" = "boot" ]; then
        local bootrom="${output_dir}/${case_name}.bootrom"
        ${CROSS_OBJCOPY} -S "${elf}" -O binary "${bootrom}"
        if [ "${2}" = "model" ]; then
            ${BIN2X} "${bootrom}" c_array > design/model/core/boot.c
        elif [ "${2}" = "rtl" ]; then
            ${BIN2X} "${bootrom}" sv_rom_header > design/rtl/core/boot.svh
            ${BIN2X} "${bootrom}" sv_rom_src > design/rtl/core/boot.sv
        fi
    fi
}

function build_model {
    mkdir -p build/hw/model
    ${HOST_CC} \
        -Wall \
        -O3 \
        -pthread \
        -I./base/model \
        -I./design/model \
        -o build/hw/model/sim_top \
        $(find base/model -name *.c) \
        $(find design/model -name *.c) \
        $(find sim/model -name *.c)
}

function build_ut {
    local target="${1}"
    local name="${2}"

    if [ "${target}" = "rtl" ]; then
        echo "build_ut: RTL UT not yet implemented"
        return
    fi

    local ut_root="ut/model"
    local find_args=("${ut_root}" -name '*.c' ! -name 'utils.c')
    if [ -n "${name}" ]; then
        if [ -d "${ut_root}/${name}" ]; then
            find_args=("${ut_root}/${name}" -name '*.c' ! -name 'utils.c')
        else
            find_args=("${ut_root}" -name "${name}.c" ! -name 'utils.c')
        fi
    fi

    local ut_srcs=($(find "${find_args[@]}"))

    if [ ${#ut_srcs[@]} -eq 0 ]; then
        echo "build_ut: no UT sources found"
        return
    fi

    for ut_src in ${ut_srcs[@]}; do
        local rel_path="${ut_src#${ut_root}/}"
        local ut_name="${rel_path%.c}"
        local ut_dir="build/hw/model/ut/$(dirname ${ut_name})"
        local ut_bin="build/hw/model/ut/${ut_name}_ut"
        mkdir -p "${ut_dir}"
        echo "  compiling ut/${ut_name} ..."
        ${HOST_CC} \
            -Wall \
            -O3 \
            -I./base/model \
            -I./design/model \
            -I./ut/model \
            -o "${ut_bin}" \
            "${ut_src}" \
            ut/model/utils.c \
            $(find base/model -name *.c) \
            $(find design/model -name *.c)
    done
    echo "build_ut: ${#ut_srcs[@]} UT(s) built."
}

function build_rtl {
    local simulator="${1}"
    local wd="$(pwd)"

    local common_args=(
        +incdir+${wd}/base/rtl \
        +incdir+${wd}/design/rtl \
        +incdir+${wd}/design/rtl/core \
    )
    local rtl_sim_src=(
        $(find ${wd}/base/rtl -name *.sv) \
        $(find ${wd}/design/rtl -name *.sv) \
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
            -CFLAGS '-std=gnu++17' \
            ${common_args[@]} \
            ${rtl_sim_src[@]} \
            $(find ${wd}/sim/rtl/verilator -name *.cc);
    fi
    cd ../../..
}

function build_fpga {
    local vendor="${1}"
    if [ "${vendor}" = "xilinx" ]; then
        local conf_path="./fpga/xilinx/proj.json"
        local out_dir="build/hw/vivado"
        rm -rf "${out_dir}"
        mkdir -p "${out_dir}"
        ${GXPR} "${conf_path}" "${out_dir}"
        cd "${out_dir}";
        vivado -mode batch -source mkprj.tcl;
        cd ../../..
    fi
}

function build_sw {
    if [ -n "${1}" ]; then
        if [ "${1}" != "boot" ]; then
            build_sw_case "${1}" "${2}"
        fi
        return
    fi

    local args=(-mindepth 1 -maxdepth 1 -type d)
    local cases=("$(find cases ${args[@]})")
    local count=0

    for case_dir in ${cases[@]}; do
        local case_name="$(basename ${case_dir})"
        if [ "${case_name}" != "boot" ]; then
            build_sw_case "${case_name}"
            count=$((count + 1))
        fi
    done
    echo "build_sw: ${count} case(s) built."
}

function build_hw {
    if [ "${1}" = "model" ]; then
        build_sw_case boot "${1}"
        build_model
    elif [ "${1}" = "rtl" ]; then
        build_sw_case boot "${1}"
        build_rtl "${2}"
    elif [ "${1}" = "fpga" ]; then
        build_fpga "${2}"
    fi
}

build_tools

if [ "${1}" = "hw" ]; then
    build_hw "${2}" "${3}"
elif [ "${1}" = "sw" ]; then
    build_sw "${2}" "${3}"
elif [ "${1}" = "ut" ]; then
    build_ut "${2}" "${3}"
fi

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
BARE_CASES_DIR="./cases/bare"
USER_CASES_DIR="./cases/user"

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
GXQSPI="./tools/gxqspi.py"

OPEN_SBI_DIR="./thirdparty/opensbi"
LINUX_DIR="./thirdparty/linux"
BUSYBOX_DIR="./thirdparty/busybox"
LINUX_CROSS_PREFIX="riscv32-unknown-linux-gnu"
LINUX_CC="${LINUX_CROSS_PREFIX}-gcc"
LINUX_USER_CFLAGS=(-Wall -Os -static -s -ffunction-sections -fdata-sections -Wl,--gc-sections)
LINUX_BUILD_DIR="./build/sw/linux"
LINUX_USER_BIN_DIR="${LINUX_BUILD_DIR}/bin"
# RV32 Linux requires the kernel image to be PMD-aligned (4MB).
LINUX_KERNEL_LOAD="0x40400000"
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
    local firmware="${output_dir}/${case_name}.fw"
    local bin="${output_dir}/${case_name}.bin"
    local hex="${output_dir}/${case_name}.hex"

    mkdir -p "${output_dir}"

    ${MAKE} -B -C "${OPEN_SBI_DIR}" \
        PLATFORM=rvbucket \
        CROSS_COMPILE="${CROSS_PREFIX}-" \
        FW_PAYLOAD_OFFSET=0x50000 \
        FW_PIC=n

    cp "${fw_elf}" "${output_dir}/${case_name}.elf"
    cp "${fw_bin}" "${firmware}"

    ${MKBIN} "${firmware}" "${bin}"
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
        rm -f "${initrd}"
        clean_linux_user_tests
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
    build_user_tests
    cp -a "${LINUX_USER_BIN_DIR}"/* "${BUSYBOX_DIR}/rootfs/bin/"

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

function build_user_tests {
    rm -rf "${LINUX_USER_BIN_DIR}"
    mkdir -p "${LINUX_USER_BIN_DIR}"

    local case_dirs=($(find "${USER_CASES_DIR}" -mindepth 1 -maxdepth 1 -type d | sort))
    local count=0
    for case_dir in "${case_dirs[@]}"; do
        local case_name="$(basename "${case_dir}")"
        local out="${LINUX_USER_BIN_DIR}/${case_name}"
        local scripts=($(find "${case_dir}" -maxdepth 1 -name '*.sh' -type f | sort))
        local srcs=($(find "${case_dir}" -name '*.c' | sort))

        if [ ${#scripts[@]} -ne 0 ]; then
            echo "  packing user/${case_name} ..."
            cp -a "${case_dir}"/* "${LINUX_USER_BIN_DIR}/"
            chmod +x "${LINUX_USER_BIN_DIR}"/*.sh
        elif [ ${#srcs[@]} -ne 0 ]; then
            echo "  compiling user/${case_name} ..."
            ${LINUX_CC} ${LINUX_USER_CFLAGS[@]} -o "${out}" "${srcs[@]}"
        else
            echo "build_sw: user case has no C sources or shell scripts: ${case_name}"
            exit 1
        fi
        count=$((count + 1))
    done
    echo "build_sw: ${count} user case(s) built."
}

function clean_linux_user_tests {
    rm -rf "${LINUX_USER_BIN_DIR}"
    echo "build_sw: linux user tests clean done."
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

    if [ -n "${clean}" ]; then
        echo "usage: $0 sw <sim|board> linux [clean]"
        exit 1
    fi

    local case_name="linux"
    local output_dir="${LINUX_BUILD_DIR}"

    local fw_dir="${OPEN_SBI_DIR}/build/platform/rvbucket/firmware"
    local fw_bin="${fw_dir}/fw_jump.bin"
    local fw_elf="${fw_dir}/fw_jump.elf"
    local kernel="${LINUX_DIR}/arch/riscv/boot/Image"
    local initrd="${BUSYBOX_DIR}/rootfs.cpio"
    local dtb="${LINUX_DIR}/arch/riscv/boot/dts/rvbucket/rvbucket.dtb"
    local firmware="${output_dir}/${case_name}.fw"
    local bin="${output_dir}/${case_name}.bin"
    local hex="${output_dir}/${case_name}.hex"

    mkdir -p "${output_dir}"

    build_linux_rootfs
    build_linux_kernel
    build_linux_opensbi

    cp "${fw_elf}" "${output_dir}/${case_name}.elf"
    cp "${fw_bin}" "${firmware}"
    cp "${dtb}" "${output_dir}/${case_name}.dtb"

    ${MKBIN} --linux "${firmware}" \
        "${kernel}" "${initrd}" "${dtb}" "${bin}" \
        "${LINUX_KERNEL_LOAD}" "${LINUX_INITRD_LOAD}" "${LINUX_DTB_LOAD}"
    ${BIN2X} "${bin}" hex > "${hex}"
}

function build_sw_case {
    local case_name="${1}"
    local profile="${2:-sim}"
    local target="${3}"

    if [ "${profile}" != "sim" ] && [ "${profile}" != "board" ]; then
        echo "build_sw_case: invalid sw profile: ${profile}"
        exit 1
    fi

    if [ "${case_name}" = "opensbi" ]; then
        build_opensbi_case
        return
    fi
    if [ "${case_name}" = "linux" ]; then
        build_linux_case "${target}"
        return
    fi

    echo "  compiling sw/${case_name} ..."

    local case_dir="${BARE_CASES_DIR}/${case_name}"
    local output_dir="build/sw/${profile}/${case_name}"

    mkdir -p "${output_dir}"

    local lds="$(find ${case_dir} -name *.lds)"
    local elf="${output_dir}/${case_name}.elf"
    local firmware="${output_dir}/${case_name}.fw"
    local bin="${output_dir}/${case_name}.bin"
    local dis="${output_dir}/${case_name}.S"
    local hex="${output_dir}/${case_name}.hex"

    local src_dirs=("${case_dir}")
    local ld_flags=()
    local cc_flags=("${CROSS_CFLAGS[@]}")
    if [ "${profile}" = "sim" ]; then
        cc_flags+=(-DRVB_SIM)
    fi
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
        ${CROSS_CC} ${cc_flags[@]} -c -o "${obj}" "${src}"
        objs+=("${obj}")
    done

    ${CROSS_LD} ${CROSS_LDFLAGS[@]} ${ld_flags[@]} -o "${elf}" "${objs[@]}"
    ${CROSS_OBJCOPY} -S "${elf}" -O binary "${firmware}"
    ${CROSS_OBJDUMP} -D "${elf}" > "${dis}"

    ${MKBIN} "${firmware}" "${bin}"
    ${BIN2X} "${bin}" hex > "${hex}"

    if [ "${case_name}" = "boot" ]; then
        local bootrom="${output_dir}/${case_name}.bootrom"
        ${CROSS_OBJCOPY} -S "${elf}" -O binary "${bootrom}"
        if [ "${target}" = "model" ]; then
            ${BIN2X} "${bootrom}" c_array > design/model/core/boot.c
        elif [ "${target}" = "rtl" ]; then
            mkdir -p sim/rtl
            ${BIN2X} "${bootrom}" sv_rom_header > sim/rtl/boot.svh
            ${BIN2X} "${bootrom}" sv_rom_src > sim/rtl/boot.sv
        elif [ "${target}" = "fpga" ]; then
            mkdir -p fpga/xilinx/rtl
            ${BIN2X} "${bootrom}" sv_rom_header > fpga/xilinx/rtl/boot.svh
            ${BIN2X} "${bootrom}" sv_rom_src > fpga/xilinx/rtl/boot.sv
        elif [ "${target}" = "asic" ]; then
            mkdir -p sim/rtl
            ${BIN2X} "${bootrom}" sv_rom_header > sim/rtl/boot.svh
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

    if [ "${target}" = "model" ] || [ -z "${target}" ]; then
        build_model_ut "${name}"
    elif [ "${target}" = "rtl" ]; then
        build_rtl_ut "${name}"
    else
        echo "usage: $0 ut [model|rtl] [name]"
        exit 1
    fi
}

function build_model_ut {
    local name="${1}"

    local ut_root="ut/model"
    local find_args=("${ut_root}" -name '*.c' ! -name 'utils.c')
    if [ -n "${name}" ]; then
        if [ -d "${ut_root}/${name}" ]; then
            find_args=("${ut_root}/${name}" -name '*.c' ! -name 'utils.c')
        elif [ -f "${ut_root}/${name}.c" ]; then
            find_args=("${ut_root}" -path "${ut_root}/${name}.c" ! -name 'utils.c')
        else
            find_args=("${ut_root}/${name}" -name '*.c' ! -name 'utils.c')
        fi
    fi

    local ut_srcs=($(find "${find_args[@]}" | sort))

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
            -pthread \
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

function build_rtl_ut {
    local name="${1}"
    local wd="$(pwd)"
    local ut_root="ut/rtl"

    local find_args=("${ut_root}" -name '*_tb.sv')
    if [ -n "${name}" ]; then
        if [ -d "${ut_root}/${name}" ]; then
            find_args=("${ut_root}/${name}" -name '*_tb.sv')
        elif [ -f "${ut_root}/${name}_tb.sv" ]; then
            find_args=("${ut_root}" -path "${ut_root}/${name}_tb.sv")
        else
            find_args=("${ut_root}/${name}" -name '*_tb.sv')
        fi
    fi

    local ut_srcs=($(find "${find_args[@]}" | sort))
    if [ ${#ut_srcs[@]} -eq 0 ]; then
        echo "build_ut: no RTL UT sources found"
        return
    fi

    local common_args=(
        +incdir+${wd}
        +incdir+${wd}/sim/rtl
        +incdir+${wd}/base/rtl
        +incdir+${wd}/design/rtl
        +incdir+${wd}/design/rtl/core
    )
    local rtl_src=(
        $(find ${wd}/base/rtl -name '*.sv')
        $(find ${wd}/design/rtl -name '*.sv')
    )

    for ut_src in ${ut_srcs[@]}; do
        local rel_path="${ut_src#ut/rtl/}"
        local ut_name="${rel_path%.sv}"
        local ut_base="$(basename "${ut_name}")"
        local top="${ut_base}"
        local ut_dir="build/hw/rtl/ut/$(dirname "${ut_name}")"
        local ut_common_args=(
            "${common_args[@]}"
            +define+RVB_NO_ITF_TRACE
            +define+RVB_ITF_CHECKER_TOP=${top}
        )
        mkdir -p "${ut_dir}"
        echo "  compiling rtl/ut/${ut_name} ..."

        (
            cd "${ut_dir}"
            vcs \
                -full64 \
                -sverilog +v2k \
                -timescale=1ns/1ps \
                -o "${ut_base}_ut" \
                -top "${top}" \
                "${ut_common_args[@]}" \
                "${rtl_src[@]}" \
                "${wd}/${ut_src}"
        )
    done
    echo "build_ut: ${#ut_srcs[@]} RTL UT(s) built."
}

function build_rtl {
    local simulator="${1}"
    local mode="${2}"
    local wd="$(pwd)"

    local common_args=(
        +incdir+${wd} \
        +incdir+${wd}/sim/rtl \
        +incdir+${wd}/base/rtl \
        +incdir+${wd}/design/rtl \
        +incdir+${wd}/design/rtl/core \
    )
    local rtl_sim_src=(
        $(find ${wd}/base/rtl -name '*.sv') \
        $(find ${wd}/design/rtl -name '*.sv') \
        ${wd}/sim/rtl/boot.sv \
        $(find ${wd}/sim/rtl/model -name '*.sv') \
        $(find ${wd}/sim/rtl/${simulator} -name '*.sv') \
    )
    local rtl_common_c_src=($(find ${wd}/sim/rtl/common -name '*.c'))

    mkdir -p "build/hw/${simulator}"
    cd "build/hw/${simulator}";
    if [ "${simulator}" = "vcs" ]; then
        local vcs_debug_args=()
        if [ "${mode}" = "debug" ]; then
            vcs_debug_args=(-debug_access+r -kdb +define+FSDB)
        fi
        vcs \
            -full64 \
            -sverilog +v2k \
            -timescale=1ns/1ps \
            -o sim_top \
            -top sim_top \
            ${vcs_debug_args[@]} \
            ${common_args[@]} \
            ${rtl_sim_src[@]} \
            ${rtl_common_c_src[@]};
    elif [ "${simulator}" = "verilator" ]; then
        verilator \
            --sc --exe --build \
            --trace --no-timing \
            --top-module sim_top \
            -CFLAGS '-std=gnu++17' \
            ${common_args[@]} \
            ${rtl_sim_src[@]} \
            ${rtl_common_c_src[@]} \
            ${wd}/sim/rtl/verilator/sim_top.cc;
    fi
    cd ../../..
}

function build_fpga {
    local vendor="${1}"
    local wd="$(pwd)"
    if [ "${vendor}" = "xilinx" ]; then
        local flow="${2:-ip}"
        local conf_path="${3:-./fpga/xilinx/proj.json}"
        local out_dir="build/hw/fpga/xilinx"
        if [ "${flow}" = "qspi" ] || [ "${flow}" = "qspi-program" ]; then
            local image="${4:-build/sw/linux/linux.bin}"
            mkdir -p "${out_dir}/logs"
            local program_args=()
            if [ "${flow}" = "qspi-program" ]; then
                program_args+=(--program)
            fi
            ${GXQSPI} --conf "${conf_path}" --image "${image}" \
                --out-dir "${out_dir}" "${program_args[@]}"
            return
        fi
        rm -rf "${out_dir}"
        mkdir -p "${out_dir}/logs"
        ${GXPR} "${conf_path}" "${out_dir}" --flow "${flow}"
        if [ -f /etc/profile.d/edaenv.sh ]; then
            source /etc/profile.d/edaenv.sh
        fi
        cd "${out_dir}";
        vivado -mode batch -source mkprj.tcl | tee "logs/${flow}.log";
        local vivado_rc=${PIPESTATUS[0]}
        cd "${wd}"
        return ${vivado_rc}
    else
        echo "usage: $0 hw fpga xilinx [project|ip|syn|impl|bit|qspi|qspi-program] [conf_json] [image]"
        exit 1
    fi
}

function build_asic {
    local flow="${1}"
    local conf="${2:-soc_cmos28lp}"
    local wd="$(pwd)"

    if [ "${flow}" = "syn" ]; then
        local conf_json=""
        if [ -f "${conf}" ]; then
            conf_json="${conf}"
        else
            conf_json="asic/conf/${conf}.json"
        fi
        if [ ! -f "${conf_json}" ]; then
            echo "build_asic: ASIC config not found: ${conf_json}"
            exit 1
        fi
        local conf_name="$(basename "${conf_json}" .json)"

        if [ -f /etc/profile.d/edaenv.sh ]; then
            source /etc/profile.d/edaenv.sh
        fi
        mkdir -p build/hw/asic/syn/logs
        python3 tools/asic_config.py \
            --root "${wd}" \
            --rtl-file "${wd}/build/hw/asic/syn/${conf_name}.rtl.f" \
            "${conf_json}" > "build/hw/asic/syn/${conf_name}.config.tcl"
        echo "build_asic: using ASIC config ${conf_json}"
        cd build/hw/asic/syn
        RVBUCKET_ROOT="${wd}" \
        RVBUCKET_ASIC_CONFIG_TCL="${wd}/build/hw/asic/syn/${conf_name}.config.tcl" \
            dc_shell \
            -f "${wd}/asic/syn/synth.tcl" \
            | tee "logs/${conf_name}.log"
        cd "${wd}"
    else
        echo "usage: $0 hw asic syn [conf_name|conf_json]"
        exit 1
    fi
}

function build_sw {
    local profile="${1:-sim}"
    local case_name="${2}"
    local case_arg="${3}"

    if [ "${profile}" != "sim" ] && [ "${profile}" != "board" ]; then
        echo "usage: $0 sw [sim|board] [case_name] [case_arg]"
        exit 1
    fi

    if [ -n "${case_name}" ]; then
        if [ "${case_name}" != "boot" ]; then
            build_sw_case "${case_name}" "${profile}" "${case_arg}"
        fi
        return
    fi

    local args=(-mindepth 1 -maxdepth 1 -type d)
    local cases=("$(find "${BARE_CASES_DIR}" ${args[@]})")
    local count=0

    for case_dir in ${cases[@]}; do
        local case_name="$(basename ${case_dir})"
        if [ "${case_name}" != "boot" ]; then
            build_sw_case "${case_name}" "${profile}"
            count=$((count + 1))
        fi
    done
    echo "build_sw: ${count} case(s) built."
}

function build_hw {
    local target="${1}"
    local boot_profile="sim"
    if [ "${target}" = "fpga" ] || [ "${target}" = "asic" ]; then
        boot_profile="board"
    fi

    build_sw_case boot "${boot_profile}" "${target}"
    if [ "${target}" = "model" ]; then
        build_model "${@:2}"
    elif [ "${target}" = "rtl" ]; then
        build_rtl "${@:2}"
    elif [ "${target}" = "fpga" ]; then
        build_fpga "${@:2}"
    elif [ "${target}" = "asic" ]; then
        build_asic "${@:2}"
    fi
}

build_tools

if [ "${1}" = "hw" ]; then
    build_hw "${@:2}"
elif [ "${1}" = "sw" ]; then
    build_sw "${@:2}"
elif [ "${1}" = "ut" ]; then
    build_ut "${@:2}"
fi

#!/bin/bash

set -e

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${ROOT}/build"
SIM_SW_DIR="${BUILD_DIR}/sw/sim"

PASS=0
FAIL=0
SKIP=0
FAIL_LOG=""

RED='\033[0;31m'
GRN='\033[0;32m'
YLW='\033[1;33m'
CYN='\033[0;36m'
RST='\033[0m'

SW_EXCLUDE=(
    boot
    test
    calc
    gpio_test
)

RTL_SW_TIMEOUT="${RTL_SW_TIMEOUT:-300}"
RTL_UT_TIMEOUT="${RTL_UT_TIMEOUT:-60}"
MODEL_SW_TIMEOUT="${MODEL_SW_TIMEOUT:-60}"
MODEL_SIM_ARGS=()

function print_usage {
    cat <<EOF
usage: $0 [model] [sw|ut] [<name>] [--model-flag ...]
       $0 [model] [sw|ut] [<name>] -- <sim_top args>
       $0 rtl <vcs|verilator> [sw] [<name>]
       $0 rtl ut [<name>]

Options:
  -h, --help  Show this help

Any other --argument is passed through to model sim_top for model SW runs.
Use -- before sim_top arguments that take values.

Environment:
  MODEL_SW_TIMEOUT           Timeout for each model SW case, default 60s
  RTL_SW_TIMEOUT             Timeout for each RTL SW case, default 300s
  RTL_UT_TIMEOUT             Timeout for each RTL UT, default 60s
EOF
}

function pass {
    printf "  ${GRN}PASS${RST}  %s\n" "$1"
    PASS=$((PASS + 1))
}

function fail {
    printf "  ${RED}FAIL${RST}  %s  %s\n" "$1" "$2"
    FAIL=$((FAIL + 1))
    FAIL_LOG+="  $1  $2"$'\n'
}

function skip {
    printf "  ${YLW}SKIP${RST}  %s  %s\n" "$1" "$2"
    SKIP=$((SKIP + 1))
}

function is_executable_file {
    [[ -f "$1" && -x "$1" ]]
}

function run_with_timeout {
    local seconds="$1"
    shift

    if command -v timeout >/dev/null 2>&1; then
        timeout "${seconds}" "$@"
        return $?
    fi

    if command -v gtimeout >/dev/null 2>&1; then
        gtimeout "${seconds}" "$@"
        return $?
    fi

    "$@" &
    local pid=$!

    (
        sleep "${seconds}"
        kill -TERM "${pid}" 2>/dev/null || true
        sleep 2
        kill -KILL "${pid}" 2>/dev/null || true
    ) &
    local watchdog=$!

    wait "${pid}"
    local status=$?

    kill "${watchdog}" 2>/dev/null || true
    wait "${watchdog}" 2>/dev/null || true

    return "${status}"
}

function get_sw_case_input {
    local name="${1}"

    SW_CASE_INPUT=""
    case "${name}" in
        calc) SW_CASE_INPUT=$'1+2*3\n' ;;
    esac
}

function sw_case_uses_sd_image {
    [[ "$1" == sdspi* ]]
}

function find_ut_bins {
    local ut_dir="$1"

    [[ -d "${ut_dir}" ]] || return 0

    while IFS= read -r -d '' bin; do
        if is_executable_file "${bin}" && [[ "${bin##*/}" == *_ut ]]; then
            printf '%s\0' "${bin}"
        fi
    done < <(find "${ut_dir}" -type f -name '*_ut' -print0 2>/dev/null)
}

function run_single_sw {
    local name="${1}"
    local bin="${SIM_SW_DIR}/${name}/${name}.bin"
    local input=""

    if [[ ! -f "${bin}" ]]; then
        echo "error: binary not found: ${bin}"
        exit 1
    fi

    local cwd="${BUILD_DIR}/hw/model"
    local rel_bin="../../sw/sim/${name}/${name}.bin"
    local case_sim_args=("${MODEL_SIM_ARGS[@]}")
    if sw_case_uses_sd_image "${name}"; then
        local sd_image="${BUILD_DIR}/hw/model/sdspi-bare.img"
        truncate -s 8M "${sd_image}"
        case_sim_args+=(--sd-image "${sd_image}")
    fi

    echo "  cd ${cwd} && ./sim_top ${case_sim_args[*]} ${rel_bin}"
    get_sw_case_input "${name}"
    input="${SW_CASE_INPUT}"
    if [[ -n "${input}" ]]; then
        cd "${cwd}" && printf '%s' "${input}" |
            "${cwd}/sim_top" "${case_sim_args[@]}" "${rel_bin}"
    else
        cd "${cwd}" && "${cwd}/sim_top" "${case_sim_args[@]}" "${rel_bin}"
    fi
}

# ── batch regression ──────────────────────────────────────────────────────

function run_sw_cases {
    local sw_dir="${SIM_SW_DIR}"
    local sim="${BUILD_DIR}/hw/model/sim_top"
    local timeout="${MODEL_SW_TIMEOUT}"

    if [[ ${#MODEL_SIM_ARGS[@]} -ne 0 ]]; then
        printf "${CYN}=== bare-metal C cases (%s) ===${RST}\n" \
            "${MODEL_SIM_ARGS[*]}"
    else
        printf "${CYN}=== bare-metal C cases ===${RST}\n"
    fi

    if [[ ! -x "${sim}" ]]; then
        skip "all SW cases" "simulator not found: ${sim}"
        return
    fi

    for case_dir in "${ROOT}"/cases/bare/*/; do
        [[ -d "${case_dir}" ]] || continue

        local name="$(basename "${case_dir}")"
        local bin="${sw_dir}/${name}/${name}.bin"
        local excluded=false

        for ex in "${SW_EXCLUDE[@]}"; do
            if [[ "${name}" == "${ex}" ]]; then
                skip "${name}" "excluded"
                excluded=true
                break
            fi
        done
        [[ "${excluded}" == true ]] && continue

        if [[ ! -f "${bin}" ]]; then
            skip "${name}" "binary not found"
            continue
        fi

        local input=""
        local status=0
        local case_sim_args=("${MODEL_SIM_ARGS[@]}")

        if sw_case_uses_sd_image "${name}"; then
            local sd_image="${BUILD_DIR}/hw/model/sdspi-bare.img"
            truncate -s 8M "${sd_image}"
            case_sim_args+=(--sd-image "${sd_image}")
        fi

        get_sw_case_input "${name}"
        input="${SW_CASE_INPUT}"
        if [[ -n "${input}" ]]; then
            if printf '%s' "${input}" |
                run_with_timeout "${timeout}" "${sim}" \
                "${case_sim_args[@]}" "${bin}" >/dev/null 2>&1; then
                status=0
            else
                status=$?
            fi
        else
            if run_with_timeout "${timeout}" "${sim}" \
                "${case_sim_args[@]}" "${bin}" </dev/null >/dev/null 2>&1; then
                status=0
            else
                status=$?
            fi
        fi

        if [[ ${status} -eq 0 ]]; then
            pass "${name}"
        else
            fail "${name}" "exit=${status}"
        fi
    done
}

function find_model_ut_bins {
    local name="${1}"
    local ut_dir="${BUILD_DIR}/hw/model/ut"
    local src_root="${ROOT}/ut/model"
    local find_root="${ut_dir}"
    local exact_bin=""

    [[ -d "${ut_dir}" ]] || return 0

    if [[ -n "${name}" ]]; then
        if [[ -d "${ut_dir}/${name}" ]]; then
            find_root="${ut_dir}/${name}"
        elif [[ -x "${ut_dir}/${name}_ut" ]]; then
            exact_bin="${ut_dir}/${name}_ut"
        else
            find_root="${ut_dir}/${name}"
        fi
    fi

    if [[ -n "${exact_bin}" ]]; then
        local rel="${exact_bin#${ut_dir}/}"
        rel="${rel%_ut}"
        [[ -f "${src_root}/${rel}.c" ]] && printf '%s\0' "${exact_bin}"
    elif [[ -d "${find_root}" ]]; then
        while IFS= read -r -d '' bin; do
            local rel="${bin#${ut_dir}/}"
            rel="${rel%_ut}"
            [[ -f "${src_root}/${rel}.c" ]] && printf '%s\0' "${bin}"
        done < <(find "${find_root}" -type f -name '*_ut' -perm -111 -print0 2>/dev/null)
    fi
}

function run_ut_tests {
    local name="${1}"
    local ut_dir="${BUILD_DIR}/hw/model/ut"
    printf "${CYN}=== module unit tests ===${RST}\n"

    local ut_bins=()
    while IFS= read -r -d '' bin; do
        ut_bins+=("${bin}")
    done < <(find_model_ut_bins "${name}")

    if [[ ${#ut_bins[@]} -eq 0 ]]; then
        skip "all UTs" "no UT binaries found (run './build.sh ut model' first)"
        return
    fi

    for bin in "${ut_bins[@]}"; do
        local rel="${bin#${ut_dir}/}"
        local label="${rel%_ut}"
        local ut_cwd="$(dirname "${bin}")"
        local ut_name="$(basename "${bin}")"

        if (cd "${ut_cwd}" && "./${ut_name}" >/dev/null 2>&1); then
            pass "${label}"
        else
            fail "${label}" "exit=${?}"
        fi
    done
}

function run_rtl_cases {
    local simulator="${1}"
    local case_name="${2}"
    local cwd="${BUILD_DIR}/hw/${simulator}"
    local sim_cmd=""

    printf "${CYN}=== RTL regression (${simulator}) ===${RST}\n"

    case "${simulator}" in
        verilator) sim_cmd="./obj_dir/Vsim_top" ;;
        vcs) sim_cmd="./sim_top" ;;
        *)
            fail "rtl/${simulator}" "unknown simulator"
            return
            ;;
    esac

    if [[ ! -x "${cwd}/${sim_cmd#./}" ]]; then
        skip "rtl/${simulator}" "simulator not found: ${cwd}/${sim_cmd#./}"
        return
    fi

    mkdir -p "${BUILD_DIR}/logs/rtl/${simulator}"

    if [[ -n "${case_name}" ]]; then
        run_one_rtl_case "${simulator}" "${sim_cmd}" "${case_name}" false
        return
    fi

    for case_dir in "${ROOT}"/cases/bare/*/; do
        [[ -d "${case_dir}" ]] || continue

        local name="$(basename "${case_dir}")"
        local excluded=false

        for ex in "${SW_EXCLUDE[@]}"; do
            if [[ "${name}" == "${ex}" ]]; then
                skip "${name}" "excluded"
                excluded=true
                break
            fi
        done
        [[ "${excluded}" == true ]] && continue

        run_one_rtl_case "${simulator}" "${sim_cmd}" "${name}" true
    done
}

function find_rtl_ut_bins {
    local name="${1}"
    local ut_dir="${BUILD_DIR}/hw/rtl/ut"
    local src_root="${ROOT}/ut/rtl"
    local find_root="${ut_dir}"
    local exact_bin=""

    [[ -d "${ut_dir}" ]] || return 0

    if [[ -n "${name}" ]]; then
        if [[ -d "${ut_dir}/${name}" ]]; then
            find_root="${ut_dir}/${name}"
        elif [[ -x "${ut_dir}/${name}_tb_ut" ]]; then
            exact_bin="${ut_dir}/${name}_tb_ut"
        else
            find_root="${ut_dir}/${name}"
        fi
    fi

    if [[ -n "${exact_bin}" ]]; then
        local rel="${exact_bin#${ut_dir}/}"
        rel="${rel%_ut}"
        [[ -f "${src_root}/${rel}.sv" ]] && printf '%s\0' "${exact_bin}"
    elif [[ -d "${find_root}" ]]; then
        while IFS= read -r -d '' bin; do
            local rel="${bin#${ut_dir}/}"
            rel="${rel%_ut}"
            [[ -f "${src_root}/${rel}.sv" ]] && printf '%s\0' "${bin}"
        done < <(find "${find_root}" -type f -name '*_tb_ut' -perm -111 -print0 2>/dev/null)
    fi
}

function run_rtl_ut_tests {
    local name="${1}"

    printf "${CYN}=== RTL module unit tests ===${RST}\n"

    local ut_bins=()
    while IFS= read -r -d '' bin; do
        ut_bins+=("${bin}")
    done < <(find_rtl_ut_bins "${name}")

    if [[ ${#ut_bins[@]} -eq 0 ]]; then
        skip "rtl/ut" "no RTL UT binaries found (run './build.sh ut rtl' first)"
        return
    fi

    mkdir -p "${BUILD_DIR}/logs/rtl/ut"

    for bin in "${ut_bins[@]}"; do
        run_one_rtl_ut "${bin}"
    done
}

function rtl_ut_label_from_bin {
    local bin="${1}"
    local ut_dir="${BUILD_DIR}/hw/rtl/ut"
    local label="${bin#${ut_dir}/}"

    label="${label%_ut}"
    label="${label%_tb}"
    printf '%s' "${label}"
}

function run_one_rtl_ut {
    local bin="${1}"
    local label="$(rtl_ut_label_from_bin "${bin}")"
    local log="${BUILD_DIR}/logs/rtl/ut/${label}.log"
    local cwd="$(dirname "${bin}")"
    local exe="./$(basename "${bin}")"
    local status=0

    mkdir -p "$(dirname "${log}")"

    if run_with_timeout "${RTL_UT_TIMEOUT}" bash -c "cd \"${cwd}\" && \"${exe}\" +ITF_CHECKER" \
        >"${log}" 2>&1; then
        status=0
    else
        status=$?
    fi

    if [[ ${status} -ne 0 ]]; then
        fail "${label}" "exit=${status} log=${log}"
        return
    fi

    if grep -q '^RVB_ITF_CHECKER' "${log}"; then
        fail "${label}" "interface checker violation log=${log}"
        return
    fi

    if grep -q '^PASS:' "${log}"; then
        pass "${label}"
    elif grep -q '^FAIL:' "${log}"; then
        local verdict
        verdict="$(grep '^FAIL:' "${log}" | tail -1)"
        fail "${label}" "${verdict} log=${log}"
    else
        fail "${label}" "no UT verdict log=${log}"
    fi
}

function run_one_rtl_case {
    local simulator="${1}"
    local sim_cmd="${2}"
    local name="${3}"
    local quiet="${4}"
    local cwd="${BUILD_DIR}/hw/${simulator}"
    local hex="${SIM_SW_DIR}/${name}/${name}.hex"
    local log="${BUILD_DIR}/logs/rtl/${simulator}/${name}.log"
    local status=0

    if [[ ! -f "${hex}" ]]; then
        fail "${name}" "hex not found"
        return
    fi

    local rel_hex="../../sw/sim/${name}/${name}.hex"
    local sim_plusargs=("+program=${rel_hex}")
    local input=""

    if sw_case_uses_sd_image "${name}"; then
        local sd_image="${BUILD_DIR}/hw/sdspi-bare.img"
        truncate -s 8M "${sd_image}"
        sim_plusargs+=("+sd_image=../sdspi-bare.img")
    fi

    get_sw_case_input "${name}"
    input="${SW_CASE_INPUT}"

    if [[ "${quiet}" != true ]]; then
        echo "  cd ${cwd} && ${sim_cmd} ${sim_plusargs[*]}"
    fi

    if [[ -n "${input}" ]]; then
        if (cd "${cwd}" && printf '%s' "${input}" |
            run_with_timeout "${RTL_SW_TIMEOUT}" "${sim_cmd}" "${sim_plusargs[@]}") \
            >"${log}" 2>&1; then
            status=0
        else
            status=$?
        fi
    else
        if (cd "${cwd}" &&
            run_with_timeout "${RTL_SW_TIMEOUT}" "${sim_cmd}" "${sim_plusargs[@]}" </dev/null) \
            >"${log}" 2>&1; then
            status=0
        else
            status=$?
        fi
    fi

    if [[ ${status} -ne 0 ]]; then
        fail "${name}" "exit=${status} log=${log}"
        return
    fi

    if grep -q '^sim_top: PASS t0=0x00000000$' "${log}"; then
        pass "${name}"
    elif grep -q '^sim_top: FAIL ' "${log}"; then
        local verdict
        verdict="$(grep '^sim_top: FAIL ' "${log}" | tail -1)"
        fail "${name}" "${verdict} log=${log}"
    else
        fail "${name}" "no sim_top verdict log=${log}"
    fi
}

# ── main ──────────────────────────────────────────────────────────────────

function main {
    local parsed_args=()

    while [[ $# -gt 0 ]]; do
        case "$1" in
            -h|--help)
                print_usage
                exit 0
                ;;
            --)
                shift
                while [[ $# -gt 0 ]]; do
                    MODEL_SIM_ARGS+=("$1")
                    shift
                done
                break
                ;;
            --*)
                MODEL_SIM_ARGS+=("$1")
                ;;
            *)
                parsed_args+=("$1")
                ;;
        esac
        shift
    done

    set -- "${parsed_args[@]}"

    local target="${1:-model}"
    local arg2="${2:-}"
    local arg3="${3:-}"
    local arg4="${4:-}"

    if [[ "${target}" == "rtl" ]]; then
        local simulator="${arg2}"
        local case_name=""

        if [[ ${#MODEL_SIM_ARGS[@]} -ne 0 ]]; then
            echo "error: model sim_top args are only supported for model SW cases"
            exit 1
        fi

        if [[ "${arg2}" == "ut" ]]; then
            run_rtl_ut_tests "${arg3}"

            local total=$((PASS + FAIL + SKIP))
            echo
            printf "${CYN}=== summary ===${RST}\n"
            printf "  total: %d  |  ${GRN}pass: %d${RST}  |  ${RED}fail: %d${RST}  |  ${YLW}skip: %d${RST}\n" \
                "${total}" "${PASS}" "${FAIL}" "${SKIP}"

            if [[ ${FAIL} -gt 0 ]]; then
                echo
                printf "${RED}FAILURES:${RST}\n%s" "${FAIL_LOG}"
                exit 1
            fi
            return
        fi

        if [[ -z "${simulator}" ]]; then
            print_usage
            exit 1
        fi

        if [[ "${arg3}" == "sw" ]]; then
            case_name="${arg4}"
        else
            case_name="${arg3}"
        fi

        run_rtl_cases "${simulator}" "${case_name}"

        local total=$((PASS + FAIL + SKIP))
        echo
        printf "${CYN}=== summary ===${RST}\n"
        printf "  total: %d  |  ${GRN}pass: %d${RST}  |  ${RED}fail: %d${RST}  |  ${YLW}skip: %d${RST}\n" \
            "${total}" "${PASS}" "${FAIL}" "${SKIP}"

        if [[ ${FAIL} -gt 0 ]]; then
            echo
            printf "${RED}FAILURES:${RST}\n%s" "${FAIL_LOG}"
            exit 1
        fi
        return
    fi

    local single_name=""
    local suite=""

    if [[ -n "${arg4}" ]]; then
        suite="${arg3}"
        single_name="${arg4}"
    elif [[ -n "${arg3}" && "${arg3}" != "sw" && "${arg3}" != "ut" ]]; then
        # model sw <name>  or  model ut <name>
        suite="${arg2}"
        single_name="${arg3}"
    else
        # batch mode
        suite="${arg2}"
    fi

    if [[ -n "${single_name}" ]]; then
        case "${target}" in
            model)
                if [[ "${suite}" == "sw" ]]; then
                    run_single_sw "${single_name}"
                elif [[ "${suite}" == "ut" ]]; then
                    if [[ ${#MODEL_SIM_ARGS[@]} -ne 0 ]]; then
                        echo "error: model sim_top args are only supported for model SW cases"
                        exit 1
                    fi
                    run_ut_tests "${single_name}"
                else
                    print_usage
                    exit 1
                fi
                ;;
            *)
                print_usage
                exit 1
                ;;
        esac
        local total=$((PASS + FAIL + SKIP))
        echo
        printf "${CYN}=== summary ===${RST}\n"
        printf "  total: %d  |  ${GRN}pass: %d${RST}  |  ${RED}fail: %d${RST}  |  ${YLW}skip: %d${RST}\n" \
            "${total}" "${PASS}" "${FAIL}" "${SKIP}"

        if [[ ${FAIL} -gt 0 ]]; then
            echo
            printf "${RED}FAILURES:${RST}\n%s" "${FAIL_LOG}"
            exit 1
        fi
        return
    fi

    case "${target}" in
        model) ;;
        *)
            print_usage
            exit 1
            ;;
    esac

    if [[ "${target}" == "model" ]]; then
        if [[ "${suite}" == "ut" ]]; then
            if [[ ${#MODEL_SIM_ARGS[@]} -ne 0 ]]; then
                echo "error: model sim_top args are only supported for model SW cases"
                exit 1
            fi
            run_ut_tests
        elif [[ "${suite}" == "sw" ]]; then
            run_sw_cases
        else
            run_sw_cases
            echo
            run_ut_tests
        fi
    fi

    local total=$((PASS + FAIL + SKIP))
    echo
    printf "${CYN}=== summary ===${RST}\n"
    printf "  total: %d  |  ${GRN}pass: %d${RST}  |  ${RED}fail: %d${RST}  |  ${YLW}skip: %d${RST}\n" \
        "${total}" "${PASS}" "${FAIL}" "${SKIP}"

    if [[ ${FAIL} -gt 0 ]]; then
        echo
        printf "${RED}FAILURES:${RST}\n%s" "${FAIL_LOG}"
        exit 1
    fi
}

main "$@"

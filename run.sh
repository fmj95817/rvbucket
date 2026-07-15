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
MODEL_SW_TIMEOUT="${MODEL_SW_TIMEOUT:-60}"
MODEL_SW_PERF_SIM_TIMEOUT="${MODEL_SW_PERF_SIM_TIMEOUT:-300}"
MODEL_SW_PERF_SIM=false
MODEL_SW_ST_SIM=false

function print_usage {
    cat <<EOF
usage: $0 [--perf-sim] [--st-sim] [model|rtl <vcs|verilator>] [sw|ut] [<name>]

Options:
  --perf-sim  Run model SW with sim_top --perf-sim
  --st-sim    Disable default SMP simulation optimization for model SW

Environment:
  MODEL_SW_TIMEOUT           Timeout for each model SW case, default 60s
  MODEL_SW_PERF_SIM_TIMEOUT  Timeout for each model SW case in --perf-sim mode, default 300s
  RTL_SW_TIMEOUT             Timeout for each RTL SW case, default 300s
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
    local sim_args=()

    if [[ ! -f "${bin}" ]]; then
        echo "error: binary not found: ${bin}"
        exit 1
    fi

    local cwd="${BUILD_DIR}/hw/model"
    local rel_bin="../../sw/sim/${name}/${name}.bin"
    if [[ "${MODEL_SW_PERF_SIM}" == true ]]; then
        sim_args+=(--perf-sim)
    fi
    if [[ "${MODEL_SW_ST_SIM}" == true ]]; then
        sim_args+=(--st-sim)
    fi

    local ufs_image=""
    if [[ "${name}" == "ufshc_mcq" ]]; then
        ufs_image="$(mktemp /tmp/rvbucket-${name}.XXXXXX.img)"
        sim_args+=(--ufs-image "${ufs_image}")
    fi

    local sim_cmd="./sim_top"
    if [[ "${MODEL_SW_PERF_SIM}" == true ]]; then
        sim_cmd+=" --perf-sim"
    fi
    if [[ "${MODEL_SW_ST_SIM}" == true ]]; then
        sim_cmd+=" --st-sim"
    fi
    if [[ -n "${ufs_image}" ]]; then
        sim_cmd+=" --ufs-image ${ufs_image}"
    fi

    echo "  cd ${cwd} && ${sim_cmd} ${rel_bin}"
    get_sw_case_input "${name}"
    input="${SW_CASE_INPUT}"
    local status=0
    if [[ -n "${input}" ]]; then
        (cd "${cwd}" && printf '%s' "${input}" |
            "${cwd}/sim_top" "${sim_args[@]}" "${rel_bin}") || status=$?
    else
        (cd "${cwd}" && "${cwd}/sim_top" "${sim_args[@]}" "${rel_bin}") || status=$?
    fi
    [[ -n "${ufs_image}" ]] && rm -f "${ufs_image}"
    return "${status}"
}

function run_single_ut {
    local name="${1}"

    local ut_bin=""
    while IFS= read -r -d '' bin; do
        if [[ "${bin##*/}" == "${name}_ut" ]]; then
            ut_bin="${bin}"
            break
        fi
    done < <(find_ut_bins "${BUILD_DIR}/hw/model/ut")

    if [[ -z "${ut_bin}" ]]; then
        echo "error: UT binary not found: ${name}_ut"
        exit 1
    fi

    local cwd="$(dirname "${ut_bin}")"
    echo "  cd ${cwd} && ./${name}_ut"
    cd "${cwd}" && "./${name}_ut"
}

function run_single_rtl_sw {
    local simulator="${1}"
    local name="${2}"

    run_rtl_cases "${simulator}" "${name}"
}

function run_single_rtl_ut {
    local simulator="${1}"
    local name="${2}"

    (void_simulator="${simulator}")
    (void_name="${name}")

    echo "error: RTL UT not yet implemented"
    exit 1
}

# ── batch regression ──────────────────────────────────────────────────────

function run_sw_cases {
    local sw_dir="${SIM_SW_DIR}"
    local sim="${BUILD_DIR}/hw/model/sim_top"
    local sim_args=()
    local timeout="${MODEL_SW_TIMEOUT}"

    if [[ "${MODEL_SW_PERF_SIM}" == true ]]; then
        sim_args+=(--perf-sim)
        timeout="${MODEL_SW_PERF_SIM_TIMEOUT}"
    fi
    if [[ "${MODEL_SW_ST_SIM}" == true ]]; then
        sim_args+=(--st-sim)
    fi

    if [[ "${MODEL_SW_PERF_SIM}" == true || "${MODEL_SW_ST_SIM}" == true ]]; then
        local mode_args=()
        [[ "${MODEL_SW_PERF_SIM}" == true ]] && mode_args+=(--perf-sim)
        [[ "${MODEL_SW_ST_SIM}" == true ]] && mode_args+=(--st-sim)
        printf "${CYN}=== bare-metal C cases (%s) ===${RST}\n" "${mode_args[*]}"
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
        local ufs_image=""
        local case_sim_args=("${sim_args[@]}")

        if [[ "${name}" == "ufshc_mcq" ]]; then
            ufs_image="$(mktemp /tmp/rvbucket-${name}.XXXXXX.img)"
            case_sim_args+=(--ufs-image "${ufs_image}")
        fi

        get_sw_case_input "${name}"
        input="${SW_CASE_INPUT}"
        if [[ -n "${input}" ]]; then
            if printf '%s' "${input}" |
                run_with_timeout "${timeout}" "${sim}" "${case_sim_args[@]}" "${bin}" >/dev/null 2>&1; then
                status=0
            else
                status=$?
            fi
        else
            if run_with_timeout "${timeout}" "${sim}" "${case_sim_args[@]}" "${bin}" </dev/null >/dev/null 2>&1; then
                status=0
            else
                status=$?
            fi
        fi
        [[ -n "${ufs_image}" ]] && rm -f "${ufs_image}"

        if [[ ${status} -eq 0 ]]; then
            pass "${name}"
        else
            fail "${name}" "exit=${status}"
        fi
    done
}

function run_ut_tests {
    local ut_dir="${BUILD_DIR}/hw/model/ut"
    printf "${CYN}=== module unit tests ===${RST}\n"

    local ut_bins=()
    while IFS= read -r -d '' bin; do
        ut_bins+=("${bin}")
    done < <(find_ut_bins "${ut_dir}")

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
    local input=""

    get_sw_case_input "${name}"
    input="${SW_CASE_INPUT}"

    if [[ "${quiet}" != true ]]; then
        echo "  cd ${cwd} && ${sim_cmd} +program=${rel_hex}"
    fi

    if [[ -n "${input}" ]]; then
        if (cd "${cwd}" && printf '%s' "${input}" |
            run_with_timeout "${RTL_SW_TIMEOUT}" "${sim_cmd}" "+program=${rel_hex}") \
            >"${log}" 2>&1; then
            status=0
        else
            status=$?
        fi
    else
        if (cd "${cwd}" &&
            run_with_timeout "${RTL_SW_TIMEOUT}" "${sim_cmd}" "+program=${rel_hex}" </dev/null) \
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
            --perf-sim)
                MODEL_SW_PERF_SIM=true
                ;;
            --st-sim)
                MODEL_SW_ST_SIM=true
                ;;
            -h|--help)
                print_usage
                exit 0
                ;;
            --)
                shift
                while [[ $# -gt 0 ]]; do
                    parsed_args+=("$1")
                    shift
                done
                break
                ;;
            --*)
                echo "error: unknown option: $1"
                print_usage
                exit 1
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

        if [[ -z "${simulator}" ]]; then
            print_usage
            exit 1
        fi

        if [[ "${MODEL_SW_PERF_SIM}" == true || "${MODEL_SW_ST_SIM}" == true ]]; then
            echo "error: --perf-sim/--st-sim is only supported for model SW cases"
            exit 1
        fi

        if [[ "${arg3}" == "sw" ]]; then
            case_name="${arg4}"
        elif [[ "${arg3}" == "ut" ]]; then
            print_usage
            exit 1
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
        # rtl vcs sw <name>  or  rtl vcs ut <name>
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
                    run_single_ut "${single_name}"
                else
                    print_usage
                    exit 1
                fi
                ;;
            rtl)
                if [[ "${suite}" == "sw" ]]; then
                    run_single_rtl_sw "${arg2}" "${single_name}"
                elif [[ "${suite}" == "ut" ]]; then
                    run_single_rtl_ut "${arg2}" "${single_name}"
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
        return
    fi

    local simulator=""

    case "${target}" in
        model) ;;
        rtl)
            simulator="${arg2}"
            suite="${arg3}"

            if [[ -z "${simulator}" ]]; then
                print_usage
                exit 1
            fi

            if [[ "${MODEL_SW_PERF_SIM}" == true || "${MODEL_SW_ST_SIM}" == true ]]; then
                echo "error: --perf-sim/--st-sim is only supported for model SW cases"
                exit 1
            fi
            ;;
        *)
            print_usage
            exit 1
            ;;
    esac

    if [[ "${target}" == "model" ]]; then
        if [[ "${suite}" == "ut" ]]; then
            run_ut_tests
        elif [[ "${suite}" == "sw" ]]; then
            run_sw_cases
        else
            run_sw_cases
            echo
            run_ut_tests
        fi
    elif [[ "${target}" == "rtl" ]]; then
        run_rtl_cases "${simulator}" "${suite}"
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

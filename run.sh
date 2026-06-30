#!/bin/bash

set -e

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${ROOT}/build"

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
    local bin="${BUILD_DIR}/sw/${name}/${name}.bin"

    if [[ ! -f "${bin}" ]]; then
        echo "error: binary not found: ${bin}"
        exit 1
    fi

    local cwd="${BUILD_DIR}/hw/model"
    echo "  cd ${cwd} && ./sim_top ../../sw/${name}/${name}.bin"
    cd "${cwd}" && "${cwd}/sim_top" "../../sw/${name}/${name}.bin"
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
    local hex="${BUILD_DIR}/sw/${name}/${name}.hex"
    local cwd="${BUILD_DIR}/hw/${simulator}"

    if [[ ! -d "${cwd}" ]]; then
        echo "error: RTL build dir not found: ${cwd}"
        exit 1
    fi

    if [[ ! -f "${hex}" ]]; then
        echo "error: hex not found: ${hex}"
        exit 1
    fi

    echo "  cd ${cwd} && ./sim_top +program=../../sw/${name}/${name}.hex"
    cd "${cwd}" && "${cwd}/sim_top" "+program=../../sw/${name}/${name}.hex"
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
    local sw_dir="${BUILD_DIR}/sw"
    local sim="${BUILD_DIR}/hw/model/sim_top"
    printf "${CYN}=== bare-metal C cases ===${RST}\n"

    if [[ ! -x "${sim}" ]]; then
        skip "all SW cases" "simulator not found: ${sim}"
        return
    fi

    for case_dir in "${ROOT}"/cases/*/; do
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

        if run_with_timeout 60 "${sim}" "${bin}" </dev/null >/dev/null 2>&1; then
            pass "${name}"
        else
            fail "${name}" "exit=${?}"
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
    local suite="${2}"

    printf "${CYN}=== RTL regression (${simulator}) ===${RST}\n"

    if [[ -n "${suite}" ]]; then
        skip "rtl/${simulator}/${suite}" "not yet implemented"
    else
        skip "rtl/${simulator}" "not yet implemented"
    fi
}

# ── main ──────────────────────────────────────────────────────────────────

function main {
    local target="${1:-model}"
    local arg2="${2:-}"
    local arg3="${3:-}"
    local arg4="${4:-}"

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
                    echo "usage: $0 model sw|ut <name>"
                    exit 1
                fi
                ;;
            rtl)
                if [[ "${suite}" == "sw" ]]; then
                    run_single_rtl_sw "${arg2}" "${single_name}"
                elif [[ "${suite}" == "ut" ]]; then
                    run_single_rtl_ut "${arg2}" "${single_name}"
                else
                    echo "usage: $0 rtl <vcs|verilator> sw|ut <name>"
                    exit 1
                fi
                ;;
            *)
                echo "usage: $0 [model|rtl] sw|ut <name>"
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
                echo "usage: $0 rtl <vcs|verilator> [sw|ut]"
                exit 1
            fi
            ;;
        *)
            echo "usage: $0 [model|rtl <vcs|verilator>] [sw|ut] [<name>]"
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
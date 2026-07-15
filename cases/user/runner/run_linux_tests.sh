#!/bin/sh

if [ -z "${NO_COLOR:-}" ]; then
    RED='\033[0;31m'
    GRN='\033[0;32m'
    YLW='\033[1;33m'
    CYN='\033[0;36m'
    RST='\033[0m'
else
    RED=""
    GRN=""
    YLW=""
    CYN=""
    RST=""
fi

TESTS='
hello|0|hello from rvbucket linux user test
rv_alu_branch|0|rv_alu_branch: PASS
rv_indirect_call|0|rv_indirect_call: PASS
rv_atomic|0|rv_atomic: PASS
vm_page|0|vm_page: PASS
syscall_fs_ipc|0|syscall_fs_ipc: PASS
fork_exec|0|fork_exec: PASS
signal_flow|0|signal_flow: PASS
dirent_stat|0|dirent_stat: PASS
heap_stress|0|heap_stress: PASS
unaligned_access|0|unaligned_access: PASS
file_mmap|0|file_mmap: PASS
time_uname|0|time_uname: PASS
mount_stability.sh|0|mount_stability: PASS
shell_smoke.sh|0|shell_smoke: PASS
ufs_f2fs.sh|0|UFS_F2FS_STACK_PASS
'

PASS=0
FAIL=0
SKIP=0
FAIL_LOG=""

pass()
{
    printf "  %bPASS%b  %s\n" "$GRN" "$RST" "$1"
    PASS=$((PASS + 1))
}

fail()
{
    printf "  %bFAIL%b  %s  %s\n" "$RED" "$RST" "$1" "$2"
    FAIL=$((FAIL + 1))
    FAIL_LOG="${FAIL_LOG}  $1  $2
"
}

skip()
{
    printf "  %bSKIP%b  %s  %s\n" "$YLW" "$RST" "$1" "$2"
    SKIP=$((SKIP + 1))
}

printf "%b=== Linux user tests ===%b\n" "$CYN" "$RST"

while IFS='|' read -r name expected_status expected_output; do
    [ -n "$name" ] || continue
    [ -n "$expected_status" ] || expected_status=0

    if [ ! -x "/bin/$name" ]; then
        fail "$name" "binary not executable"
        continue
    fi

    output=$(/bin/"$name" 2>&1)
    status=$?

    if [ "$status" -ne "$expected_status" ]; then
        fail "$name" "exit=$status expected=$expected_status"
        FAIL_LOG="${FAIL_LOG}    stdout: $output
"
        continue
    fi

    if [ "$output" != "$expected_output" ]; then
        fail "$name" "output mismatch"
        FAIL_LOG="${FAIL_LOG}    expected: $expected_output
    actual:   $output
"
        continue
    fi

    pass "$name"
done <<EOF
$TESTS
EOF

total=$((PASS + FAIL + SKIP))

echo
printf "%b=== summary ===%b\n" "$CYN" "$RST"
printf "  total: %d  |  %bpass: %d%b  |  %bfail: %d%b  |  %bskip: %d%b\n" \
    "$total" "$GRN" "$PASS" "$RST" "$RED" "$FAIL" "$RST" "$YLW" "$SKIP" "$RST"

if [ "$FAIL" -ne 0 ]; then
    echo
    printf "%bFAILURES:%b\n%s" "$RED" "$RST" "$FAIL_LOG"
    exit 1
fi

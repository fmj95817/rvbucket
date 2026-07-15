#!/bin/sh

set -eu

test -r /proc/cpuinfo
test -d /sys
mkdir -p /tmp/rvbucket_shell_smoke
printf 'rvbucket-shell\n' > /tmp/rvbucket_shell_smoke/data
test "$(cat /tmp/rvbucket_shell_smoke/data)" = "rvbucket-shell"
test "$(wc -c < /tmp/rvbucket_shell_smoke/data)" -eq 15
rm -f /tmp/rvbucket_shell_smoke/data
rmdir /tmp/rvbucket_shell_smoke

echo "shell_smoke: PASS"

#!/bin/sh

set -eu

ref="$(mount)"
i=1
while [ "$i" -lt 8 ]; do
    cur="$(mount)"
    if [ "$cur" != "$ref" ]; then
        echo "mount_stability: FAIL iteration=$i"
        echo "expected:"
        echo "$ref"
        echo "actual:"
        echo "$cur"
        exit 1
    fi
    i=$((i + 1))
done

echo "mount_stability: PASS"

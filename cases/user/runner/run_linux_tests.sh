#!/bin/sh

TESTS='
hello|hello from rvbucket linux user test
'

echo "=== rvbucket linux user tests ==="

echo "$TESTS" | while IFS='|' read -r name expected; do
    [ -n "$name" ] || continue

    echo "[ RUN      ] $name"
    output=$(/bin/"$name" 2>&1)
    status=$?

    if [ "$status" -ne 0 ]; then
        echo "[  FAILED  ] $name: exit=$status"
        echo "$output"
        exit 1
    fi

    if [ "$output" != "$expected" ]; then
        echo "[  FAILED  ] $name: output mismatch"
        echo "expected: $expected"
        echo "actual:   $output"
        exit 1
    fi

    echo "[       OK ] $name"
done || exit 1

echo "=== all linux user tests passed ==="

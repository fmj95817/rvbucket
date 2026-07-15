#!/bin/sh

set -e

MNT=/mnt/ufs_f2fs
PATTERN="${MNT}/rvbucket_pattern.txt"
BLOB="${MNT}/blob.zero"

mkdir -p /dev "${MNT}"
mount -t devtmpfs devtmpfs /dev 2>/dev/null || true

[ -b /dev/sda ]
[ -c /dev/zero ]

mount -t f2fs /dev/sda "${MNT}"
trap 'umount "${MNT}" 2>/dev/null || true' EXIT

rm -f "${PATTERN}" "${BLOB}"
: > "${PATTERN}"

i=0
while [ "${i}" -lt 512 ]; do
    printf 'rvbucket-ufs-f2fs-line-%03d-0123456789abcdef\n' "${i}" >> "${PATTERN}"
    i=$((i + 1))
done

h1="$(sha256sum "${PATTERN}" | cut -d ' ' -f 1)"
dd if=/dev/zero of="${BLOB}" bs=4096 count=8 >/dev/null 2>&1
sync

umount "${MNT}"
mount -t f2fs /dev/sda "${MNT}"

h2="$(sha256sum "${PATTERN}" | cut -d ' ' -f 1)"
[ "${h1}" = "${h2}" ]
[ "$(stat -c %s "${BLOB}")" = "32768" ]

umount "${MNT}"
trap - EXIT

echo "UFS_F2FS_STACK_PASS"

#!/bin/sh

set -eu

dev=/dev/mmcblk0p2
mnt=/mnt/sdspi-root
mounted=0

[ -b "${dev}" ] || { echo "sdspi_ext4: missing ${dev}"; exit 1; }

if awk '$1 == "/dev/mmcblk0p2" && $2 == "/" && $3 == "ext4" {
    found = 1
} END { exit !found }' /proc/mounts; then
    mnt=/tmp
else
    mkdir -p "${mnt}"
    mount -t ext4 "${dev}" "${mnt}"
    mounted=1
fi
test_file=${mnt}/rvbucket-sdspi-test
printf 'rvbucket-sdspi-ext4\n' > "${test_file}"
sync
result="$(cat "${test_file}")"
rm -f "${test_file}"
if [ "${mounted}" -eq 1 ]; then
    umount "${mnt}"
fi

[ "${result}" = "rvbucket-sdspi-ext4" ] || {
    echo "sdspi_ext4: data mismatch"
    exit 1
}
echo "sdspi_ext4: PASS"

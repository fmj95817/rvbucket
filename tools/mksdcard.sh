#!/bin/bash

set -euo pipefail

usage()
{
    echo "usage: $0 [--size-mib <MiB>] <linux.bin> <rootfs-dir> <sd.img>" >&2
    exit 1
}

disk_size_mib=128
if [[ "${1:-}" == "--size-mib" ]]; then
    [[ $# -ge 2 ]] || usage
    disk_size_mib="$2"
    shift 2
fi
[[ $# -eq 3 ]] || usage

linux_bin="$1"
rootfs_dir="$2"
output="$3"
[[ -f "${linux_bin}" ]] || { echo "missing linux image: ${linux_bin}" >&2; exit 1; }
[[ -d "${rootfs_dir}" ]] || { echo "missing rootfs directory: ${rootfs_dir}" >&2; exit 1; }
[[ "${disk_size_mib}" =~ ^[0-9]+$ ]] || usage

sfdisk_bin="$(command -v sfdisk || true)"
mke2fs_bin="$(command -v mke2fs || true)"
[[ -n "${sfdisk_bin}" ]] || sfdisk_bin=/usr/sbin/sfdisk
[[ -n "${mke2fs_bin}" ]] || mke2fs_bin=/sbin/mke2fs
[[ -x "${sfdisk_bin}" ]] || { echo "sfdisk is required" >&2; exit 1; }
[[ -x "${mke2fs_bin}" ]] || { echo "mke2fs is required" >&2; exit 1; }

sector_size=512
align_sectors=2048
disk_sectors=$((disk_size_mib * 1024 * 1024 / sector_size))
linux_bytes="$(stat -c %s "${linux_bin}")"
p1_start=${align_sectors}
p1_payload_sectors=$(((linux_bytes + sector_size - 1) / sector_size))
p1_sectors=$((((p1_payload_sectors + align_sectors - 1) / align_sectors) * align_sectors))
p2_start=$((p1_start + p1_sectors))
p2_sectors=$((disk_sectors - p2_start - align_sectors))
if (( p2_sectors < 32768 )); then
    echo "disk is too small for linux.bin and an ext4 rootfs" >&2
    exit 1
fi

mkdir -p "$(dirname "${output}")"
rootfs_img="$(mktemp "${TMPDIR:-/tmp}/rvbucket-rootfs-XXXXXX.img")"
trap 'rm -f "${rootfs_img}"' EXIT

rm -f "${output}"
truncate -s $((disk_sectors * sector_size)) "${output}"
{
    echo "label: dos"
    echo "unit: sectors"
    echo
    echo "${p1_start},${p1_sectors},da,*"
    echo "${p2_start},${p2_sectors},83"
} | "${sfdisk_bin}" "${output}" >/dev/null

truncate -s $((p2_sectors * sector_size)) "${rootfs_img}"
"${mke2fs_bin}" -q -F -t ext4 -b 4096 -m 0 -L rvbucket-rootfs \
    -d "${rootfs_dir}" "${rootfs_img}"

dd if="${linux_bin}" of="${output}" bs=${sector_size} seek=${p1_start} \
    conv=notrunc status=none
dd if="${rootfs_img}" of="${output}" bs=${sector_size} seek=${p2_start} \
    conv=notrunc,sparse status=none

echo "created ${output}"
echo "  p1 raw:  start=${p1_start}, sectors=${p1_sectors}, payload=${linux_bytes} bytes"
echo "  p2 ext4: start=${p2_start}, sectors=${p2_sectors}, label=rvbucket-rootfs"

#!/bin/bash

set -euo pipefail

HEADER_SIZE=4096
BLOCK_SIZE=4096
IMAGE_VERSION=1
DEFAULT_SIZE=128M

usage()
{
    echo "usage: $0 <output.img> [size]" >&2
    echo "  size accepts truncate syntax and defaults to ${DEFAULT_SIZE}" >&2
}

find_mkfs_f2fs()
{
    if [[ -n "${MKFS_F2FS:-}" ]]; then
        [[ -x "${MKFS_F2FS}" ]] || {
            echo "mkufsimg: MKFS_F2FS is not executable: ${MKFS_F2FS}" >&2
            return 1
        }
        printf '%s\n' "${MKFS_F2FS}"
        return
    fi

    local tool
    tool="$(command -v mkfs.f2fs 2>/dev/null || true)"
    if [[ -n "${tool}" ]]; then
        printf '%s\n' "${tool}"
        return
    fi

    for tool in /usr/sbin/mkfs.f2fs /sbin/mkfs.f2fs; do
        if [[ -x "${tool}" ]]; then
            printf '%s\n' "${tool}"
            return
        fi
    done

    echo "mkufsimg: mkfs.f2fs not found; install f2fs-tools" >&2
    return 1
}

write_le()
{
    local value="$1"
    local width="$2"
    local byte
    local i

    for ((i = 0; i < width; i++)); do
        byte=$((value & 0xff))
        printf "\\$(printf '%03o' "${byte}")"
        value=$((value >> 8))
    done
}

if [[ $# -lt 1 || $# -gt 2 ]]; then
    usage
    exit 1
fi

output="$1"
storage_arg="${2:-${DEFAULT_SIZE}}"
if [[ -z "${output}" ]]; then
    usage
    exit 1
fi
output_dir="$(dirname -- "${output}")"
output_name="$(basename -- "${output}")"

mkdir -p "${output_dir}"
mkfs_f2fs="$(find_mkfs_f2fs)"
tmp_dir="$(mktemp -d "${output_dir}/.${output_name}.XXXXXX")"
trap 'rm -rf "${tmp_dir}"' EXIT

raw_image="${tmp_dir}/storage.raw"
header="${tmp_dir}/header.bin"
ufs_image="${tmp_dir}/image.img"

truncate -s "${storage_arg}" "${raw_image}"
storage_size="$(wc -c < "${raw_image}")"
storage_size="${storage_size//[[:space:]]/}"

if [[ ! "${storage_size}" =~ ^[0-9]+$ ]] ||
   ((storage_size == 0 || storage_size > 0xffffffff)); then
    echo "mkufsimg: storage size must be between 1 and 4294967295 bytes" >&2
    exit 1
fi
if ((storage_size % BLOCK_SIZE != 0)); then
    echo "mkufsimg: storage size must be aligned to ${BLOCK_SIZE} bytes" >&2
    exit 1
fi

"${mkfs_f2fs}" -f "${raw_image}"

{
    printf '%s' "RVBUFS01"
    write_le "${IMAGE_VERSION}" 4
    write_le "${HEADER_SIZE}" 4
    write_le "${storage_size}" 8
    write_le "${BLOCK_SIZE}" 4
    write_le 0 4
} > "${header}"
truncate -s "${HEADER_SIZE}" "${header}"

cat "${header}" "${raw_image}" > "${ufs_image}"
expected_size=$((HEADER_SIZE + storage_size))
actual_size="$(wc -c < "${ufs_image}")"
actual_size="${actual_size//[[:space:]]/}"
if ((actual_size != expected_size)); then
    echo "mkufsimg: output size mismatch: got ${actual_size}, expected ${expected_size}" >&2
    exit 1
fi

mv -f "${ufs_image}" "${output}"
printf 'mkufsimg: created %s (%u-byte header + %u-byte F2FS payload)\n' \
    "${output}" "${HEADER_SIZE}" "${storage_size}"

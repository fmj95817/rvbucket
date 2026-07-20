#!/bin/sh

switch_sdcard_rootfs()
{
    local dev=/dev/mmcblk0p2
    local newroot=/newroot

    [ "$$" -eq 1 ] || return 2
    [ -b "${dev}" ] || return 1

    mkdir -p "${newroot}"
    mount -t ext4 "${dev}" "${newroot}" || return 1
    mount --move /dev "${newroot}/dev" || return 1
    mount --move /proc "${newroot}/proc" || return 1
    mount --move /sys "${newroot}/sys" || return 1
    exec switch_root "${newroot}" /init
}

if [ "${SWITCH_SDROOT_SOURCE_ONLY:-0}" != 1 ]; then
    if [ "$$" -ne 1 ]; then
        echo "switch_sdroot: run 'exec /bin/switch_sdroot.sh' from the initramfs shell" >&2
        exit 2
    fi
    if ! switch_sdcard_rootfs; then
        echo "switch_sdroot: SD card rootfs unavailable" >&2
        exit 1
    fi
fi

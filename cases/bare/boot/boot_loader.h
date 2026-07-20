#ifndef BOOT_LOADER_H
#define BOOT_LOADER_H

#include "boot_image.h"
#include "boot_media.h"

typedef enum boot_load_result {
    BOOT_LOAD_OK = 0,
    BOOT_LOAD_NOT_ATTEMPTED = 1,
    BOOT_LOAD_ERR_HEADER_READ = -1,
    BOOT_LOAD_ERR_HEADER_INVALID = -2,
    BOOT_LOAD_ERR_FIRMWARE_READ = -3,
    BOOT_LOAD_ERR_KERNEL_READ = -4,
    BOOT_LOAD_ERR_INITRD_READ = -5,
    BOOT_LOAD_ERR_DTB_READ = -6
} boot_load_result_t;

extern boot_load_result_t boot_load_image(boot_media_t *media,
    rvb_bin_header_t *header);

#endif

#ifndef BOOT_MEDIA_H
#define BOOT_MEDIA_H

#include <stdint.h>
#include "drivers/sdspi/sdspi.h"

typedef enum boot_media_kind {
    BOOT_MEDIA_QSPI,
    BOOT_MEDIA_SDCARD
} boot_media_kind_t;

typedef enum boot_media_result {
    BOOT_MEDIA_OK = 0,
    BOOT_MEDIA_ERR_RANGE = -1,
    BOOT_MEDIA_ERR_IO = -2,
    BOOT_MEDIA_ERR_SD_INIT = -3,
    BOOT_MEDIA_ERR_MBR_READ = -4,
    BOOT_MEDIA_ERR_MBR_SIGNATURE = -5,
    BOOT_MEDIA_ERR_PARTITION_TYPE = -6,
    BOOT_MEDIA_ERR_PARTITION_RANGE = -7
} boot_media_result_t;

typedef struct boot_media {
    boot_media_kind_t kind;
    uint32_t start_lba;
    uint32_t sectors;
    sdspi_dev_t sdspi;
} boot_media_t;

extern boot_media_result_t boot_media_open(boot_media_t *media,
    boot_media_kind_t kind);
extern int boot_media_range_valid(const boot_media_t *media, uint32_t offset,
    uint32_t size);
extern boot_media_result_t boot_media_read(boot_media_t *media,
    uint32_t offset, void *dst, uint32_t size);

#endif

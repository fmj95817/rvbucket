#ifndef BOOT_IMAGE_H
#define BOOT_IMAGE_H

#include <stdint.h>

#define RVB_BIN_TYPE_BARE  0u
#define RVB_BIN_TYPE_LINUX 1u

typedef enum rvb_bin_section {
    RVB_BIN_SECTION_FIRMWARE = 0,
    RVB_BIN_SECTION_KERNEL,
    RVB_BIN_SECTION_INITRD,
    RVB_BIN_SECTION_DTB,
    RVB_BIN_SECTION_COUNT
} rvb_bin_section_t;

typedef struct rvb_bin_header {
    uint32_t type;
    uint32_t firmware_size;
    uint32_t reserved;
    uint32_t kernel_size;
    uint32_t initrd_size;
    uint32_t dtb_size;
    uint32_t kernel_load;
    uint32_t initrd_load;
    uint32_t dtb_load;
    uint32_t padding;
} rvb_bin_header_t;

#define RVB_BIN_HEADER_SIZE ((uint32_t)sizeof(rvb_bin_header_t))

static inline uint32_t rvb_bin_align4(uint32_t value)
{
    return (value + 3u) & ~3u;
}

_Static_assert(sizeof(rvb_bin_header_t) == 40u,
    "rvbucket binary header must remain 40 bytes");

#endif

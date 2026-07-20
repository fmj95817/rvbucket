#include "boot_loader.h"

#include <stdint.h>
#include "boot_config.h"
#include "boot_console.h"
#include "drivers/cache/cache.h"

typedef struct boot_section {
    rvb_bin_section_t id;
    uint32_t load_addr;
    uint32_t size;
    boot_load_result_t read_error;
} boot_section_t;

static int ranges_overlap(uint32_t addr, uint32_t size, uint32_t reserved_addr,
    uint32_t reserved_size)
{
    return size != 0u && addr < reserved_addr + reserved_size &&
        reserved_addr < addr + size;
}

static int load_range_valid(uint32_t addr, uint32_t size)
{
    uint32_t stack_addr = BOOT_FIRMWARE_BASE_ADDR +
        BOOT_FIRMWARE_WINDOW_SIZE - BOOT_STACK_SIZE;

    if (size == 0u) {
        return 1;
    }
    if ((addr & 3u) != 0u || addr < BOOT_DDR_BASE_ADDR ||
        addr >= BOOT_DDR_END_ADDR || size > BOOT_DDR_END_ADDR - addr) {
        return 0;
    }
    return !ranges_overlap(addr, size, stack_addr, BOOT_STACK_SIZE) &&
        !ranges_overlap(addr, size, BOOT_SD_STAGE_ADDR, BOOT_SD_STAGE_SIZE);
}

static uint32_t build_sections(const rvb_bin_header_t *header,
    boot_section_t sections[RVB_BIN_SECTION_COUNT])
{
    sections[RVB_BIN_SECTION_FIRMWARE] = (boot_section_t){
        RVB_BIN_SECTION_FIRMWARE, BOOT_FIRMWARE_BASE_ADDR,
        header->firmware_size, BOOT_LOAD_ERR_FIRMWARE_READ
    };
    sections[RVB_BIN_SECTION_KERNEL] = (boot_section_t){
        RVB_BIN_SECTION_KERNEL, header->kernel_load, header->kernel_size,
        BOOT_LOAD_ERR_KERNEL_READ
    };
    sections[RVB_BIN_SECTION_INITRD] = (boot_section_t){
        RVB_BIN_SECTION_INITRD, header->initrd_load, header->initrd_size,
        BOOT_LOAD_ERR_INITRD_READ
    };
    sections[RVB_BIN_SECTION_DTB] = (boot_section_t){
        RVB_BIN_SECTION_DTB, header->dtb_load, header->dtb_size,
        BOOT_LOAD_ERR_DTB_READ
    };
    return header->type == RVB_BIN_TYPE_BARE ? 1u : RVB_BIN_SECTION_COUNT;
}

static int header_valid(const rvb_bin_header_t *header, boot_media_t *media,
    const boot_section_t sections[RVB_BIN_SECTION_COUNT],
    uint32_t section_count)
{
    uint32_t total = RVB_BIN_HEADER_SIZE;

    if (header->type > RVB_BIN_TYPE_LINUX || header->reserved != 0u ||
        header->padding != 0u || header->firmware_size == 0u ||
        header->firmware_size > BOOT_FIRMWARE_WINDOW_SIZE - BOOT_STACK_SIZE) {
        return 0;
    }
    if (header->type == RVB_BIN_TYPE_BARE &&
        (header->kernel_size != 0u || header->initrd_size != 0u ||
        header->dtb_size != 0u)) {
        return 0;
    }
    for (uint32_t i = 0; i < section_count; i++) {
        uint32_t aligned = rvb_bin_align4(sections[i].size);

        if (!load_range_valid(sections[i].load_addr, sections[i].size) ||
            aligned < sections[i].size || aligned > UINT32_MAX - total) {
            return 0;
        }
        total += aligned;
    }
    return boot_media_range_valid(media, 0u, total);
}

static boot_load_result_t load_section(boot_media_t *media,
    const boot_section_t *section, uint32_t offset)
{
    uint32_t loaded = 0u;

    while (loaded < section->size) {
        uint32_t chunk = section->size - loaded;

        if (chunk > BOOT_PROGRESS_CHUNK_SIZE) {
            chunk = BOOT_PROGRESS_CHUNK_SIZE;
        }
        if (boot_media_read(media, offset + loaded,
            (void *)(section->load_addr + loaded), chunk) != BOOT_MEDIA_OK) {
            return section->read_error;
        }
        loaded += chunk;
        if (boot_console_enabled()) {
            boot_console_progress(section->id, loaded, section->size);
        }
    }
    if (section->size != 0u) {
        cache_clean_range((const void *)section->load_addr, section->size);
    }
    return BOOT_LOAD_OK;
}

boot_load_result_t boot_load_image(boot_media_t *media,
    rvb_bin_header_t *header)
{
    boot_section_t sections[RVB_BIN_SECTION_COUNT];
    uint32_t offset = RVB_BIN_HEADER_SIZE;
    uint32_t section_count;

    if (boot_media_read(media, 0u, header, sizeof(*header)) != BOOT_MEDIA_OK) {
        return BOOT_LOAD_ERR_HEADER_READ;
    }
    section_count = build_sections(header, sections);
    if (!header_valid(header, media, sections, section_count)) {
        return BOOT_LOAD_ERR_HEADER_INVALID;
    }

    for (uint32_t i = 0; i < section_count; i++) {
        boot_load_result_t result = load_section(media, &sections[i], offset);

        if (result != BOOT_LOAD_OK) {
            return result;
        }
        offset += rvb_bin_align4(sections[i].size);
    }
    return BOOT_LOAD_OK;
}

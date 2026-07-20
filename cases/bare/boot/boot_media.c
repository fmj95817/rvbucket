#include "boot_media.h"

#include <stdint.h>
#include "boot_config.h"

#define SD_SECTOR_SHIFT  9u
#define SD_SECTOR_SIZE   (1u << SD_SECTOR_SHIFT)
#define SD_MAX_BLOCKS    (BOOT_SD_STAGE_SIZE / SD_SECTOR_SIZE)
#define SD_PART_TYPE_RAW 0xdau

#define MBR_PART0_TYPE_OFFSET    450u
#define MBR_PART0_LBA_OFFSET     454u
#define MBR_PART0_SECTORS_OFFSET 458u
#define MBR_SIGNATURE_OFFSET     510u

static uint8_t *const sd_stage = (uint8_t *)BOOT_SD_STAGE_ADDR;

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
        ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static void copy_bytes(const uint8_t *src, void *dst_ptr, uint32_t size)
{
    volatile uint8_t *dst = dst_ptr;

    while (size >= sizeof(uint32_t) &&
        (((uintptr_t)src | (uintptr_t)dst) & 3u) == 0u) {
        *(volatile uint32_t *)dst = *(const uint32_t *)src;
        src += sizeof(uint32_t);
        dst += sizeof(uint32_t);
        size -= sizeof(uint32_t);
    }
    while (size-- != 0u) {
        *dst++ = *src++;
    }
}

int boot_media_range_valid(const boot_media_t *media, uint32_t offset,
    uint32_t size)
{
    uint32_t end;
    uint32_t sectors;

    if (size > UINT32_MAX - offset) {
        return 0;
    }
    end = offset + size;
    sectors = end == 0u ? 0u : ((end - 1u) >> SD_SECTOR_SHIFT) + 1u;
    return sectors <= media->sectors;
}

static boot_media_result_t sd_read(boot_media_t *media, uint32_t offset,
    void *dst_ptr, uint32_t size)
{
    uint8_t *dst = dst_ptr;

    while (size != 0u) {
        uint32_t sector_offset = offset & (SD_SECTOR_SIZE - 1u);
        uint32_t copy_size = BOOT_SD_STAGE_SIZE - sector_offset;
        uint32_t blocks;

        if (copy_size > size) {
            copy_size = size;
        }
        blocks = (sector_offset + copy_size + SD_SECTOR_SIZE - 1u) >>
            SD_SECTOR_SHIFT;
        if (blocks == 0u || blocks > SD_MAX_BLOCKS ||
            sdspi_read_blocks(&media->sdspi,
                media->start_lba + (offset >> SD_SECTOR_SHIFT), blocks,
                sd_stage) != 0) {
            return BOOT_MEDIA_ERR_IO;
        }
        copy_bytes(sd_stage + sector_offset, dst, copy_size);
        dst += copy_size;
        offset += copy_size;
        size -= copy_size;
    }
    return BOOT_MEDIA_OK;
}

boot_media_result_t boot_media_read(boot_media_t *media, uint32_t offset,
    void *dst, uint32_t size)
{
    if (!boot_media_range_valid(media, offset, size)) {
        return BOOT_MEDIA_ERR_RANGE;
    }
    if (media->kind == BOOT_MEDIA_QSPI) {
        copy_bytes((const uint8_t *)BOOT_FLASH_BASE_ADDR + offset, dst, size);
        return BOOT_MEDIA_OK;
    }
    return sd_read(media, offset, dst, size);
}

boot_media_result_t boot_media_open(boot_media_t *media,
    boot_media_kind_t kind)
{
    media->kind = kind;
    media->start_lba = 0u;
    media->sectors = 0u;
    if (kind == BOOT_MEDIA_QSPI) {
        media->sectors = BOOT_FLASH_SIZE / SD_SECTOR_SIZE;
        return BOOT_MEDIA_OK;
    }

    if (sdspi_init(&media->sdspi, SDSPI_WAIT_POLL) != 0) {
        return BOOT_MEDIA_ERR_SD_INIT;
    }
    if (sdspi_read_blocks(&media->sdspi, 0u, 1u, sd_stage) != 0) {
        return BOOT_MEDIA_ERR_MBR_READ;
    }
    if (sd_stage[MBR_SIGNATURE_OFFSET] != 0x55u ||
        sd_stage[MBR_SIGNATURE_OFFSET + 1u] != 0xaau) {
        return BOOT_MEDIA_ERR_MBR_SIGNATURE;
    }
    if (sd_stage[MBR_PART0_TYPE_OFFSET] != SD_PART_TYPE_RAW) {
        return BOOT_MEDIA_ERR_PARTITION_TYPE;
    }

    media->start_lba = read_le32(sd_stage + MBR_PART0_LBA_OFFSET);
    media->sectors = read_le32(sd_stage + MBR_PART0_SECTORS_OFFSET);
    if (media->start_lba == 0u || media->sectors == 0u ||
        media->sectors > UINT32_MAX - media->start_lba) {
        return BOOT_MEDIA_ERR_PARTITION_RANGE;
    }
    return BOOT_MEDIA_OK;
}

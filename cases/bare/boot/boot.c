#include <stdint.h>

#include "drivers/pcm/pcm_map.h"
#include "drivers/sdspi/sdspi.h"

#define FLASH_BASE           ((const uint8_t *)0x80000000u)
#define FLASH_SIZE           0x02000000u
#define FIRMWARE_BASE        ((volatile uint8_t *)0x40000000u)
#define FIRMWARE_WINDOW_SIZE 0x00200000u
#define BOOT_STACK_SIZE      0x00002000u
#define DDR_BASE             0x40000000u
#define DDR_END              0x50000000u
#define SD_STAGE             ((uint8_t *)0x4fff0000u)
#define SD_STAGE_SIZE        0x00010000u

#define UART_BC      (*(volatile uint32_t *)0x30000000u)
#define UART_TX      (*(volatile uint32_t *)0x30000004u)
#define GPIO_IN      (*(volatile uint32_t *)0x30001000u)
#define GPIO_MODE_HI (*(volatile uint32_t *)0x30001008u)
#define PCM_BASE     0x30003000u
#define PCM_CLEAR    (*(volatile uint32_t *)(PCM_BASE + PCM_REG_CLEAR))

#define SD_SECTOR_SHIFT      9u
#define SD_SECTOR_SIZE       (1u << SD_SECTOR_SHIFT)
#define SD_MAX_BLOCKS        (SD_STAGE_SIZE / SD_SECTOR_SIZE)
#define SD_PART_TYPE_RAW     0xdau
#define BIN_HEADER_SIZE      40u
#define BIN_TYPE_BARE        0u
#define BIN_TYPE_LINUX       1u
#define CACHE_BLOCK_SIZE     64u
#define BOOT_PROG_GPIO_PIN   20u
#define BOOT_SOURCE_GPIO_PIN 21u
#define PROGRESS_CHUNK_SIZE  0x4000u

typedef struct {
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
} bin_header_t;

struct boot_source;
typedef int (*boot_read_fn)(const struct boot_source *source,
    uint32_t offset, volatile uint8_t *dst, uint32_t size);

typedef struct boot_source {
    boot_read_fn read;
    sdspi_dev_t *sdspi;
    uint32_t start_lba;
    uint32_t sectors;
} boot_source_t;

typedef struct {
    volatile uint8_t *dst;
    uint32_t size;
} boot_section_t;

static uint32_t align4(uint32_t value)
{
    return (value + 3u) & ~3u;
}

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
        ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static int progress_on(void)
{
    return (GPIO_IN >> BOOT_PROG_GPIO_PIN) & 1u;
}

static void uart_putc(char value)
{
    UART_TX = (uint32_t)value;
}

static __attribute__((noinline)) void uart_puts(const char *text)
{
    while (*text != '\0') {
        uart_putc(*text++);
    }
}

static __attribute__((noinline)) void print_dec(uint32_t value)
{
    char digits[10];
    uint32_t count = 0;

    do {
        digits[count++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value != 0u);
    while (count != 0u) {
        uart_putc(digits[--count]);
    }
}

static __attribute__((noinline)) void print_progress(uint32_t section,
    uint32_t loaded, uint32_t total)
{
    uint32_t loaded_kib = (loaded + 1023u) / 1024u;
    uint32_t total_kib = (total + 1023u) / 1024u;

    uart_puts("\rload ");
    if (section == 0u) {
        uart_puts("FIRMWARE");
    } else if (section == 1u) {
        uart_puts("KERNEL");
    } else if (section == 2u) {
        uart_puts("INITRD");
    } else {
        uart_puts("DTB");
    }
    uart_puts(": ");
    print_dec(loaded_kib);
    uart_puts("kB / ");
    print_dec(total_kib);
    uart_puts("kB (");
    print_dec(total_kib == 0u ? 100u : loaded_kib * 100u / total_kib);
    uart_puts(loaded == total ? "%)\033[K\n" : "%)\033[K");
}

static void copy_bytes(const uint8_t *src, volatile uint8_t *dst,
    uint32_t size)
{
    while (size >= sizeof(uint32_t)) {
        *(volatile uint32_t *)dst = *(const uint32_t *)src;
        src += sizeof(uint32_t);
        dst += sizeof(uint32_t);
        size -= sizeof(uint32_t);
    }
    while (size-- != 0u) {
        *dst++ = *src++;
    }
}

static void cache_clean(uint32_t addr, uint32_t size)
{
    uint32_t end = (addr + size + CACHE_BLOCK_SIZE - 1u) &
        ~(CACHE_BLOCK_SIZE - 1u);

    addr &= ~(CACHE_BLOCK_SIZE - 1u);
    for (; addr < end; addr += CACHE_BLOCK_SIZE) {
        __asm__ volatile (".insn i 0x0f, 0x2, x0, %0, 1"
            : : "r" (addr) : "memory");
    }
    __asm__ volatile ("fence rw, rw" : : : "memory");
}

static int source_range_valid(const boot_source_t *source, uint32_t offset,
    uint32_t size)
{
    uint32_t end;
    uint32_t sectors;

    if (size > UINT32_MAX - offset) {
        return 0;
    }
    end = offset + size;
    sectors = end == 0u ? 0u : ((end - 1u) >> SD_SECTOR_SHIFT) + 1u;
    return sectors <= source->sectors;
}

static int qspi_read(const boot_source_t *source, uint32_t offset,
    volatile uint8_t *dst, uint32_t size)
{
    (void)source;
    copy_bytes(FLASH_BASE + offset, dst, size);
    return 0;
}

static int sd_read(const boot_source_t *source, uint32_t offset,
    volatile uint8_t *dst, uint32_t size)
{
    while (size != 0u) {
        uint32_t sector_offset = offset & (SD_SECTOR_SIZE - 1u);
        uint32_t copy_size = SD_STAGE_SIZE - sector_offset;
        uint32_t blocks;

        if (copy_size > size) {
            copy_size = size;
        }
        blocks = (sector_offset + copy_size + SD_SECTOR_SIZE - 1u) >>
            SD_SECTOR_SHIFT;
        if (blocks == 0u || blocks > SD_MAX_BLOCKS ||
            sdspi_read_blocks(source->sdspi,
                source->start_lba + (offset >> SD_SECTOR_SHIFT), blocks,
                SD_STAGE) != 0) {
            return -1;
        }
        copy_bytes(SD_STAGE + sector_offset, dst, copy_size);
        dst += copy_size;
        offset += copy_size;
        size -= copy_size;
    }
    return 0;
}

static int boot_read(const boot_source_t *source, uint32_t offset,
    volatile uint8_t *dst, uint32_t size)
{
    uint32_t dst_addr;

    if (!source_range_valid(source, offset, size) ||
        source->read(source, offset, dst, size) != 0) {
        return -1;
    }
    dst_addr = (uint32_t)dst;
    if (size != 0u && dst_addr >= DDR_BASE && dst_addr < DDR_END) {
        cache_clean(dst_addr, size);
    }
    return 0;
}

static int ddr_range_valid(uint32_t addr, uint32_t size)
{
    return size == 0u || (addr >= DDR_BASE && addr < DDR_END &&
        size <= DDR_END - addr);
}

static int header_valid(const bin_header_t *header,
    const boot_source_t *source)
{
    uint32_t total = BIN_HEADER_SIZE;
    const uint32_t sizes[4] = {
        header->firmware_size, header->kernel_size,
        header->initrd_size, header->dtb_size
    };

    if (header->type > BIN_TYPE_LINUX || header->reserved != 0u ||
        header->firmware_size == 0u ||
        header->firmware_size > FIRMWARE_WINDOW_SIZE - BOOT_STACK_SIZE) {
        return 0;
    }
    if (header->type == BIN_TYPE_BARE &&
        (header->kernel_size != 0u || header->initrd_size != 0u ||
        header->dtb_size != 0u)) {
        return 0;
    }
    if (header->type == BIN_TYPE_LINUX &&
        (!ddr_range_valid(header->kernel_load, header->kernel_size) ||
        !ddr_range_valid(header->initrd_load, header->initrd_size) ||
        !ddr_range_valid(header->dtb_load, header->dtb_size))) {
        return 0;
    }

    for (uint32_t i = 0; i < 4u; i++) {
        uint32_t aligned = align4(sizes[i]);
        if (aligned < sizes[i] || aligned > UINT32_MAX - total) {
            return 0;
        }
        total += aligned;
    }
    return source_range_valid(source, 0u, total);
}

static int load_image(const boot_source_t *source, bin_header_t *header)
{
    uint32_t offset = BIN_HEADER_SIZE;
    uint32_t section_count;
    boot_section_t sections[4];

    if (boot_read(source, 0u, (volatile uint8_t *)header,
        sizeof(*header)) != 0 || !header_valid(header, source)) {
        return -1;
    }

    sections[0] = (boot_section_t){ FIRMWARE_BASE, header->firmware_size };
    sections[1] = (boot_section_t){
        (volatile uint8_t *)header->kernel_load, header->kernel_size
    };
    sections[2] = (boot_section_t){
        (volatile uint8_t *)header->initrd_load, header->initrd_size
    };
    sections[3] = (boot_section_t){
        (volatile uint8_t *)header->dtb_load, header->dtb_size
    };
    section_count = header->type == BIN_TYPE_BARE ? 1u : 4u;
    for (uint32_t i = 0; i < section_count; i++) {
        uint32_t loaded = 0u;
        while (loaded < sections[i].size) {
            uint32_t chunk = sections[i].size - loaded;
            if (chunk > PROGRESS_CHUNK_SIZE) {
                chunk = PROGRESS_CHUNK_SIZE;
            }
            if (boot_read(source, offset + loaded,
                sections[i].dst + loaded, chunk) != 0) {
                return -1;
            }
            loaded += chunk;
            if (progress_on()) {
                print_progress(i, loaded, sections[i].size);
            }
        }
        offset += align4(sections[i].size);
    }
    return 0;
}

static int sd_source_init(boot_source_t *source, sdspi_dev_t *sdspi)
{
    uint32_t start_lba;
    uint32_t sectors;

    if (sdspi_init(sdspi, SDSPI_WAIT_POLL) != 0 ||
        sdspi_read_blocks(sdspi, 0u, 1u, SD_STAGE) != 0 ||
        SD_STAGE[510] != 0x55u || SD_STAGE[511] != 0xaau ||
        SD_STAGE[450] != SD_PART_TYPE_RAW) {
        return -1;
    }

    start_lba = read_le32(SD_STAGE + 454);
    sectors = read_le32(SD_STAGE + 458);
    if (start_lba == 0u || sectors == 0u ||
        sectors > UINT32_MAX - start_lba) {
        return -1;
    }
    source->read = sd_read;
    source->sdspi = sdspi;
    source->start_lba = start_lba;
    source->sectors = sectors;
    return 0;
}

static int boot_from_sdcard(void)
{
    return (GPIO_IN >> BOOT_SOURCE_GPIO_PIN) & 1u;
}

static void boot_jump(const bin_header_t *header)
{
    PCM_CLEAR = 1u;
    __asm__ volatile ("fence rw, rw\n.word 0x0000100f" : : : "memory");
    if (header->type == BIN_TYPE_BARE) {
        __asm__ volatile ("jr %0" : : "r" (FIRMWARE_BASE));
    }
    __asm__ volatile (
        "mv a0, zero\n\t"
        "mv a1, %1\n\t"
        "jr %0"
        : : "r" (FIRMWARE_BASE), "r" (header->dtb_load) : "a0", "a1");
}

void boot_main(void)
{
    boot_source_t source;
    sdspi_dev_t sdspi;
    bin_header_t header;
    int sd_boot;
    int source_ok;

#ifdef RVB_SIM
    UART_BC = 9u;
#else
    UART_BC = 867u;
#endif
    GPIO_MODE_HI = (GPIO_MODE_HI & ~((3u << 8) | (3u << 10))) |
        (1u << 8) | (1u << 10);

    sd_boot = boot_from_sdcard();
    if (sd_boot) {
        source_ok = sd_source_init(&source, &sdspi);
    } else {
        source.read = qspi_read;
        source.sdspi = 0;
        source.start_lba = 0u;
        source.sectors = FLASH_SIZE / SD_SECTOR_SIZE;
        source_ok = 0;
    }

    if (progress_on()) {
        uart_puts(sd_boot ? "boot: SD\n" : "boot: QSPI\n");
        uart_puts(">>>>> starting rvbucket bootloader ...\n");
    }
    if (source_ok == 0 && load_image(&source, &header) == 0) {
        if (progress_on()) {
            uart_puts(header.type == BIN_TYPE_BARE ?
                ">>>>> bootloader done, jumping to user program ...\n" :
                ">>>>> bootloader done, jumping to OpenSBI ...\n");
        }
        boot_jump(&header);
    }

    if (progress_on()) {
        uart_puts(sd_boot ? "boot: no valid SD image\n" :
            "boot: no valid QSPI image\n");
    } else {
        uart_puts("boot: no valid image\n");
    }
    for (;;) {
    }
}

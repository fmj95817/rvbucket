#include <stdint.h>

#include "boot_config.h"
#include "boot_console.h"
#include "boot_loader.h"
#include "boot_media.h"
#include "drivers/cache/cache.h"
#include "drivers/gpio/gpio.h"
#include "drivers/pcm/pcm.h"

static boot_media_kind_t selected_media(void)
{
    return ((gpio_read() >> BOOT_SOURCE_GPIO_PIN) & 1u) != 0u ?
        BOOT_MEDIA_SDCARD : BOOT_MEDIA_QSPI;
}

static void boot_jump(const rvb_bin_header_t *header)
    __attribute__((noreturn));

static void boot_jump(const rvb_bin_header_t *header)
{
    pcm_clear();
    cache_sync_instruction_stream();
    if (header->type == RVB_BIN_TYPE_BARE) {
        __asm__ volatile ("jr %0" : : "r" (BOOT_FIRMWARE_BASE_ADDR));
    } else {
        __asm__ volatile (
            "mv a0, zero\n\t"
            "mv a1, %1\n\t"
            "jr %0"
            : : "r" (BOOT_FIRMWARE_BASE_ADDR), "r" (header->dtb_load)
            : "a0", "a1");
    }
    __builtin_unreachable();
}

void boot_main(void)
{
    boot_media_t media;
    rvb_bin_header_t header;
    boot_media_kind_t kind;

    boot_console_init();
    gpio_set_mode(BOOT_PROGRESS_GPIO_PIN, GPIO_MODE_IN);
    gpio_set_mode(BOOT_SOURCE_GPIO_PIN, GPIO_MODE_IN);
    kind = selected_media();

    if (boot_console_enabled()) {
        boot_console_puts(kind == BOOT_MEDIA_SDCARD ?
            "boot: SD\n" : "boot: QSPI\n");
        boot_console_puts(">>>>> starting rvbucket bootloader ...\n");
    }
    if (boot_media_open(&media, kind) == 0 &&
        boot_load_image(&media, &header) == 0) {
        if (boot_console_enabled()) {
            boot_console_puts(header.type == RVB_BIN_TYPE_BARE ?
                ">>>>> bootloader done, jumping to user program ...\n" :
                ">>>>> bootloader done, jumping to OpenSBI ...\n");
        }
        boot_jump(&header);
    }

    if (boot_console_enabled()) {
        boot_console_puts(kind == BOOT_MEDIA_SDCARD ?
            "boot: no valid SD image\n" : "boot: no valid QSPI image\n");
    } else {
        boot_console_puts("boot: no valid image\n");
    }
    for (;;) {
    }
}

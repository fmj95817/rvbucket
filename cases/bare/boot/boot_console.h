#ifndef BOOT_CONSOLE_H
#define BOOT_CONSOLE_H

#include <stdint.h>
#include "boot_image.h"

extern void boot_console_init(void);
extern int boot_console_enabled(void);
extern void boot_console_puts(const char *text);
extern void boot_console_put_hex32(uint32_t value);
extern void boot_console_progress(rvb_bin_section_t section, uint32_t loaded,
    uint32_t total);

#endif

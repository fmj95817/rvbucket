#include "boot_console.h"

#include "boot_config.h"
#include "drivers/gpio/gpio.h"
#include "drivers/uart/uart.h"

static void print_dec(uint32_t value)
{
    char digits[10];
    uint32_t count = 0;

    do {
        digits[count++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value != 0u);
    while (count != 0u) {
        uart_write(digits[--count]);
    }
}

void boot_console_init(void)
{
#ifdef RVB_SIM
    uart_init(9u);
#else
    uart_init(867u);
#endif
}

int boot_console_enabled(void)
{
    return (gpio_read() >> BOOT_PROGRESS_GPIO_PIN) & 1u;
}

void boot_console_puts(const char *text)
{
    while (*text != '\0') {
        uart_write(*text++);
    }
}

void boot_console_progress(rvb_bin_section_t section, uint32_t loaded,
    uint32_t total)
{
    uint32_t loaded_kib = (loaded + 1023u) / 1024u;
    uint32_t total_kib = (total + 1023u) / 1024u;

    boot_console_puts("\rload ");
    switch (section) {
    case RVB_BIN_SECTION_FIRMWARE:
        boot_console_puts("FIRMWARE");
        break;
    case RVB_BIN_SECTION_KERNEL:
        boot_console_puts("KERNEL");
        break;
    case RVB_BIN_SECTION_INITRD:
        boot_console_puts("INITRD");
        break;
    default:
        boot_console_puts("DTB");
        break;
    }
    boot_console_puts(": ");
    print_dec(loaded_kib);
    boot_console_puts("kB / ");
    print_dec(total_kib);
    boot_console_puts("kB (");
    print_dec(total_kib == 0u ? 100u : loaded_kib * 100u / total_kib);
    boot_console_puts(loaded == total ? "%)\033[K\n" : "%)\033[K");
}

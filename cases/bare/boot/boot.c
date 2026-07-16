#include <stdint.h>

#include "drivers/pcm/pcm_map.h"

#define FLASH_BASE   ((const uint32_t *)0x80000000)
#define FIRMWARE_BASE ((volatile uint32_t *)0x40000000)
#define UART_BC      (*(volatile uint32_t *)0x30000000u)
#define UART_TX      (*(volatile uint32_t *)0x30000004u)
#define GPIO_IN      (*(volatile uint32_t *)0x30001000u)
#define GPIO_MODE_LO (*(volatile uint32_t *)0x30001004u)
#define GPIO_MODE_HI (*(volatile uint32_t *)0x30001008u)

#define PCM_BASE     0x30003000u
#define PCM_CLEAR    (*(volatile uint32_t *)(PCM_BASE + PCM_REG_CLEAR))

#define PROG_CHUNK_SHIFT 14u
#define PROG_CHUNK_BYTES (1u << PROG_CHUNK_SHIFT)
#define PROG_CHUNK_WORDS (PROG_CHUNK_BYTES >> 2)
#define PROG_CHUNK_MASK  (PROG_CHUNK_BYTES - 1u)

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

static uint32_t align4(uint32_t v)
{
    return (v + 3u) & ~3u;
}

static uint32_t div_u(uint32_t a, uint32_t b)
{
    uint32_t q;
    __asm__ volatile("divu %0, %1, %2" : "=r"(q) : "r"(a), "r"(b));
    return q;
}

static uint32_t rem_u(uint32_t a, uint32_t b)
{
    uint32_t r;
    __asm__ volatile("remu %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
    return r;
}

static void gpio_set_input(uint32_t pin)
{
    volatile uint32_t *reg;
    uint32_t shift;
    if (pin < 16) {
        reg = &GPIO_MODE_LO;
        shift = pin * 2;
    } else {
        reg = &GPIO_MODE_HI;
        shift = (pin - 16) * 2;
    }
    *reg = (*reg & ~(3u << shift)) | (1u << shift);
}

static int progress_on(void)
{
    return (GPIO_IN >> 20) & 1u;
}

static void uart_putc(char c)
{
    UART_TX = (uint32_t)c;
}

static void uart_init(void)
{
#ifdef RVB_SIM
    UART_BC = 9u;
#else
    UART_BC = 867u;
#endif
}

static void uart_puts(const char *s)
{
    while (*s) {
        uart_putc(*s++);
    }
}

static void print_dec(uint32_t n)
{
    char buf[10];
    int i = 0;
    do {
        buf[i++] = '0' + rem_u(n, 10);
        n = div_u(n, 10);
    } while (n);
    while (i) {
        uart_putc(buf[--i]);
    }
}

static void print_progress(uint32_t sec, uint32_t cur_kb,
                           uint32_t kb_total, uint32_t pct, int done)
{
    switch (sec) {
        case 0:
            uart_puts("\rload FIRMWARE: ");
            break;
        case 1:
            uart_puts("\rload KERNEL: ");
            break;
        case 2:
            uart_puts("\rload INITRD: ");
            break;
        case 3:
            uart_puts("\rload DTB: ");
            break;
    }
    print_dec(cur_kb);
    uart_puts("kB / ");
    print_dec(kb_total);
    uart_puts("kB (");
    print_dec(pct);
    uart_puts("%)\033[K");
    if (done) {
        uart_putc('\n');
    }
}

static void copy_sec(const uint32_t *src, volatile uint32_t *dst,
                     uint32_t size, uint32_t sec)
{
    uint32_t words = (size + 3u) >> 2;
    uint32_t kb_total = div_u(size + 1023, 1024);
    uint32_t bytes = 0, next_tick = PROG_CHUNK_BYTES;

    for (uint32_t i = 0; i < words; i++) {
        dst[i] = src[i];
        bytes += 4;
        if (bytes < next_tick) {
            continue;
        }
        next_tick += PROG_CHUNK_BYTES;

        if (progress_on()) {
            uint32_t cur_kb = div_u(bytes + 1023, 1024);
            uint32_t pct = div_u(cur_kb * 100, kb_total);
            print_progress(sec, cur_kb, kb_total, pct, 0);
        }
    }

    if (progress_on()) {
        print_progress(sec, kb_total, kb_total, 100, 1);
    }
}

static void perf_clear(void)
{
    PCM_CLEAR = 1u;
}

static void sync_loaded_image(void)
{
    __asm__ volatile("fence rw, rw\n.word 0x0000100f" ::: "memory");
}

void boot_main(void)
{
    uart_init();
    gpio_set_input(20);

    if (progress_on()) {
        uart_puts("\033[?25l>>>>> starting rvbucket bootloader ...\n");
    }

    const bin_header_t *hdr = (const bin_header_t *)FLASH_BASE;
    const uint8_t *src = (const uint8_t *)hdr + 40;
    uint32_t a;

    copy_sec((const uint32_t *)src, FIRMWARE_BASE, hdr->firmware_size, 0);
    a = align4(hdr->firmware_size);
    src += a;

    if (hdr->type != 1) {
        if (progress_on()) {
            uart_puts("\033[?25h>>>>> bootloader done, jumping to user program ...\n");
        }
        sync_loaded_image();
        perf_clear();
        __asm__ volatile("jr %0" : : "r"(FIRMWARE_BASE));
    }

    copy_sec((const uint32_t *)src, (volatile uint32_t *)hdr->kernel_load, hdr->kernel_size, 1);
    a = align4(hdr->kernel_size);
    src += a;

    copy_sec((const uint32_t *)src, (volatile uint32_t *)hdr->initrd_load, hdr->initrd_size, 2);
    a = align4(hdr->initrd_size);
    src += a;

    copy_sec((const uint32_t *)src, (volatile uint32_t *)hdr->dtb_load, hdr->dtb_size, 3);

    if (progress_on()) {
        uart_puts("\033[?25h>>>>> bootloader done, jumping to OpenSBI ...\n");
    }
    sync_loaded_image();
    perf_clear();
    __asm__ volatile(
        "mv a0, zero\n\t"
        "mv a1, %1\n\t"
        "jr %0"
        :
        : "r"(FIRMWARE_BASE), "r"(hdr->dtb_load)
        : "a0", "a1"
    );
}

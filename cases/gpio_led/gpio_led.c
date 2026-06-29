#include <stdio.h>
#include <stdint.h>
#include "drivers/gpio/gpio.h"
#include "drivers/timer/gtimer.h"

static const int spiral[] = {
    0, 1, 2, 3, 7, 11, 15, 14, 13, 12, 8, 4, 5, 6, 10, 9
};

static void delay_ms(uint32_t ms)
{
    /* timer runs at sim clock rate; assume ~1 MHz for demo */
    gtimer_start(ms * 1000u);
    gtimer_wait();
}

int main(void)
{
    printf("gpio_led: spiral start\n");

    /* single-LED sweep */
    for (int round = 0; round < 3; round++) {
        for (int i = 0; i < 16; i++) {
            gpio_write(1u << spiral[i]);
            printf("  led[%02d] = 0x%04lx\n", spiral[i],
                   (unsigned long)(1u << spiral[i]));
            delay_ms(50);
        }
    }

    /* tail effect */
    for (int round = 0; round < 3; round++) {
        uint32_t val = 0;
        for (int i = 0; i < 16; i++) {
            val |= 1u << spiral[i];
            gpio_write(val);
            delay_ms(80);
        }
        for (int i = 15; i >= 0; i--) {
            val &= ~(1u << spiral[i]);
            gpio_write(val);
            delay_ms(80);
        }
    }

    gpio_write(0);
    printf("gpio_led: done\n");
    return 0;
}

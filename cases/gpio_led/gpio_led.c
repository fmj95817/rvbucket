#include <stdio.h>
#include <stdint.h>
#include "drivers/gpio/gpio.h"

int main(void)
{
    printf("gpio_led: start\n");

    /* running light: cycle one LED at a time [0..15] */
    for (int round = 0; round < 10; round++) {
        for (int i = 0; i < 16; i++) {
            uint32_t val = 1u << i;
            gpio_write(val);
            printf("  led[%02d] = 0x%04lx\n", i, (unsigned long)val);
            /* delay loop */
            for (volatile int d = 0; d < 50000; d++) { }
        }
    }

    gpio_write(0);
    printf("gpio_led: done\n");

    return 0;
}

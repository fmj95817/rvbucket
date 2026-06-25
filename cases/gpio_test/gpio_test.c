#include <stdio.h>
#include <stdint.h>
#include "drivers/gpio/gpio.h"

int main()
{
    printf("gpio_test: start\n");

    uint32_t test_vals[] = { 0xdeadbeef, 0xcafebabe, 0x12345678, 0x00000000 };
    uint32_t passed = 0;
    uint32_t total = sizeof(test_vals) / sizeof(test_vals[0]);

    for (uint32_t i = 0; i < total; i++) {
        gpio_write(test_vals[i]);
        uint32_t rd = gpio_read();

        printf("  write=%lx read=%lx", (unsigned long)test_vals[i], (unsigned long)rd);

        if (rd == test_vals[i]) {
            printf(" [PASS]\n");
            passed++;
        } else {
            printf(" [FAIL]\n");
        }
    }

    printf("gpio_test: %lx/%lx passed\n", (unsigned long)passed, (unsigned long)total);

    return 0;
}

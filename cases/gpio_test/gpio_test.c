#include <stdio.h>
#include <stdint.h>
#include "drivers/gpio/gpio.h"

static void delay(void)
{
    for (volatile int d = 0; d < 2000; d++) { }
}

int main(void)
{
    printf("gpio_test: start\n");

    /* configure input pins */
    printf("gpio_test: setting GPIO 16-19 as input+irq, 20-23 as input\n");
    for (uint32_t pin = 16; pin <= 19; pin++)
        gpio_set_mode(pin, GPIO_MODE_IN_IRQ);
    for (uint32_t pin = 20; pin <= 23; pin++)
        gpio_set_mode(pin, GPIO_MODE_IN);

    printf("gpio_test: terminal cmd mode (ESC):\n");
    printf("  p1/p2 = toggle LED 0/1\n");
    printf("  s1/s2 = toggle LED 2/3\n");
    printf("  r     = reset,  i = interactive\n");

    uint32_t last_in = gpio_read() & 0xff0000u;
    uint32_t led_state = 0;

    while (1) {
        delay();
        uint32_t cur = gpio_read();
        uint32_t cur_in = cur & 0xff0000u;

        /* p1 = GPIO 16 → each edge toggles LED 0 */
        if ((cur_in ^ last_in) & (1u << 16)) {
            led_state ^= 1u << 0;
            printf("  p1 -> LED 0 %s  (leds=0x%02lx)\n",
                   (led_state & 1u) ? "ON" : "OFF", (unsigned long)led_state);
        }
        /* p2 = GPIO 17 → each edge toggles LED 1 */
        if ((cur_in ^ last_in) & (1u << 17)) {
            led_state ^= 1u << 1;
            printf("  p2 -> LED 1 %s  (leds=0x%02lx)\n",
                   (led_state & 2u) ? "ON" : "OFF", (unsigned long)led_state);
        }
        /* s1 = GPIO 20 → LED 2 on/off */
        if ((cur_in ^ last_in) & (1u << 20)) {
            if (cur_in & (1u << 20)) led_state |= 1u << 2;
            else                     led_state &= ~(1u << 2);
            printf("  s1 -> LED 2 %s  (leds=0x%02lx)\n",
                   (led_state & 4u) ? "ON" : "OFF", (unsigned long)led_state);
        }
        /* s2 = GPIO 21 → LED 3 on/off */
        if ((cur_in ^ last_in) & (1u << 21)) {
            if (cur_in & (1u << 21)) led_state |= 1u << 3;
            else                     led_state &= ~(1u << 3);
            printf("  s2 -> LED 3 %s  (leds=0x%02lx)\n",
                   (led_state & 8u) ? "ON" : "OFF", (unsigned long)led_state);
        }

        last_in = cur_in;
        gpio_write(led_state);
    }

    return 0;
}

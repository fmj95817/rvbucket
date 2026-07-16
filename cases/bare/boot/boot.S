    .section .text.boot, "ax"
    .globl _boot
    .type _boot,@function

_boot:
    /* Reserve the top 8KB of the 2MB DDR firmware window for the stack. */
    li sp, 0x40200000
    call boot_main
    j .

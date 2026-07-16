    .section .text.boot, "ax"
    .globl _boot
    .type _boot,@function

_boot:
    /* Reserve the top 8KB of the 2MB DDR firmware window for the stack. */
    li sp, 0x40200000

    /* Check firmware_size + stack_size <= firmware window size. */
    li t0, 0x80000000
    lw t1, 4(t0)
    li t2, 0x2000
    add t1, t1, t2
    li t2, 0x200000
    bgeu t2, t1, 1f
    j .

1:
    call boot_main
    j .

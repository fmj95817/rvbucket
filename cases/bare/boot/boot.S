    .section .text.boot, "ax"
    .globl _boot
    .type _boot,@function

_boot:
    /* sp = DTCM_BASE + DTCM_SIZE (256KB, grows down) */
    li sp, 0x20040000

    /* read flash header to check dtcm overflow */
    li t0, 0x80000000
    lw t1, 0(t0)
    li t2, 1
    bltu t2, t1, 1f          /* first word > 1 → bare: itcm_size */

    /* new header: type (t1) is 0 or 1 */
    lw t1, 8(t0)             /* dtcm_size */
    j 2f

1:  /* bare header */
    lw t1, 4(t0)             /* dtcm_size */

2:  /* check dtcm_size + stack_margin <= DTCM_SIZE */
    li t2, 2048              /* 2KB stack margin */
    add t1, t1, t2
    li t2, 0x40000           /* DTCM_SIZE = 256KB */
    bge t2, t1, 3f
    j .                      /* overflow: spin forever */

3:
    call boot_main
    j .

    .section .text
    .globl _boot
	.type _boot,@function

_boot:
    /* x1 = flash_addr */
    li x1, 0x80000000
    /* x2 = itcm_addr */
    li x2, 0x10000000
    /* x3 = dtcm_addr */
    li x3, 0x20000000

    /* read itcm size to x4 */
    lw x4, 0(x1)
    /* read dtcm size to x5 */
    lw x5, 4(x1)
    /* move x1 to itcm data */
    addi x1, x1, 8

    /* loop var x8 = 0 */
    li x8, 0

itcm_loop:
    /* while (x8 < itcm_size) */
    bge x8, x4, load_itcm_done

    /* read word to x7 from flash */
    add x6, x1, x8
    lw x7, 0(x6)

    /* store word x7 to ITCM */
    add x6, x2, x8
    sw x7, 0(x6)

    /* x8 += 4 */
    addi x8, x8, 4

    j itcm_loop

load_itcm_done:
    /* clear loop var x8 */
    li x8, 0
    /* move x1 to itcm data */
    add x1, x1, x4

dtcm_loop:
    /* while (x8 < dtcm_size) */
    bge x8, x5, load_dtcm_done

    /* read word to x7 from flash */
    add x6, x1, x8
    lw x7, 0(x6)

    /* store word x7 to DTCM */
    add x6, x3, x8
    sw x7, 0(x6)

    /* x8 += 4 */
    addi x8, x8, 4

    j dtcm_loop

load_dtcm_done:
    /* jump to user program */
    li x9, 0x10000000
    jalr 0(x9)

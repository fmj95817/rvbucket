    .section .text
    .globl _boot
	.type _boot,@function

_boot:
    /* x1 = flash_addr */
    li x1, 0x80000000
    /* x3 = tcm_addr */
    li x2, 0x10000000

    /* read program size to x4 */
    lw x4, 0(x1)

    /* move x1 to program data */
    addi x1, x1, 4

    /* loop var x3 = 0 */
    li x3, 0

loop:
    /* loop copy */
    bge x3, x4, load_done

    /* read word to x6 from flash */
    add x5, x1, x3
    lw x6, 0(x5)

    /* store word x6 to TCM */
    add x5, x2, x3
    sw x6, 0(x5)

    /* x3 += 4 */
    addi x3, x3, 4

    j loop

load_done:
    /* jump to user program */
    li x7, 0x10000000
    jalr 0(x7)

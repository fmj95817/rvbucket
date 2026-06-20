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

    /* Read image header. New format starts with type. Legacy format starts with itcm size. */
    lw x31, 0(x1)
    li x30, 1
    bltu x30, x31, legacy_header

    /* read itcm size to x4 */
    lw x4, 4(x1)
    /* read dtcm size to x5 */
    lw x5, 8(x1)
    /* read linux payload metadata */
    lw x10, 12(x1)
    lw x11, 16(x1)
    lw x14, 20(x1)
    lw x12, 24(x1)
    lw x13, 28(x1)
    lw x15, 32(x1)
    /* move x1 to itcm data */
    addi x1, x1, 40
    j header_done

legacy_header:
    /* legacy image: first word is itcm size */
    lw x4, 0(x1)
    /* read dtcm size to x5 */
    lw x5, 4(x1)
    /* legacy image behaves like bare type */
    li x31, 0
    li x10, 0
    li x11, 0
    li x12, 0
    li x13, 0
    li x14, 0
    li x15, 0
    /* move x1 to itcm data */
    addi x1, x1, 8

header_done:
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
    /* move x1 to aligned dtcm data */
    addi x6, x4, 3
    andi x6, x6, -4
    add x1, x1, x6

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
    /* move x1 to aligned dtcm data end */
    addi x6, x5, 3
    andi x6, x6, -4
    add x1, x1, x6

    /* bare image has no DDR payload */
    beqz x31, boot_done

    /* copy linux kernel to DDR: x10=size, x12=load address */
    li x8, 0
kernel_loop:
    bge x8, x10, load_kernel_done
    add x6, x1, x8
    lw x7, 0(x6)
    add x6, x12, x8
    sw x7, 0(x6)
    addi x8, x8, 4
    j kernel_loop

load_kernel_done:
    addi x6, x10, 3
    andi x6, x6, -4
    add x1, x1, x6

    /* copy initrd to DDR: x11=size, x13=load address */
    li x8, 0
initrd_loop:
    bge x8, x11, load_initrd_done
    add x6, x1, x8
    lw x7, 0(x6)
    add x6, x13, x8
    sw x7, 0(x6)
    addi x8, x8, 4
    j initrd_loop

load_initrd_done:
    addi x6, x11, 3
    andi x6, x6, -4
    add x1, x1, x6

    /* copy device tree to DDR: x14=size, x15=load address */
    li x8, 0
dtb_loop:
    bge x8, x14, boot_done
    add x6, x1, x8
    lw x7, 0(x6)
    add x6, x15, x8
    sw x7, 0(x6)
    addi x8, x8, 4
    j dtb_loop

boot_done:
    beqz x31, bare_jump
    li x10, 0
    add x11, x15, x0

bare_jump:
    /* jump to user program */
    li x9, 0x10000000
    jalr 0(x9)

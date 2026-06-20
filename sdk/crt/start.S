    .section .text.entry
    .align 2
    .globl _trap_entry
    .type _trap_entry,@function

_trap_entry:
    addi sp, sp, -64
    sw ra,  0(sp)
    sw t0,  4(sp)
    sw t1,  8(sp)
    sw t2, 12(sp)
    sw a0, 16(sp)
    sw a1, 20(sp)
    sw a2, 24(sp)
    sw a3, 28(sp)
    sw a4, 32(sp)
    sw a5, 36(sp)
    sw a6, 40(sp)
    sw a7, 44(sp)
    sw t3, 48(sp)
    sw t4, 52(sp)
    sw t5, 56(sp)
    sw t6, 60(sp)

    csrr a0, mcause
    csrr a1, mepc
    csrr a2, mtval
    call trap_handler
    csrw mepc, a0

    lw ra,  0(sp)
    lw t0,  4(sp)
    lw t1,  8(sp)
    lw t2, 12(sp)
    lw a0, 16(sp)
    lw a1, 20(sp)
    lw a2, 24(sp)
    lw a3, 28(sp)
    lw a4, 32(sp)
    lw a5, 36(sp)
    lw a6, 40(sp)
    lw a7, 44(sp)
    lw t3, 48(sp)
    lw t4, 52(sp)
    lw t5, 56(sp)
    lw t6, 60(sp)
    addi sp, sp, 64
    mret

    .section .text
    .weak trap_handler
    .globl trap_handler
    .type trap_handler,@function

trap_handler:
    bgez a0, 1f
    /* interrupt: return mepc unchanged */
    mv a0, a1
    ret
1:
    /* exception: skip the faulting instruction */
    addi a0, a1, 4
    ret

    .section .init
    .globl _start
    .type _start,@function

_start:
    li x1, 0
    li x2, 0
    li x3, 0
    li x4, 0
    li x5, 0
    li x6, 0
    li x7, 0
    li x8, 0
    li x9, 0
    li x10, 0
    li x11, 0
    li x12, 0
    li x13, 0
    li x14, 0
    li x15, 0
    li x16, 0
    li x17, 0
    li x18, 0
    li x19, 0
    li x20, 0
    li x21, 0
    li x22, 0
    li x23, 0
    li x24, 0
    li x25, 0
    li x26, 0
    li x27, 0
    li x28, 0
    li x29, 0
    li x30, 0
    li x31, 0

    lui sp, %hi(__stack_top)
    addi sp, sp, %lo(__stack_top)

    la gp, __global_pointer$

    la t0, _trap_entry
    csrw mtvec, t0

    li a0, 9
    call uart_init

    call main

    li a0, 0x10
    call uart_write

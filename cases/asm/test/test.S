    .section .text
    .globl _asm_test
	.type _asm_test,@function

_asm_test:
    li x1, 0
    li x2, 0
    li x3, 0
    li x4, 0
    li x5, 0
    li x6, 0
    li x7, 0
    li x8, 0
    li x9, 3
    addi x1, x9, 11
    addi x9, x9, 3

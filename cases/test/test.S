    .section .text
    .globl _asm_test
	.type _asm_test,@function

_asm_test:
    li x1, 0
    li x2, 0
    li x3, 0
loop:
    addi x3, x3, 5
    li x9, 3
    addi x1, x9, 11
    addi x9, x9, 3
    sw x9, 0(x1)
    lw x10, 0(x1)
    j loop
    li x4, 100

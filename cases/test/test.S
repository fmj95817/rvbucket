    .section .text
    .globl _asm_test
	.type _asm_test,@function

_asm_test:
    li x1, 0
    li x2, 0x02000100
    li x3, 0x05000200
    li x4, 0x123
    sw x4, 0(x1)
    sw x4, 0(x2)
    sw x4, 0(x3)
    lw x5, 0(x1)
    lw x6, 0(x2)
    lw x7, 0(x3)
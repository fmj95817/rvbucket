    .section .text.l1_id_payload, "ax", @progbits
    .balign 64
    .global l1_id_payload_start
    .global l1_id_payload_end
    .type l1_id_payload_start, @function

l1_id_payload_start:
    li      t0, 32
    li      t2, 1
    li      t4, 0
1:
    li      t1, 0x41000000
    .rept 1024
    sw      t2, 0(t1)
    lw      t3, 0(t1)
    xor     t4, t4, t3
    addi    t1, t1, 64
    addi    t2, t2, 1
    nop
    nop
    nop
    nop
    .endr
    addi    t0, t0, -1
    beqz    t0, 2f
    j       1b
2:
    mv      a0, t4
    ret
l1_id_payload_end:
    .size l1_id_payload_start, .-l1_id_payload_start

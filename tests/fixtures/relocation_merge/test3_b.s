@ Test 3b : Relocation R_ARM_JUMP24 (b) dans .text
@ But : saut inconditionnel avec b, correction addend divisé par 4

.section .text
.global jumper3
jumper3:
    cmp r0, #0
    bne target3
    b target3
    nop

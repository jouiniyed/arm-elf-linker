@ Test 2a : Relocation R_ARM_CALL (bl) dans .text
@ But : vérifier correction offset et addend/4 pour relocations CALL

.section .text
.global target2
target2:
    mov r0, #100
    mov r1, #200
    add r0, r0, r1
    bx lr

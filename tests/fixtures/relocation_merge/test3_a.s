@ Test 3a : Relocation R_ARM_JUMP24 (b) dans .text
@ But : vérifier correction offset et addend/4 pour relocations JUMP24

.section .text
.global target3
target3:
    mov r0, #50
    mov r1, #30
    sub r0, r0, r1
    bx lr

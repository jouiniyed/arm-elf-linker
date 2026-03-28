@ Test 1a : Relocation R_ARM_ABS32 dans .text
@ But : vérifier correction offset et addend pour relocations absolues

.section .text
.global target1
target1:
    mov r0, #10
    mov r1, #20
    add r2, r0, r1
    bx lr

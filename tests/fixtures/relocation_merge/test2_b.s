@ Test 2b : Relocation R_ARM_CALL (bl) dans .text
@ But : appel de fonction avec bl, correction addend divisé par 4

.section .text
.global caller2
caller2:
    bl target2
    mov r0, #42
    bx lr

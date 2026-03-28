@ Test 4a : Relocation R_ARM_CALL
@ But : verifier correction des relocations d'appel de fonction

.section .text
.global helper
helper:
    lsl r0, r0, #1
    bx lr

@ Test 4b : Relocation R_ARM_CALL
@ But : appel bl vers fonction de fichier A, relocation a corriger

.section .text
.global process
process:
    push {lr}
    bl helper
    add r0, r0, #5
    pop {pc}

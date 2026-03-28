@ Test 6b : Relocations uniquement dans fichier B
@ But : toutes relocations proviennent du second fichier

.section .text
.global caller6
caller6:
    bl simple6
    mov r0, #0
    bx lr

.global user6
user6:
    ldr r0, =simple6
    blx r0
    bx lr

@ Test 7a : Combinaison de types de relocations
@ But : mélange ABS32, CALL et JUMP24 dans même fichier

.section .text
.global target7a
target7a:
    mov r0, #7
    bx lr

.global target7b
target7b:
    mov r1, #77
    bx lr

.section .data
.global ptr7a
ptr7a:
    .word target7a
    .word target7b

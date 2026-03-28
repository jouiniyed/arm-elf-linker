@ Test 7b : Combinaison de types de relocations
@ But : multiples relocations de types différents vers fichier A

.section .text
.global caller7
caller7:
    bl target7a
    b target7b
    ldr r2, =target7a
    ldr r3, =target7b
    ldr r4, =ptr7a
    bx lr

.section .data
.global data7b
data7b:
    .word target7a
    .word target7b
    .word ptr7a

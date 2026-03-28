@ Test 6a : Aucune relocation dans fichier A
@ But : vérifier que absence de relocations ne pose pas problème

.section .text
.global simple6
simple6:
    mov r0, #100
    mov r1, #200
    mov r2, #300
    add r0, r0, r1
    add r0, r0, r2
    bx lr

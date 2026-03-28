@ Test 2b : Symbole global 'calcul' défini ici, UND dans a
.text
.global calcul
calcul:
    mov r0, #100
    mov r1, #200
    add r0, r0, r1
    bx lr

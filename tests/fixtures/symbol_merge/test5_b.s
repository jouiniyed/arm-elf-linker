@ Test 5b : Symbole global 'uniquement_dans_b' présent seulement ici
.text
.global uniquement_dans_b
uniquement_dans_b:
    mov r0, #99
    bx lr

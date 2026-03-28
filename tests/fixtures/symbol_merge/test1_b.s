@ Test 1b : Symbole global 'fonction' UND (référencé mais non défini)
.text
.global appelant
.global fonction
appelant:
    bl fonction
    bx lr

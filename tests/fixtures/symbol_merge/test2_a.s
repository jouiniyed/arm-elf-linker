@ Test 2a : Symbole global 'calcul' UND (référencé mais non défini)
.text
.global main
.global calcul
main:
    bl calcul
    bx lr

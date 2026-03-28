@ Test 3b : Symbole global 'externe' UND dans les deux fichiers
.text
.global fonction_b
.global externe
fonction_b:
    bl externe
    bx lr

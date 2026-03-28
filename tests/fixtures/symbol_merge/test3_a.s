@ Test 3a : Symbole global 'externe' UND dans les deux fichiers
.text
.global fonction_a
.global externe
fonction_a:
    bl externe
    bx lr

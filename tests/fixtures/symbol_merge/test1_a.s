@ Test 1a : Symbole global 'fonction' défini ici, UND dans b
.text
.global fonction
fonction:
    mov r0, #42
    bx lr

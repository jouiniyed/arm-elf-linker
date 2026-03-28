@ Test 7b : Symbole 'conflit' défini aussi ici (INVALIDE - doit échouer)
.text
.global conflit
conflit:
    mov r0, #222
    bx lr

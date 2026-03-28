@ Test 6b : Double definition de symbole global
@ But : meme symbole defini => doit echouer

.section .data
.global duplicate_symbol
duplicate_symbol:
    .word 200

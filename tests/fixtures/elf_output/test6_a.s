@ Test 6a : Double definition de symbole global
@ But : detecter erreur de double definition

.section .data
.global duplicate_symbol
duplicate_symbol:
    .word 100

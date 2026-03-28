@ Test 2b : Concatenation de sections .text
@ But : verifier que symboles du fichier B sont decales correctement

.section .text
.global sub
sub:
    sub r0, r0, r1
    bx lr

.global divs
divs:
    sdiv r0, r0, r1
    bx lr

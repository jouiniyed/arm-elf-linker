@ Test 6b : Section .text avec symbole qui doit être décalé
.text
.global suite
suite:
    mov r0, #15
    mov r1, #25
    sub r2, r1, r0
    bx lr

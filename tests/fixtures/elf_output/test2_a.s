@ Test 2a : Concatenation de sections .text
@ But : verifier fusion de sections identiques et decalage des symboles

.section .text
.global add
add:
    add r0, r0, r1
    bx lr

.global mul
mul:
    mul r0, r0, r1
    bx lr

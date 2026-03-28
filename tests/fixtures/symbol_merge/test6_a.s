@ Test 6a : Section .text avec symbole à offset 0
.text
.global debut
debut:
    mov r0, #5
    mov r1, #10
    add r2, r0, r1
    bx lr

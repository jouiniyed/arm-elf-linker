@ Test 1 : Section .text commune aux deux fichiers
.text
.global func2
func2:
    mov r0, #5
    mov r1, #15
    sub r2, r1, r0
    bx lr

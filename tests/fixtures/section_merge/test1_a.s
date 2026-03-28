@ Test 1 : Section .text commune aux deux fichiers
.text
.global func1
func1:
    mov r0, #10
    mov r1, #20
    add r2, r0, r1
    bx lr

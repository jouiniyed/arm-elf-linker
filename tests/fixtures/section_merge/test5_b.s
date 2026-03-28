@ Test 5 : Fonction plus petite pour offset différent
.text
.global small_function
small_function:
    mov r0, #99
    bx lr

.data
.global flag
flag:
    .byte 1
    .align 2

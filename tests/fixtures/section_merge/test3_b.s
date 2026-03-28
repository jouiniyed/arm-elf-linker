@ Test 3 : Section .rodata uniquement dans fichier B
.text
.global print
print:
    ldr r0, =message
    bx lr

.section .rodata
.global message
message:
    .asciz "Hello World"
    .align 2

@ Test 3b : Resolution de symbole UNDEF
@ But : symbole defini dans B, resout le UNDEF de A

.section .text
.global get_value
get_value:
    mov r0, #32
    bx lr

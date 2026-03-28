@ Test 3a : Resolution de symbole UNDEF
@ But : symbole externe reference dans A, defini dans B

.section .text
.global compute
compute:
    push {lr}
    bl get_value
    add r0, r0, #10
    pop {pc}

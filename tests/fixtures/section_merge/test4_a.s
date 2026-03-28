@ Test 4 : Plusieurs sections PROGBITS communes (.text + .data + .rodata)
.text
.global compute
compute:
    mov r0, #100
    bx lr

.data
.global counter
counter:
    .word 0

.section .rodata
.global PI
PI:
    .word 0x40490FDB  @ 3.14159 en float

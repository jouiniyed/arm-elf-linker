@ Test 4 : Plusieurs sections PROGBITS communes (.text + .data + .rodata)
.text
.global process
process:
    mov r0, #200
    mov r1, #300
    bx lr

.data
.global result
result:
    .word 0
    .word 0

.section .rodata
.global E
E:
    .word 0x402DF854  @ 2.71828 en float

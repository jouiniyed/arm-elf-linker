@ Test 2 : Section .data uniquement dans fichier A
.text
.global start
start:
    mov r0, #0
    bx lr

.data
.global var1
var1:
    .word 0x12345678
    .word 0xABCDEF00

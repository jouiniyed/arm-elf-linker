@ Test 4a : Relocations dans section .data
@ But : vérifier correction relocations dans sections non-.text

.section .text
.global func4
func4:
    mov r0, #1
    bx lr

.section .data
.global data4
data4:
    .word 0x12345678
    .word func4

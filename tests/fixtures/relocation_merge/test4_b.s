@ Test 4b : Relocations dans section .data
@ But : références vers symboles du fichier A depuis .data du fichier B

.section .text
.global func4b
func4b:
    ldr r0, =data4
    ldr r1, [r0]
    bx lr

.section .data
.global data4b
data4b:
    .word func4
    .word 0xABCDEF00

@ Test 5a : Plusieurs sections avec relocations
@ But : vérifier gestion correcte des deltas pour plusieurs sections

.section .text
.global func5_text
func5_text:
    mov r0, #5
    bx lr

.section .data
.global data5
data5:
    .word 0x11111111
    .word 0x22222222

.section .rodata
.global rodata5
rodata5:
    .word 0x33333333
    .word func5_text

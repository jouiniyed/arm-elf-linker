@ Test 5b : Plusieurs sections avec relocations
@ But : relocations croisées entre différentes sections

.section .text
.global func5b_text
func5b_text:
    bl func5_text
    ldr r0, =data5
    ldr r1, =rodata5
    bx lr

.section .data
.global data5b
data5b:
    .word func5_text
    .word rodata5

.section .rodata
.global rodata5b
rodata5b:
    .word data5
    .word 0x44444444

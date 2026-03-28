@ Test 1b : Relocation R_ARM_ABS32 dans .text
@ But : référence absolue vers target1, correction avec delta de section

.section .text
.global func1
func1:
    ldr r0, =target1
    ldr r1, [r0]
    bx lr

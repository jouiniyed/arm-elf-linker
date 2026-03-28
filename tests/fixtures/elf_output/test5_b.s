@ Test 5b : Relocation R_ARM_ABS32 sur donnees
@ But : reference absolue vers donnee de A, relocation a corriger

.section .text
.global get_data_ptr
get_data_ptr:
    ldr r0, =global_data
    bx lr

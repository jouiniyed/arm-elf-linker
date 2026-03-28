@ Test 5a : Relocation R_ARM_ABS32 sur donnees
@ But : verifier correction des relocations absolues sur donnees

.section .data
.global global_data
global_data:
    .word 0x12345678

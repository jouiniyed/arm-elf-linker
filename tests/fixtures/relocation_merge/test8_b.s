@ Test 8b : Relocations avec symboles locaux SECTION
@ But : références via SECTION nécessitent correction addend spéciale

.section .text
local_func_b:
    mov r0, #88
    bx lr

.global global8b
global8b:
    bl global8a
    bl local_func_b
    ldr r0, =global_data8a
    bx lr

.section .data
local_data_b:
    .word 0xDDDDDDDD

.global global_data8b
global_data8b:
    .word global_data8a
    .word local_data_b
    .word 0xEEEEEEEE

@ Test 8a : Relocations avec symboles locaux SECTION
@ But : vérifier correction addend pour symboles SECTION

.section .text
local_func:
    mov r0, #8
    bx lr

.global global8a
global8a:
    bl local_func
    mov r1, #80
    bx lr

.section .data
local_data:
    .word 0xAAAAAAAA
    .word 0xBBBBBBBB

.global global_data8a
global_data8a:
    .word local_data
    .word 0xCCCCCCCC

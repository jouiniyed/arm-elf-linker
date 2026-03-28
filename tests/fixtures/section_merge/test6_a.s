@ Test 6 : Sections vides pour tester edge case
.text
.global empty_test
empty_test:
    nop
    bx lr

.data
@ Section data vide

.bss
.global uninit
uninit:
    .space 4

@ Test 6 : Sections vides pour tester edge case
.text
.global other_empty
other_empty:
    nop
    nop
    bx lr

.data
@ Section data vide aussi

@ Test 5 : Test tailles différentes pour vérifier les offsets
.text
.global big_function
big_function:
    push {r4, r5, r6, lr}
    mov r0, #1
    mov r1, #2
    mov r2, #3
    mov r3, #4
    mov r4, #5
    mov r5, #6
    mov r6, #7
    add r0, r0, r1
    add r0, r0, r2
    add r0, r0, r3
    add r0, r0, r4
    add r0, r0, r5
    add r0, r0, r6
    pop {r4, r5, r6, pc}

.data
.global array
array:
    .word 1, 2, 3, 4, 5, 6, 7, 8

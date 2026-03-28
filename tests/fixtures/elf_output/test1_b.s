@ Test 1b : ELF valide basique - fusion simple
@ But : verifier production d'un fichier ELF valide et linkable

.section .text
.global func_b
func_b:
    mov r0, #17
    bx lr

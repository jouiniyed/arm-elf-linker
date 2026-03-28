@ Test 1a : ELF valide basique - fusion simple
@ But : verifier production d'un fichier ELF valide et linkable

.section .text
.global func_a
func_a:
    mov r0, #42
    bx lr

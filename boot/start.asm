[BITS 32]
[GLOBAL start]
[EXTERN kernel_main]

section .start

start:
    call kernel_main
.hang:
    hlt
    jmp .hang

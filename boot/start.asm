[BITS 64]
[GLOBAL start]
[EXTERN kernel_main]

[EXTERN __init_array_start]
[EXTERN __init_array_end]

section .start

start:
    ; call c++ constructors
    push rdi
    push rsi
    push r12
    mov r12, __init_array_start

.call_constructors:
    cmp r12, __init_array_end
    je .call_constructors_end

    call [r12]
    add r12, 8
    jmp .call_constructors

.call_constructors_end:
    pop r12
    pop rsi
    pop rdi
    call kernel_main
.hang:
    cli
    hlt
    jmp .hang

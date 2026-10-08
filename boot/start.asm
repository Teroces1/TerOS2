[BITS 64]
[GLOBAL start]
[EXTERN kernel_main]

[EXTERN __init_array_start]
[EXTERN __init_array_end]
[EXTERN _kernel_load_size]
[EXTERN _kernel_load_end]
[EXTERN _kernel_end]
; [EXTERN initial_stack_top]

section .start
start:
    mov r14, rdi

    mov rdi, 0x100000
    add rdi, _kernel_load_size ; RDI = start of .bss at 1 MB mark

    mov rax, _kernel_end
    mov rbx, _kernel_load_end
    sub rax, rbx
    mov rcx, rax

    add rcx, 3
    shr rcx, 2                 ; Convert to dwords

    xor eax, eax               ; Fill with 0
    rep stosd

    ; call c++ constructors
    mov rsp, kernel_stack_top
    mov r12, qword __init_array_start
    mov rax, [0xFF0302343]  ; should auto fault here
    

.call_constructors:
    mov r13, qword __init_array_end
    cmp r12, r13
    je .call_constructors_end

    call [r12]
    add r12, 8
    jmp .call_constructors

.call_constructors_end:
    mov rdi, r14
    call kernel_main
.hang:
    cli
    hlt
    jmp .hang


section .bss
align 16
kernel_stack_bottom:
    resb 16384          ; Allocate 16 KiB of uninitialized bytes
kernel_stack_top:
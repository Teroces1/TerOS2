[BITS 64]
[GLOBAL start]
[EXTERN kernel_main]

[EXTERN __init_array_start]
[EXTERN __init_array_end]
[EXTERN _kernel_load_size]
[EXTERN _kernel_load_end]
[EXTERN _kernel_end]
; [EXTERN initial_stack_top]

trigger_triple_fault:
    lidt [null_idt]    ; Load the invalid IDT pointer
    int3               ; Trigger an interrupt (causes #GP/#DF -> Triple Fault)
    
    .loop:
        hlt
        jmp .loop

section .data
align 8
null_idt:
    dw 0                ; IDT limit = 0
    dq 0                ; IDT base  = 0

section .start
start:
    mov r14, rdi

    mov rdi, _kernel_load_end ; RDI = start of .bss at 1 MB mark

    mov rcx, _kernel_end
    sub rcx, rdi

    add rcx, 3
    shr rcx, 2                 ; Convert to dwords

    xor eax, eax               ; Fill with 0
    rep stosd

    ; call c++ constructors
    mov rsp, kernel_stack_top
    mov r12, __init_array_start
    mov r13, __init_array_end
    

.call_constructors:
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
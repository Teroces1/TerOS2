global gdt_flush

gdt_flush:
    lgdt [rdi]

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    pop rdx
    push 0x08
    push rdx
    retfq


extern interruptHandler

%macro ISR_NOERRCODE 1
global _isr%1
_isr%1:
    push qword 0         ; Push dummy error code
    push qword %1        ; Push interrupt vector number
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
global _isr%1
_isr%1:
    ; CPU already pushed error code onto stack
    push qword %1        ; Push interrupt vector number
    jmp isr_common_stub
%endmacro

; Define CPU Exceptions (0-31)
ISR_NOERRCODE 0          ; #DE Divide Error
ISR_NOERRCODE 1          ; #DB Debug
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_NOERRCODE 9
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 31

ISR_ERRCODE   8          ; #DF Double Fault
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13         ; #GP General Protection Fault
ISR_ERRCODE   14         ; #PF Page Fault
ISR_ERRCODE   17
ISR_ERRCODE   21
ISR_ERRCODE   29
ISR_ERRCODE   30

; Hardware IRQs (32-47)
ISR_NOERRCODE 32         ; IRQ0 (Timer)
ISR_NOERRCODE 33         ; IRQ1 (Keyboard)

isr_common_stub:
    ; Push all general-purpose registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp         ; Pass stack pointer (registers struct) as 1st arg to C
    call interruptHandler

    ; Restore registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16          ; Clean up pushed error code and vector number
    iretq                ; 64-bit interrupt return
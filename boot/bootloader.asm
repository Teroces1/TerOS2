; ---------------------------
; STAGE 2 OF BOOTLOADER
; ---------------------------

%define NUM_SECTORS 30
%define KERNEL_SECTOR 5
%define KERNEL_START_SEGMENT 0x2000
%define KERNEL_START 0x20000

[BITS 16]         ; We’re in 16-bit Real Mode

org 0x7E00

jmp Stage2_Start


Print_String:
    push si
_print_loop:
    mov al, [si]
    cmp al, 0
    je _print_exit
_print_continue:
    mov ah, 0x0E
    int 0x10

    inc si
    jmp _print_loop
_print_exit:
    pop si
    ret

Hexadecimal:     db "0123456789ABCDEF"
Print_hex:
    mov al, dl
    shr al, 4
    mov bx, Hexadecimal
    mov ah, 0
    add bx, ax
    mov al, byte [bx]

    mov ah, 0x0E
    int 0x10
    ;==========
    mov al, dl
    and al, 15
    mov bx, Hexadecimal
    mov ah, 0
    add bx, ax
    mov al, byte [bx]

    mov ah, 0x0E
    int 0x10
    ret


disk_error:
    mov si, Str_disk_error
    call Print_String
    jmp $

Stage2_Start:
    mov byte [BootDrive], dl
    mov byte [UsingLBA], al
    push ax

    xor ax, ax
    mov es, ax
    mov ds, ax
    mov si, Str_stage_2_loaded
    call Print_String


    mov word [VBEInfoBlock], 'VB'      ; 'V' 'B'
    mov word [VBEInfoBlock+2], 'E2'    ; 'E' '2'

    ; get all the vsa info modes
    xor ax, ax
    mov bx, VBEInfoBlock
    mov di, bx
    mov es, ax ; segment of buffer
    mov ax, 0x4F00        ; VESA Get Controller Info
    int 0x10

    cmp ax, 0x004F
    jne VBE_failed

    ; get ready for looping through all the modes
    mov ax, 0x1000
    mov es, ax
    xor di, di

    mov si, [VBEInfoBlock + 14] ; the pointer to the modes list are located at index 14
    mov ax, [VBEInfoBlock + 16] ; the segment the pointer should point to
    mov fs, ax

    xor bx, bx  ; start loop counter (to see how many modes got saved)
.mode_loop:
    mov cx, [fs:si]             ; get the mode number
    cmp cx, 0xFFFF              ; the last entry of the list is 0xFFFF
    je .mode_loop_exit

    ; save the current registers
    push si
    push es
    push bx
    push fs
    push cx

    mov ax, 0x4F01          ; VESA Get Mode Info function
    int 0x10

    ; test if its (Mode 31): 1024x768 @ 32bpp | Framebuffer: 0xFD000000
    ; x-res: 18, y-res: 20, bpp: 25, buff: 40

    pop bx  ; bx should contain the mode number
    mov ax, [es:di + 18]
    cmp ax, 1024
    jne .mode_part2
    mov ax, [es:di + 20]
    cmp ax, 768
    jne .mode_part2
    xor ax, ax
    mov al, [es:di + 25]
    cmp al, 32
    jne .mode_part2

    ; save the frame buffer
    mov ax, [es:di+40]
    mov [FrameBuffer], ax
    mov ax, [es:di+42]
    mov [FrameBuffer+2], ax
    
    ; save the bytes per scan line
    mov ax, [es:di+16]
    mov [BytesPerScanLine], ax

    

    mov ax, 0x4F02          ; VBE function: Set VBE Mode
    or bx, 0xC000          ; add the flags to use linear frame buffer (0x4000), and to not clear screen (0x8000)

    mov [ModeSelected], bx
    int 0x10

    cmp ax, 0x004F
    jne VBE_failed

.mode_part2:

    ; restore saved registers
    pop fs
    pop bx
    pop es
    pop si

    cmp ax, 0x004F          ; check if success
    jne .mode_loop_continue

    add di, 256             ; incremenet destination
    inc bx                  ; incremenet counter

.mode_loop_continue:
    add si, 2
    jmp .mode_loop
.mode_loop_exit:
    ; now all the information should be saved at 0x10000
    mov [NumModes], bx

    mov si, Str_modes_read
    call Print_String
    mov dl, bl
    call Print_hex


    ; AX = 4F01h (Get Mode Info)
    ; CX = mode number
    ; ES:DI = 256-byte buffer (aligned, below 1 MiB)

    ; mov ax, 0x4F01
    ; mov cx, 0x118        ; e.g. 0x118
    ; mov di, VBEModeInfoBlock
    ; mov bx, 0x0000
    ; mov es, bx ; segment of buffer
    ; int 0x10

    ; jc  VBE_failed
    ; cmp ax, 0x004F
    ; jne VBE_failed
     ; write "VBE2" into first 4 bytes
    ; mov word [VBEInfoBlock], 'VB'      ; 'V' 'B'
    ; mov word [VBEInfoBlock+2], 'E2'    ; 'E' '2'

    ; mov ax, 0x4F00
    ; mov di, VBEInfoBlock
    ; mov bx, 0x0000
    ; mov es, bx                         ; if buffer is in same segment as code
    ; int 0x10

    ; jc VBE_failed
    ; cmp ax, 0x004F
    ; jne VBE_failed
    ;;;;;;;;;;;;;;;;


    pop ax
    cmp al, 0
    je ReadTryCHS
    jmp ReadTryLBA

VBE_failed:
    mov si, Str_VesaFailed
    call Print_String
    jmp $
; ---------------------------
; Strings
; ---------------------------
Str_stage_2_loaded:
    db 0x0D, 0x0A, "Stage 2 finished loading...", 0x0D, 0x0A, 0
Str_disk_error:
    db 0x0D, 0x0A, "!!!! Critical Error !!!", 0x0D, 0x0A, \
        "An error occured while loading sectors from disk.", 0x0D, 0x0A, 0

Str_VesaFailed:
    db "Vesa has failed to load...", 0x0D, 0x0A, 0

Str_modes_read:
    db "All modes information has been read, count: 0x", 0
Str_test:
    db "A", 0
; ---------------------------
; DAP for loading kernel
; ---------------------------
Struct_DiskAddressPacket:
    db 0x10       ; size of packet (16 bytes)
    db 0          ; reserved
    dw NUM_SECTORS; number of sectors to read
    dw 0x0000     ; buffer offset
    dw KERNEL_START_SEGMENT     ; buffer segment
    dq KERNEL_SECTOR ; starting LBA (sector 4)

; ---------------------------
; Variables
; ---------------------------
align 16
VariablesPacket:
BootDrive:      db 0
Retries:        db 0
NumModes:       db 0
UsingLBA:       db 0
ModeSelected:   dw 0xFFFF
FrameBuffer: dd 0x00000000
BytesPerScanLine: dw 0x0000

align 16
VBEInfoBlock: times 512 db 0
; struc VBEModeInfoBlock				;	VesaModeInfoBlock_size = 256 bytes
; 	.ModeAttributes		resw 1
; 	.FirstWindowAttributes	resb 1
; 	.SecondWindowAttributes	resb 1
; 	.WindowGranularity	resw 1		;	in KB
; 	.WindowSize		resw 1		;	in KB
; 	.FirstWindowSegment	resw 1		;	0 if not supported
; 	.SecondWindowSegment	resw 1		;	0 if not supported
; 	.WindowFunctionPtr	resd 1
; 	.BytesPerScanLine	resw 1

; 	;	Added in Revision 1.2
; 	.Width			resw 1		;	in pixels(graphics)/columns(text)
; 	.Height			resw 1		;	in pixels(graphics)/columns(text)
; 	.CharWidth		resb 1		;	in pixels
; 	.CharHeight		resb 1		;	in pixels
; 	.PlanesCount		resb 1
; 	.BitsPerPixel		resb 1
; 	.BanksCount		resb 1
; 	.MemoryModel		resb 1		;	http://www.ctyme.com/intr/rb-0274.htm#Table82
; 	.BankSize		resb 1		;	in KB
; 	.ImagePagesCount	resb 1		;	count - 1
; 	.Reserved1		resb 1		;	equals 0 in Revision 1.0-2.0, 1 in 3.0

; 	.RedMaskSize		resb 1
; 	.RedFieldPosition	resb 1
; 	.GreenMaskSize		resb 1
; 	.GreenFieldPosition	resb 1
; 	.BlueMaskSize		resb 1
; 	.BlueFieldPosition	resb 1
; 	.ReservedMaskSize	resb 1
; 	.ReservedMaskPosition	resb 1
; 	.DirectColorModeInfo	resb 1

; 	;	Added in Revision 2.0
; 	.LFBAddress		resd 1
; 	.OffscreenMemoryOffset	resd 1
; 	.OffscreenMemorySize	resw 1		;	in KB
; 	.Reserved2		resb 206	;	available in Revision 3.0, but useless for now
; endstruc
; ; VBEModeInfoBlock: times 256 db 0


; ---------------------------
; GDT (flat, ring 0)
; ---------------------------
gdt_start:
    dq 0x0000000000000000        ; Null descriptor (mandatory)
    dq 0x00CF9A000000FFFF        ; 32-bit Kernel Code (Selector 0x08)
    dq 0x00CF92000000FFFF        ; 32-bit Kernel Data (Selector 0x10)
    dq 0x00209A0000000000        ; 64-bit Kernel Code (Selector 0x18)
gdt_end:

gdt_ptr:
    dw gdt_end - gdt_start - 1
    dd gdt_start





; =================================================================================
; =================================================================================
; Utility
; =================================================================================
; =================================================================================


; ---------------------------
; VESA Graphics Setup
; ---------------------------
    
; Input: AX = 0x4F02 (VESA Set Mode)
;        BX = Mode number (bit 14 = linear framebuffer)
; Returns: AL = 0x4F if successful, AH = status



; set_vesa_mode:
;     mov ax, 0x4F02      ; VESA set mode function
;     mov bx, 0x118        ; Example: 1024x768x256 color (linear framebuffer bit = 0x4000)
;                           ; So if you want linear: bx = 0x40118
;     int 0x10             ; BIOS interrupt
;     ret

ReadTryLBA:     ; read using logical block addressing
    mov si, Struct_DiskAddressPacket

    mov ah, 0x42
    mov dl, [BootDrive]
    int 0x13
    jc ReadTryCHS

    jmp ReadOK

ReadTryCHS:     ; cylinder, head, sector
    ; Load 2 sectors at 0000:10000h (ES=1000h, BX=0000h)

    mov ax, KERNEL_START_SEGMENT  ; load at 0x10000 (segment 1000 * 16 = address 10000)
    mov es, ax
    xor bx, bx

    ; Optional: reset disk first
    mov dl, [BootDrive]
    xor ah, ah           ; AH=00h, reset disk
    int 0x13
_ReadTryCHS_loop:
    ; read the vge stub sector
    mov dl, [BootDrive]
    mov ah, 0x02         ; read sectors (CHS)
    mov al, 1  ; number of sectors
    xor ch, ch           ; cylinder 0
    mov cl, KERNEL_SECTOR ; sector 2
    xor dh, dh           ; head 0
    ; ES:BX already set
    int 0x13
    jc .fail

    ; read kernel

    mov dl, [BootDrive]
    mov ah, 0x03         ; read sectors (CHS)
    mov al, NUM_SECTORS  ; number of sectors
    xor ch, ch           ; cylinder 0
    mov cl, KERNEL_SECTOR+1 ; sector 2
    xor dh, dh           ; head 0
    ; ES:BX already set
    int 0x13
    jnc ReadOK

.fail:
    ; on error: reset and retry (up to 3 attempts)
    mov dl, [BootDrive]
    xor ah, ah
    int 0x13

    inc byte [Retries]
    cmp byte [Retries], 3
    jb _ReadTryCHS_loop
    jmp disk_error

; ---------------------------
; Protected Mode Start, Final Stage of Bootloader
; ---------------------------

enable_A20:
    ; Try fast A20 via port 0x92
    in   al, 0x92
    or   al, 00000010b    ; set A20 enable
    and  al, 11111110b    ; clear reset bit
    out  0x92, al
    ; verify: read back and test bit 1
    in   al, 0x92
    test al, 00000010b
    jnz .done

    ; Fallback: keyboard controller method (8042)
    ; Wait input buffer empty
    call wait_input_empty
    mov al, 0xD1
    out 0x64, al          ; command: write output port
    call wait_input_empty
    mov al, 0xDF          ; set A20 via output port (bit1=1)
    out 0x60, al
    ; optionally verify by testing address wrap behavior
.done:
    ret

wait_input_empty:
    in al, 0x64
    test al, 2
    jnz wait_input_empty
    ret


ReadOK:
    ; switch to protected mode and then jump to kernel at 0x2000:0000
    call enable_A20
    lgdt [gdt_ptr]

    mov eax, cr0
    or  eax, 1               ; set PE bit
    mov cr0, eax

    ; Far jump to 32-bit code segment (selector 0x08)
    jmp 0x08:ProtectedModeStart

[BITS 32]
ProtectedModeStart:
    mov ax, 0x10         ; data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x00900000  ; pick a safe stack (example)


    push VariablesPacket
    push VBEInfoBlock
    jmp 0x08:KERNEL_START

times 1536 - ($ - $$) db 0

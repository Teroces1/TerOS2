[BITS 16]         ; 16-bit real mode

org 0x7C00        ; loads at 0x7c00

JMP Start


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

Print_newline:
    ; mov bh, 0
    ; mov ah, 0x03
    ; mov al, 0x00
    ; int 0x10

    ; inc dh
    ; xor dl, dl

    ; mov bh, 0
    ; mov ah, 0x02
    ; mov al, 0x00
    ; int 0x10

    mov al, 0x0D
    mov ah, 0x0E
    int 0x10
    mov al, 0x0A
    mov ah, 0x0E
    int 0x10


    ret


Print_String:
    push si
_print_loop:
    mov al, [si]
    cmp al, 0
    je _print_exit
    ; cmp al, 0x0A
    ; jne _print_continue

    ; call Print_newline
    ; inc si
    ; jmp _print_loop
_print_continue:
    mov ah, 0x0E
    int 0x10

    inc si
    jmp _print_loop
_print_exit:
    pop si
    ret


Start:
    ; ==================
    ; Set up segments
    ; ==================

    cli             ; disable interrupts

    xor ax, ax      ; set ds, es, ss to 0, and stack to start at 0x7c00
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov [BootDrive], dl

    ; ==================
    ; print message
    ; ==================


    ;go to line 10
    mov bh, 0
    mov ah, 0x02
    mov al, 0x00
    mov dh, 15
    mov dl, 0
    int 0x10

    ; print out stats
    mov si, Str_boot_stats
    call Print_String
    
    mov dl, [BootDrive]
    call Print_hex

    mov si, Str_boot_message
    call Print_String

    ; ==================
    ; load more sectors
    ; ==================

    ; determind if LBA is possible
    ; in:  DL = boot drive (saved from bios)
    ; out: ZF/CF/flags+registers indicate support
    mov dl, [BootDrive]
    mov ah, 0x41           ; "Extensions Present?" function
    mov bx, 0x55AA         ; set a handshake code
    int 0x13
    jc  .no_ext            ; carry = 1 means not supported

    cmp bx, 0xAA55         ; get the returned handshake
    jne .no_ext

    test cx, 1             ; bit 0 set => extended disk access (42h/43h...) supported
    jz  .no_ext


    ; -> extensions present, use AH=42h
    jmp .use_lba




    .no_ext:
        ; -> fall back to CHS (AH=02h)
        jmp Read2ndStage_CHS
    .use_lba:
        inc byte [UsingLBA]
Read2ndStage_LBA:     ; read using logical block addressing
    mov si, Str_disk_lba
    call Print_String
    mov si, Struct_DiskAddressPacket_2ndStage
    mov ah, 0x42
    mov dl, [BootDrive]
    int 0x13
    jc Read2ndStage_CHS

    jmp JumpToStage2
Read2ndStage_CHS:     ; cylinder, head, sector
    mov si, Str_disk_chs
    call Print_String

    xor ax, ax
    mov [UsingLBA], al
    ; Load 2 sectors at 0000:10000h (ES=1000h, BX=0000h)
    mov ax, 0x1000  ; load at 0x10000 (segment 1000 * 16 = address 10000)
    mov es, ax
    xor bx, bx

    ; reset disk first
    mov dl, [BootDrive]
    xor ah, ah           ; AH=00h, reset disk
    int 0x13
_ReadTryCHS_loop:
    mov dl, [BootDrive]
    mov ah, 0x02         ; read sectors (CHS)
    mov al, 3            ; number of sectors
    xor ch, ch           ; cylinder 0
    mov cl, 2            ; sector 2
    xor dh, dh           ; head 0
    ; ES:BX already set
    int 0x13
    jnc JumpToStage2

    ; on error: reset and retry (up to 3 attempts)
    mov dl, [BootDrive]
    xor ah, ah
    int 0x13

    inc byte [Retries]
    cmp byte [Retries], 3
    jb _ReadTryCHS_loop
    jmp disk_error
    


disk_error:
    mov si, Str_disk_error
    call Print_String
    jmp $

JumpToStage2:
    mov dl, [BootDrive]
    mov al, [UsingLBA]

    jmp 0x0000:0x7E00

; ---------------------------
; Disk Read Vars
; ---------------------------

UsingLBA:        db 0
Retries:         db 0
BootDrive:       db 0

Struct_DiskAddressPacket_2ndStage:
    db 0x10       ; size of packet (16 bytes)
    db 0          ; reserved
    dw 3          ; number of sectors to read
    dw 0x7E00     ; buffer offset
    dw 0x0000     ; buffer segment
    dq 0x00000001 ; starting LBA (sector 1)




; ---------------------------
; Strings
; ---------------------------

Hexadecimal:     db "0123456789ABCDEF"


Str_boot_stats:
    db "Reading from drive: 0x",0
Str_boot_message:
    db 0x0D, 0x0A, 0x0A, "Loading TerOS V0.0...", 0x0D, 0x0A, 0
Str_disk_error:
    db 0x0D, 0x0A, "!!!! Critical Error !!!", 0x0D, 0x0A, \
        "An error occured while loading sectors from disk.", 0x0D, 0x0A, 0
Str_disk_chs:
    db " -Using CHS", 0x0D, 0x0A, 0
Str_disk_lba:
    db " -Using LBA", 0x0D, 0x0A, 0



times 510 - ($ - $$) db 0   ; fill up empty space with 0
dw 0xAA55           ; boot signature


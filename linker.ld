ENTRY(start)

SECTIONS {
    /* Tell the compiler your kernel starts exactly at 0x20000 */
    . = 0x20000;

    .text :
    {
        *(.start)         /* 1. Enforces start.asm entry executes first */
        KEEP(*(.realmode)) /* 2. Keeps any realmode initialization tags packed tightly */
        *(.text)          /* 3. Your compiled C code instructions */
        *(.text.*)
        *(.rodata)         /* 4. CRITICAL FIX: Strings packed right against code with NO gaps */
        *(.rodata.*)
    }

    .data :
    {
        *(.data)
        *(.data.*)
    }

    .bss :
    {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }
}

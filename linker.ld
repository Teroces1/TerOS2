ENTRY(start)

/* Standard x86_64 higher-half base */
KERNEL_VMA = 0xFFFFFFFF80000000;
/* The physical address your bootloader actually loads the kernel at */
KERNEL_LMA = 0x100000; 

SECTIONS {
    /* 
     * The code expects to run at VMA + LMA. 
     * e.g., 0xFFFFFFFF80030000 
     */
    /* . = KERNEL_VMA + KERNEL_LMA; */

    . = KERNEL_VMA + KERNEL_LMA;

    _kernel_start = .;

    /* AT() ensures it goes into the flat binary at the physical offset, preventing huge file sizes */
    .text : AT(ADDR(.text) - KERNEL_VMA)
    {
        *(.start)
        KEEP(*(.realmode))
        *(.text)
        *(.text.*)
    }

    .rodata : AT(ADDR(.rodata) - KERNEL_VMA)
    {
        *(.rodata)
        *(.rodata.*)
        
        /* C++ Global Constructors */
        . = ALIGN(8);
        PROVIDE(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE(__init_array_end = .);
    }

    .data : AT(ADDR(.data) - KERNEL_VMA)
    {
        *(.data)
        *(.data.*)
    }

    _kernel_load_end = .;

    .bss : AT(ADDR(.bss) - KERNEL_VMA)
    {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }

    _kernel_end = .;
    _kernel_size = _kernel_end - _kernel_start;
    _kernel_load_size = _kernel_load_end - _kernel_start;
}
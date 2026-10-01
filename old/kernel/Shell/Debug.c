#include "../../lib/string.h"
#include <stdbool.h>
#include <stdint.h>
#include "Shell.h"
#include "Debug.h"

volatile EntryPacket *DEBUG_ENTRYPACKET;
volatile VBEInfoBlock *DEBUG_VBEINFO;
const volatile VBEModeInfo *DEBUG_VBEMODEINFO = (volatile VBEModeInfo *)(0x10000);

void DEBUG_init(EntryPacket *entry, VBEInfoBlock *vbeinfo) {
    DEBUG_ENTRYPACKET = entry;
    DEBUG_VBEINFO = vbeinfo;
}

void DEBUG_print_entry() {
    char digits[33];

    SHELL_Print("\5gBoot Statistics:");

    // Boot Drive
    SHELL_Print("\n\5y  Boot Drive: \5d0x");
    SHELL_Print(STR_int2str((int) DEBUG_ENTRYPACKET->BootDrive, digits, 16));
    SHELL_Print(" (");
    SHELL_Print(STR_int2str((int) DEBUG_ENTRYPACKET->BootDrive, digits, 10));
    SHELL_Print(")");

    // Using LBA
    SHELL_Print("\n\5y  Using LBA: \5d");
    if (DEBUG_ENTRYPACKET->UsingLBA)
        SHELL_Print("true");
    else
        SHELL_Print("false");

    // Drive Read Retries
    SHELL_Print("\n\5y  Drive Read Failures: \5d");
    SHELL_Print(STR_int2str((int) DEBUG_ENTRYPACKET->ReadRetries, digits, 10));

    // Drive Geometry
    SHELL_Print("\n\5y  Drive Heads: \5d");
    SHELL_Print(STR_int2str((int) DEBUG_ENTRYPACKET->DriveHeads, digits, 10));
    SHELL_Print("\n\5y  Drive Sectors/Track: \5d");
    SHELL_Print(STR_int2str((int) DEBUG_ENTRYPACKET->DriveSectors, digits, 10));

    // VBE Modes
    SHELL_Print("\n\n\5y  Total VBE Modes: \5d");
    SHELL_Print(STR_int2str((int) DEBUG_ENTRYPACKET->TotalModes, digits, 10));

    // VBE Modes Failed
    SHELL_Print("\n\5y  VBE Modes Read Failed: \5d");
    SHELL_Print(STR_int2str((int) DEBUG_ENTRYPACKET->FailedModes, digits, 10));

    // Mode Selected
    SHELL_Print("\n\5y  VBE Mode Selected: \5d0x");
    SHELL_Print(STR_int2str((int) DEBUG_ENTRYPACKET->ModeSelected, digits, 16));
    SHELL_Print(" (");
    SHELL_Print(STR_int2str((int) DEBUG_ENTRYPACKET->ModeSelected, digits, 10));
    SHELL_Print(")");

    // VBE Mode Selected Index
    SHELL_Print("\n\5y  VBE Mode Selected Index: \5d");
    SHELL_Print(STR_int2str((int) DEBUG_ENTRYPACKET->ModeIndexSelected, digits, 10));

    // Base Memory
    SHELL_Print("\n\n\5y  Base Memory: \5d");
    SHELL_Print(STR_int2str((int) DEBUG_ENTRYPACKET->BaseMemoryKB, digits, 10));
    SHELL_Print(" KB");

    // CPU Vendor
    SHELL_Print("\n\5y  CPU Vendor: \5d");
    SHELL_Print(DEBUG_ENTRYPACKET->CPUVendor);

    // CPU Features (Raw Hex)
    SHELL_Print("\n\5y  CPU Features: \5d0x");
    SHELL_Print(STR_int2str((unsigned int) DEBUG_ENTRYPACKET->CPUFeatures, digits, 16));

    // --- Decoded CPU Features (Indented by 4 spaces instead of 2) ---
    
    // Bit 0: FPU (Floating Point Unit)
    if (DEBUG_ENTRYPACKET->CPUFeatures & (1 << 0))
        SHELL_Print("\n\5y    FPU (Floating Point): \5dSupported");
    
    // Bit 3: PSE (Page Size Extension - allows 4MB/2MB pages, which you are using!)
    if (DEBUG_ENTRYPACKET->CPUFeatures & (1 << 3))
        SHELL_Print("\n\5y    PSE (Huge Pages): \5dSupported");
    
    // Bit 4: TSC (Time Stamp Counter - great for precise timing/sleep functions)
    if (DEBUG_ENTRYPACKET->CPUFeatures & (1 << 4))
        SHELL_Print("\n\5y    TSC (Time Stamp Counter): \5dSupported");
    
    // Bit 9: APIC (Advanced Programmable Interrupt Controller)
    if (DEBUG_ENTRYPACKET->CPUFeatures & (1 << 9))
        SHELL_Print("\n\5y    APIC: \5dSupported");
    
    // Bit 25: SSE (Streaming SIMD Extensions)
    if (DEBUG_ENTRYPACKET->CPUFeatures & (1 << 25))
        SHELL_Print("\n\5y    SSE: \5dSupported");
    
    // Bit 26: SSE2
    if (DEBUG_ENTRYPACKET->CPUFeatures & (1 << 26))
        SHELL_Print("\n\5y    SSE2: \5dSupported");
    
    // CPU Mode
    SHELL_Print("\n\n\5y  CPU Mode: \5d64-Bit Long Mode\n");

    SHELL_Print("\n\n\5gSystem Memory Map (E820):");
    
    for (int i = 0; i < DEBUG_ENTRYPACKET->E820Count; i++) {
        E820Entry* entry = &DEBUG_ENTRYPACKET->E820Map[i];
        
        SHELL_Print("\n\5y  Base: \5d0x");
        SHELL_Print(STR_64int2str((entry->BaseAddress), digits, 16));
        
        SHELL_Print(" \5y| Length: \5d0x");
        SHELL_Print(STR_64int2str((entry->Length), digits, 16));
        
        SHELL_Print(" \5y| Type: \5d");
        
        // Decode the memory type
        switch (entry->Type) {
            case 1: SHELL_Print("Usable RAM"); break;
            case 2: SHELL_Print("Reserved (Hardware)"); break;
            case 3: SHELL_Print("ACPI Reclaimable"); break;
            case 4: SHELL_Print("ACPI NVS"); break;
            case 5: SHELL_Print("Bad Memory"); break;
            default: SHELL_Print("Unknown"); break;
        }
    }

    SHELL_putChar('\n');
}

void DEBUG_print_VBE() {
    char digits[17];
    SHELL_Print("\5gVBE Information:");

    // 1. Signature
    SHELL_Print("\n  \5ySignature: \5d");
    SHELL_putChar(DEBUG_VBEINFO->signature[0]);
    SHELL_putChar(DEBUG_VBEINFO->signature[1]);
    SHELL_putChar(DEBUG_VBEINFO->signature[2]);
    SHELL_putChar(DEBUG_VBEINFO->signature[3]);

    // 2. Version
    SHELL_Print("\n  \5yVersion: \5d0x");
    STR_int2str(DEBUG_VBEINFO->version, digits, 16);
    SHELL_Print(digits);

    // Helper macro to convert real-mode segment:offset (Far Pointer) to a linear memory address
    #define FAR_PTR_TO_LINEAR(fp) (void *)(((fp >> 16) << 4) + (fp & 0xFFFF))

    // 3. OEM Name String
    SHELL_Print("\n  \5yOEM Name: \5d");
    if (DEBUG_VBEINFO->oem_string_ptr) {
        SHELL_Print((char *)FAR_PTR_TO_LINEAR(DEBUG_VBEINFO->oem_string_ptr));
    } else {
        SHELL_Print("None");
    }

    // 4. Capabilities
    SHELL_Print("\n  \5yCapabilities: \5d0x");
    STR_int2str(DEBUG_VBEINFO->capabilities, digits, 16);
    SHELL_Print(digits);

    // 5. Video Mode Pointer (Prints raw segment:offset hex value)
    SHELL_Print("\n  \5yVideo Mode Ptr: \5d0x");
    STR_int2str(DEBUG_VBEINFO->video_mode_ptr, digits, 16);
    SHELL_Print(digits);

    // 6. Total Memory
    SHELL_Print("\n  \5yTotal Memory: \5d");
    STR_int2str(DEBUG_VBEINFO->total_memory * 64, digits, 10); // Convert 64KB blocks to KB
    SHELL_Print(digits);
    SHELL_Print(" KB");

    // 7. OEM Software Revision
    SHELL_Print("\n  \5yOEM Software Rev: \5d0x");
    STR_int2str(DEBUG_VBEINFO->oem_software_rev, digits, 16);
    SHELL_Print(digits);

    // 8. OEM Vendor Name
    SHELL_Print("\n  \5yVendor Name: \5d");
    if (DEBUG_VBEINFO->oem_vendor_name_ptr) {
        SHELL_Print((char *)FAR_PTR_TO_LINEAR(DEBUG_VBEINFO->oem_vendor_name_ptr));
    } else {
        SHELL_Print("None");
    }

    // 9. OEM Product Name
    SHELL_Print("\n  \5yProduct Name: \5d");
    if (DEBUG_VBEINFO->oem_product_name_ptr) {
        SHELL_Print((char *)FAR_PTR_TO_LINEAR(DEBUG_VBEINFO->oem_product_name_ptr));
    } else {
        SHELL_Print("None");
    }

    // 10. OEM Product Revision
    SHELL_Print("\n  \5yProduct Rev: \5d");
    if (DEBUG_VBEINFO->oem_product_rev_ptr) {
        SHELL_Print((char *)FAR_PTR_TO_LINEAR(DEBUG_VBEINFO->oem_product_rev_ptr));
    } else {
        SHELL_Print("None");
    }

    #undef FAR_PTR_TO_LINEAR

    SHELL_putChar('\n');
}

void DEBUG_print_modes(void) {
    char digits[17];
    char paddingBuffer[17];

    SHELL_Print("\5gVBE Modes Available (");
    SHELL_Print(STR_int2str(DEBUG_ENTRYPACKET->TotalModes, digits, 10));
    SHELL_Print("):\n");
    for (int i = 0; i < DEBUG_ENTRYPACKET->TotalModes; i++) {
        if (i == DEBUG_ENTRYPACKET->ModeIndexSelected) {
            SHELL_Print("\5c-->\5y");
        } else {
            SHELL_Print("   \5y");

        }
        SHELL_Print(STR_rpad(STR_int2str(i, digits, 10), paddingBuffer, 3, ' '));
        SHELL_Print(" : \5r");


        char *res = STR_int2str(DEBUG_VBEMODEINFO[i].x_resolution, digits, 10);
        int xreslen = STR_strlen(res);
        SHELL_Print(res);
        SHELL_Print("x");
        res = STR_int2str(DEBUG_VBEMODEINFO[i].y_resolution, digits, 10);
        int yreslen = STR_strlen(res);
        SHELL_Print(res);

        // add padding
        for (int i = 0; i < (8-xreslen-yreslen); i++) {
            SHELL_putChar(' ');
        }

        SHELL_Print(" \5m @ ");
        SHELL_Print(STR_int2str(DEBUG_VBEMODEINFO[i].bits_per_pixel, digits, 10));
        SHELL_Print("bpp\n");
    }

    SHELL_putChar('\n');
}

void DEBUG_print_mode_info(int modeIndex) {
    if (modeIndex < 0 || modeIndex >= DEBUG_ENTRYPACKET->TotalModes) {
        SHELL_Print("\5rInvalid Mode Index!\n");
        return;
    }

    VBEModeInfo *info = &DEBUG_VBEMODEINFO[modeIndex];
    char digits[17];
    char padBuf[17];

    SHELL_Print("\5g=== VBE Mode ");
    SHELL_Print(STR_int2str(modeIndex, digits, 10));
    SHELL_Print(" Details ===\5y\n");

    // Basic Dimensions & Memory Layout
    SHELL_Print(" Resolution   : \5r");
    SHELL_Print(STR_int2str(info->x_resolution, digits, 10));
    SHELL_Print("x");
    SHELL_Print(STR_int2str(info->y_resolution, digits, 10));
    SHELL_Print(" \5m@ ");
    SHELL_Print(STR_int2str(info->bits_per_pixel, digits, 10));
    SHELL_Print(" bpp\5y\n");

    SHELL_Print(" Pitch (BPL)  : ");
    SHELL_Print(STR_int2str(info->bytes_per_scanline, digits, 10));
    SHELL_Print(" bytes\n");

    SHELL_Print(" Framebuffer  : \5c0x");
    SHELL_Print(STR_int2str(info->physical_base_ptr, digits, 16));
    SHELL_Print("\5y\n");

    // Attributes & Memory Model
    SHELL_Print(" Attributes   : 0x");
    SHELL_Print(STR_int2str(info->mode_attributes, digits, 16));
    SHELL_Print(" (LFB: ");
    SHELL_Print((info->mode_attributes & 0x80) ? "\5gYES\5y" : "\5rNO\5y");
    SHELL_Print(", Color: ");
    SHELL_Print((info->mode_attributes & 0x08) ? "\5gYES\5y" : "\5rNO\5y");
    SHELL_Print(", Graphics: ");
    SHELL_Print((info->mode_attributes & 0x10) ? "\5gYES\5y" : "\5rNO\5y");
    SHELL_Print(")\n");

    SHELL_Print(" Memory Model : ");
    switch (info->memory_model) {
        case 0x00: SHELL_Print("Text Mode"); break;
        case 0x01: SHELL_Print("CGA Graphics"); break;
        case 0x02: SHELL_Print("Hercules Graphics"); break;
        case 0x03: SHELL_Print("Planar (4-Plane)"); break;
        case 0x04: SHELL_Print("Packed Pixel (Indexed)"); break;
        case 0x05: SHELL_Print("Non-Chain 4 (256 color)"); break;
        case 0x06: SHELL_Print("Direct Color (RGB)"); break;
        case 0x07: SHELL_Print("YUV"); break;
        default:   SHELL_Print("Unknown/Reserved"); break;
    }
    SHELL_Print("\n");

    // Direct Color / RGBA Layout (if applicable)
    if (info->memory_model == 0x06 || info->bits_per_pixel >= 15) {
        SHELL_Print(" Color Layout : ");
        SHELL_Print("\5rR:");
        SHELL_Print(STR_int2str(info->red_mask_size, digits, 10));
        SHELL_Print("@");
        SHELL_Print(STR_int2str(info->red_field_position, digits, 10));

        SHELL_Print(" \5gG:");
        SHELL_Print(STR_int2str(info->green_mask_size, digits, 10));
        SHELL_Print("@");
        SHELL_Print(STR_int2str(info->green_field_position, digits, 10));

        SHELL_Print(" \5cB:");
        SHELL_Print(STR_int2str(info->blue_mask_size, digits, 10));
        SHELL_Print("@");
        SHELL_Print(STR_int2str(info->blue_field_position, digits, 10));

        if (info->reserved_mask_size > 0) {
            SHELL_Print(" \5mA:");
            SHELL_Print(STR_int2str(info->reserved_mask_size, digits, 10));
            SHELL_Print("@");
            SHELL_Print(STR_int2str(info->reserved_field_position, digits, 10));
        }
        SHELL_Print("\5y\n");
    }

    // Hardware Specs
    SHELL_Print(" Pages / Planes: ");
    SHELL_Print(STR_int2str(info->number_of_image_pages + 1, digits, 10));
    SHELL_Print(" page(s) / ");
    SHELL_Print(STR_int2str(info->number_of_planes, digits, 10));
    SHELL_Print(" plane(s)\n");

    // Real Mode Windowing (legacy)
    SHELL_Print(" Banking/Win  : SegA=0x");
    SHELL_Print(STR_int2str(info->win_a_segment, digits, 16));
    SHELL_Print(" Gran=");
    SHELL_Print(STR_int2str(info->win_granularity, digits, 10));
    SHELL_Print("KB Size=");
    SHELL_Print(STR_int2str(info->win_size, digits, 10));
    SHELL_Print("KB\n");

    SHELL_putChar('\n');
}

void DEBUG_print_segment() {
    SHELL_Print("\5ySegment Registers:\n");
    char digits[33];
    

    uint16_t seg;
    __asm__ volatile("mov %%cs, %0" : "=r"(seg));
    SHELL_Print(" \5yCS: 0x");
    SHELL_Print(STR_64int2str(seg, digits, 16));

    __asm__ volatile("mov %%ds, %0" : "=r"(seg));
    SHELL_Print("\n \5yDS: 0x");
    SHELL_Print(STR_64int2str(seg, digits, 16));

    __asm__ volatile("mov %%es, %0" : "=r"(seg));
    SHELL_Print("\n \5yES: 0x");
    SHELL_Print(STR_64int2str(seg, digits, 16));

    __asm__ volatile("mov %%fs, %0" : "=r"(seg));
    SHELL_Print("\n \5yFS: 0x");
    SHELL_Print(STR_64int2str(seg, digits, 16));
    
    __asm__ volatile("mov %%gs, %0" : "=r"(seg));
    SHELL_Print("\n \5yGS: 0x");
    SHELL_Print(STR_64int2str(seg, digits, 16));

    __asm__ volatile("mov %%Ss, %0" : "=r"(seg));
    SHELL_Print("\n \5ySS: 0x");
    SHELL_Print(STR_64int2str(seg, digits, 16));

    SHELL_putChar('\n');
}

void DEBUG_print_cregs() {
    SHELL_Print("\5gControl Registers:\n");
    char digits[33];
    uint64_t cr;

    __asm__ volatile("mov %%cr0, %0" : "=r"(cr));
    SHELL_Print("  \5yCR0: \5d0x");
    SHELL_Print(STR_64int2str(cr, digits, 16));

    __asm__ volatile("mov %%cr2, %0" : "=r"(cr));
    SHELL_Print("\n  \5yCR2: \5d0x");
    SHELL_Print(STR_64int2str(cr, digits, 16));

    __asm__ volatile("mov %%cr3, %0" : "=r"(cr));
    SHELL_Print("\n  \5yCR3: \5d0x");
    SHELL_Print(STR_64int2str(cr, digits, 16));

    __asm__ volatile("mov %%cr4, %0" : "=r"(cr));
    SHELL_Print("\n  \5yCR4: \5d0x");
    SHELL_Print(STR_64int2str(cr, digits, 16));

    SHELL_putChar('\n');
}

void DEBUG_print_tables() {
    SHELL_Print("\5gDescriptor Table Pointers:\n");
    char digits[33];

    // x86_64 pseudo-descriptors must be explicitly packed
    struct __attribute__((packed)) {
        uint16_t limit;
        uint64_t base;
    } gdt_ptr, idt_ptr;

    __asm__ volatile("sgdt %0" : "=m"(gdt_ptr));
    SHELL_Print("  \5yGDT Base: \5d0x");
    SHELL_Print(STR_64int2str(gdt_ptr.base, digits, 16));
    SHELL_Print("\n  \5yGDT Limit: \5d0x");
    SHELL_Print(STR_64int2str(gdt_ptr.limit, digits, 16));

    __asm__ volatile("sidt %0" : "=m"(idt_ptr));
    SHELL_Print("\n  \5yIDT Base: \5d0x");
    SHELL_Print(STR_64int2str(idt_ptr.base, digits, 16));
    SHELL_Print("\n  \5yIDT Limit: \5d0x");
    SHELL_Print(STR_64int2str(idt_ptr.limit, digits, 16));

    SHELL_putChar('\n');
}


void DEBUG_print_efer() {
    SHELL_Print("\5gExtended Feature Enable Register (EFER):\n");
    char digits[33];
    
    uint32_t low, high;
    // 0xC0000080 is the EFER MSR constant
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(0xC0000080));

    uint64_t efer = ((uint64_t)high << 32) | low;

    SHELL_Print("  \5yEFER MSR: \5d0x");
    SHELL_Print(STR_64int2str(efer, digits, 16));

    // Decode crucial flags
    SHELL_Print("\n    \5ySCE (Syscall Extension): \5d");
    SHELL_Print((efer & (1 << 0)) ? "Enabled" : "Disabled");

    SHELL_Print("\n    \5yLME (Long Mode Enable):  \5d");
    SHELL_Print((efer & (1 << 8)) ? "Enabled" : "Disabled");

    SHELL_Print("\n    \5yLMA (Long Mode Active):  \5d");
    SHELL_Print((efer & (1 << 10)) ? "Active" : "Inactive");

    SHELL_Print("\n    \5yNXE (No-Execute Enable): \5d");
    SHELL_Print((efer & (1 << 11)) ? "Enabled" : "Disabled");

    SHELL_putChar('\n');
}


void DEBUG_print_msr(uint32_t msr_id) {
    char digits[33];
    uint32_t low, high;

    SHELL_Print("\5gMSR Register Lookup:\n");
    SHELL_Print("  \5yTarget Address: \5d0x");
    SHELL_Print(STR_int2str(msr_id, digits, 16));

    // Safely query the dynamic MSR identifier
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr_id));
    uint64_t msr_val = ((uint64_t)high << 32) | low;

    SHELL_Print("\n  \5yValue:          \5d0x");
    SHELL_Print(STR_64int2str(msr_val, digits, 16));
    SHELL_putChar('\n');
}

void DEBUG_print_memdump(uint64_t address, int rowCount) {
    char digits[33];
    uint8_t *ptr = (uint8_t *)address;

    SHELL_Print("\5gMemory Hex Dump at 0x");
    SHELL_Print(STR_64int2str(address, digits, 16));
    SHELL_Print(":\n");

    // Print 4 rows of 16 bytes each
    for (int i = 0; i < rowCount; i++) {
        SHELL_Print("  \5y");
        SHELL_Print(STR_64int2str(address + (i * 16), digits, 16));
        SHELL_Print(": \5d");

        // 1. Hex Representation
        for (int j = 0; j < 16; j++) {
            uint8_t val = ptr[(i * 16) + j];
            // Primitive hex padding check
            if (val < 16) {
                SHELL_putChar('0');
            }
            SHELL_Print(STR_int2str(val, digits, 16));
            SHELL_putChar(' ');
        }

        SHELL_Print(" \5m| ");

        // 2. Safe ASCII Character representation
        for (int j = 0; j < 16; j++) {
            uint8_t val = ptr[(i * 16) + j];
            // Filter out non-printable codes safely
            if (val >= 32 && val <= 126) {
                SHELL_putChar((char)val);
            } else {
                SHELL_putChar('.');
            }
        }
        SHELL_putChar('\n');
    }
}

void DEBUG_print_idt_gate(int vector) {
    char digits[33];
    
    struct __attribute__((packed)) {
        uint16_t limit;
        uint64_t base;
    } idt_ptr;

    // Pull the active location of the table matrix
    __asm__ volatile("sidt %0" : "=m"(idt_ptr));

    // An x86_64 IDT entry is exactly 16 bytes wide
    struct __attribute__((packed)) {
        uint16_t offset_low;
        uint16_t selector;
        uint8_t  ist;
        uint8_t  types_attr;
        uint16_t offset_mid;
        uint32_t offset_high;
        uint32_t reserved;
    } *gate = (void *)(idt_ptr.base + (vector * 16));

    // Bounds checking based on the CPU reported limit size
    if ((vector * 16) >= idt_ptr.limit) {
        SHELL_Print("\5rError: Vector index out of active IDT boundary limit!\n");
        return;
    }

    // Reconstruct the 64-bit function pointer offset destination
    uint64_t handler_addr = ((uint64_t)gate->offset_high << 32) | 
                            ((uint64_t)gate->offset_mid << 16)  | 
                            gate->offset_low;

    SHELL_Print("\5gIDT Interrupt Vector ");
    SHELL_Print(STR_int2str(vector, digits, 10));
    SHELL_Print(":\n");

    SHELL_Print("  \5yHandler ISR Offset: \5d0x");
    SHELL_Print(STR_64int2str(handler_addr, digits, 16));

    SHELL_Print("\n  \5yTarget Selector:   \5d0x");
    SHELL_Print(STR_int2str(gate->selector, digits, 16));

    SHELL_Print("\n  \5yAttributes Byte:   \5d0x");
    SHELL_Print(STR_int2str(gate->types_attr, digits, 16));
    SHELL_putChar('\n');
}
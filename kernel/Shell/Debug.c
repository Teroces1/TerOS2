#include "../../lib/string.h"
#include <stdbool.h>
#include <stdint.h>
#include "Shell.h"
#include "Debug.h"

volatile EntryPacket *DEBUG_ENTRYPACKET;
volatile VBEInfoBlock *DEBUG_VBEINFO;
const volatile VBEModeInfo *DEBUG_VBEMODEINFO = (volatile VBEModeInfo *)(0x10000);

char *encodeStrings[] = {
    [0] = "\\5gBoot Statistics:",
    [1] = "\n\\5y  Boot Drive: \\5d0x",
    [2] = "\n\\5y  Using LBA: \\5d",
    [3] = "\n\\5y  Drive Read Failures: \\5d",
    [4] = "\n\n\\5y  Total VBE Modes: \\5d",
    [5] = "\n\\5y  VBE Modes Read Failed: \\5d",
    [6] = "\n\\5y  VBE Mode Selected: \\5d0x",
    [7] = "\n\\5y  VBE Mode Selected Index: \\5d",
    [8] = "\n\n\\5y  CPU Mode: \\5d64-Bit Long Mode",
    NULL
};

void DEBUG_init(EntryPacket *entry, VBEInfoBlock *vbeinfo) {
    DEBUG_ENTRYPACKET = entry;
    DEBUG_VBEINFO = vbeinfo;

    int i = 0;
    while (encodeStrings[i] != NULL) {
        STR_CEncode(encodeStrings[i]);
        i++;
    }
}

void DEBUG_print_entry() {
    char digits[33];

    SHELL_Print(STR_CEncode("\\5gBoot Statistics:"));

    // Boot Drive
    SHELL_Print(STR_CEncode("\n\\5y  Boot Drive: \\5d0x"));
    SHELL_Print(STR_int2str((int) DEBUG_ENTRYPACKET->BootDrive, digits, 16));
    SHELL_Print(STR_CEncode(" ("));
    SHELL_Print(STR_int2str((int) DEBUG_ENTRYPACKET->BootDrive, digits, 10));
    SHELL_Print(STR_CEncode(")"));

    // Using LBA
    SHELL_Print(STR_CEncode("\n\\5y  Using LBA: \\5d"));
    if (DEBUG_ENTRYPACKET->UsingLBA)
        SHELL_Print(STR_CEncode("true"));
    else
        SHELL_Print(STR_CEncode("false"));

    // Drive Read Retries
    SHELL_Print(STR_CEncode("\n\\5y  Drive Read Failures: \\5d"));
    SHELL_Print(STR_int2str((int) DEBUG_ENTRYPACKET->ReadRetries, digits, 10));

    // Drive Geometry
    SHELL_Print(STR_CEncode("\n\\5y  Drive Heads: \\5d"));
    SHELL_Print(STR_int2str((int) DEBUG_ENTRYPACKET->DriveHeads, digits, 10));
    SHELL_Print(STR_CEncode("\n\\5y  Drive Sectors/Track: \\5d"));
    SHELL_Print(STR_int2str((int) DEBUG_ENTRYPACKET->DriveSectors, digits, 10));

    // VBE Modes
    SHELL_Print(STR_CEncode("\n\n\\5y  Total VBE Modes: \\5d"));
    SHELL_Print(STR_int2str((int) DEBUG_ENTRYPACKET->TotalModes, digits, 10));

    // VBE Modes Failed
    SHELL_Print(STR_CEncode("\n\\5y  VBE Modes Read Failed: \\5d"));
    SHELL_Print(STR_int2str((int) DEBUG_ENTRYPACKET->FailedModes, digits, 10));

    // Mode Selected
    SHELL_Print(STR_CEncode("\n\\5y  VBE Mode Selected: \\5d0x"));
    SHELL_Print(STR_int2str((int) DEBUG_ENTRYPACKET->ModeSelected, digits, 16));
    SHELL_Print(STR_CEncode(" ("));
    SHELL_Print(STR_int2str((int) DEBUG_ENTRYPACKET->ModeSelected, digits, 10));
    SHELL_Print(STR_CEncode(")"));

    // VBE Mode Selected Index
    SHELL_Print(STR_CEncode("\n\\5y  VBE Mode Selected Index: \\5d"));
    SHELL_Print(STR_int2str((int) DEBUG_ENTRYPACKET->ModeIndexSelected, digits, 10));

    // Base Memory
    SHELL_Print(STR_CEncode("\n\n\\5y  Base Memory: \\5d"));
    SHELL_Print(STR_int2str((int) DEBUG_ENTRYPACKET->BaseMemoryKB, digits, 10));
    SHELL_Print(STR_CEncode(" KB"));

    // CPU Vendor
    SHELL_Print(STR_CEncode("\n\\5y  CPU Vendor: \\5d"));
    SHELL_Print(DEBUG_ENTRYPACKET->CPUVendor);

    // CPU Features (Raw Hex)
    SHELL_Print(STR_CEncode("\n\\5y  CPU Features: \\5d0x"));
    SHELL_Print(STR_int2str((unsigned int) DEBUG_ENTRYPACKET->CPUFeatures, digits, 16));

    // --- Decoded CPU Features (Indented by 4 spaces instead of 2) ---
    
    // Bit 0: FPU (Floating Point Unit)
    if (DEBUG_ENTRYPACKET->CPUFeatures & (1 << 0))
        SHELL_Print(STR_CEncode("\n\\5y    FPU (Floating Point): \\5dSupported"));
    
    // Bit 3: PSE (Page Size Extension - allows 4MB/2MB pages, which you are using!)
    if (DEBUG_ENTRYPACKET->CPUFeatures & (1 << 3))
        SHELL_Print(STR_CEncode("\n\\5y    PSE (Huge Pages): \\5dSupported"));
    
    // Bit 4: TSC (Time Stamp Counter - great for precise timing/sleep functions)
    if (DEBUG_ENTRYPACKET->CPUFeatures & (1 << 4))
        SHELL_Print(STR_CEncode("\n\\5y    TSC (Time Stamp Counter): \\5dSupported"));
    
    // Bit 9: APIC (Advanced Programmable Interrupt Controller)
    if (DEBUG_ENTRYPACKET->CPUFeatures & (1 << 9))
        SHELL_Print(STR_CEncode("\n\\5y    APIC: \\5dSupported"));
    
    // Bit 25: SSE (Streaming SIMD Extensions)
    if (DEBUG_ENTRYPACKET->CPUFeatures & (1 << 25))
        SHELL_Print(STR_CEncode("\n\\5y    SSE: \\5dSupported"));
    
    // Bit 26: SSE2
    if (DEBUG_ENTRYPACKET->CPUFeatures & (1 << 26))
        SHELL_Print(STR_CEncode("\n\\5y    SSE2: \\5dSupported"));
    
    // CPU Mode
    SHELL_Print(STR_CEncode("\n\n\\5y  CPU Mode: \\5d64-Bit Long Mode\n"));

    SHELL_Print(STR_CEncode("\n\n\\5gSystem Memory Map (E820):"));
    
    for (int i = 0; i < DEBUG_ENTRYPACKET->E820Count; i++) {
        E820Entry* entry = &DEBUG_ENTRYPACKET->E820Map[i];
        
        SHELL_Print(STR_CEncode("\n\\5y  Base: \\5d0x"));
        SHELL_Print(STR_64int2str((entry->BaseAddress), digits, 16));
        
        SHELL_Print(STR_CEncode(" \\5y| Length: \\5d0x"));
        SHELL_Print(STR_64int2str((entry->Length), digits, 16));
        
        SHELL_Print(STR_CEncode(" \\5y| Type: \\5d"));
        
        // Decode the memory type
        switch (entry->Type) {
            case 1: SHELL_Print(STR_CEncode("Usable RAM")); break;
            case 2: SHELL_Print(STR_CEncode("Reserved (Hardware)")); break;
            case 3: SHELL_Print(STR_CEncode("ACPI Reclaimable")); break;
            case 4: SHELL_Print(STR_CEncode("ACPI NVS")); break;
            case 5: SHELL_Print(STR_CEncode("Bad Memory")); break;
            default: SHELL_Print(STR_CEncode("Unknown")); break;
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
        SHELL_Print("bbm\n");
    }

    SHELL_putChar('\n');
}

void DEBUG_print_mode_info(int modeIndex) {

}
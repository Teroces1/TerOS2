#include "../../lib/string.h"
#include <stdbool.h>
#include <stdint.h>
#include "Shell.h"
#include "Debug.h"

EntryPacket *DEBUG_ENTRYPACKET;

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

void DEBUG_init(EntryPacket *entry) {
    DEBUG_ENTRYPACKET = entry;

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
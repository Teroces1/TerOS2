#ifndef DEBUG_H
#define DEBUG_H

typedef struct __attribute__((packed)) {
    uint64_t BaseAddress;
    uint64_t Length;
    uint32_t Type;
    uint32_t ACPI_Extended_Attributes;
} E820Entry;

typedef struct __attribute__((packed)) {
    uint8_t BootDrive;
    uint8_t ReadRetries;
    uint8_t TotalModes;
    uint8_t FailedModes;
    uint8_t UsingLBA;
    uint16_t ModeSelected; // default 0xFFFF if none
    uint8_t ModeIndexSelected;
    uint32_t FrameBuffer;
    uint16_t BytesPerScanline;
    char CPUVendor[13];
    uint16_t BaseMemoryKB;
    uint8_t DriveHeads;
    uint8_t DriveSectors;
    uint32_t CPUFeatures;
    uint16_t E820Count;
    E820Entry E820Map[32];
} EntryPacket;


void DEBUG_init(EntryPacket *entry);

void DEBUG_print_entry(void);

#endif
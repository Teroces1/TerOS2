#ifndef TYPES_H
#define TYPES_H

namespace Kernel {
    using uint8_t  = __UINT8_TYPE__;
    using uint16_t = __UINT16_TYPE__;
    using uint32_t = __UINT32_TYPE__;
    using uint64_t = __UINT64_TYPE__;

    namespace EntryTypes {
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

        typedef struct __attribute__((packed)) {
            char signature[4];       // Should be "VESA"
            uint16_t version;        // e.g., 0x0200 for VBE 2.0
            uint32_t oem_string_ptr;
            uint32_t capabilities;
            uint32_t video_mode_ptr;
            uint16_t total_memory;   // In 64KB blocks
            uint16_t oem_software_rev;
            uint32_t oem_vendor_name_ptr;
            uint32_t oem_product_name_ptr;
            uint32_t oem_product_rev_ptr;
            uint8_t reserved[222];
            uint8_t oem_data[256];
        } VBEInfoBlock;
    }
}

#endif
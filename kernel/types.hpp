#ifndef TYPES_H
#define TYPES_H

namespace Kernel {
    using uint8_t  = __UINT8_TYPE__;
    using uint16_t = __UINT16_TYPE__;
    using uint32_t = __UINT32_TYPE__;
    using uint64_t = __UINT64_TYPE__;

    namespace EntryTypes {
        struct __attribute__((packed)) E820Entry{
            uint64_t BaseAddress;
            uint64_t Length;
            uint32_t Type;
            uint32_t ACPI_Extended_Attributes;
        };


        struct __attribute__((packed)) EntryPacket {
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
            uint32_t CPU_ExtendedFeatures;
            uint16_t E820Count;
            E820Entry E820Map[32];
            uint8_t VBEEnabled;
        };

        struct __attribute__((packed)) VBEInfoBlock{
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
        };

        struct __attribute__((packed)) VBEModeInfo {
            uint16_t mode_attributes;
            uint8_t win_a_attributes;
            uint8_t win_b_attributes;
            uint16_t win_granularity;
            uint16_t win_size;
            uint16_t win_a_segment;
            uint16_t win_b_segment;
            uint32_t win_func_ptr;
            uint16_t bytes_per_scanline;    // 18 bytes
            // VBE 1.2+
            uint16_t x_resolution;      // ptr+18
            uint16_t y_resolution;      // ptr +20
            uint8_t x_charsize;
            uint8_t y_charsize;
            uint8_t number_of_planes;
            uint8_t bits_per_pixel;     // ptr + 25
            uint8_t number_of_banks;
            uint8_t memory_model;
            uint8_t bank_size;
            uint8_t number_of_image_pages;
            uint8_t reserved1;
            // Direct Color fields
            uint8_t red_mask_size;
            uint8_t red_field_position;
            uint8_t green_mask_size;
            uint8_t green_field_position;
            uint8_t blue_mask_size;
            uint8_t blue_field_position;
            uint8_t reserved_mask_size;
            uint8_t reserved_field_position;
            uint8_t direct_color_mode_info;
            // VBE 2.0+
            uint32_t physical_base_ptr; // THIS IS THE FRAMEBUFFER ADDRESS! // ptr + 40
            uint32_t reserved2;
            uint16_t reserved3;
            uint8_t reserved4[206];
        };
    }
}

#endif
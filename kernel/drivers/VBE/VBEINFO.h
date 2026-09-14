#ifndef VBEINFO_H
#define VBEINFO_H

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

#endif
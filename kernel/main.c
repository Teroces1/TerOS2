#include <stdint.h>
#include "drivers/VBE.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

volatile uint16_t* const vga_buffer = (uint16_t*)0xB8000;

// ============================================================================
// VBE Structures
// Must be packed so GCC doesn't add padding bytes that ruin the layout!
// ============================================================================

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

typedef struct __attribute__((packed)) {
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
} VBEModeInfo;

// ============================================================================
// Display & Utility Functions
// ============================================================================

void clear_screen(uint8_t color) {
    uint16_t blank = (' ' | (color << 8));
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = blank;
    }
}

void print_string(const char* str, uint8_t color, int x, int y) {
    int index = (y * VGA_WIDTH) + x;
    for (int i = 0; str[i] != '\0'; i++) {
        vga_buffer[index++] = (str[i] | (color << 8));
        if (index >= VGA_WIDTH * VGA_HEIGHT) break;
    }
}

// Converts a number to a string (base 10 for decimal, base 16 for hex)
void itoa(uint32_t value, char* str, int base) {
    char temp[32];
    int i = 0;
    
    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    while (value > 0) {
        int remainder = value % base;
        temp[i++] = (remainder < 10) ? (remainder + '0') : (remainder - 10 + 'A');
        value /= base;
    }

    // Reverse the string
    int j = 0;
    while (i > 0) {
        str[j++] = temp[--i];
    }
    str[j] = '\0';
}

// Helper to easily print numbers at a location
void print_num(uint32_t value, int base, uint8_t color, int x, int y) {
    char buf[32];
    itoa(value, buf, base);
    print_string(buf, color, x, y);
}

// ============================================================================
// VBE Printing Logic
// ============================================================================

void print_vbe_info(VBEInfoBlock* info, uint32_t num_modes, int start_y) {
    print_string("--- VBE Global Info ---", 0x0E, 0, start_y);
    
    // Print Signature safely (it might not be null-terminated)
    char sig[5] = { info->signature[0], info->signature[1], info->signature[2], info->signature[3], 0 };
    print_string("Signature: ", 0x0F, 0, start_y + 1);
    print_string(sig, 0x0A, 12, start_y + 1);

    print_string("Version (Hex): ", 0x0F, 0, start_y + 2);
    print_num(info->version, 16, 0x0A, 15, start_y + 2);

    print_string("Total VRAM (64KB blocks): ", 0x0F, 0, start_y + 3);
    print_num(info->total_memory, 10, 0x0A, 26, start_y + 3);

    print_string("Modes Found: ", 0x0F, 0, start_y + 4);
    print_num(num_modes, 10, 0x0A, 13, start_y + 4);
}

void print_mode_info(uint32_t index, int start_y) {
    // Treat 0x10000 as a massive array of VBEModeInfo structs
    VBEModeInfo* modes_array = (VBEModeInfo*)0x10000;
    VBEModeInfo* target_mode = &modes_array[index];

    print_string("--- VBE Mode Details [Index ", 0x0B, 0, start_y);
    print_num(index, 10, 0x0B, 28, start_y);
    print_string("] ---", 0x0B, 30, start_y);

    print_string("Resolution: ", 0x0F, 0, start_y + 1);
    print_num(target_mode->x_resolution, 10, 0x0A, 12, start_y + 1);
    print_string("x", 0x0F, 16, start_y + 1);
    print_num(target_mode->y_resolution, 10, 0x0A, 18, start_y + 1);

    print_string("Color Depth (BPP): ", 0x0F, 0, start_y + 2);
    print_num(target_mode->bits_per_pixel, 10, 0x0A, 19, start_y + 2);

    print_string("Framebuffer Base (Hex): ", 0x0F, 0, start_y + 3);
    print_num(target_mode->physical_base_ptr, 16, 0x0A, 24, start_y + 3);
}

// ============================================================================
// Kernel Entry
// ============================================================================

void kernel_main(VBEInfoBlock* vbe_info, uint32_t num_modes) {
    clear_screen(0x00); // Black background

    // Print the general VBE controller info
    print_vbe_info(vbe_info, num_modes, 1);

    // Print specific info about Mode 0 (the first one your bootloader found)
    // You can change '0' to any index < num_modes to inspect different modes
    print_mode_info(0, 7);

    // Print specific info about Mode 1 (the second one)
    if (num_modes > 1) {
        print_mode_info(31, 12);
    }

    while (1) {
        __asm__ volatile("hlt");
    }
}
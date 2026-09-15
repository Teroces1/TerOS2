#include <stdint.h>
#include "drivers/VBE/VBE.h"
#include "drivers/VBE/VBEINFO.h"
#include "../lib/string.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25



// ============================================================================
// Kernel Entry
// ============================================================================
/*
BootDrive:      db 0
Retries:        db 0
NumModes:       db 0
UsingLBA:       db 0
ModeSelected:   db 0xFFFF
FrameBuffer: dd 0x0000
*/
typedef struct __attribute__((packed)) {
    uint8_t BootDrive;
    uint8_t ReadRetries;
    uint8_t TotalModes;
    uint8_t UsingLBA;
    uint16_t ModeIndexSelected; // default 0xFFFF if none
    uint32_t FrameBuffer;
    uint16_t BytesPerScanline;
} EntryPacket;



void kernel_main(const EntryPacket* entryPacket, const VBEInfoBlock* vbe_info) {
    VBE_framebuffer = 0x800000;//entryPacket->FrameBuffer;
    VBE_bytesPerScanline = entryPacket->BytesPerScanline;
    VBE_ClearFrontScreen(VBEC_GREEN); // Black background
    VBE_ClearScreen(VBEC_BLACK);

    char digits[10];

    STR_int2str(VBE_bytesPerScanline, digits, 10);

    VBE_PutString("Hello from 64-bit land!", 600, 410, VBEC_WHITE);
    VBE_PutString(digits, 600, 426, VBEC_WHITE);

    VBE_SetPixel(0,0, VBEC_WHITE);
    VBE_SetPixel(512,384, VBEC_GREEN);
    VBE_SetPixel(1023,767, VBEC_YELLOW);

    VBE_DrawRectangle(600, 400, 8, 8, VBEC_WHITE);

    VBE_DrawRectangle(610, 400, 8, 8, VBEC_PURPLE);
    VBE_DrawRectangle(620, 400, 8, 8, VBEC_WHITE);

    // VBE_PutString("Hello from 64-bit land!", 600, 410, VBEC_WHITE);

    VBE_Swap();



    // for (int y = 0; y < 786; y++) {
    //     // Pre-calculate vertical color components to save CPU cycles
    //     int r_base = (y * 255) / 786;
    //     int b_base = 255 - r_base;

    //     for (int x = 0; x < 1024; x++) {
    //         // Horizontal color components
    //         int g = (x * 255) / 1024;
            
    //         // Mix horizontal and vertical components for a diagonal effect
    //         int r = r_base;
    //         int b = b_base;

    //         // Combine into a 32-bit integer: 0x00BBGGRR
    //         // Alpha (highest byte) remains 0x00
    //         unsigned int color = (b << 16) | (g << 8) | r;

    //         // Paint the pixel
    //         VBE_SetPixel(x, y, color);
    //     }
    // }

    int hue = 0;
    int iters = 0;
    while (1) {
        // hue = 0;
        for (int y = 0; y < 786; y++) {
            hue = (hue + 1) % 256;
            // Calculate the color for this specific row
            uint32_t row_color = VBE_FROM_HUE(hue);
            
            // Calculate the starting index for this row (handling the VBE pitch/stride correctly)
            // VBE_bytesPerScanline is usually 4096 bytes, which is 1024 uint32_t pixels.
            int row_offset = y * (VBE_bytesPerScanline / 4);

            for (int x = 0; x < 1024; x++) {
                VBE_backframebuffer[row_offset + x] = row_color;
            }
        }
        VBE_PutString("Frames: ", 0, 0, VBEC_BLACK, VBEC_WHITE);
        STR_int2str(iters, digits, 10);
        VBE_PutString(digits, 64, 0, VBEC_BLACK, VBEC_WHITE);
        
        VBE_Swap();
        // wait_vsync();
        iters++;
    }

    while (1) {
        __asm__ volatile("hlt");
    }
}
#include <stdint.h>
#include "drivers/VBE/VBE.h"
#include "drivers/VBE/VBEINFO.h"

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

static inline unsigned char inb(unsigned short port) {
    unsigned char data;
    __asm__ volatile("inb %1, %0" : "=a"(data) : "Nd"(port));
    return data;
}

static inline void wait_vsync(void) {
    // 1. If we are currently inside a VBlank, wait for it to end first
    while ((inb(0x3DA) & 0x08));
    
    // 2. Now wait for the next VBlank interval to start
    while (!(inb(0x3DA) & 0x08));
}

void kernel_main(const VBEInfoBlock* vbe_info, const EntryPacket* entryPacket) {
    VBE_framebuffer = entryPacket->FrameBuffer;
    VBE_bytesPerScanline = entryPacket->BytesPerScanline;
    VBE_ClearScreen(VBEC_BLACK); // Black background

    VBE_SetPixel(0,0, VBEC_WHITE);
    VBE_SetPixel(512,384, VBEC_GREEN);
    VBE_SetPixel(1023,767, VBEC_YELLOW);

    for (int y = 0; y < 786; y++) {
        // Pre-calculate vertical color components to save CPU cycles
        int r_base = (y * 255) / 786;
        int b_base = 255 - r_base;

        for (int x = 0; x < 1024; x++) {
            // Horizontal color components
            int g = (x * 255) / 1024;
            
            // Mix horizontal and vertical components for a diagonal effect
            int r = r_base;
            int b = b_base;

            // Combine into a 32-bit integer: 0x00BBGGRR
            // Alpha (highest byte) remains 0x00
            unsigned int color = (b << 16) | (g << 8) | r;

            // Paint the pixel
            VBE_SetPixel(x, y, color);
        }
    }

    int hue = 0;
    while (1) {
        // hue = 0;
        for (int y = 0; y < 786; y++) {
            hue = (hue+1) % 256;
            for (int x = 0; x < 1024; x++) {
                VBE_SetPixel(x,y,VBE_HueToRGBPureInt(hue));
            }
        }
        VBE_Swap();
        // wait_vsync();
    }

    while (1) {
        __asm__ volatile("hlt");
    }
}
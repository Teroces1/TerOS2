#define SCREEN_WIDTH  1024
#define SCREEN_HEIGHT 768
#define SCREEN_BPP    32
#define BYTES_PER_PIXEL 4
#include "VBE.h"
#include <stdint.h>
#include "characters.h"

volatile uint32_t* volatile VBE_framebuffer;
unsigned int VBE_bytesPerScanline;

volatile uint32_t* volatile VBE_backframebuffer = (volatile uint32_t* volatile) 0x01000000;   // for now picking a random spot

void fast_flush(volatile uint32_t* dest, volatile uint32_t* src, int pixels) {
    for (int i = 0; i < pixels; i++) {
        dest[i] = src[i];
    }
}

void VBE_Swap() {
    fast_flush(VBE_framebuffer, VBE_backframebuffer, VBE_bytesPerScanline * SCREEN_HEIGHT);
}

void VBE_ClearScreen(unsigned int color) {
    for (int i = 0; i < SCREEN_WIDTH*SCREEN_HEIGHT; i++) {
        VBE_backframebuffer[i] = color;
    }
}

void VBE_ClearFrontScreen(unsigned int color) {
    for (int i = 0; i < SCREEN_WIDTH*SCREEN_HEIGHT; i++) {
        VBE_framebuffer[i] = color;
    }
}

inline unsigned int VBE_FROM_RGB(unsigned int r, unsigned int g, unsigned int b) {
    return r << 16 | g << 8 | b;
}

unsigned int VBE_FROM_HUE(unsigned int hue) {
    // Scale hue to the 0-1530 spectrum
    unsigned int hp = hue * 6;
    unsigned int sector = hp / 255;
    unsigned int remainder = hp % 255;

    switch (sector) {
    case 0: // Red to Yellow
        return VBE_FROM_RGB(255, remainder, 0);
    case 1: // Yellow to Green
        return VBE_FROM_RGB(255 - remainder, 255, 0);
    case 2: // Green to Cyan
        return VBE_FROM_RGB(0, 255, remainder);
    case 3: // Cyan to Blue
        return VBE_FROM_RGB(0, 255 - remainder, 255);
    case 4: // Blue to Magenta
        return VBE_FROM_RGB(remainder, 0, 255);
    case 6: // edge case: red
        return VBEC_RED;
    default: // Magenta back to Red (Sector 5)
        return VBE_FROM_RGB(255, 0, 255 - remainder);
    }
}

void VBE_SetPixel(int x, int y, unsigned int color) {
    VBE_backframebuffer[y * (VBE_bytesPerScanline / BYTES_PER_PIXEL) + x] = color;
}


void VBE_DrawRectangle(int x, int y, int width, int height, unsigned int color) {
    for (int i = x; i < x+width; i++) {
        for (int j = y; j < y+height; j++) {
            VBE_SetPixel(i, j, color);
        }
    }
}

void VBE_PutCharacter(char c, int x, int y, unsigned int color, unsigned int backColor) {
    uint8_t *map = FONT[(int) c];

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (map[col] & (0x80 >> row)) {
                VBE_SetPixel(x + row, y + col, color);
            } else {
                VBE_SetPixel(x + row, y + col, backColor);
            }
        }
    }
}

void VBE_PutString(const char *str, int x, int y, unsigned int color, unsigned int backColor) {
    int i = 0;
    while (str[i] != '\0') {
        VBE_PutCharacter(str[i], x + 10*i, y, color, backColor);
        i++;
    }
}



// TODO

// static inline unsigned char inb(unsigned short port) {
//     unsigned char data;
//     __asm__ volatile("inb %1, %0" : "=a"(data) : "Nd"(port));
//     return data;
// }

// static inline void wait_vsync(void) {
//     // 1. If we are currently inside a VBlank, wait for it to end first
//     while ((inb(0x3DA) & 0x08));
    
//     // 2. Now wait for the next VBlank interval to start
//     while (!(inb(0x3DA) & 0x08));
// }
#define SCREEN_WIDTH  1024
#define SCREEN_HEIGHT 768
#define SCREEN_BPP    32
#include "VBE.h"


volatile unsigned int* volatile VBE_framebuffer;

void VBE_ClearScreen(unsigned int color) {
    for (int i = 0; i < SCREEN_WIDTH*SCREEN_HEIGHT; i++) {
        VBE_framebuffer[i] = color;
    }
}
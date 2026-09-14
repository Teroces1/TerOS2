#ifndef VBE_H
#define VBE_H

extern volatile unsigned int* volatile VBE_framebuffer;

#define VBEC_RED 0x00FF0000
#define VBEC_BLUE 0x000000FF
#define VBEC_GREEN 0x0000FF00
#define VBEC_BLACK 0x00000000
#define VBEC_WHITE 0x00FFFFFF
#define VBEC_YELLOW 0x00FFFF00
#define VBEC_CYAN 0x0000FFFF
#define VBEC_PURPLE 0x00FF00FF

#endif
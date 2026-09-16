#include <stdint.h>
#include "drivers/VBE/VBE.h"
#include "drivers/VBE/VBEINFO.h"
#include "../lib/string.h"
#include "interrupts.h"
#include "WindowManager.h"
#include "Shell/Debug.h"

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




void kernel_main(const EntryPacket* entryPacket, const VBEInfoBlock* vbe_info) {
    VBE_framebuffer = 0x800000;//entryPacket->FrameBuffer;
    VBE_bytesPerScanline = entryPacket->BytesPerScanline;
    VBE_ClearFrontScreen(VBEC_BLACK); // Black background

    DEBUG_init(entryPacket, vbe_info);
    WINDOW_Init();


    init_interrupts();

    // SHELL_Print("Hello World!");
    WINDOW_Render();

    

    // char digits[10];

    // STR_int2str(entryPacket->ModeIndexSelected, digits, 10);

    // SHELL_Print(digits);

    // VBE_PutString("Hello from 64-bit land!", 600, 410, VBEC_WHITE);
    // VBE_PutString(digits, 600, 426, VBEC_WHITE);

    // VBE_SetPixel(0,0, VBEC_WHITE);
    // VBE_SetPixel(512,384, VBEC_GREEN);
    // VBE_SetPixel(1023,767, VBEC_YELLOW);

    // VBE_DrawRectangle(600, 400, 8, 8, VBEC_WHITE);

    // VBE_DrawRectangle(610, 400, 8, 8, VBEC_PURPLE);
    // VBE_DrawRectangle(620, 400, 8, 8, VBEC_WHITE);

    while (1) {
        WINDOW_Render();
    }


    // VBE_PutString("Hello from 64-bit land!", 600, 410, VBEC_WHITE);



    while (1) {
        __asm__ volatile("hlt");
    }
}
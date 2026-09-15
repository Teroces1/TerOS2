#include "../VBE/VBE.h"
#include "../../ports.h"
#include "keyboard.h"

const char scancode_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	'9', '0', '-', '=', '\b',	
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,	
 '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*',   0, ' '
};

struct interrupt_frame {
    uint64_t ip;
    uint64_t cs;
    uint64_t flags;
    uint64_t sp;
    uint64_t ss;
};

__attribute__((interrupt)) void keyboard_isr(struct interrupt_frame* frame) {
    uint8_t scancode = inb(0x60);
    
    if (!(scancode & 0x80)) {
        if (scancode < 58) {
            char ascii = scancode_ascii[scancode];
            
            // Send this ascii char directly into your Terminal Logic!
            VBE_PrintChar(ascii); 
        }
    }

    // Send EOI to PIC
    outb(0x20, 0x20);
}
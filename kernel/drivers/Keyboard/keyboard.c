#include "../VBE/VBE.h"
#include "../../ports.h"
#include "keyboard.h"
#include "../../WindowManager.h"
#include <stdbool.h>
#include <stdint.h>

const char scancode_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	'9', '0', '-', '=', '\b',	
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,	
 '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*',   0, ' '
};

const char extended_chars[128] = {
    [0x53] = 'd',   // delete
    [0x52] = 'i',   // insert

    [0x47] = 'h',   // home
    [0x4F] = 'e',   // end
    [0x49] = 'e',   // page up
    [0x51] = 'q',   // page down
    [0x48] = 'U',   // arrow up
    [0x50] = 'D',   // arrow down
    [0x4B] = 'L',   // left arrow
    [0x4D] = 'R',   // right arrow

    [0x1C] = '\r',   // keypad enter
    [0x35] = '/',   // keypad divide
};

char KEYBOARD_SECONDARY[128] = {
    ['a'] = 'A',
    ['b'] = 'B',
    ['c'] = 'C',
    ['d'] = 'D',
    ['e'] = 'E',
    ['f'] = 'F',
    ['g'] = 'G',
    ['h'] = 'H',
    ['i'] = 'I',
    ['j'] = 'J',
    ['k'] = 'K',
    ['l'] = 'L',
    ['m'] = 'M',
    ['n'] = 'N',
    ['o'] = 'O',
    ['p'] = 'P',
    ['q'] = 'Q',
    ['r'] = 'R',
    ['s'] = 'S',
    ['t'] = 'T',
    ['u'] = 'U',
    ['v'] = 'V',
    ['w'] = 'W',
    ['x'] = 'X',
    ['y'] = 'Y',
    ['z'] = 'Z',

    ['`'] = '~',
    ['1'] = '!',
    ['2'] = '@',
    ['3'] = '#',
    ['4'] = '$',
    ['5'] = '%',
    ['6'] = '^',
    ['7'] = '&',
    ['8'] = '*',
    ['9'] = '(',
    ['0'] = ')',
    ['-'] = '_',
    ['='] = '+',
    ['['] = '{',
    [']'] = '}',
    ['\\'] = '|',
    [';'] = ':',
    ['\''] = '"',
    [','] = '<',
    ['.'] = '>',
    ['/'] = '?',

    [' '] = ' ',
};

struct interrupt_frame {
    uint64_t ip;
    uint64_t cs;
    uint64_t flags;
    uint64_t sp;
    uint64_t ss;
};



KEYBOARD_STATE state = {0};

bool extended = false;

__attribute__((interrupt)) void keyboard_isr(struct interrupt_frame* frame) {
    uint8_t scancode = inb(0x60);


    if (scancode == 0xE0) {
        extended = true;
        outb(0x20, 0x20);
        return;
    }
    
    bool isPressed = !(scancode & 0x80);
    bool isSpecial = false;
    bool isControl = false;

    scancode = scancode & 0x7F;
    if (!extended) {
        switch(scancode) {
            case 0x2A:  // lshift
                state.LShift = isPressed;
                state.Secondary = state.CapsLock ^ (state.LShift || state.RShift);
                isSpecial = true;
                break;
            case 0x36:  // rshift
                state.RShift = isPressed;
                state.Secondary = state.CapsLock ^ (state.LShift || state.RShift);
                isSpecial = true;
                break;
            case 0x1D:  // control
                state.Ctrl = isPressed;
                isSpecial = true;
                break;
            case 0x38:  // alt
                state.Alt = isPressed;
                isSpecial = true;
                break;
            case 0x3A:  // caps lock
                if (isPressed)
                    state.CapsLock = !state.CapsLock;
                state.Secondary = state.CapsLock ^ (state.LShift || state.RShift);
                isSpecial = true;
                break;
            case 0x0E:  // backspace (has ascii)
            case 0x1C:  // enter (has ascii)
            case 0x01:  // ecsape (has ascii)
                isControl = true;
                break;
        }
    } else {
        switch(scancode) {
            case 0x1D:  // rctrl
                state.RCtrl = isPressed;
                isSpecial = true;
                break;
            case 0x38:  // ralt (AltGr)
                state.RAlt = isPressed;
                isSpecial = true;
                break;
        }
    }
    if (scancode < 128) {
        char ascii = extended ? extended_chars[scancode] : scancode_ascii[scancode];
        
        // Send this ascii char directly into your Terminal Logic!
        // VBE_PrintChar(ascii);


        uint16_t code = 
            (isPressed << 8) |
            (isSpecial << 9) |
            ((isControl || extended) << 10) |

            (state.Secondary << 11) |

            ((state.LShift || state.RShift) << 13) |
            ((state.Ctrl || state.RCtrl) << 14) |
            ((state.Alt || state.RAlt) << 15) |

            ascii;


        WINDOW_OnInput(code);
    }
    
    extended = false;
    // Send EOI to PIC
    outb(0x20, 0x20);
}
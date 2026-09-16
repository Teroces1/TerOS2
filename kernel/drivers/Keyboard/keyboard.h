#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdbool.h>

struct interrupt_frame;

__attribute__((interrupt)) void keyboard_isr(struct interrupt_frame* frame);

typedef struct {
    bool Secondary;  // for easier lookup, updated automatically
    bool LShift;
    bool RShift;
    bool Ctrl;
    bool Alt;
    bool RCtrl;
    bool RAlt;
    bool CapsLock;
} KEYBOARD_STATE;

extern char KEYBOARD_SECONDARY[];

#endif
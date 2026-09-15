#ifndef KEYBOARD_H
#define KEYBOARD_H

struct interrupt_frame; 

__attribute__((interrupt)) void keyboard_isr(struct interrupt_frame* frame);

#endif
#include <stdint.h>
#include "ports.h"

// Attribute flags for a 64-bit Interrupt Gate: 
// 0x8E = Present (1), Ring 0 (00), Lower bits mandatory (01110) -> 10001110
#define IDT_TA_INTERRUPT_GATE 0x8E 

typedef struct {
    uint16_t isr_low;      // The lower 16 bits of the ISR's address
    uint16_t kernel_cs;    // The 64-bit Code Segment selector (your GDT 0x18)
    uint8_t  ist;          // Interrupt Stack Table offset (set to 0 for default)
    uint8_t  attributes;   // Type and attributes (0x8E)
    uint16_t isr_mid;      // The middle 16 bits of the ISR's address
    uint32_t isr_high;     // The higher 32 bits of the ISR's address
    uint32_t reserved;     // Set to 0
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idtr_t;

// Reserve space for all 256 possible x86 interrupts
idt_entry_t idt[256];
idtr_t idtr;

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t attributes) {
    uint64_t addr = (uint64_t)isr;
    
    idt[vector].isr_low    = (uint16_t)(addr & 0xFFFF);
    idt[vector].kernel_cs  = 0x18; // Your 64-bit Code Segment Selector!
    idt[vector].ist        = 0;
    idt[vector].attributes = attributes;
    idt[vector].isr_mid    = (uint16_t)((addr >> 16) & 0xFFFF);
    idt[vector].isr_high   = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
    idt[vector].reserved   = 0;
}


// so for some reason, the motherboard sends hardware interrupts with the same code as cpu exceptions
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

void pic_remap(void) {
    // Send Initialization Command Word 1 (ICW1) to both PICs
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);

    // ICW2: Vector Offsets. Map Master PIC to 0x20, Slave to 0x28
    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);

    // ICW3: Tell Master PIC that Slave PIC is at IRQ2 (0x04)
    outb(PIC1_DATA, 0x04);
    // Tell Slave PIC its cascade identity (0x02)
    outb(PIC2_DATA, 0x02);

    // ICW4: Force 8086 mode environment
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    // Mask all interrupts by default except the keyboard (IRQ 1)
    // Keyboard is bit 1. (0xFD = 11111101 binary)
    outb(PIC1_DATA, 0xFD); 
    outb(PIC2_DATA, 0xFF); // Disable all on Slave PIC
}
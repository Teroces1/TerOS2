#include "types.hpp"
#include "MemoryManager.hpp"

#include "InterruptManager.hpp"

extern "C" {
    extern void gdt_flush(uint64_t);
}


extern void CPU_EXCEPTION(INT::Registers *regs);


namespace GDT {
    static GdtEntry gdt[5] __attribute__((aligned(16)));
    static GdtPtr gdtPtr;

    void GdtSetGate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
        gdt[num].baseLow    = static_cast<uint16_t>(base & 0xFFFF);
        gdt[num].baseMiddle = static_cast<uint8_t>(base >> 16) & 0xFF;
        gdt[num].baseHigh   = static_cast<uint8_t>(base >> 24) & 0xFF;

        gdt[num].limitLow   = static_cast<uint16_t>(limit & 0xFFFF);
        gdt[num].granularity = static_cast<uint8_t>(limit >> 16) & 0x0F;
        gdt[num].granularity |= gran & 0xF0;
        gdt[num].access      = access;
    }

    void InitGdt() {
        gdtPtr.limit = sizeof(gdt) - 1;  // 5 entries
        gdtPtr.base  = reinterpret_cast<uint64_t>(&gdt);

        // 0x00: Null Descriptor
        GdtSetGate(0, 0, 0, 0, 0);

        // 0x08: Kernel Code 64-bit (Access=0x9A, Granularity=0xA0)
        GdtSetGate(1, 0, 0, 0x9A, 0xA0);

        // 0x10: Kernel Data (Access=0x92, Granularity=0x00)
        GdtSetGate(2, 0, 0, 0x92, 0x00);

        // 0x1B: User Data Segment (Selector = 0x18 | DPL 3 = 0x1B)
        GdtSetGate(3, 0, 0, 0xF2, 0x00);

        // 0x23: User Code 64-bit (Selector = 0x20 | DPL 3 = 0x23)
        GdtSetGate(4, 0, 0, 0xFA, 0xA0);

        gdt_flush((uint64_t)&gdtPtr);
    }
}

namespace INT {
    constexpr uint16_t PIC1_COMMAND = 0x20;
    constexpr uint16_t PIC1_DATA    = 0x21;
    constexpr uint16_t PIC2_COMMAND = 0xA0;
    constexpr uint16_t PIC2_DATA    = 0xA1;

    static IdtEntry idt[256] __attribute__((aligned(16)));
    static IdtPtr idtPtr;

    InterruptManager* InterruptManager::Instance = nullptr;

    static void idtSetGate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
        idt[num].offsetLow     = static_cast<uint16_t>(base & 0xFFFF);
        idt[num].selector      = sel;
        idt[num].ist           = 0;
        idt[num].typeAttributes = flags;
        idt[num].offsetMiddle  = static_cast<uint16_t>((base >> 16) & 0xFFFF);
        idt[num].offsetHigh    = static_cast<uint32_t>((base >> 32) & 0xFFFFFFFF);
        idt[num].zero          = 0;
    }



    extern "C" void interruptHandler(Registers *regs) {
        if (regs->int_no < 32) {
            // cpu exception
            CPU_EXCEPTION(regs);
            while (1) __asm__ volatile("hlt");
        }

        if (regs->int_no >= 32 && regs->int_no <= 47) {
            if (regs->int_no == 33) {
                uint8_t scancode = inb(0x60);
            }

            // Send End of Interrupt (EOI) to PICs
            if (regs->int_no >= 40) {
                outb(PIC2_COMMAND, 0x20); // send to slave if IRQ 8..15
            }
            outb(PIC1_COMMAND, 0x20);     // send to master
        }
    }

    InterruptManager::InterruptManager() {
        Instance = this;

        GDT::InitGdt();

        idtPtr.base = reinterpret_cast<uint64_t>(&idt);
        idtPtr.limit = sizeof(idt) - 1;


        // now, remap PIC hardware interrupts from 0 - 15 to 32-47
        remapPIC();


        idtSetGate(0 , reinterpret_cast<uint64_t>(_isr0 ), 0x08, 0x8E);
        idtSetGate(1 , reinterpret_cast<uint64_t>(_isr1 ), 0x08, 0x8E);
        idtSetGate(8 , reinterpret_cast<uint64_t>(_isr8 ), 0x08, 0x8E);
        idtSetGate(13, reinterpret_cast<uint64_t>(_isr13), 0x08, 0x8E);
        idtSetGate(14, reinterpret_cast<uint64_t>(_isr14), 0x08, 0x8E)  ;
        idtSetGate(32, reinterpret_cast<uint64_t>(_isr32), 0x08, 0x8E);
        idtSetGate(33, reinterpret_cast<uint64_t>(_isr33), 0x08, 0x8E);

        asm volatile ("lidt %0" : : "m"(idtPtr));
        asm volatile ("sti");
    }


    void InterruptManager::remapPIC() {
        uint8_t a1 = inb(PIC1_DATA);
        uint8_t a2 = inb(PIC2_DATA);

        // Initialization Command Word (ICW1)
        outb(PIC1_COMMAND, 0x11);
        outb(PIC2_COMMAND, 0x11);

        // ICW2: Vector offsets (Master = 32, Slave = 40)
        outb(PIC1_DATA, 0x20); // IRQ 0..7  -> Vectors 32..39
        outb(PIC2_DATA, 0x28); // IRQ 8..15 -> Vectors 40..47

        // ICW3: Cascading layout
        outb(PIC1_DATA, 0x04); // Master PIC sees slave at IRQ2
        outb(PIC2_DATA, 0x02); // Slave PIC identity

        // ICW4: Environment info (8086 mode)
        outb(PIC1_DATA, 0x01);
        outb(PIC2_DATA, 0x01);

        outb(PIC1_DATA, a1);
        outb(PIC2_DATA, a2);
    }
}
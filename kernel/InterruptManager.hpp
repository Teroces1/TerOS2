#include "types.hpp"
#include "MemoryManager.hpp"
#ifndef INTERRUPTMANAGER_H
#define INTERRUPTMANAGER_H

using namespace Kernel;


namespace GDT {
    struct __attribute__((packed)) GdtEntry {
        uint16_t limitLow;
        uint16_t baseLow;
        uint8_t  baseMiddle;
        uint8_t  access;
        uint8_t  granularity;
        uint8_t  baseHigh;
    };

    struct __attribute__((packed)) GdtTssEntry {
        struct GdtEntry low;
        uint32_t baseHighest;
        uint32_t reserved;
    };

    struct __attribute__((packed)) GdtPtr {
        uint16_t limit;
        uint64_t base;
    };


    void GdtSetGate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

    void InitGdt();
}


namespace INT {
    inline void outb(uint16_t port, uint8_t val) {
        asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
    }

    inline uint8_t inb(uint16_t port) {
        uint8_t ret;
        asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
        return ret;
    }


    struct __attribute__((packed)) IdtEntry {
        uint16_t offsetLow;       // Target handler address bits 0..15
        uint16_t selector;        // Code Segment Selector in GDT (0x08 for kernel code)
        uint8_t  ist;             // Interrupt Stack Table index (bits 0..2), rest 0
        uint8_t  typeAttributes;  // Gate type, DPL, Present bit
        uint16_t offsetMiddle;    // Target handler address bits 16..31
        uint32_t offsetHigh;      // Target handler address bits 32..63
        uint32_t zero;            // Reserved (set to 0)
    };

    struct __attribute__((packed)) IdtPtr {
        uint16_t limit;
        uint64_t base;
    };

    struct __attribute__((packed)) Registers {
        uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
        uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
        uint64_t int_no, err_code;
        uint64_t rip, cs, rflags, rsp, ss;
    };

    

    extern "C" {
        extern void _isr0(void);     // divide by zero
        extern void _isr1(void);     // debug
        extern void _isr2(void);     // non maskable interrupt
        extern void _isr3(void);     // breakpoint (__asm__ volatile("int $3");)
        extern void _isr4(void);     // bound ragne exceeded
        extern void _isr5(void);     // overflow
        extern void _isr6(void);     // invalid opcode
        extern void _isr7(void);     // device not available
        extern void _isr8(void);     // double fault
        extern void _isr9(void);     // coprocessor segment overrun (legacy)
        extern void _isr10(void);    // invalid tss
        extern void _isr11(void);    // segment not present
        extern void _isr12(void);    // stack segment fault
        extern void _isr13(void);    // general protection fault
        extern void _isr14(void);    // page fault
        extern void _isr15(void);    // reserved
        extern void _isr16(void);    // x87 floating point exception
        extern void _isr17(void);    // alignment check
        extern void _isr18(void);    // machine check
        extern void _isr19(void);    // simd floating point exception
        extern void _isr20(void);    // virtualization exception
        extern void _isr21(void);    // reserved
        extern void _isr22(void);    // reserved
        extern void _isr23(void);    // reserved
        extern void _isr24(void);    // reserved
        extern void _isr25(void);    // reserved
        extern void _isr26(void);    // reserved
        extern void _isr27(void);    // reserved
        extern void _isr28(void);    // reserved
        extern void _isr29(void);    // reserved
        extern void _isr30(void);    // security execption
        extern void _isr31(void);    // reserved
        extern void _isr32(void);    // timer
        extern void _isr33(void);    // keyboard
    }

    using InterruptHandler = void (*)(Registers*);

    extern "C" void interruptHandler(Registers *regs);

    class InterruptManager {
    public:
        InterruptManager();

        InterruptManager& GetManager() {
            return *Instance;
        }

        void enableInterrupts();

        void RegisterHandler(uint8_t handlerIndex, InterruptHandler handler);

        void RegisterFallbackHandler(InterruptHandler handler);

    private:
        static InterruptManager *Instance;

        void remapPIC();

    };
}

#endif
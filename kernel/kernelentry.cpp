#include "types.hpp"
#include "VBE/VBE.hpp"
#include "VBE/FONT.h"
#include "MemoryManager.hpp"
#include "InterruptManager.hpp"


// alignas(16) static uint8_t kernelStack[16384]; // 16 KiB stack
// extern "C" {
//     volatile uint64_t initial_stack_top = reinterpret_cast<uint64_t>(kernelStack) + sizeof(kernelStack);
// }

using namespace Kernel;

volatile const uint16_t *BootSettings = (volatile uint16_t*) (0x1000);

// the bootloader puts the info block array at 0x10000
volatile const EntryTypes::VBEModeInfo *VBEInfos = (volatile EntryTypes::VBEModeInfo *) (0x10000);

volatile uint8_t *frameBufferVirtual = (volatile uint8_t*) (0x800000); // where it is in virtual memory

VBE::Display *primaryDisplay;

const char* ExceptionMessages[32] = {
        "Divide by Zero",               // 0
        "Debug",                        // 1
        "Non-Maskable Interrupt",       // 2
        "Breakpoint",                   // 3
        "Overflow",                     // 4
        "Bound Range Exceeded",         // 5
        "Invalid Opcode",               // 6
        "Device Not Available",         // 7
        "Double Fault",                 // 8
        "Coprocessor Segment Overrun",  // 9 (Legacy)
        "Invalid TSS",                  // 10
        "Segment Not Present",          // 11
        "Stack-Segment Fault",          // 12
        "General Protection Fault",     // 13
        "Page Fault",                   // 14
        "Reserved",                     // 15
        "x87 Floating-Point Exception", // 16
        "Alignment Check",              // 17
        "Machine Check",                // 18
        "SIMD Floating-Point Exception",// 19
        "Virtualization Exception",     // 20
        "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", 
        "Reserved", "Reserved", "Reserved", "Reserved", 
        "Security Exception",           // 30
        "Reserved"                      // 31
    };

void CPU_EXCEPTION(INT::Registers *regs) {
    VBE::Display &display = *primaryDisplay;

    const VBE::Color back = display.GetRGB(0,0,255);
    const VBE::Color color = display.GetRGB(255,255,255);

    display.ClearFront(back);
    display.PutStringFront(":(", 16, 0, 0, color, back);

    display.PutStringFront("=== CPU EXCEPTION ===", 0, 256, color, back);
    
    // Print the human-readable error!
    if (regs->int_no < 32) {
        display.PutStringFront(ExceptionMessages[regs->int_no], 0, 272, color, back);
    } else {
        display.PutStringFront("Unknown Exception", 0, 272, color, back);
    }

    display.PutStringFront("Interrupt Number: ", 0, 288, color, back);
    display.PutULLFront(regs->int_no, 10, 18*8, 288, color, back);

    // If it's a Page Fault (14), print CR2
    if (regs->int_no == 14) {
        uint64_t faulting_address;
        __asm__ volatile("mov %%cr2, %0" : "=r" (faulting_address));
        
        display.PutStringFront("Faulting Address: 0x", 0, 320, color, back);
        display.PutULLFront(faulting_address, 16, 20*8, 320, color, back);
    }

    // Print Instruction Pointer to know WHERE it crashed
    display.PutStringFront("Instruction Pointer (RIP): 0x", 0, 336, color, back);
    display.PutULLFront(regs->rip, 16, 29*8, 336, color, back);


}





extern "C" void kernel_main(const EntryTypes::EntryPacket* entryPacket, const EntryTypes::VBEInfoBlock* vbe_info) {
    const volatile EntryTypes::VBEModeInfo &vbemode = VBEInfos[entryPacket->ModeIndexSelected];

    VBE::Display mainDisplay(vbemode, frameBufferVirtual);
    primaryDisplay = &mainDisplay;

    mainDisplay.ClearFront();

    // if (Supports1GBPages()) {
    //     mainDisplay.ClearFront(mainDisplay.GetRGB(100, 205, 255));
    // } else {
    //     mainDisplay.ClearFront(mainDisplay.GetRGB(255, 005, 100));
    // }

    MEM::MemoryManager memMgr;

    memMgr.InitializeUsableMemoryMap(entryPacket);

    mainDisplay.testPrint("max usable ram: ", memMgr.totalRAMSize, 10);
    mainDisplay.testPrint("hardware rsrvd: ", memMgr.hardwareUsedRAM, 10);
    mainDisplay.testPrint("software rsrvd: ", memMgr.otherReservedRAM, 10);
    mainDisplay.testPrint("ram usage: (initially its only kernel itself and the ram bitmaps)");
    mainDisplay.testPrint("init  used RAM: ", memMgr.GetOccupiedRamAmount() - memMgr.otherReservedRAM, 10);
    

    memMgr.InitPaging(static_cast<uint64_t>(entryPacket->FrameBuffer), 0x400000);
    mainDisplay.SetFrameBuffer(reinterpret_cast<volatile uint8_t*>(MEM::frameBufferAddressSpace));

    mainDisplay.testPrint(" <   paging init   >");
    
    mainDisplay.testPrint("page  used RAM: ", memMgr.GetOccupiedRamAmount() - memMgr.otherReservedRAM, 10);

    memMgr.InitHeap(0x4000);    // 16 KiB
    mainDisplay.testPrint(" <   heap init /w 16 KiB   >");

    mainDisplay.testPrint("heap  used RAM: ", memMgr.GetOccupiedRamAmount() - memMgr.otherReservedRAM, 10);

    mainDisplay.testPrint(" <   kmalloc(0x3F00)   >");
    uint8_t *testPtr1 = static_cast<uint8_t*>(memMgr.kmalloc(0x3F00));

    mainDisplay.testPrint("heap  used RAM: ", memMgr.GetOccupiedRamAmount() - memMgr.otherReservedRAM, 10);

    mainDisplay.testPrint(" <   kmalloc(0x1000)   >");
    uint8_t *testPtr2 = static_cast<uint8_t*>(memMgr.kmalloc(0x1000));

    mainDisplay.testPrint("heap  used RAM: ", memMgr.GetOccupiedRamAmount() - memMgr.otherReservedRAM, 10);

    mainDisplay.testPrint(" <   kfree(ptr1)   >");
    memMgr.kfree(testPtr1);
    mainDisplay.testPrint("heap  used RAM: ", memMgr.GetOccupiedRamAmount() - memMgr.otherReservedRAM, 10);

    MEM::HeapData heapData;

    memMgr.getTotalHeapUsed(heapData);

    mainDisplay.testPrint("RAM used by heap: ", heapData.HeapStructSize, 10);
    mainDisplay.testPrint("Heap Size: ", heapData.TotalCapacity, 10);
    mainDisplay.testPrint("Heap Used: ", heapData.TotalUsed, 10);
    mainDisplay.testPrint("Num Blocks: ", heapData.TotalBlocks, 10);

    void *ptrsList[100];

    for (int i = 0; i < 100; i++) {
        ptrsList[i] = memMgr.kmalloc(4095 - sizeof(MEM::HeapNode));    // allocate one less bytes than page size on purpose
    }
    
    mainDisplay.testPrint(" <   malloc (100 pointers)   >");
    mainDisplay.testPrint("heap  used RAM: ", memMgr.GetOccupiedRamAmount() - memMgr.otherReservedRAM, 10);


    memMgr.getTotalHeapUsed(heapData);

    mainDisplay.testPrint("RAM used by heap: ", heapData.HeapStructSize, 10);
    mainDisplay.testPrint("Heap Size: ", heapData.TotalCapacity, 10);
    mainDisplay.testPrint("Heap Used: ", heapData.TotalUsed, 10);
    mainDisplay.testPrint("Num Blocks: ", heapData.TotalBlocks, 10);


    for (int i = 0; i < 100; i++) {
        memMgr.kfree(ptrsList[i]);
    }

    mainDisplay.testPrint(" <   free (100 pointers)   >");
    mainDisplay.testPrint("heap  used RAM: ", memMgr.GetOccupiedRamAmount() - memMgr.otherReservedRAM, 10);


    memMgr.getTotalHeapUsed(heapData);

    mainDisplay.testPrint("RAM used by heap: ", heapData.HeapStructSize, 10);
    mainDisplay.testPrint("Heap Size: ", heapData.TotalCapacity, 10);
    mainDisplay.testPrint("Heap Used: ", heapData.TotalUsed, 10);
    mainDisplay.testPrint("Num Blocks: ", heapData.TotalBlocks, 10);
        

    INT::InterruptManager intMgr;
    intMgr.RegisterFallbackHandler(CPU_EXCEPTION);
    for (uint8_t i = 0; i < 32; i++) {
        intMgr.RegisterHandler(i, CPU_EXCEPTION);
    }
    intMgr.enableInterrupts();

    // -- should trigger divide by 0
    int test = 5;
    int test2 = 0;
    int test3 = test / test2;

    int count = 0;
    mainDisplay.ClearFront();
    mainDisplay.testPrintLine = 0;


    // will allocate memory forever, and attempt to write to it. at some point, memory will run out, kmalloc will return nullptr, and writing to it will cause page fault
    while (1) {
        uint8_t *ptr = reinterpret_cast<uint8_t*>(memMgr.kmalloc(1500));
        // if (count %50 == 0)
            mainDisplay.testPrint("heap  used RAM: ", memMgr.GetOccupiedRamAmount() - memMgr.otherReservedRAM, 10);
        if (count %2000 != 0)
            mainDisplay.testPrintLine --;
        *ptr = 3;
        
        count++;
    }

    // -- should trigger page fault
    // uint64_t testAddr = 0x5238;
    // *reinterpret_cast<uint8_t *>(testAddr) = 5;


    while (1) {
        __asm__ volatile("hlt");
    }
}
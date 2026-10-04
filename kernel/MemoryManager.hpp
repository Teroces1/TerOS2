#include "types.hpp"

#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

using namespace Kernel;

namespace MEM {
    constexpr uint64_t frameBufferAddressSpace = 0xFFFF'FF70'0000'0000; // dedicates 64 GiB right before virtual memory
    constexpr uint64_t virtualMemory = 0xFFFF'FF80'0000'0000;
    constexpr uint64_t maxHHDM = 0xFFFF'FFC0'0000'0000;

    constexpr uint64_t kernelHeapStart = 0xFFFF'FFC0'0000'0000;
    constexpr uint64_t kernelHeapEnd = 0xFFFF'FFFF'8000'0000;

    constexpr uint64_t kernelVirtAddr = 0xFFFF'FFFF'8000'0000;
    constexpr uint64_t kernelPhysAddr = 0x000000;   // located at 0x100000, but page starting at 0


    struct HeapNode {   // should i use packed here? it would reduce the struct size to the minimum so might be useful
        HeapNode *prev; // need to store prev for grouping multiple free nodes, right?
        HeapNode *next;
        uint32_t size;
        bool free;
    };

    struct HeapData {
        uint64_t TotalCapacity;
        uint64_t HeapStructSize;
        int TotalBlocks;
        uint64_t TotalUsed;
    };


    class MemoryManager {
    public:
        MemoryManager();

        // MemoryManager(const MemoryManager) = delete;
        // MemoryManager& operator=(const MemoryManager&) = delete;
        // ~MemoryManager() = default;

        static MemoryManager& GetManager() {
            return *Instance;
        }

        int InitializeUsableMemoryMap(const EntryTypes::EntryPacket *entryPacket);

        uint64_t GetOccupiedRamAmount();

        uint64_t RequestFrame();

        void FreeFrame(uint64_t frameAddr);

        inline uint64_t FormatPageEntry(uint64_t address, bool writeEnabled, bool userLevel, bool massivePage) {
            return address | 1u | static_cast<uint64_t>(writeEnabled) << 1 | static_cast<uint64_t>(userLevel) << 2 | static_cast<uint64_t>(massivePage) << 7;
        }

        int InitPaging(uint64_t realFrameBufferAddr, uint64_t frameBufferSize);

        uint64_t CreateSinglePageMap();

        template <typename T = void>
        inline T* PhysicalAddressToPtr(uint64_t addr) {
            return reinterpret_cast<T*>(pagingEnabled ? virtualMemory + addr : addr);
        }

        uint64_t* LoadPageEntry(uint64_t entry);

        int MapAddress(uint64_t virtAddr, uint64_t realAddr, bool writeEnabled, bool userLevel, int pageSizeLevel);

        int RequestPage(uint64_t virtAddr);


        int InitHeap(int startingSizeBytes);

        void * kmalloc(int bytes);

        void getTotalHeapUsed(HeapData &data);


        void kfree(void *ptr);

        uint64_t totalRAMSize;
        uint64_t hardwareUsedRAM;
        uint64_t otherReservedRAM;

    private:
        static MemoryManager *Instance;
        uint8_t *usableMemoryMap;
        int memoryMapLength;
        uint64_t maxUsableRealAddr;
        uint64_t pml4RealAddr;
        bool supports1GBPages = false;
        bool pagingEnabled = false;     // bootloader enables basic paging, so use identity mapped addresses until this turns on
        int lastAllocatedFrameIndex = 0;    // points to the last index + 1


        HeapNode *head;

        inline bool _Supports1GBPages();
    };
}

#endif
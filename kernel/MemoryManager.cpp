#include "MemoryManager.hpp"
#include "VBE/VBE.hpp"
using namespace MEM;

MemoryManager* MemoryManager::Instance = nullptr;

MemoryManager::MemoryManager() : supports1GBPages(_Supports1GBPages()) {
    Instance = this;
}
extern "C" {
    extern const char _kernel_start;
    extern const char _kernel_end;
}

int MemoryManager::InitializeUsableMemoryMap(const EntryTypes::EntryPacket *entryPacket) {
    uint64_t highestAddress = 0;
    uint64_t largestBlockStart = 0;
    uint64_t largestBlockSize = 0;

    uint64_t kernelStartAddr = reinterpret_cast<uint64_t>(&_kernel_start) & ~(kernelVirtAddr);
    uint64_t kernelEndAddr = reinterpret_cast<uint64_t>(&_kernel_end) & ~(kernelVirtAddr);

    totalRAMSize = 0;
    hardwareUsedRAM = 0;
    otherReservedRAM = 0;


    for (int i = 0; i < entryPacket->E820Count; i++) {
        const EntryTypes::E820Entry &entry = entryPacket->E820Map[i];

        if (entry.Type == 1) {
            uint64_t endAddr = entry.BaseAddress + entry.Length;
            if (endAddr > highestAddress) {
                highestAddress = endAddr;
            }

            if (entry.Length > largestBlockSize && entry.BaseAddress <= 0x200000) { // safety check to ensure low memory is used
                largestBlockSize = entry.Length;
                largestBlockStart = entry.BaseAddress;
            }

            totalRAMSize += (endAddr & ~4095) - ((entry.BaseAddress + 4095) & ~4095);
        } else {
            hardwareUsedRAM += entry.Length;
        }
    }
    
    if (highestAddress == 0) {
        return 2;   // error code 1 (will display error to screen)
    }

    maxUsableRealAddr = highestAddress;

    
    memoryMapLength = static_cast<int>((highestAddress-1) / (8 * 4096)) + 1;    // hopefully this rounding trick will work to always round up
    int newLength = (memoryMapLength + 7) & ~7;   // rounding up to multiple of 8

    hardwareUsedRAM += 4096*(newLength - memoryMapLength);
    memoryMapLength = newLength;

    if (largestBlockStart <= kernelStartAddr && largestBlockStart + memoryMapLength >= kernelStartAddr) {
        // this is where the kernel is too, so we must move the largest block start to be where the kernel ends
        largestBlockStart = kernelEndAddr;
    }
    usableMemoryMap = reinterpret_cast<uint8_t*>(largestBlockStart);


    usableMemoryMap[0+memoryMapLength-1] = 0xFF;


    for (int i = 0; i < memoryMapLength; i++) {
        usableMemoryMap[i] = 0xFF;  // set to all 1s (used) to init
    }


    for (int i = 0; i < entryPacket->E820Count; i++) {
        const EntryTypes::E820Entry &entry = entryPacket->E820Map[i];

        if (entry.Type == 1) {
            uint64_t endAddr = (entry.BaseAddress + entry.Length) & ~0xFFF; // round down
            
            // set to 0 for free

            uint64_t currentAddr = (entry.BaseAddress + 4095) & ~0xFFF; // round up

            while (currentAddr < endAddr) {
                if ((currentAddr >= largestBlockStart && currentAddr < largestBlockStart+memoryMapLength) ||
                    (currentAddr >= (kernelStartAddr & ~0xFFF) && currentAddr < ((kernelEndAddr + 4095) & ~0xFFF))) { // automatically allocate
                    currentAddr += 4096;
                    continue;   // skip this cause its already taken up by the memory map and kernel
                } else if (currentAddr < 0x100000) { // automatically allocate
                    currentAddr += 4096;
                    otherReservedRAM += 4096;   // count it as other reserved for memory count stats
                    continue;   // skip this cause its reserved ram
                }
                int page = static_cast<int>((currentAddr) / 4096);

                int bit = page % 8;
                int index = page / 8;

                usableMemoryMap[index] &= ~(1u << bit);

                currentAddr += 4096;
            }

            if (endAddr >= highestAddress) {
                break;
            }
        }
    }

    return 0;
}

uint64_t MemoryManager::GetOccupiedRamAmount() {
    uint64_t * raw = reinterpret_cast<uint64_t*>(usableMemoryMap);
    uint64_t count = 0;
    for (int i = 0; i < memoryMapLength/8; i++) {
        if (raw[i] == 0) {
            count += 64;
            continue;
        } else if (raw[i] == 0xFFFFFFFFFFFFFFFF) {
            continue;
        } else {
            count += 64 - __builtin_popcountll(raw[i]);
        }
    }

    return totalRAMSize - count * 4096;
}

uint64_t MemoryManager::RequestFrame() {
    for (int i = lastAllocatedFrameIndex; i < memoryMapLength; i++) {
        if (usableMemoryMap[i] != 0xFF) {
            int bit = __builtin_ctz(~usableMemoryMap[i]);
            usableMemoryMap[i] |= static_cast<uint8_t>(1u << bit);

            lastAllocatedFrameIndex = i;

            return (i*8 + bit) * 4096;
        }
    }

    for (int i = 0; i < lastAllocatedFrameIndex; i ++) {
        if (usableMemoryMap[i] != 0xFF) {
            int bit = __builtin_ctz(~usableMemoryMap[i]);
            usableMemoryMap[i] |= static_cast<uint8_t>(1u << bit);

            lastAllocatedFrameIndex = i;

            return (i*8 + bit) * 4096;
        }
    }

    return 0;   // or change it to something else?
}

void MemoryManager::FreeFrame(uint64_t frameAddr) {
    frameAddr /= 4096;
    int index = frameAddr / 8;
    int bit = frameAddr % 8;

    if (index >= memoryMapLength) return;

    usableMemoryMap[index] &= static_cast<uint8_t>(~(1u << bit));
}


int MemoryManager::InitPaging(uint64_t realFrameBufferAddr, uint64_t frameBufferSize) {
    // first create pml4
    pml4RealAddr = CreateSinglePageMap();
    if (pml4RealAddr == 0) {
        return 1;   // error
    }
    
    //         (16 1s) 1111110... 
    // map HHDM 0xFFFF FF80 0000 0000 - FFFF FFBF FFFF FFFF     # provides for 256 gib max ram
    // leave unmapped: 0xFFFF FFC0 0000 0000 - 0xFFFF FFFF 7FFF FFFF    # other 254 gib virtual addresses given to kernel heap

    // Kernel lives at virtual 0xFFFFFFFF80000000, which must be memory mapped to 0x100000 (where the kernel is loaded into)


    // constexpr uint64_t HHDMSIZE = 0x0000'0040'0000'0000;
    for (uint64_t realAddress = 0; (virtualMemory + realAddress) < maxHHDM && realAddress < maxUsableRealAddr; realAddress+=0x40000000) {    // add 1 GiB
        if (MapAddress(virtualMemory + realAddress, realAddress, true, false, 2))
            return 2;
    }

    // map frame buffer address
    if (realFrameBufferAddr & 0x1FFFFF) { // make sure the given real frame buffer is aligned to 2mb large pages
        return 4;
    }
    int frameBufferPagesReq = (frameBufferSize - 1) / 4096 + 1;
    for (int i = 0; i < frameBufferPagesReq; i++) {
        MapAddress(frameBufferAddressSpace + 0x200000*i, realFrameBufferAddr + 0x200000*i, true, false, 1);
    }

    if (MapAddress(kernelVirtAddr, kernelPhysAddr, true, false, 2)) return 2;
    if (MapAddress(kernelVirtAddr + 0x40000000, kernelPhysAddr + 0x40000000, true, false, 2)) return 2;


    // then set the new pml4 address into cr3

    __asm__ volatile (
        "movq %0, %%cr3"
        :
        : "r" (pml4RealAddr)
        : "memory"
    );

    pagingEnabled = true;
    
    // usable memory map will always be <2 mb (cause usable ram pretty much has to exist there in like almost all systems prob)
    // so since it was identity mapped before, this should work cleanly
    usableMemoryMap = PhysicalAddressToPtr<uint8_t>(reinterpret_cast<uint64_t>(usableMemoryMap));

    return 0;
}

uint64_t MemoryManager::CreateSinglePageMap() {
    uint64_t addr = RequestFrame();
    if (addr == 0) {
        return 0;
    }

    uint64_t *rawMem = PhysicalAddressToPtr<uint64_t>(addr);
    for (int i = 0; i < 512; i++) {
        rawMem[i] = 0;
    }

    return addr;    // real address
}


uint64_t* MemoryManager::LoadPageEntry(uint64_t entry) {
    // since the kernel virtual address points to 0
    uint64_t addr = (entry & 0x000FFFFFFFFFF000ULL);   // i hope this is correct

    return PhysicalAddressToPtr<uint64_t>(addr);
}

int MemoryManager::MapAddress(uint64_t virtAddr, uint64_t realAddr, bool writeEnabled, bool userLevel, int pageSizeLevel) {    // page size level 0: 4kib, 1: 2mib, 3: 1gib
    int pml4Index = (virtAddr >> 39) & 0x1FF;
    uint64_t *pml4_ptr = LoadPageEntry(pml4RealAddr);

    if (!(pml4_ptr[pml4Index] & 0x1)) {
        uint64_t physAddr = CreateSinglePageMap();
        if (physAddr == 0) return 1;
        pml4_ptr[pml4Index] = FormatPageEntry(physAddr, true, true, false);
    }   // cant have large pages here

    int pdptIndex = (virtAddr>>30) & 0x1FF;
    uint64_t *pdpt_ptr = LoadPageEntry(pml4_ptr[pml4Index]);

    if (pageSizeLevel == 2) {
        if (supports1GBPages) {
            pdpt_ptr[pdptIndex] = FormatPageEntry(realAddr, writeEnabled, userLevel, true);
        } else {
            if (!(pdpt_ptr[pdptIndex] & 0x1)) {
                uint64_t physAddr = CreateSinglePageMap();
                if (physAddr == 0) return 1;
                pdpt_ptr[pdptIndex] = FormatPageEntry(physAddr, true, true, false);    // setting it all the fales?
            } else {    // if it contains anything at all (entire table is overwritten)
                return 3;   // already allocated (deallocate first)
            }

            uint64_t *pd_ptr = LoadPageEntry(pdpt_ptr[pdptIndex]);
            for (int pdIndex = 0; pdIndex < 512; pdIndex++) {
                pd_ptr[pdIndex] = FormatPageEntry(realAddr + pdIndex*0x200000, writeEnabled, userLevel, true);
            }
        }
    } else {
        if (!(pdpt_ptr[pdptIndex] & 0x1)) {
            uint64_t physAddr = CreateSinglePageMap();
            if (physAddr == 0) return 1;
            pdpt_ptr[pdptIndex] = FormatPageEntry(physAddr, true, true, false);    // setting it all the fales?
        } else if (pdpt_ptr[pdptIndex] & 0x80) {    // gb pages
            return 3;   // already allocated (deallocate first)
        }

        
        int pdIndex = (virtAddr>>21) & 0x1FF;
        uint64_t *pd_ptr = LoadPageEntry(pdpt_ptr[pdptIndex]);

        if (pageSizeLevel == 1) {
            pd_ptr[pdIndex] = FormatPageEntry(realAddr, writeEnabled, userLevel, true);
        } else {
            if (!(pd_ptr[pdIndex] & 0x1)) {
                uint64_t physAddr = CreateSinglePageMap();
                if (physAddr == 0) return 1;
                pd_ptr[pdIndex] = FormatPageEntry(physAddr, true, true, false);    // setting it all the fales?
            } else if (pd_ptr[pdIndex] & 0x80) {    // mb pages
                return 3;   // already allocated (deallocate first)
            }

            int ptIndex = (virtAddr>>12) & 0x1FF;
            uint64_t *pt_ptr = LoadPageEntry(pd_ptr[pdIndex]);
            if (pt_ptr[ptIndex] & 0x1) {
                return 3;   // already allocated (deallocate first)
            }
            pt_ptr[ptIndex] = FormatPageEntry(realAddr, writeEnabled, userLevel, false);
        }
    }

    return 0;
}

int MemoryManager::RequestPage(uint64_t virtAddr) {    // allocate 4kib physical memory for a virtual address
    uint64_t realAddr = RequestFrame();

    if (realAddr == 0) {
        return 1;
    }

    return MapAddress(virtAddr, realAddr, true, false, 0);
}


int MemoryManager::InitHeap(int startingSizeBytes) {
    uint64_t pagesReq = (startingSizeBytes-1) / 4096 + 1;

    for (uint64_t pageAddr = 0; pageAddr < pagesReq*4096; pageAddr+=4096) {
        int error = RequestPage(kernelHeapStart + pageAddr);
        if (error) return error;
    }

    // now startingSizeBytes bytes should be allocated starting from heapStart

    head = reinterpret_cast<HeapNode*>(kernelHeapStart);
    head -> prev = nullptr;
    head -> next = nullptr;
    head -> size = (pagesReq * 4096) - sizeof(HeapNode);
    head -> free = true;

    return 0;
}



void * MemoryManager::kmalloc(int bytes) {
    uint32_t totalNeeded = bytes + sizeof(HeapNode);

    HeapNode *current = head;
    while (current -> next != nullptr) {
        if (current->size >= totalNeeded && current->free) {
            break;
        }

        current = current->next;
    }

    // malloc always leaves the end of the heap free, so if no blocks match the size, then the last block should be currently free
    if (!current->free) {
        return nullptr; // error (something went wrong)
    }


    if (current -> next == nullptr && current->size < totalNeeded + sizeof(HeapNode)) {
        // new pages must be allocated, and current is currently the end of the linked list
        int extraNeeded = totalNeeded - current->size + sizeof(HeapNode);   // adding an extra free block to the end

        int pagesNeeded = (extraNeeded-1) / 4096 + 1;
        uint64_t currentAddr = (reinterpret_cast<uint64_t>(current) + current->size + sizeof(HeapNode));
        uint64_t startingPage = (currentAddr + 4095) & ~4095ULL;
        uint64_t endingPage = startingPage + 4096*pagesNeeded;
        for (uint64_t pageAddr = startingPage; pageAddr < endingPage; pageAddr+=4096) {
            int error = RequestPage(pageAddr);
            if (error) return nullptr;  // what else can i do with the error code?
        }

        current->size += pagesNeeded * 4096; // remember, subtracting a heap node cause a last node has been added
    }

    
    uint32_t remainingSize = current->size - bytes; // not using total needed, cause remaining size must include the header
    if (remainingSize < sizeof(HeapNode)) {
        remainingSize = 0;  // let it just have the few bytes extra
    }

    current->size -= remainingSize;
    current->free = false;

    if (remainingSize > 0) {
        HeapNode *newNode = reinterpret_cast<HeapNode *>(reinterpret_cast<uint64_t>(current) + sizeof(HeapNode) + current->size);

        newNode -> size = remainingSize - sizeof(HeapNode); // subtracting remaining size now:
        newNode -> prev = current;
        newNode -> next = current->next;
        current -> next = newNode;

        newNode -> free = true;

        if (newNode->next != nullptr && newNode -> next -> free) {
            newNode -> size += newNode -> next -> size + sizeof(HeapNode);
            newNode -> next = newNode -> next -> next;
            if (newNode -> next != nullptr) {
                newNode -> next -> prev = newNode;
            }
        } else if (newNode -> next != nullptr) {
            newNode -> next -> prev = newNode;
        }
    } else {
        // nothing, and this cant ever happen if its last node in the heap, right?
        // this only happens if a block in the middle of a list happens to perfectly be the exact requested size
    }
    

    return static_cast<void*>(reinterpret_cast<uint8_t *>(current) + sizeof(HeapNode));
}


void MemoryManager::kfree(void *ptr) {
    HeapNode *node = reinterpret_cast<HeapNode *>(reinterpret_cast<uint64_t>(ptr) - sizeof(HeapNode));

    node -> free = true;

    if (node -> prev != nullptr && node -> prev -> free) {
        node -> prev -> next = node -> next;
        node -> prev -> size += node -> size + sizeof(HeapNode);
        if (node->next != nullptr) {
            node->next->prev = node->prev;
        }
        node = node -> prev;
    }

    if (node -> next != nullptr && node -> next -> free) {
        node -> size += node -> next -> size + sizeof(HeapNode);
        node -> next = node -> next -> next;
        if (node->next != nullptr) {
            node->next->prev = node;
        }
    }

    // todo: check if its the last node, and if it is and its free, maybe unallocate those unused pages?

}

void MemoryManager::getTotalHeapUsed(HeapData &data) {
    data.TotalBlocks = 0;
    data.TotalCapacity = 0;
    data.TotalUsed = 0;
    data.HeapStructSize = 0;
    HeapNode *current = head;
    while (current != nullptr) {
        data.TotalBlocks ++;

        data.TotalCapacity += current->size;
        if (!current->free) {
            data.TotalUsed += current->size;
        }

        if (current -> next == nullptr) {
            uint64_t end = reinterpret_cast<uint64_t>(current) + sizeof(HeapData) + current->size;
            data.HeapStructSize = end - reinterpret_cast<uint64_t>(head);
        }

        current = current->next;
    }
}

inline bool MemoryManager::_Supports1GBPages() {
    uint32_t edx;
    // CPUID leaf 0x80000001. We only care about EDX.
    __asm__ volatile (
        "cpuid"
        : "=d" (edx)
        : "a" (0x80000001)
        : "ebx", "ecx"
    );
    return (edx & (1 << 26)) != 0;
}
#include <efi.h>
#include <efilib.h>

void stall() {
    while (1) __asm__ volatile("hlt");
}

typedef struct {
    // UINT32                     Version;               // Structure version (usually 0)
    UINT32                     HorizontalResolution;  // Screen width in pixels
    UINT32                     VerticalResolution;    // Screen height in pixels
    UINT32                     PixelFormat;           // Color layout mapping
    // EFI_PIXEL_BITMASK          PixelInformation;      // Valid ONLY if PixelFormat == PixelBitMask
    UINT32                     PixelsPerScanLine;     // Stride padding per row
    UINT8                      Available;
} ModeInfo;

typedef struct __attribute__((packed)) {
    UINT64 FrameBufferBase;
    UINT64 FrameBufferSize;

    UINT32 NumModes;
    UINT32 ModeSelected;
    ModeInfo *ModeTablePtr;

    UINT64 MemoryMapBase;
    UINT64 MemoryMapSize;
    UINT64 DescriptorSize;
    UINT32 DescriptorVersion;

} KernelEntryPacket;

typedef UINT64 page_table_t[512];
const EFI_PHYSICAL_ADDRESS KernelAddress = (EFI_PHYSICAL_ADDRESS) 0x100000;

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);

    *((volatile UINT64 *)(0xF8473FF0392384)) = 34; // shouldnt this make it crash?
    Print(L"UEFI has finished laoding... Bootloader starting...\r\n");
    while (1) __asm__ volatile("hlt");

    EFI_STATUS status;
    EFI_LOADED_IMAGE_PROTOCOL *loadedImage = NULL;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fileSystem = NULL;
    EFI_GUID lipGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    status = uefi_call_wrapper(BS->HandleProtocol, 3, ImageHandle, &lipGuid, (void**)&loadedImage);

    if (EFI_ERROR(status)) {
        Print(L"Error Getting EFI_LOADED_IMAGE_PROTOCOL!\r\n");
        stall();
    }

    status = uefi_call_wrapper(BS->HandleProtocol, 3, loadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void**) &fileSystem);
    if (EFI_ERROR(status)) {
        Print(L"Error getting file system protocol!\r\n");
        stall();
    }

    EFI_FILE_PROTOCOL *root = NULL;
    status = uefi_call_wrapper(fileSystem->OpenVolume, 2, fileSystem, &root);

    if (EFI_ERROR(status) || root == NULL) {
        Print(L"Error getting root file protocol!\r\n");
        stall();
    }

    EFI_FILE_HANDLE kernelFile = NULL;
    status = uefi_call_wrapper(root->Open, 5, root, &kernelFile, L"\\kernel.bin", EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
        Print(L"Error loading kernel.bin!\r\n");
        stall();
    }

    EFI_FILE_INFO *kernelFileInfo = LibFileInfo(kernelFile);
    if (kernelFileInfo == NULL) {
        Print(L"Error getting loading kernel.bin file info!\r\n");
        stall();
    }

    UINTN numPages = (kernelFileInfo->FileSize - 1) / 4096 + 1;

    FreePool(kernelFileInfo);   // unallocate the mem again

    status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAddress, EfiLoaderData, numPages, &KernelAddress);
    if (EFI_ERROR(status)) {
        Print(L"Error while allocating space for kernel.elf!\r\n");
        stall();
    }

    UINTN bufferSize = numPages * 4096;
    status = uefi_call_wrapper(kernelFile->Read, 3, kernelFile, &bufferSize, (VOID*)KernelAddress);

    if (EFI_ERROR(status)) {
        Print(L"Error while reading kernel.bin!\r\n");
        stall();
    }

    // now generate the new page tables for 0xFFFFFFFF80000000

    // we need 1 pml4, 2 pdpt, 3 pd
    EFI_PHYSICAL_ADDRESS tablesAddress = 0x9FFFFF;
    status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateMaxAddress, EfiRuntimeServicesData, 6, &tablesAddress);

    
    if (EFI_ERROR(status)) {
        Print(L"Error while allocating space for page tables!\r\n");
        stall();
    }

    
    page_table_t *pageTables = (page_table_t*) tablesAddress;

    pageTables[0][511] = ((UINT64) &(pageTables[1])) | 0x3;
    pageTables[1][510] = ((UINT64) &(pageTables[2])) | 0x3;
    pageTables[1][511] = ((UINT64) &(pageTables[3])) | 0x3;

    pageTables[0][0]   = ((UINT64) &(pageTables[4])) | 0x3;
    pageTables[4][0]   = ((UINT64) &(pageTables[5])) | 0x3;

    for (int i = 0; i < 512; i++) {
        pageTables[2][i] = (0x200000ull * i) | 0x83;    // massive page, read+write
    }
    for (int i = 0; i < 512; i++) {
        pageTables[3][i] = (0x40000000 + 0x200000ull * i) | 0x83;    // massive page, read+write
    }
    for (int i = 0; i < 512; i++) { // also make sure first gb is also identity mapped
        pageTables[5][i] = (0x200000ull * i) | 0x83;    // massive page, read+write
    }

    // locate GOP, load mode
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    status = uefi_call_wrapper(BS->LocateProtocol, 3, &gEfiGraphicsOutputProtocolGuid, NULL, (VOID**)&gop);
    if (EFI_ERROR(status)) {
        Print(L"Error while getting GOP!\r\n");
        stall();
    }

    // allocate space for the mode list
    ModeInfo *modeInfoTable = 0x100000;
    UINT32 infoTablePages = (sizeof(ModeInfo) * gop->Mode->MaxMode - 1) / 4096 + 1;
    status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateMaxAddress, EfiRuntimeServicesData, infoTablePages, (void**) &modeInfoTable);

    if (EFI_ERROR(status)) {
        Print(L"Error while allocating space for GOP mode information table!\r\n");
        stall();
    }

    UINTN sizeOfInfo;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *modeInfo;
    BOOLEAN modeWasEnabled = FALSE;

    for (UINT32 mode = 0; mode < gop->Mode->MaxMode; mode++) {
        status = uefi_call_wrapper(gop->QueryMode, 4, gop, mode, &sizeOfInfo, &modeInfo);

        if (EFI_ERROR(status)) {
            Print(L"[non critical] Error while getting mode information for GOP mode %d!\r\n", mode);

            modeInfoTable[mode].Available = 0;

            continue;
        }

        modeInfoTable[mode].Available = 1;
        modeInfoTable[mode].HorizontalResolution = modeInfo->HorizontalResolution;
        modeInfoTable[mode].VerticalResolution   = modeInfo->VerticalResolution;
        modeInfoTable[mode].PixelFormat          = modeInfo->PixelFormat;
        modeInfoTable[mode].PixelsPerScanLine    = modeInfo->PixelsPerScanLine;



        if (modeInfo->HorizontalResolution == 1920 && modeInfo->VerticalResolution == 1080 &&
            (modeInfo->PixelFormat == PixelRedGreenBlueReserved8BitPerColor ||
             modeInfo->PixelFormat == PixelBlueGreenRedReserved8BitPerColor)) {
            status = uefi_call_wrapper(gop->SetMode, 2, gop, mode);
            if (EFI_ERROR(status)) {
                Print(L"Unable to enable mode %d!\r\n", mode);
                stall();
            }
            
            modeWasEnabled = TRUE;
        }

        FreePool(modeInfo);
    }

    // get memory map
    UINTN memoryMapSize = 0;
    EFI_MEMORY_DESCRIPTOR *memoryMap = 0x100000;    // memory map should be stored under 1 mb
    UINTN mapKey = 0;
    UINTN descriptorSize = 0;
    UINT32 descriptorVersion = 0;
    status = uefi_call_wrapper(BS->GetMemoryMap, 5, &memoryMapSize, NULL, &mapKey, &descriptorSize, &descriptorVersion);

    if (status != EFI_BUFFER_TOO_SMALL) {
        Print(L"Error while getting memory map!\r\n");
        stall();
    }

    memoryMapSize += descriptorSize * 4;
    UINT32 memoryMapPages = (memoryMapSize - 1) / 4096 + 1;
    status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateMaxAddress, EfiRuntimeServicesData, memoryMapPages, (void**)&memoryMap);
    if (EFI_ERROR(status)) {
        Print(L"Error while allocating memory for memory map buffer!\r\n");
        stall();
    }

    // construct entry packet, and make final get memory map call
    KernelEntryPacket *entryPacket = 0x100000;
    status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateMaxAddress, EfiRuntimeServicesData, 1, (void**)&entryPacket);
    if (EFI_ERROR(status)) {
        Print(L"Error while allocating memory for entry packet!\r\n");
        stall();
    }

    entryPacket->FrameBufferBase   = gop->Mode->FrameBufferBase;
    entryPacket->FrameBufferSize   = gop->Mode->FrameBufferSize;
    entryPacket->ModeSelected      = gop->Mode->Mode;
    entryPacket->NumModes          = gop->Mode->MaxMode;
    entryPacket->ModeTablePtr      = modeInfoTable;

    status = uefi_call_wrapper(BS->GetMemoryMap, 5, &memoryMapSize, memoryMap, &mapKey, &descriptorSize, &descriptorVersion);
    if (EFI_ERROR(status)) {
        Print(L"Error getting memory map (final call)!\r\n");
        stall();
    }

    entryPacket->MemoryMapBase     = (UINT64) memoryMap;
    entryPacket->MemoryMapSize     = memoryMapSize;
    entryPacket->DescriptorSize    = descriptorSize;
    entryPacket->DescriptorVersion = descriptorVersion;

    status = uefi_call_wrapper(BS->ExitBootServices, 2, ImageHandle, mapKey);
    if (EFI_ERROR(status)) {
        Print(L"ExitBootServices failed!\r\n");
        stall();
    }

    __asm__ volatile("mov %0, %%cr3" : : "r"((uint64_t)pageTables) : "memory");
    
    
    // call kernel
    void (*kernel_entry)(KernelEntryPacket*) = (void (*)(KernelEntryPacket*)) 0x0000000080100000;
    kernel_entry(entryPacket);


    // while (1) __asm__ volatile("hlt");

    return EFI_SUCCESS;
}
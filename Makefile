# ==============================================================================
# Auto-Configuring OS Build System (UEFI + Custom Bootloader)
# ==============================================================================

# --- Toolchain Configuration ---
TOOLCHAIN_PREFIX = x86_64-linux-gnu-
CC      = $(TOOLCHAIN_PREFIX)gcc
CXX     = $(TOOLCHAIN_PREFIX)g++
LD      = $(TOOLCHAIN_PREFIX)ld
OBJCOPY = $(TOOLCHAIN_PREFIX)objcopy
AS      = nasm

# --- Kernel Compiler / Linker Flags ---
CFLAGS  = -ffreestanding -m64 -O0 -Wall -Wextra -MMD -MP -fno-pic -fno-pie -fno-plt -fno-stack-protector -mno-red-zone -mgeneral-regs-only -mcmodel=kernel
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti -Wall -Wextra -mpopcnt
LDFLAGS = -T linker.ld -m elf_x86_64 -z noexecstack --no-warn-rwx-segments

# --- Directories ---
BUILD_DIR = build
BOOT_DIR  = boot
OLD_DIR   = old

# ==============================================================================
# GNU-EFI Configuration (Dynamically finding paths)
# ==============================================================================
EFI_ARCH = x86_64
EFI_INC  = /usr/include/efi

# Find the GNU-EFI linker script and CRT object dynamically (paths vary by distro)
EFI_LDS     := $(shell find /usr/lib /usr/lib64 /usr/lib/x86_64-linux-gnu -name 'elf_$(EFI_ARCH)_efi.lds' -print -quit 2>/dev/null)
EFI_CRT_OBJ := $(shell find /usr/lib /usr/lib64 /usr/lib/x86_64-linux-gnu -name 'crt0-efi-$(EFI_ARCH).o' -print -quit 2>/dev/null)
EFI_LIB_DIR := $(dir $(EFI_CRT_OBJ))

EFI_CFLAGS  = -fpic -ffreestanding -fno-stack-protector -fno-strict-aliasing \
              -fshort-wchar -mno-red-zone -maccumulate-outgoing-args \
              -I$(EFI_INC) -I$(EFI_INC)/$(EFI_ARCH) -I$(EFI_INC)/protocol \
              -Wall -Wextra -O2

EFI_LDFLAGS = -shared -Bsymbolic -L$(EFI_LIB_DIR) -T $(EFI_LDS) $(EFI_CRT_OBJ)
EFI_LIBS    = -lefi -lgnuefi

# ==============================================================================
# Dynamic File Discovery
# ==============================================================================
# Find Kernel sources (Excluding the boot directory so the bootloader isn't linked into the kernel)
C_SOURCES   := $(shell find . -type f -name '*.c' -not -path "*/\.*" -not -path "./$(BUILD_DIR)/*" -not -path "./$(OLD_DIR)/*" -not -path "./$(BOOT_DIR)/*")
CXX_SOURCES := $(shell find . -type f -name '*.cpp' -not -path "*/\.*" -not -path "./$(BUILD_DIR)/*" -not -path "./$(OLD_DIR)/*" -not -path "./$(BOOT_DIR)/*")
ASM_SOURCES := $(shell find . -type f -name '*.asm' -not -path "*/\.*" -not -path "./$(BUILD_DIR)/*" -not -path "./$(OLD_DIR)/*" -not -path "./$(BOOT_DIR)/*")

ENTRY_ASM := $(BOOT_DIR)/start.asm
ENTRY_OBJ := $(BUILD_DIR)/$(BOOT_DIR)/start.o

# Map Kernel sources to objects
C_OBJS   := $(patsubst ./%.c, $(BUILD_DIR)/%.o, $(C_SOURCES))
CXX_OBJS := $(patsubst ./%.cpp, $(BUILD_DIR)/%.o, $(CXX_SOURCES))
ASM_OBJS := $(patsubst ./%.asm, $(BUILD_DIR)/%.o, $(ASM_SOURCES))
KERNEL_OBJS := $(ENTRY_OBJ) $(C_OBJS) $(ASM_OBJS) $(CXX_OBJS)

IMAGE = $(BUILD_DIR)/os.img

# --- UEFI / Emulator Configuration ---
OVMF_FD ?= /usr/share/OVMF/OVMF_CODE_4M.fd
QEMU = qemu-system-x86_64
QEMU_FLAGS = -cpu host -smp 2 -m 512M -no-reboot -no-shutdown -machine q35,smm=on,accel=kvm -drive if=pflash,format=raw,readonly=on,file=$(OVMF_FD) -drive format=raw,file=${BUILD_DIR}/os.img


# ==============================================================================
# Build Rules
# ==============================================================================
.PHONY: all clean run runclean check_efi

all: check_efi $(IMAGE)

-include $(KERNEL_OBJS:.o=.d)

# --- 1. Bootloader Compilation (GNU-EFI) ---
$(BUILD_DIR)/bootloader.o: $(BOOT_DIR)/bootloader.c
	@mkdir -p $(dir $@)
	$(CC) $(EFI_CFLAGS) -c $< -o $@

$(BUILD_DIR)/bootloader.so: $(BUILD_DIR)/bootloader.o
	$(LD) $(EFI_LDFLAGS) $< -o $@ $(EFI_LIBS)

$(BUILD_DIR)/BOOTX64.EFI: $(BUILD_DIR)/bootloader.so
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym \
	           -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc \
	           --target efi-app-$(EFI_ARCH) $< $@

# --- 2. Kernel Compilation ---
$(BUILD_DIR)/boot/start.o: $(BOOT_DIR)/start.asm
	@mkdir -p $(dir $@)
	$(AS) -f elf64 $< -o $@

$(BUILD_DIR)/%.o: %.asm
	@mkdir -p $(dir $@)
	$(AS) -f elf64 $< -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.elf: $(KERNEL_OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS)

$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/kernel.elf
	$(OBJCOPY) -O binary $< $@

# --- 3. Construct Final UEFI Disk Image (Hardware Compatible) ---
$(IMAGE): $(BUILD_DIR)/BOOTX64.EFI $(BUILD_DIR)/kernel.bin
	@mkdir -p $(dir $@)
	@echo "Creating Hardware-Compatible UEFI disk image..."
	
	# 1. Create a blank 64MB image
	@dd if=/dev/zero of=$(IMAGE) bs=1M count=64 status=none
	
	# 2. Create a GPT partition table and an EFI System Partition (ESP)
	# The partition starts at 1MB (2048 sectors) to ensure proper alignment.
	@parted -s $(IMAGE) mklabel gpt
	@parted -s $(IMAGE) mkpart ESP fat32 2048s 100%
	@parted -s $(IMAGE) set 1 esp on
	
	# 3. Format the partition as FAT32. 
	# We use @@1048576 to tell mtools to format at the 1MB offset (2048 sectors * 512 bytes)
	@mformat -i $(IMAGE)@@1048576 -F -v "OS_EFI" ::
	
	# 4. Create the UEFI boot directory structure
	@mmd -i $(IMAGE)@@1048576 ::/EFI
	@mmd -i $(IMAGE)@@1048576 ::/EFI/BOOT
	
	# 5. Copy the Bootloader and Kernel into the image
	@echo " -> Injecting BOOTX64.EFI..."
	@mcopy -i $(IMAGE)@@1048576 $(BUILD_DIR)/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI
	
	@echo " -> Injecting kernel.elf..."
	@mcopy -i $(IMAGE)@@1048576 $(BUILD_DIR)/kernel.bin ::/kernel.bin

# 	@echo " -> Injecting startup.nsh..."
# 	@mcopy -i $(IMAGE)@@1048576 $(BOOT_DIR)/startup.nsh ::/startup.nsh
	
	@echo "UEFI OS Image built successfully! Ready for QEMU or USB flashing."

# ==============================================================================
# Utility Commands
# ==============================================================================
check_efi:
	@if [ -z "$(EFI_LDS)" ] || [ -z "$(EFI_CRT_OBJ)" ]; then \
		echo "[!] ERROR: GNU-EFI libraries not found!"; \
		echo "[!] Please install gnu-efi (e.g., sudo apt install gnu-efi)."; \
		exit 1; \
	fi

clean:
	rm -rf $(BUILD_DIR)

run: $(IMAGE)
	@if [ ! -f $(OVMF_FD) ]; then \
		echo "[!] ERROR: OVMF firmware not found at $(OVMF_FD)"; \
		echo "[!] Please install 'ovmf' or update the OVMF_FD path in the Makefile."; \
		exit 1; \
	fi
	$(QEMU) $(QEMU_FLAGS)

runclean: clean run
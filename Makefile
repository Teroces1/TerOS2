# ==============================================================================
# Auto-Configuring OS Build System
# ==============================================================================

# --- Toolchain Configuration ---
# Swap to x86_64-elf- when moving to 64-bit Long Mode
TOOLCHAIN_PREFIX = i686-linux-gnu-#i686-elf-
CC      = $(TOOLCHAIN_PREFIX)gcc
LD      = $(TOOLCHAIN_PREFIX)ld
OBJCOPY = $(TOOLCHAIN_PREFIX)objcopy
AS      = nasm

# --- Compiler / Linker Flags ---
# Swap -m32 to -m64 for Long Mode (and add -mno-red-zone -mgeneral-regs-only)
CFLAGS  = -ffreestanding -m32 -Os -Wall -Wextra -MMD -MP  -fno-pic -fno-pie -fno-plt -fno-stack-protector
# Swap elf_i386 to elf_x86_64 for Long Mode
LDFLAGS = -T linker.ld -m elf_i386

# --- Directories ---
BUILD_DIR = build
BOOT_DIR  = boot

# ==============================================================================
# Dynamic File Discovery
# ==============================================================================
# We define explicitly which files are flat binary bootloader stages.
BOOT_FLAT_SRCS = ./$(BOOT_DIR)/bootinit.asm ./$(BOOT_DIR)/bootloader.asm ./$(BOOT_DIR)/vge_stub.asm

# Find ALL C and ASM files, ignoring hidden folders and the build dir.
C_SOURCES   := $(shell find . -type f -name '*.c' -not -path "*/\.*" -not -path "*/python/*" -not -path "./$(BUILD_DIR)/*")
ALL_ASM     := $(shell find . -type f -name '*.asm' -not -path "*/\.*" -not -path "*/python/*" -not -path "./$(BUILD_DIR)/*")

# The Kernel ASM sources are ALL ASM files EXCEPT the flat binary bootloader stages.
# Notice this naturally allows `boot/start.asm` to become part of the kernel objects automatically!
ASM_SOURCES := $(filter-out $(BOOT_FLAT_SRCS), $(ALL_ASM))

# Map sources to objects
C_OBJS   := $(patsubst ./%.c, $(BUILD_DIR)/%.o, $(C_SOURCES))
ASM_OBJS := $(patsubst ./%.asm, $(BUILD_DIR)/%.o, $(ASM_SOURCES))
OBJS     := $(C_OBJS) $(ASM_OBJS)

# ==============================================================================
# Auto-Configuration Logic
# ==============================================================================
# Check what actually exists in your directory right now
HAS_BOOTINIT := $(wildcard $(BOOT_DIR)/bootinit.asm)
HAS_STAGE2   := $(wildcard $(BOOT_DIR)/bootloader.asm)
HAS_VGESTUB  := $(wildcard $(BOOT_DIR)/vge_stub.asm)
HAS_KERNEL   := $(strip $(OBJS))

# Build our list of dependencies for the final OS image dynamically
IMAGE_DEPS := 
ifneq ($(HAS_BOOTINIT),)
    IMAGE_DEPS += $(BUILD_DIR)/bootinit.bin
endif
ifneq ($(HAS_STAGE2),)
    IMAGE_DEPS += $(BUILD_DIR)/bootloader.bin
endif
ifneq ($(HAS_VGESTUB),)
    IMAGE_DEPS += $(BUILD_DIR)/vge_stub.bin
endif
ifneq ($(HAS_KERNEL),)
    IMAGE_DEPS += $(BUILD_DIR)/kernel.bin
endif

IMAGE = $(BUILD_DIR)/os.img

# Emulator Configuration
QEMU = qemu-system-x86_64
QEMU_FLAGS = -drive format=raw,file=$(IMAGE) -m 128M -d guest_errors -no-reboot -no-shutdown

# ==============================================================================
# Build Rules
# ==============================================================================
.PHONY: all clean run runclean debug_config

all: $(IMAGE)

-include $(OBJS:.o=.d)

# 1. Bootloader Assembly (Flat Binaries)
$(BUILD_DIR)/%.bin: $(BOOT_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(AS) -f bin $< -o $@

# 2. Kernel Assembly (ELF32 -> change to elf64 for long mode)
$(BUILD_DIR)/%.o: %.asm
	@mkdir -p $(dir $@)
	$(AS) -f elf32 $< -o $@

# 3. Kernel C Compilation
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# 4. Link ELF Kernel
# This rule is protected: It will cleanly fail if start.asm is missing!
$(BUILD_DIR)/kernel.elf: $(OBJS)
	@mkdir -p $(dir $@)
	@if [ ! -f $(BOOT_DIR)/start.asm ]; then \
		echo "\n[!] BUILD ERROR: Missing Kernel Entry Point!"; \
		echo "[!] You have C/ASM files to build the kernel, but $(BOOT_DIR)/start.asm is missing."; \
		echo "[!] The kernel needs this file to act as the entry point. Create it to continue.\n"; \
		exit 1; \
	fi
	$(LD) $(LDFLAGS) -o $@ $(BUILD_DIR)/boot/start.o $(filter-out $(BUILD_DIR)/boot/start.o, $(OBJS))

# 5. Extract Flat Binary from ELF
$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/kernel.elf
	$(OBJCOPY) -O binary $< $@
	@echo "Kernel size: $$(stat -c%s $@) bytes"

# 6. Construct Final Disk Image (Auto-Adapting)
$(IMAGE): $(IMAGE_DEPS)
	@mkdir -p $(dir $@)
	@echo "Assembling disk image..."
	@dd if=/dev/zero of=$(IMAGE) bs=512 count=2880 status=none
	@if [ -f $(BUILD_DIR)/bootinit.bin ]; then \
		echo " -> Injecting bootinit (Sector 0)..."; \
		dd if=$(BUILD_DIR)/bootinit.bin of=$(IMAGE) conv=notrunc status=none; \
	fi
	@if [ -f $(BUILD_DIR)/bootloader.bin ]; then \
		echo " -> Injecting stage2 (Sector 1)..."; \
		dd if=$(BUILD_DIR)/bootloader.bin of=$(IMAGE) bs=512 seek=1 conv=notrunc status=none; \
	fi
	@if [ -f $(BUILD_DIR)/vge_stub.bin ]; then \
		echo " -> Injecting vge_stub (Sector 4)..."; \
		dd if=$(BUILD_DIR)/vge_stub.bin of=$(IMAGE) bs=512 seek=4 conv=notrunc status=none; \
	fi
	@if [ -f $(BUILD_DIR)/kernel.bin ]; then \
		echo " -> Injecting Kernel (Sector 5)..."; \
		dd if=$(BUILD_DIR)/kernel.bin of=$(IMAGE) bs=512 seek=5 conv=notrunc status=none; \
	fi
	@echo "OS Image built successfully!"

# ==============================================================================
# Utility Commands
# ==============================================================================
clean:
	rm -rf $(BUILD_DIR)

run: $(IMAGE)
	$(QEMU) $(QEMU_FLAGS)

runclean: clean run

# Helpful debug tool to see what the Makefile is currently detecting
debug_config:
	@echo "Detected Files:"
	@echo "  Bootinit:   $(if $(HAS_BOOTINIT),YES,NO)"
	@echo "  Stage 2:    $(if $(HAS_STAGE2),YES,NO)"
	@echo "  VGE Stub:   $(if $(HAS_VGESTUB),YES,NO)"
	@echo "  Kernel:     $(if $(HAS_KERNEL),YES,NO)"
	@echo "Image Dependencies: $(IMAGE_DEPS)"
BOOT 		= boot
BOOT_NAME	= BOOTX64.EFI
ISO_ENTRY	= EFI/BOOT/$(BOOT_NAME)

BOOT_CC 		= x86_64-w64-mingw32-gcc
BOOT_CFLAGS 	= -std=c17 -ffreestanding -fno-stack-protector -m64 -nostdlib \
				-fno-stack-check -maccumulate-outgoing-args -fpic -fshort-wchar -mno-red-zone \
				-Wall -Wextra -Werror -I include -Wl,--subsystem,10 -e EfiMain -O2 -nostartfiles

KERNEL_CC		= gcc
KERNEL_CFLAGS	= -std=c17 -ffreestanding -pie -fPIE -m64 -mno-red-zone -Wall -Wextra -Werror -nostdlib \
				-I include -nostartfiles -O2

KERNEL			= kernel

OVMF_PATH 		= OVMF.fd

KERNEL_SOURCES = $(wildcard src/kernel/*.c)
KERNEL_OBJECTS = $(patsubst src/kernel/%.c,build/kernel/%.o,$(KERNEL_SOURCES))

all: iso emul

iso: clean initBootDir bootloader kernel
	@echo Building disk image...
	@xorriso -report_about WARNING -as mkisofs -iso-level 3 -o SOS.ISO -full-iso9660-filenames -volid "SOS" -eltorito-alt-boot -e $(ISO_ENTRY) -no-emul-boot -isohybrid-gpt-basdat iso/

bootloader:
	@echo [BOOT] Building bootloader...
	@nasm src/boot/trampoline.asm -o build/boot/trampoline.bin -f bin
	@objcopy build/boot/trampoline.bin build/boot/trampoline.o -I binary -O elf64-x86-64 -B i386:x86-64
	@$(BOOT_CC) $(BOOT_CFLAGS) src/boot/main.c build/boot/trampoline.o -o iso/$(ISO_ENTRY)

kernel: $(KERNEL_OBJECTS)
	@echo [KERNEL] Linking kernel...
	@nasm -f elf64 src/kernel/isr.asm -o build/kernel/isr.o
	@$(KERNEL_CC) $(KERNEL_OBJECTS) build/kernel/isr.o -o iso/$(KERNEL).elf $(KERNEL_CFLAGS)

build/kernel/%.o: src/kernel/%.c
	@echo [KERNEL] Building $*...
	@$(KERNEL_CC) -c $< -o $@ $(KERNEL_CFLAGS)

initBootDir:
	@cp src/boot/files.cfg iso/EFI/BOOT/files.cfg
	@cp src/boot/main.bmft iso/main.bmft

clean:
	@echo Cleaning build files...
	@rm -rf iso/ build/ SOS.ISO
	@echo Making clean build directories...
	@mkdir -p iso/EFI/BOOT iso/EFI/LOGS build/boot build/kernel

emul:
	@echo Starting emulation...
	@qemu-system-x86_64 -drive format=raw,file=fat:rw:iso/ -bios $(OVMF_PATH) -net none -d int 2> stderr.log

setup-ubuntu:
	sudo apt update && sudo apt upgrade
	sudo apt install xorriso qemu-system gcc-mingw-w64

setup-arch:
	sudo pacman -Syu
	sudo pacman -S xorriso qemu-desktop mingw-w64-gcc

setup-msys:
	pacman -Syu
	pacman -S xorriso mingw-w64-x86_64-qemu mingw-w64-x86_64-gcc

test:
	mkdir -p a/{b,c}

.PHONY: all emul setup kernel iso bootloader initBootDir

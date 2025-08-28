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

KERNEL		= kernel

OVMF_PATH = OVMF.fd

all: build emul

build: clean
	@echo Making clean build directories...
	@mkdir -p iso/EFI/BOOT iso/EFI/LOGS build/boot build/kernel

	@echo Building bootloader...
	@nasm src/boot/trampoline.asm -o build/boot/trampoline.bin -f bin
	@objcopy build/boot/trampoline.bin build/boot/trampoline.o -I binary -O elf64-x86-64 -B i386:x86-64
	@$(BOOT_CC) $(BOOT_CFLAGS) src/boot/main.c build/boot/trampoline.o -o iso/$(ISO_ENTRY)


	@echo Building kernel...
	@$(KERNEL_CC) src/kernel/main.c -o iso/$(KERNEL).elf $(KERNEL_CFLAGS)

	@echo Building disk image...
	@xorriso -report_about WARNING -as mkisofs -iso-level 3 -o SOS.ISO -full-iso9660-filenames -volid "SOS" -eltorito-alt-boot -e $(ISO_ENTRY) -no-emul-boot -isohybrid-gpt-basdat iso/

clean:
	@echo Cleaning build files...
	@rm -rf iso/ build/ SOS.ISO

emul:
	@echo Starting emulation...
	@qemu-system-x86_64 -drive format=raw,file=fat:rw:iso/ -bios $(OVMF_PATH) -net none

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

.PHONY: all build emul setup

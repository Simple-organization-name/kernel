BOOT_CC 		= x86_64-w64-mingw32-gcc
BOOT_CFLAGS 	= -std=c17 -ffreestanding -fno-stack-protector -m64 -nostdlib -fno-stack-check -fpic -fshort-wchar \
				-mno-red-zone -maccumulate-outgoing-args -Wall -Wextra -Werror -I include -Wl,--subsystem,10 -e EfiMain \
				-O2 -nostartfiles

BOOT 		= boot
BOOT_NAME	= BOOTX64.EFI
ISO_ENTRY	= EFI/BOOT/$(BOOT_NAME)

KERNEL_CC		= gcc -ffreestanding
KERNEL_CFLAGS	= -std=c17 -ffreestanding -fno-pic -m64 -mno-red-zone -Wall -Wextra -Werror -I include -nostartfiles

KERNEL		= kernel

OVMF_PATH = OVMF.fd

all: build emul

build: clean
	@echo Making clean build directories...
	@mkdir -p iso/EFI/BOOT/ build

	@echo Building bootloader...
	@$(BOOT_CC) $(BOOT_CFLAGS) src/$(BOOT).c -o iso/$(ISO_ENTRY)

	@echo Building kernel...
	@$(KERNEL_CC) src/$(KERNEL).c -o iso/$(KERNEL).elf $(KERNEL_CFLAGS)

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

.PHONY: all build emul setup

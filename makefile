CC 			= x86_64-w64-mingw32-gcc
CFLAGS 		= -std=c17 -ffreestanding -fno-stack-protector -m64 -nostdlib -fno-stack-check -fpic -fshort-wchar \
				-mno-red-zone -maccumulate-outgoing-args -Wall -Wextra -I include -Wl,--subsystem,10 -e EfiMain \
				-O2

FILE		= boot
BOOT_NAME	= BOOTX64.EFI
ISO_ENTRY	= EFI/BOOT/$(BOOT_NAME)

OVMF_PATH = OVMF.fd

all: build emul

build: clean
	mkdir -p iso/EFI/BOOT/ build
	$(CC) $(CFLAGS) src/$(FILE).c -o iso/$(ISO_ENTRY)
	xorriso -as mkisofs -iso-level 3 -o SOS.ISO -full-iso9660-filenames -volid "SOS" -eltorito-alt-boot -e $(ISO_ENTRY) -no-emul-boot -isohybrid-gpt-basdat iso/

clean:
	rm -rf iso/ build/ SOS.ISO

emul:
	qemu-system-x86_64 -drive format=raw,file=fat:rw:iso/ -bios $(OVMF_PATH) -net none

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

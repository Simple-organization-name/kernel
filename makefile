CC 			= x86_64-w64-mingw32-gcc
CFLAGS 		= -ffreestanding -fno-stack-protector -nostdlib -fno-stack-check -fpic -fshort-wchar \
				-mno-red-zone -maccumulate-outgoing-args -Wall -I /usr/include/ -I/usr/include/x86_64-linux-gnu/ -Wl,--subsystem,10 -e EfiMain 

FILE		= boot
BOOT_NAME	= BOOTX64.EFI
ISO_ENTRY	= EFI/BOOT/$(BOOT_NAME)

OVMF_PATH = /usr/share/ovmf/OVMF.fd

all: build emul

build:
	mkdir -p iso/EFI/BOOT/ build
	$(CC) $(CFLAGS) src/$(FILE).c -o iso/$(ISO_ENTRY)
	xorriso -as mkisofs -iso-level 3 -o SOS.ISO -full-iso9660-filenames -volid "SOS" -eltorito-alt-boot -e $(ISO_ENTRY) -no-emul-boot -isohybrid-gpt-basdat iso/

emul:
	qemu-system-x86_64 -drive format=raw,file=fat:rw:iso/ -bios $(OVMF_PATH) -net none

setup:
	sudo apt update && sudo apt upgrade
	sudo apt install xorriso qemu-system gcc-mingw-w64

.PHONY: all build emul setup

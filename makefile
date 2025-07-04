BOOT_NAME = BOOTX64.EFI
ISO_ENTRY = EFI/BOOT/$(BOOT_NAME)

OVMF_PATH = /usr/share/ovmf/OVMF.fd

all: iso emul

iso:
	nasm -f win64 src/main.asm -o build/main.o
	lld-link /subsystem:efi_application /nodefaultlib /entry:efi_main build/main.o /out:iso/$(ISO_ENTRY)
	xorriso -as mkisofs -iso-level 3 -o SOS.ISO -full-iso9660-filenames -volid "SOS" -eltorito-alt-boot -e $(ISO_ENTRY) -no-emul-boot -isohybrid-gpt-basdat iso/

emul:
	qemu-system-x86_64 -drive format=raw,file=fat:rw:iso/ -bios $(OVMF_PATH) -net none

setup:
	sudo apt install lld genisoimage qemu-system ovmf

.PHONY: all iso emul setup
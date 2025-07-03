BOOT_NAME = BOOTX64.EFI
ISO_ENTRY = EFI/BOOT/$(BOOT_NAME)

OVMF_PATH = /usr/share/ovmf/OVMF.fd

all: iso shell

iso:
	nasm -f win64 src/main.asm -o build/main.o
	lld-link /subsystem:efi_application /nodefaultlib /entry:efi_main build/main.o /out:iso/$(ISO_ENTRY)
	cp -f iso/$(ISO_ENTRY) drive/$(BOOT_NAME)
	xorriso -as mkisofs -iso-level 3 -o SOS.ISO -full-iso9660-filenames -volid "SOS" -eltorito-alt-boot -e $(ISO_ENTRY) -no-emul-boot -isohybrid-gpt-basdat iso/

emul:
	qemu-system-x86_64 -bios $(OVMF_PATH) -cdrom SOS.ISO -m 512M -boot d -net none

shell:
	qemu-system-x86_64 -bios $(OVMF_PATH) -net none -drive format=raw,file=fat:rw:drive/

setup:
	sudo apt install lld genisoimage qemu-system ovmf

.PHONY: all iso emul shell setup
all: iso emul

iso:
	nasm -f win64 src/main.asm -o build/main.o
	lld-link /subsystem:efi_application /nodefaultlib /entry:efi_main build/main.o /out:iso/EFI/BOOT/BOOTx64.EFI
	cp -f iso/EFI/BOOT/BOOTx64.EFI drive/BOOTx64.EFI
# xorriso -as mkisofs -R -J -joliet-long -o SOS.ISO -V "UEFI_BOOT" -eltorito-alt-boot -e EFI/BOOT/BOOTx64.EFI -no-emul-boot -isohybrid-gpt-basdat iso/
	xorriso -as mkisofs -iso-level 3 -o SOS.ISO -full-iso9660-filenames -volid "SOS" -eltorito-alt-boot -e EFI/BOOT/BOOTx64.EFI -no-emul-boot -isohybrid-gpt-basdat iso/
# genisoimage -A "SOS" -V "SOS" -volset "SOS" -r -v -o SOS.ISO -eltorito-alt-boot -e EFI/BOOT/BOOTx64.EFI -no-emul-boot iso/

emul:
	qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -cdrom SOS.ISO -m 512M -net none

shell:
	qemu-system-x86_64 -bios OVMF.fd -net none -drive format=raw,file=fat:rw:drive/

setup:
	sudo apt install lld genisoimage qemu-system ovmf

.PHONY: all iso emul setup
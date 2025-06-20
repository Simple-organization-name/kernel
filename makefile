all:
	nasm -f win64 src/main.asm -o build/main.o
	lld-link -subsystem:efi_application -nodefaultlib -entry:EfiMain build/main.o -out:iso/EFI/BOOT/BOOTx64.EFI
	cp -f iso/EFI/BOOT/BOOTx64.EFI drive/BOOTx64.EFI
	genisoimage -U -A "UEFI-ISO" -V "UEFI-ISO" -volset "UEFI-ISO" -J -joliet-long -r -v -T -o output.iso -efi-boot EFI/BOOT/BOOTx64.EFI -no-emul-boot iso/
	qemu-system-x86_64 -bios OVMF.fd -net none -drive format=raw,file=fat:rw:drive/
# qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -cdrom output.iso -net none
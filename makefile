# nasm -f win64 src/main.asm -o build/main.o
# lld-link -subsystem:efi_application -nodefaultlib -entry:__start__ build/main.o -out:iso/EFI/BOOT/BOOTX64.EFI

# genisoimage -U -A "UEFI-ISO" -V "UEFI-ISO" -volset "UEFI-ISO" -J -joliet-long -r -v -T -o output.iso -efi-boot EFI/BOOT/BOOTX64.EFI -no-emul-boot iso/

# qemu-system-x86_64 -cdrom output.iso -bios /usr/share/ovmf/OVMF.fd -net none
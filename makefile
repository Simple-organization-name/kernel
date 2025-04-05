.PHONY:all

all: bootloader run

bootloader:
	nasm -f bin -o bootloader.bin bootloader.asm

run:
	qemu-system-x86_64 --drive file=bootloader.bin,format=raw

kernel_all: kernel kernel_run

kernel:
	nasm -f elf64 -o kernel.o kernel.asm
	nasm -f elf64 -o multiboot_header.o multiboot_header.asm
	ld -T linker.ld -o kernel.bin kernel.o multiboot_header.o
	cp kernel.bin iso/boot/kernel.bin
	grub-mkrescue -o sos.iso iso/

kernel_run:
	sudo qemu-system-x86_64 -enable-kvm -m 512M -cdrom sos.iso -bios /usr/share/ovmf/OVMF.fd -cpu host
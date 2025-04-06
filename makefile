.PHONY:all

all: bootloader run

bootloader:
	nasm -f bin -o bin/bootloader.bin src/bootloader.asm

run:
	qemu-system-x86_64 --drive file=bin/bootloader.bin,format=raw

kernel_all: kernel kernel_run

kernel:
	nasm -f elf32 -o build/kernel.o src/kernel.asm
	nasm -f elf32 -o build/multiboot_header.o src/multiboot_header.asm
	ld -m elf_i386 -T linker.ld -o bin/kernel.elf build/kernel.o build/multiboot_header.o
	cp bin/kernel.elf iso/boot/kernel.elf
	grub-mkrescue -o sos.iso iso/

kernel_run:
	sudo qemu-system-x86_64 -enable-kvm -m 512M -cdrom sos.iso -bios /usr/share/ovmf/OVMF.fd -cpu host
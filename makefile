.PHONY:all

all: bootloader run

bootloader:
	nasm -f bin -o bin/bootloader.bin src/bootloader.asm

run:
	qemu-system-x86_64 --drive file=bin/bootloader.bin,format=raw

kernelAll: kernel kernelRun

kernel:
	nasm -f elf32 -o build/kernel.o src/kernel.asm
	nasm -f elf32 -o build/multiboot_header.o src/multiboot_header.asm
	ld -m elf_i386 -T linker.ld -o bin/kernel.elf build/kernel.o build/multiboot_header.o
	cp bin/kernel.elf iso/boot/kernel.elf
	grub-mkrescue -o sos.iso iso/

kernelRun:
	sudo qemu-system-x86_64 -enable-kvm -m 512M -cdrom sos.iso -bios /usr/share/ovmf/OVMF.fd -cpu host

kernel64All: kernel64 kernel64Run

kernel64:
	nasm -f elf64 -o build/kernel64.o src/kernel64.asm
	nasm -f elf64 -o build/multiboot_header.o src/multiboot_header.asm
	ld -m elf_x86_64 -T linker.ld -o bin/kernel.elf build/kernel64.o build/multiboot_header.o
	cp bin/kernel.elf iso/boot/kernel.elf
	grub-mkrescue -o sos64.iso iso/

kernel64Run:
	sudo qemu-system-x86_64 -enable-kvm -m 512M -cdrom sos64.iso -bios /usr/share/ovmf/OVMF.fd -cpu host
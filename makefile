.PHONY:all
all:
	nasm -f bin bootloader.asm -o bootloader.bin
	qemu-system-x86_64 --drive file=bootloader.bin,format=raw
.PHONY:all

all: bootloader run

bootloader:
	nasm -f bin -o bootloader.bin bootloader.asm

run:
	qemu-system-x86_64 --drive file=bootloader.bin,format=raw
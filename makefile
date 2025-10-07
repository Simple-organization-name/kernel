BOOT 			= boot
BOOT_NAME		= BOOTX64.EFI
ISO_ENTRY		= EFI/BOOT/$(BOOT_NAME)

BOOT_CC 		= x86_64-w64-mingw32-gcc
BOOT_CFLAGS 	= -std=c17 -ffreestanding -fno-stack-protector -m64 -nostdlib \
				-fno-stack-check -maccumulate-outgoing-args -fpic -fshort-wchar -mno-red-zone \
				-Wall -Wextra -Werror -I include -Wl,--subsystem,10 -e EfiMain -O2 -nostartfiles

KERNEL_CC		= gcc
KERNEL_CFLAGS	= -std=c17 -ffreestanding -pie -fPIE -m64 -mno-red-zone -Wall -Wextra -Werror -nostdlib \
				-I include -nostartfiles -O2

KERNEL_ASM		= nasm
KERNEL_ASMFLAGS = -f elf64 #-w+orphan-labels -w+number-overflow -w+all -Werror -O2 -X gnu

KERNEL			= kernel

OVMF_PATH 		= OVMF.fd

KERNEL_SOURCES_ASM = $(wildcard src/kernel/*.asm)
KERNEL_OBJECTS_ASM = $(patsubst src/kernel/%.asm,build/kernel/%.o,$(KERNEL_SOURCES_ASM))

KERNEL_SOURCES_C = $(wildcard src/kernel/*.c)
KERNEL_OBJECTS_C = $(patsubst src/kernel/%.c,build/kernel/%.o,$(KERNEL_SOURCES_C))

all: iso emul

iso: clean initBootDir bootloader kernel
	@echo Building disk image...
	@xorriso -report_about WARNING -as mkisofs -iso-level 3 -o SOS.ISO -full-iso9660-filenames -volid "SOS" -eltorito-alt-boot -e $(ISO_ENTRY) -no-emul-boot -isohybrid-gpt-basdat iso/

bootloader:
	@printf "[  BOOT  ][ ASM ] Building trampoline.asm...\n"
	@nasm src/boot/trampoline.asm -o build/boot/trampoline.bin -f bin -w+orphan-labels -w+number-overflow -w+all -Werror -O2
	@objcopy build/boot/trampoline.bin build/boot/trampoline.o -I binary -O elf64-x86-64 -B i386:x86-64
	@printf "[  BOOT  ][  C  ] Building boot.c...\n"
	@$(BOOT_CC) $(BOOT_CFLAGS) src/boot/main.c build/boot/trampoline.o -o iso/$(ISO_ENTRY)

kernel: $(KERNEL_OBJECTS_ASM) $(KERNEL_OBJECTS_C)
	@printf "[ KERNEL ] Linking kernel...\n"
	@$(KERNEL_CC) $(KERNEL_OBJECTS_C) $(KERNEL_OBJECTS_ASM) -o iso/$(KERNEL).elf $(KERNEL_CFLAGS)

build/kernel/%.o: src/kernel/%.c
	@printf "[ KERNEL ][  C  ] Building $*.c...\n"
	@$(KERNEL_CC) -c $< -o $@ $(KERNEL_CFLAGS)

build/kernel/%.o: src/kernel/%.asm
	@printf "[ KERNEL ][ ASM ] Building $*.asm...\n"
	@$(KERNEL_ASM) $< -o $@ $(KERNEL_ASMFLAGS)

initBootDir:
	@cp src/files.cfg iso/EFI/BOOT/files.cfg
	@cp src/main.bmft iso/main.bmft

clean:
	@echo Cleaning build files...
	@rm -rf iso/ build/ SOS.ISO
	@echo Making clean build directories...
	@mkdir -p iso/EFI/BOOT iso/EFI/LOGS build/boot build/kernel

emul:
	@echo Starting emulation...
	@qemu-system-x86_64 -drive format=raw,file=fat:rw:iso/ -bios $(OVMF_PATH) -net none -d int 2> stderr.log

setup-ubuntu:
	sudo apt update && sudo apt upgrade
	sudo apt install xorriso qemu-system gcc-mingw-w64

setup-arch:
	sudo pacman -Syu
	sudo pacman -S xorriso qemu-desktop mingw-w64-gcc

setup-msys:
	pacman -Syu
	pacman -S xorriso mingw-w64-x86_64-qemu mingw-w64-x86_64-gcc

.PHONY: all emul setup kernel iso bootloader initBootDir

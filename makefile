INCLUDE 	= -I /usr/include/efi

all:
	gcc -c main.c -fno-stack-protector -fpic -fshort-wchar -mno-red-zone ${INCLUDE} -I /usr/include/efi/x86_64 -DEFI_FUNCTION_WRAPPER -o main.o

	ld main.o                         		\
		./x86_64/gnuefi/crt0-efi-x86_64.o 	\
		-nostdlib                      		\
		-znocombreloc                  		\
		-T elf_x86_64_efi.lds 				\
		-shared                        		\
		-Bsymbolic                     		\
		-L .								\
		-lgnuefi							\
		-lefi								\
		-o main.so

	objcopy -j .text            	    \
		-j .sdata               		\
		-j .data                		\
		-j .dynamic             		\
		-j .dynsym              		\
		-j .rel                 		\
		-j .rela                		\
		-j .reloc               		\
		--target=efi-app-x86_64 		\
		main.so                 		\
		main.efi
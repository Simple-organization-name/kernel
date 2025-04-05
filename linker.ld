ENTRY(__START__)

SECTIONS {
    . = 0x100000; /* Load kernel at 1MB */
    .multiboot2 ALIGN(8) : { *(.multiboot2) }
    .text ALIGN(4) : { *(.text) }
    .rodata ALIGN(4) : { *(.rodata) }
    .data ALIGN(4) : { *(.data) }
    .bss ALIGN(4) : { *(.bss) }
}
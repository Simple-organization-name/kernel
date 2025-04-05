ENTRY(__START__)

SECTIONS {
    . = 0x100000; /* Load kernel at 1MB */
    .text ALIGN(4) : { *(.multiboot) *(.text) }
    .rodata ALIGN(4) : { *(.rodata) }
    .data ALIGN(4) : { *(.data) }
    .bss ALIGN(4) : { *(.bss) }
}
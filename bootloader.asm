bits    16
org     0x7C00

boot_main:
    jmp boot_main

times 510-($-$$) db 0
dw 0xAA55
bits    16
org     0x7C00

main:
    jmp main


times 510-($-$$) db 0
dw 0x55AA
bits    16
org     0x7C00


main:
    mov     si,     helloWorld
    call    print
    jmp     $

helloWorld  db 'Hello World !', 0
print:
    ; Print a string
    ; Args:
    ;   si: pointer to the string to print
    mov     ah,     0x0E
    .loop:
        mov     al,     byte [si]
        cmp     al,     0
        je      .end

        int     0x10

        inc     si
        jmp     .loop

    .end:
        ret

times 510 - ($-$$) db 0
dw 0xAA55
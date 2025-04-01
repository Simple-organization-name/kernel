[org     0x7C00]      ; BIOS put us in RAM at address 0x7C00
[bits    16]          ; On first load, CPU is in 16-bit "real" mode

mov     [bootDriveId],  dl      ; Keep this, might be handy later on

realMode:
    .realMain:
        ; Init 16-bit mode temporary stack
        mov     bp,     0x7BFF
        mov     sp,     bp

        ; loads the next 63 sectors right after
        .loadBigBoot:
            mov     ah,     0x02
            mov     al,     63
            mov     ch,     0
            mov     cl,     2
            mov     dh,     0
            mov     dl,     byte [bootDriveId]
            xor     bx,     bx
            mov     es,     bx
            mov     bx,     0x7E00
            int     0x13
            ret
        
        ; Enabling A20-gate
        mov     ax,     0x2403      ; Ask if A20-gate is supported
        int     0x15
        jc      .a20NotSupported

        mov     ax,     0x2402      ; Check the status of the A20-gate
        int     0x15
        jc      .a20StatusError
        cmp     al,     1           ; Check if A20-gate us already enabled
        jmp     .a20Enabled

        mov     ax,     0x2401      ; Activate the A20-gate
        int     0x15
        jc      .a20NotToggleableByBIOS

    .a20Enabled:
        ; mov     si,     .a20EnabledMsg
        ; call    .print

        ; Switch to 32-bit "protected" mode
        cli                             ; disable interrupts
        lgdt    [gdt32.descriptor]      ; load gdt

        ; Set up Protected Mode
        mov     eax,    cr0
        or      eax,    1
        mov     cr0,    eax

        jmp     0x8:protectedMode
        hlt

    .a20NotSupported:
    .a20StatusError:
    .a20NotToggleableByBIOS:
        ; mov     si,     .a20ErrorMsg
        ; call    .print
        hlt

    ; .a20EnabledMsg db "A20 enabled", 0x0A, 0x0D, 0
    ; .a20ErrorMsg db "Error when trying to enable A20", 0x0A , 0x0D, 0

    ; .print:
    ;     ; Print a string in 16 bit mode
    ;     ; Args:
    ;     ;   si: pointer to the string to print
    ;     mov     ah,     0x0E
    ;     .print.loop:
    ;         mov     al,     byte [si]
    ;         cmp     al,     0
    ;         je      .print.end

    ;         int     0x10

    ;         inc     si
    ;         jmp     .print.loop
    ;     .print.end:
    ;         ret

    %include "gdt32.inc"


[bits    32]

protectedMode:
    .protectedMain:
        call    .clear

        mov     esi,    .testString
        xor     edi,    edi
        call    .print
        hlt

        ; switch to long mode first
        jmp     0x8:longMode.longMain

    .videoMemory equ 0xB8000
    .videoMemoryEnd equ 0xBFFFF

    .testString db 'This is a print in 32bit mode', 0

    .print:
        ; Print a string in 32 bit protected mode
        ; Args:
        ;   esi: Pointer to the string to print
        ;   edi: The offset in the video memory (1 char is 2 byte)
        mov     edx,    .videoMemory
        add     edx,    edi
        mov     ah,     0x0f            ; White char on black
        .print.loop:
            mov     al,     byte [esi]
            cmp     al,     0
            je      .print.end

            mov     [edx],  ax              ; Move the char in the char video memory
            inc     esi
            add     edx,    2               ; Seek to the next position in video memory (1 char printed is a dword ah:color, ax:char)
            jmp     .print.loop

        .print.end:
            ret

    .clear:
        ; Clear the screen
        xor     eax,    eax                 ; Set the value to store in the video memory to 0

        mov     edi,    .videoMemory        ; Set the buffer to clear to the video memory 
        mov     ecx,    .videoMemoryEnd
        sub     ecx,    edx                 ; Set ecx to the size of the buffer

        rep stosb
        ret


[bits    64]

longMode:
    .longMain:
        jmp $


bootDriveId     db  0


times   510 - ($-$$) db 0   ; Fill up rest of first sector 
dw      0xAA55              ; Boot signature to tell BIOS it can boot the disk

mbrEnd:

; rest of bootloader here

times  (512*63) - ($ - mbrEnd) db 0
[org     0x7C00]      ; BIOS put us in RAM at address 0x7C00
[bits    16]          ; On first load, CPU is in 16-bit "real" mode

mov     [bootDriveId],  dl      ; Keep this, might be handy later on

realMain:
    ; Init 16-bit mode temporary stack
    mov     bp,     0x7BFF
    mov     sp,     bp

    ; Enabling A20-gate
    mov     ax,     0x2403      ; Ask if A20-gate is supported
    int     0x15
    jc      a20NotSupported

    mov     ax,     0x2402      ; Check the status of the A20-gate
    int     0x15
    jc      a20StatusError
    cmp     al,     1
    jmp     .a20Enabled

    mov     ax,     0x2401      ; Activate the A20-gate
    int     0x15
    jc      a20NotToggleableByBIOS

realMain.a20Enabled:
    mov     si,     a20Enabled
    call    _print16

    ; Switch to 32-bit "protected" mode
    cli         ; disable interrupts
    lgdt    [gdt32.descriptor]    ; load gdt
    
    jmp     $
    ; switch to protected mode first
    jmp     0x8:protectedMain

a20NotSupported:
a20StatusError:
a20NotToggleableByBIOS:
    mov     si,     a20Error
    call    _print16
    jmp     $

a20Enabled db "A20 enabled", 0x0A, 0x0D, 0
a20Error db "Error when trying to enable A20", 0x0A , 0x0D, 0

_print16:
    ; Print a string in 16 bit mode
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

%include "gdt32.inc"


bits    32
protectedMain:

    jmp $
    ; switch to long mode first
    jmp     0x8:longMain


bits    64
longMain:
    jmp $

bootDriveId     db  0
    

times   510 - ($-$$) db 0   ; Fill up rest of first sector 
dw      0xAA55              ; Boot signature to tell BIOS it can boot the disk
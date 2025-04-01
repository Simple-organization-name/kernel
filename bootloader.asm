bits    16          ; On first load, CPU is in 16-bit "real" mode
org     0x7C00      ; And the BIOS put us in RAM at address 0x7C00


mov     [bootDriveId],  dl      ; Keep this, might be handy later on

realMain:
    ; Enabling A20-gate
    mov     ax,     0x2403 ; Ask if A20-gate is supported
    int     0x15
    jc      a20NotSupported

    mov     ax,     0x2402 ; Check the status of the A20-gate
    int     0x15
    jc      a20StatusError
    cmp     al,     1
    jmp     .a20Enabled

    mov     ax,     0x2401 ; Activate the A20-gate
    int     0x15
    jc      a20NotToggleableByBIOS

realMain.a20Enabled:
    mov     si,     a20Enabled
    call    _print16

    ; Switch to 32-bit "protected" mode
    cli         ; disable interrupts
    lgdt    [gdt.descriptor]    ; load gdt
    
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

bits    32
protectedMain:

    jmp $
    ; switch to long mode first
    jmp     0x8:longMain

bits    64
longMain:
    jmp $


bootDriveId     db  0

gdt:
.null:
    dq  0
.code:
    dw  0xffff      ; segment length bits 0-15
    dw  0x0000      ; segment base bits 0-15
    db  0x00        ; segment base bits 16-23
    db  0b10011111  ; access byte (refer to docs online)
    db  0b11001111  ; flags (bits) + segment length bits 16-19
    db  0x00        ; segment base bits 24-31
.data:
    dw  0xffff
    dw  0x0000
    db  0x00
    db  0b10010011
    db  0b11001111
    db  0x00
.end:
.descriptor:
    dw  .end - .null    ; gdt size
    dd  gdt             ; gdt address

    

times   510 - ($-$$) db 0   ; Fill up rest of first sector 
dw      0xAA55              ; Boot signature to tell BIOS it can boot the disk
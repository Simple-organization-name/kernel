bits    16          ; On first load, CPU is in 16-bit "real" mode
org     0x7C00      ; And the BIOS put us in RAM at address 0x7C00


mov     [bootDriveId],  dl      ; Keep this, might be handy later on

realMain:

    ; Switch to 32-bit "protected" mode
    cli         ; disable interrupts
    lgdt    [gdt.descriptor]    ; load gdt
    
    jmp     $
    ; switch to protected mode first
    jmp     0x8:protectedMain

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
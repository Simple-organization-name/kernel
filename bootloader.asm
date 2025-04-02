[org     0x7C00]      ; BIOS put us in RAM at address 0x7C00

[bits    16]          ; On first load, CPU is in 16-bit "real" mode
realMode:
    mov     [bootDriveId],  dl      ; Keep this, might be handy later on

    .setupStack:
        ; Init 16-bit mode temporary stack
        mov     bp,     0x7BFF
        mov     sp,     bp

    ; loads the next 63 sectors right after
    .loadBigBoot:
        mov     ah,     0x02    ; sub-function read sectors from drive
        mov     al,     63      ; how many sectors to read
        mov     ch,     0       ; nb of the cylinder from which to read
        mov     cl,     2       ; starting sector (drives are indexed from 1. cursed.)
        mov     dh,     0       ; head ? idk man just put 0
        mov     dl,     byte [bootDriveId]  ; what drive to read from, saved that at first instruction
        xor     bx,     bx      ; put 0 in bx cuz you can only move regs to es
        mov     es,     bx      ; segment of the address
        mov     bx,     0x7E00  ; offset of the address         (address es:bx is es*16 + bx)
        int     0x13            ; call interrupt
        jc      .loadBigBoot    ; cf is set on error

    .enableA20Gate:
        ; Enabling A20-gate
        mov     ax,     0x2403      ; Ask if A20-gate is supported
        int     0x15
        jc      .a20NotSupported

        mov     ax,     0x2402      ; Check the status of the A20-gate
        int     0x15
        jc      .a20StatusError
        cmp     al,     1           ; Check if A20-gate us already enabled
        jmp     .switch32Bit

        mov     ax,     0x2401      ; Activate the A20-gate
        int     0x15
        jc      .a20NotToggleableByBIOS

    .switch32Bit:
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
        hlt

    %include "gdt32.inc"


[bits    32]
protectedMode:
    .testCPUIDAvailable:
        pushfd                  ;   > Put EFLAGS in eax
        pop     eax             ; /
        mov     ecx,    eax     ; save in ecx
        xor     eax,    1 << 21 ; flip CPUID bit
        push    eax             ;   > put altered flags back
        popfd                   ; /
        pushfd                  ;   > retrieve it again
        pop     eax             ; /
        push    ecx             ;   > put back original flags 
        popfd                   ; /
        cmp     eax,    ecx     ; check if altered CPUID bit has been reset
        je      .noCPUID

    .testLongModeAvailable:
        mov     eax,    0x80000000  ; get highest cpuid call available
        cpuid
        cmp     eax,    0x80000000  ; if lower or equal to that, no long mode available
        jbe     .noLongMode

        mov     eax,    0x80000001  ; Extended processor information
        cpuid
        test    edx,    1 << 29     ; check if long mode bit is set
        jz      .noLongMode         ; if not set, well then bruh

    .switch64Bit:
        call    .clear

        mov     esi,    .testString
        xor     edi,    edi
        call    .print
        hlt

        ; switch to long mode first
        jmp     0x8:longMode.main

    .noCPUID:
    .noLongMode:
        hlt

    .videoMemory    equ 0xB8000
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
        sub     ecx,    edi                 ; Set ecx to the size of the buffer

        rep stosb
        ret


[bits    64]
longMode:
    .main:
        jmp $


bootloaderMBREnd:
    bootDriveId     db  0

    times   510 - ($-$$) db 0   ; Fill up rest of first sector 
    dw      0xAA55              ; Boot signature to tell BIOS it can boot the disk



; rest of bootloader here

times  (512*64) - ($ - $$) db 0
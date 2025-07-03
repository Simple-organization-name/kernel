[bits 64]

section .text
    %include "src/efi.inc"

    global efi_main
    efi_main:
        ; UEFI boots with the pointer to the efi system table in rdx
        mov     [ptr_efiSystemTable],   rdx

        ; Setup stack for the EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.OutputString()
        sub     rsp,    8 ; Align stack to 16 bytes
        mov     rcx,    [rdx + EFI_SYSTEM_TABLE.ConOut]
        mov     rdx,    hello
        call    [rcx + EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.OutputString]
        add     rsp,    8

        jmp     $
        ; xor     rax,    rax
        ; ret

section .bss
    ptr_efiSystemTable  resq 1

section .data
    hello dw __utf16__('Hello world !'), 0x000D, 0x000A, 0x0000

[bits 64]
%include "src/efi_struct.inc"

section .bss
    ptr_efiSystemTable  resq 1

section .data
    hello dw __utf16__ "Hello world !", 0xd, 0xa, 0

section .text
    global EfiMain
    EfiMain:
        mov     [ptr_efiSystemTable],   rdx

        ; Setup stack for the _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.OutputString()
        sub     rsp,    4*8
        mov     rcx,    [rdx + EFI_SYSTEM_TABLE.ConOut]
        mov     rdx,    hello
        call    [rcx + _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.OutputString]
        add     rsp,    4*8

        xor     rax,    rax
        ret

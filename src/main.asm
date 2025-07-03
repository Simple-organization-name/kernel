[bits 64]

section .text
global efi_main
struc EFI_TABLE_HEADER
    .Signature              resq 1
    .Revision               resd 1
    .HeaderSize             resd 1
    .CRC32                  resd 1
    .Reserved               resd 1
endstruc

struc EFI_SYSTEM_TABLE
    .Hdr                    resb EFI_TABLE_HEADER_size 
    .FirmwareVendor         resq 1
    .FirmwareRevision       resd 1
    .offset1                resd 1 ; offset because need to be aligned
    .ConsoleInHandle        resq 1
    .ConIn                  resq 1
    .ConsoleOutHandle       resq 1
    .ConOut                 resq 1 ; pointer to the _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
    .StandardErrorHandle    resq 1
    .StdErr                 resq 1
    .RuntimeServices        resq 1
    .BootServices           resq 1
    .NumberOfTableEntries   resq 1
    .ConfigurationTable     resq 1
endstruc

struc EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
    .Reset                  resq 1
    ; Args:
    ;   rcx (qword): The EFI_SYSTEM_TABLE pointer
    ;   rdx (qword): The string to print
    .OutputString           resq 1
    .TestString	            resq 1
    .QueryMode	            resq 1
    .SetMode	            resq 1
    .SetAttribute           resq 1
    .ClearScreen            resq 1
    .SetCursorPosition      resq 1
    .EnableCursor           resq 1
    .Mode                   resq 1
endstruc

efi_main:
    ; UEFI boots with the pointer to the efi system table in rdx
    mov     [ptr_efiSystemTable],   rdx

    ; Setup stack for the EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.OutputString()
    sub     rsp,    8 ; Don't know why
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

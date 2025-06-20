[bits 64]

section .bss
    ; these are just prototypes of structures, used to get the offset of a field in a struct, not to store any datas
    ; when a struct is created, NASM create a struct_name_size
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
        .ConOut                 resq 1 ; pointer to the EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
        .StandardErrorHandle    resq 1
        .StdErr                 resq 1
        .RuntimeServices        resq 1
        .BootServices           resq 1
        .NumberOfTableEntries   resq 1
        .ConfigurationTable     resq 1
    endstruc

    struc EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
        .Reset                  resq 1
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

section .data
    hello dw __utf16__ "Hello world !", 0xd, 0xa, 0

section .text
    global __start__
    __start__:
        mov     rcx,    [rdx + EFI_SYSTEM_TABLE.ConOut]

        ; Setup stack for the EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.OutputString()
        sub     rsp,    4*8
        lea     rdx,    [rel hello]
        call    [rcx + EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.OutputString]
        add     rsp,    4*8

        ret

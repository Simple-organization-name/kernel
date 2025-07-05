[bits 64]

section .bss
    efiImageHandle              resq 1  ; Handle to the EFI image
    efiSystemTable              resq 1  ; Pointer to the EFI system table

    efiLoadedImageProtocol      resq 1  ; Pointer to the EFI loaded image protocol
    efiSimpleFileSystemProtocol resq 1  ; Pointer to the EFI simple file system protocol
    efiFileProtocol             resq 1  ; Pointer to the EFI file protocol

section .data
    hello dw __utf16__("Hello world from SOS !"), 0xd, 0xa, 0

    ok dw __utf16__("OK"), 0xd, 0xa, 0

    creatingLogFile dw __utf16__("Creating log file..."), 0xd, 0xa, 0
    getEfiLoadedImageProtocolError dw __utf16__("Error getting the EFI_LOADED_IMAGE_PROTOCOL"), 0xd, 0xa, 0
    getEfiLoadedImageProtocolSuccess dw __utf16__("EFI_LOADED_IMAGE_PROTOCOL successfully retrieved"), 0xd, 0xa, 0
    getEfiSimpleFileSystemProtocolError dw __utf16__("Error getting the EFI_SIMPLE_FILE_SYSTEM_PROTOCOL"), 0xd, 0xa, 0
    getEfiSimpleFileSystemProtocolSuccess dw __utf16__("EFI_SIMPLE_FILE_SYSTEM_PROTOCOL successfully retrieved"), 0xd, 0xa, 0
    getEfiFileProtocolError dw __utf16__("Error opening the root directory"), 0xd, 0xa, 0
    getEfiFileProtocolSuccess dw __utf16__("EFI_FILE_PROTOCOL successfully retrieved"), 0xd, 0xa, 0

section .text
    align 8
    %include "efi.inc"

    global efiMain
    efiMain:
        ; UEFI boots with the pointer to the efi image handle in rcx and efi system table in rdx
        mov     [efiImageHandle],   rcx
        mov     [efiSystemTable],   rdx

        mov     rdx,    hello
        call    print

        mov     rdx,    creatingLogFile
        call    print
        call    createLogFile

        jmp     $
        ; xor     rax,    rax
        ; ret

    print:
        ; rdx: Pointer to the string to print (must be UTF-16)
        mov     rax,    [efiSystemTable]
        mov     rcx,    [rax + EFI_SYSTEM_TABLE.ConOut]
        call    [rcx + EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.OutputString]
        ret

    createLogFile:
        ; Create a log file in the EFI file system

        ; Get the EFI_LOADED_IMAGE_PROTOCOL
        mov     rcx,    [efiImageHandle]
        mov     rdx,    EFI_LOADED_IMAGE_PROTOCOL_GUID
        mov     r8,     efiLoadedImageProtocol
        mov     rax,    [efiSystemTable]
        mov     rax,    [rax + EFI_SYSTEM_TABLE.BootServices]
        call    [rax + EFI_BOOT_SERVICES.HandleProtocol]

        mov     rdx,    ok
        call    print

        cmp     rax,    0 ; Check if the protocol was successfully retrieved
        je      .gotEfiLoadedImageProtocol ; if it is zero, jmp to the next section

        ; Handle the error
        mov     rdx,    getEfiLoadedImageProtocolError
        call    print
        jmp     $

        .gotEfiLoadedImageProtocol:
        mov     rdx,    getEfiLoadedImageProtocolSuccess
        call    print

        ; Get the SIMPLE_FILE_SYSTEM_PROTOCOL
        mov     rcx,    [efiLoadedImageProtocol]
        mov     rcx,    [rcx + EFI_LOADED_IMAGE_PROTOCOL.DeviceHandle] ; Get the device handle
        mov     rdx,    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID
        mov     r8,     efiSimpleFileSystemProtocol
        mov     rax,    [efiSystemTable]
        mov     rax,    [rax + EFI_SYSTEM_TABLE.BootServices]
        call    [rax + EFI_BOOT_SERVICES.HandleProtocol]

        cmp     rax,    0 ; Check if the protocol was successfully retrieved
        je      .gotEfiSimpleFileSystemProtocol ; if it is zero, jmp to the next section

        ; Handle the error
        mov     rdx,    getEfiSimpleFileSystemProtocolError
        call    print
        jmp     $

        .gotEfiSimpleFileSystemProtocol:
        mov     rdx,    getEfiSimpleFileSystemProtocolSuccess
        call    print

        ; Open the root directory of the file system
        mov     rcx,    [efiSimpleFileSystemProtocol]
        mov     rdx,    efiFileProtocol
        mov     rax,    [efiSystemTable]
        call    [rax + EFI_SIMPLE_FILE_SYSTEM_PROTOCOL.OpenVolume]

        cmp     rax,    0 ; Check if the root directory was successfully opened
        je      .gotEfiFileProtocol ; if it is zero, jmp to the next section

        ; Handle the error
        mov     rdx,    getEfiFileProtocolError
        call    print
        jmp     $

        .gotEfiFileProtocol:
        mov     rdx,    getEfiFileProtocolSuccess
        call    print

        ; Create a new file named "log.txt"

        jmp     $

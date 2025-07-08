[bits 64]

section .bss
    temp                        resq 1
    efiImageHandle              resq 1  ; Handle to the EFI image
    efiSystemTable              resq 1  ; Pointer to the EFI system table

    efiLoadedImageProtocol      resq 1  ; Pointer to the EFI loaded image protocol
    efiSimpleFileSystemProtocol resq 1  ; Pointer to the EFI simple file system protocol
    efiRootFileProtocol         resq 1  ; Pointer to the EFI file protocol for root
    efiLogFileProtocol          resq 1  ; Pointer to the EFI file protocol for BOOT.LOG

section .data
    hello dw __utf16__("Hello world from SOS !"), 0xd, 0xa, 0

    logFileName dw __utf16__("BOOT.LOG"), 0

    creatingLogFile dw __utf16__("Creating log file..."), 0xd, 0xa, 0
    getEfiLoadedImageProtocolError dw __utf16__("Error getting the EFI_LOADED_IMAGE_PROTOCOL"), 0xd, 0xa, 0
    getEfiLoadedImageProtocolSuccess dw __utf16__("EFI_LOADED_IMAGE_PROTOCOL successfully retrieved"), 0xd, 0xa, 0
    getEfiSimpleFileSystemProtocolError dw __utf16__("Error getting the EFI_SIMPLE_FILE_SYSTEM_PROTOCOL"), 0xd, 0xa, 0
    getEfiSimpleFileSystemProtocolSuccess dw __utf16__("EFI_SIMPLE_FILE_SYSTEM_PROTOCOL successfully retrieved"), 0xd, 0xa, 0
    getEfiRootFileProtocolError dw __utf16__("Error opening the root directory"), 0xd, 0xa, 0
    getEfiRootFileProtocolSuccess dw __utf16__("root EFI_FILE_PROTOCOL successfully retrieved"), 0xd, 0xa, 0
    getEfiLogFileProtocolError dw __utf16__("Error opening the log file"), 0xd, 0xa, 0
    getEfiLogFileProtocolSuccess dw __utf16__("log EFI_FILE_PROTOCOL successfully retrieved"), 0xd, 0xa, 0


section .text
    align 8
    %include "efi.inc"

    global efiMain
    efiMain:
        ; UEFI boots with the pointer to the efi image handle in rcx and efi system table in rdx
        mov     [efiImageHandle],   rcx
        mov     [efiSystemTable],   rdx

        lea     rdx,    [rel hello]
        call    print

        lea     rdx,    [rel creatingLogFile]
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
        lea     rdx,    [rel EFI_LOADED_IMAGE_PROTOCOL_GUID]
        lea     r8,     [rel efiLoadedImageProtocol]
        mov     rax,    [efiSystemTable]
        mov     rax,    [rax + EFI_SYSTEM_TABLE.BootServices]
        call    [rax + EFI_BOOT_SERVICES.HandleProtocol]

        cmp     rax,    0 ; Check if the protocol was successfully retrieved
        je      .gotEfiLoadedImageProtocol ; if it is zero, jmp to the next section

        ; Handle the error
        lea     rdx,    [rel getEfiLoadedImageProtocolError]
        call    print
        jmp     $

        .gotEfiLoadedImageProtocol:
        lea     rdx,    [rel getEfiLoadedImageProtocolSuccess]
        call    print

        ; Get the SIMPLE_FILE_SYSTEM_PROTOCOL
        mov     rcx,    [efiLoadedImageProtocol]
        mov     rcx,    [rcx + EFI_LOADED_IMAGE_PROTOCOL.DeviceHandle] ; Get the device handle
        lea     rdx,    [rel EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID]
        lea     r8,     [rel efiSimpleFileSystemProtocol]
        mov     rax,    [efiSystemTable]
        mov     rax,    [rax + EFI_SYSTEM_TABLE.BootServices]
        call    [rax + EFI_BOOT_SERVICES.HandleProtocol]

        cmp     rax,    0 ; Check if the protocol was successfully retrieved
        je      .gotEfiSimpleFileSystemProtocol ; if it is zero, jmp to the next section

        ; Handle the error
        lea     rdx,    [rel getEfiSimpleFileSystemProtocolError]
        call    print
        jmp     $

        .gotEfiSimpleFileSystemProtocol:
        lea     rdx,    [rel getEfiSimpleFileSystemProtocolSuccess]
        call    print

        ; Open the root directory of the file system
        mov     rcx,    [efiSimpleFileSystemProtocol]
        lea     rdx,    [rel efiRootFileProtocol]
        mov     rax,    [efiSimpleFileSystemProtocol]
        call    [rax + EFI_SIMPLE_FILE_SYSTEM_PROTOCOL.OpenVolume]

        cmp     rax,    0 ; Check if the root directory was successfully opened
        je      .gotEfiFileProtocol ; if it is zero, jmp to the next section

        ; Handle the error
        lea     rdx,    [rel getEfiRootFileProtocolError]
        call    print
        jmp     $

        .gotEfiFileProtocol:
        lea     rdx,    [rel getEfiRootFileProtocolSuccess]
        call    print

        ; Create BOOT.LOG file
        mov     rcx,    [efiRootFileProtocol]
        lea     rdx,    [rel efiLogFileProtocol]
        mov     r8,     [rel logFileName]
        mov     r9,     EFI_FILE_MODE_CREATE
        push    qword   0
        mov     rax,    [efiRootFileProtocol]
        call    [rax + EFI_FILE_PROTOCOL.Open]
        pop     r8

        cmp     rax,    0 ; Check if the log file was successfully opened
        je      .gotEfiLogFileProtocol ; if it is zero, jmp to
        ; Handle the error
        lea     rdx,    [rel getEfiLogFileProtocolError]
        call    print
        jmp     $

        .gotEfiLogFileProtocol:
        lea     rdx,    [rel getEfiLogFileProtocolSuccess]
        call    print

        ret


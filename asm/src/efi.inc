; Include file for EFI

EFI_LOADED_IMAGE_PROTOCOL_GUID:
    dd 0x5B1B31A1
    dw 0x9562
    dw 0x11d2
    db 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B

EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID:
    dd 0x964e5b22
    dw 0x6459
    dw 0x11d2
    db 0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b

EFI_FILE_MODE_READ      dq 0x0000000000000001
EFI_FILE_MODE_WRITE     dq 0x0000000000000003
EFI_FILE_MODE_CREATE    dq 0x8000000000000003

struc EFI_TABLE_HEADER
    .Signature              resq 1
    .Revision               resd 1
    .HeaderSize             resd 1
    .CRC32                  resd 1
    .Reserved               resd 1
endstruc

struc EFI_SYSTEM_TABLE
    .Hdr                    resb EFI_TABLE_HEADER_size ; 3x 8 bytes
    .FirmwareVendor         resq 1
    .FirmwareRevision       resd 1
    .pad0                   resd 1 ; padding
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

struc EFI_BOOT_SERVICES
    .Hdr                                resb EFI_TABLE_HEADER_size ; 3x 8 bytes
    .RaiseTPL                           resq 1
    .RestoreTPL                         resq 1
    .AllocatePages                      resq 1
    .FreePages                          resq 1
    .GetMemoryMap                       resq 1
    .AllocatePool                       resq 1
    .FreePool                           resq 1
    .CreateEvent                        resq 1
    .SetTimer                           resq 1
    .WaitForEvent                       resq 1
    .SignalEvent                        resq 1
    .CloseEvent                         resq 1
    .CheckEvent                         resq 1
    .InstallProtocolInterface           resq 1
    .ReinstallProtocolInterface         resq 1
    .UninstallProtocolInterface         resq 1
    .HandleProtocol                     resq 1
    .Reserved                           resq 1
    .RegisterProtocolNotify             resq 1
    .LocateHandle                       resq 1
    .LocateDevicePath                   resq 1
    .InstallConfigurationTable          resq 1
    .LoadImage                          resq 1
    .StartImage                         resq 1
    .Exit                               resq 1
    .UnloadImage                        resq 1
    .ExitBootServices                   resq 1
    .GetNextMonotonicCount              resq 1
    .Stall                              resq 1
    .SetWatchdogTimer                   resq 1
    .ConnectController                  resq 1
    .DisconnectController               resq 1
    .OpenProtocol                       resq 1
    .CloseProtocol                      resq 1
    .OpenProtocolInformation            resq 1
    .ProtocolsPerHandle                 resq 1
    .LocateHandleBuffer                 resq 1
    .LocateProtocol                     resq 1
    .InstallMultipleProtocolInterfaces  resq 1
    .UninstallMultipleProtocolInterfaces resq 1
    .CalculateCrc32                     resq 1
    .CopyMem                            resq 1
    .SetMem                             resq 1
    .CreateEventEx                      resq 1
endstruc

struc EFI_LOADED_IMAGE_PROTOCOL
    .Revision           resd 1      ; UINT32
    .pad0               resd 1      ; Padding for alignment (to 8 bytes)
    .ParentHandle       resq 1      ; EFI_HANDLE
    .SystemTable        resq 1      ; EFI_SYSTEM_TABLE *
    .DeviceHandle       resq 1      ; EFI_HANDLE
    .FilePath           resq 1      ; EFI_DEVICE_PATH_PROTOCOL *
    .Reserved           resq 1      ; VOID *
    .LoadOptionsSize    resd 1      ; UINT32
    .pad1               resd 1      ; Padding for alignment (to 8 bytes)
    .LoadOptions        resq 1      ; VOID *
    .ImageBase          resq 1      ; VOID *
    .ImageSize          resq 1      ; UINT64
    .ImageCodeType      resq 1      ; EFI_MEMORY_TYPE
    .ImageDataType      resq 1      ; EFI_MEMORY_TYPE
    .Unload             resq 1      ; EFI_IMAGE_UNLOAD
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

struc EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
    .Revision       resq 1  ; UINT64 Revision
    .OpenVolume     resq 1  ; EFI_SIMPLE_FILE_SYSTEM_PROTOCOL.OpenVolume function pointer
endstruc

struc EFI_FILE_PROTOCOL
    .Revision           resq 1  ; UINT64
    .Open               resq 1  ; EFI_FILE_OPEN
    .Close              resq 1  ; EFI_FILE_CLOSE
    .Delete             resq 1  ; EFI_FILE_DELETE
    .Read               resq 1  ; EFI_FILE_READ
    .Write              resq 1  ; EFI_FILE_WRITE
    .GetPosition        resq 1  ; EFI_FILE_GET_POSITION
    .SetPosition        resq 1  ; EFI_FILE_SET_POSITION
    .GetInfo            resq 1  ; EFI_FILE_GET_INFO
    .SetInfo            resq 1  ; EFI_FILE_SET_INFO
    .Flush              resq 1  ; EFI_FILE_FLUSH
    ; UEFI 2.0+ extensions:
    .OpenEx             resq 1  ; EFI_FILE_OPEN_EX
    .ReadEx             resq 1  ; EFI_FILE_READ_EX
    .WriteEx            resq 1  ; EFI_FILE_WRITE_EX
    .FlushEx            resq 1  ; EFI_FILE_FLUSH_EX
endstruc
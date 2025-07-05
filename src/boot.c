#include <efi/efi.h>

EFI_FILE_PROTOCOL *logFile;

uint64_t createLogFile(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable);

EFI_STATUS EFIAPI EfiMain(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable) {
    (void)imageHandle;
    SIMPLE_TEXT_OUTPUT_INTERFACE* ConOut = systemTable->ConOut;
    ConOut->ClearScreen(systemTable->ConOut);

    

    ConOut->SetAttribute(ConOut, EFI_GREEN);
    ConOut->OutputString(ConOut, u"Booting up SOS !...\r\n");
    ConOut->SetAttribute(ConOut, EFI_WHITE);

    ConOut->OutputString(ConOut, u"Creating log file...\r\n");

    uint64_t err;
    err = createLogFile(imageHandle, systemTable);

    if (err != 0) ConOut->OutputString(ConOut, u"Failed to create log file\r\n");
    else ConOut->OutputString(ConOut, u"Log file successfully created !\r\n");

    while (1)
    {
    }
    return EFI_SUCCESS;
}

uint64_t createLogFile(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable) {
    EFI_BOOT_SERVICES *bootServices = systemTable->BootServices;

    EFI_STATUS status;
    EFI_GUID guid;

    EFI_LOADED_IMAGE_PROTOCOL *loadedImage;
    guid = (EFI_GUID)EFI_LOADED_IMAGE_PROTOCOL_GUID;
    status = bootServices->HandleProtocol(imageHandle, &guid, (void**)&loadedImage);
    if (status != EFI_SUCCESS) {
        systemTable->ConOut->OutputString(systemTable->ConOut, u"Error: Could not retrieve EFI_LOADED_IMAGE_PROTOCOL\r\n");
        return 1;
    }

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
    guid = (EFI_GUID)EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    status = bootServices->HandleProtocol(loadedImage->DeviceHandle, &guid, (void**)&fs);
    if (status != EFI_SUCCESS) {
        systemTable->ConOut->OutputString(systemTable->ConOut, u"Error: Could not retrieve EFI_SIMPLE_FILE_SYSTEM_PROTOCOL\r\n");
        return 1;
    }

    EFI_FILE_PROTOCOL *vol;
    status = fs->OpenVolume(fs, &vol);
    if (status != EFI_SUCCESS) {
        systemTable->ConOut->OutputString(systemTable->ConOut, u"Error: Could not open volume\r\n");
        return 1;
    }

    static const CHAR16 logName[] = u"log.txt";
    UINT64 mode = (UINT64)(EFI_FILE_MODE_CREATE|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_READ);
    status = vol->Open(vol, &logFile, logName, mode, 0);
    if (status != EFI_SUCCESS) {
        systemTable->ConOut->OutputString(systemTable->ConOut, u"Error: Could not create file \"log.txt\"\r\n");
        return 1;
    }
    
    CHAR8 msg[] = "Hello from SOS !";
    UINTN bufferSize = sizeof(msg);
    status = logFile->Write(logFile, &bufferSize, msg);
    if (status != EFI_SUCCESS) {
        systemTable->ConOut->OutputString(systemTable->ConOut, u"Error: Could not write in log file\r\n");
        return 1;
    }

    return 0;
}
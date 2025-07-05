#include <efi/efi.h>

EFI_FILE_PROTOCOL *root = NULL;
EFI_FILE_PROTOCOL *logFile = NULL;

EFI_STATUS EFIAPI openRootDir(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable);
EFI_STATUS EFIAPI createLogFile(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable);
EFI_STATUS EFIAPI intToString(UINTN number, CHAR16 *buffer, UINTN bufferSize);

static inline void EFIAPI Error(SIMPLE_TEXT_OUTPUT_INTERFACE *ConOut, EFI_STATUS status, CHAR16 *msg) {
    ConOut->OutputString(ConOut, u"Error: ");
    ConOut->OutputString(ConOut, msg);
    CHAR16 buffer[20];
    if (intToString(status, buffer, 20) == EFI_SUCCESS) {
        ConOut->OutputString(ConOut, u" (");
        ConOut->OutputString(ConOut, buffer);
        ConOut->OutputString(ConOut, u")\r\n");
    } else ConOut->OutputString(ConOut, u"(N/A)\r\n");
}

EFI_STATUS EFIAPI EfiMain(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable) {
    (void)imageHandle;
    SIMPLE_TEXT_OUTPUT_INTERFACE* ConOut = systemTable->ConOut;
    ConOut->ClearScreen(systemTable->ConOut);

    ConOut->SetAttribute(ConOut, EFI_GREEN);
    ConOut->OutputString(ConOut, u"Booting up SOS !...\r\n");
    ConOut->SetAttribute(ConOut, EFI_WHITE);

    EFI_STATUS status;
    
    status = openRootDir(imageHandle, systemTable);
    if (status != EFI_SUCCESS) {
        Error(ConOut, status, u"Failed to open root dir");
        while (1) {}
    }
    
    ConOut->OutputString(ConOut, u"Creating log file...\r\n");
    status = createLogFile(imageHandle, systemTable);
    
    if (status != 0) ConOut->OutputString(ConOut, u"Failed to create log file\r\n");
    else ConOut->OutputString(ConOut, u"Log file successfully created !\r\n");

    while (1)
    {
    }
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI openRootDir(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable) {
    EFI_BOOT_SERVICES *bootServices = systemTable->BootServices;

    EFI_STATUS status;

    EFI_LOADED_IMAGE_PROTOCOL *loadedImage;
    status = bootServices->HandleProtocol(imageHandle, &(EFI_GUID)EFI_LOADED_IMAGE_PROTOCOL_GUID, (void**)&loadedImage);
    if (status != EFI_SUCCESS) {
        Error(systemTable->ConOut, status, u"Could not retrieve EFI_LOADED_IMAGE_PROTOCOL");
        return status;
    }

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
    status = bootServices->HandleProtocol(loadedImage->DeviceHandle, &(EFI_GUID)EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID, (void**)&fs);
    if (status != EFI_SUCCESS) {
        Error(systemTable->ConOut, status, u"Could not retrieve EFI_SIMPLE_FILE_SYSTEM_PROTOCOL");
        return status;
    }

    EFI_FILE_PROTOCOL *vol;
    status = fs->OpenVolume(fs, &vol);
    if (status != EFI_SUCCESS) {
        Error(systemTable->ConOut, status, u"Could not open volume");
        return status;
    }
    
    status = vol->Open(vol, &root, u"\\", (UINT64)EFI_FILE_MODE_READ, 0);
    if (status != EFI_SUCCESS) {
        Error(systemTable->ConOut, status, u"Could not open root directory");
        return status;
    }

    if (root == NULL) {
        Error(systemTable->ConOut, status, u"Could not open root directory");
        return status;
    }

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI createLogFile(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable) {
    EFI_STATUS status;

    CHAR16 *logName = u"\\boot.log";
    status = root->Open(root, &logFile, logName, (UINT64)EFI_FILE_MODE_CREATE|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_READ, 0);
    if (status != EFI_SUCCESS) {
        Error(systemTable->ConOut, status, u"Could not create file \"boot.log\"");
        return status;
    }

    CHAR8 msg[] = u8"Hello from SOS !";
    UINTN bufferSize = sizeof(msg);
    status = logFile->Write(logFile, &bufferSize, msg);
    if (status != EFI_SUCCESS) {
        Error(systemTable->ConOut, status, u"Could not write in log file");
        return status;
    }

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI intToString(UINTN number, CHAR16 *buffer, UINTN bufferSize) {
    // Get the number of digits
    UINT8 digits = 0;
    UINTN temp = number;
    while (temp != 0) {
        digits++;
        temp /= 10;
    }

    // Check if the buffer is large enough
    if (digits + 1 > bufferSize) {
        return EFI_BUFFER_TOO_SMALL;
    }

    // Convert the number to string
    buffer[digits] = '\0'; // Null-terminate the string
    for (UINT8 i = digits; i > 0; i--) {
        buffer[i - 1] = (CHAR16)((number % 10) + '0'); // Convert digit to character
        number /= 10; // Remove the last digit
    }

    return EFI_SUCCESS;
}
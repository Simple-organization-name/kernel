#include <efi/efi.h>

#undef EFI_FILE_MODE_READ
#undef EFI_FILE_MODE_WRITE
#undef EFI_FILE_MODE_CREATE
#define EFI_FILE_MODE_READ      (UINT64)0x0000000000000001ULL
#define EFI_FILE_MODE_WRITE     (UINT64)0x0000000000000003ULL
#define EFI_FILE_MODE_CREATE    (UINT64)0x8000000000000003ULL

#define EfiPrint(stream, msg) stream->OutputString(stream, msg)

EFI_STATUS EFIAPI openRootDir(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable, EFI_FILE_PROTOCOL **root);
EFI_STATUS EFIAPI createLogFile(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable, EFI_FILE_PROTOCOL *root, EFI_FILE_PROTOCOL **logFile);
EFI_STATUS EFIAPI intToString(UINT64 number, CHAR16 *buffer, UINT64 bufferSize);

/**
 * \brief Prints an error message
 * \param ConOut The console output of UEFI
 * \param status The status to print (EFI_STATUS)
 * \param msg The message to print with the status code (must be a utf16 string)
 */
static inline void EFIAPI Error(SIMPLE_TEXT_OUTPUT_INTERFACE *ConOut, EFI_STATUS status, CHAR16 *msg) {
    EfiPrint(ConOut, u"Error: ");
    EfiPrint(ConOut, msg);
    CHAR16 buffer[20];
    if (intToString(status, buffer, 20) == EFI_SUCCESS) {
        EfiPrint(ConOut, u" (");
        EfiPrint(ConOut, buffer);
        EfiPrint(ConOut, u")\r\n");
    } else EfiPrint(ConOut, u"(N/A)\r\n");
}

/**
 * \brief UEFI entry point
 */
EFI_STATUS EFIAPI EfiMain(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable) {
    (void)imageHandle;
    SIMPLE_TEXT_OUTPUT_INTERFACE* ConOut = systemTable->ConOut;
    ConOut->ClearScreen(systemTable->ConOut);

    ConOut->SetAttribute(ConOut, EFI_GREEN);
    EfiPrint(ConOut, u"Booting up SOS !...\r\n");
    ConOut->SetAttribute(ConOut, EFI_WHITE);

    EFI_STATUS status;
    EFI_FILE_PROTOCOL *root = NULL, *logFile = NULL;
    
    status = openRootDir(imageHandle, systemTable, &root);
    if (status != EFI_SUCCESS) {
        Error(ConOut, status, u"Failed to open root dir");
        while (1) {}
    }

    EfiPrint(ConOut, u"Creating log file...\r\n");
    status = createLogFile(imageHandle, systemTable, root, &logFile);

    if (status != 0) EfiPrint(ConOut, u"Failed to create log file\r\n");
    else EfiPrint(ConOut, u"Log file successfully created !\r\n");

    while (1) {}
    return EFI_SUCCESS;
}

/**
 * \brief Opens the root directory
 * \param root Where to store the pointer of the EFI_FILE_PROTOCOL of the root directory
 */
EFI_STATUS EFIAPI openRootDir(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable, EFI_FILE_PROTOCOL **root) {
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

    status = fs->OpenVolume(fs, root);
    if (status != EFI_SUCCESS) {
        Error(systemTable->ConOut, status, u"Could not open root");
        return status;
    }

    if (!*root) {
        Error(systemTable->ConOut, status, u"Could not open root");
        return status;
    }

    return EFI_SUCCESS;
}

/**
 * \brief Creates a LOG directory at the root and a BOOT.LOG file
 * \param root The pointer to the EFI_FILE_PROTOCOL of the root directory
 * \param logFile Where to store the pointer of the EFI_FILE_PROTOCOL of the log file BOOT.LOG
 */
EFI_STATUS EFIAPI createLogFile(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable, EFI_FILE_PROTOCOL *root, EFI_FILE_PROTOCOL **logFile) {
    EFI_STATUS status;

    CHAR16 *logName = (CHAR16 *)L"\\BOOT.LOG";

    // status = root->Open(root, logFile, logName, EFI_FILE_MODE_WRITE, 0);
    // if (status == EFI_SUCCESS)
    //     status = (*logFile)->Delete(*logFile);
    // else
    //     Error(systemTable->ConOut, status, u"File already deleted or non-existant");

    // if (status != EFI_SUCCESS)
    //     Error(systemTable->ConOut, status, u"Unable to remove \"BOOT.LOG\"");

    status = root->Open(root, logFile, logName, EFI_FILE_MODE_CREATE, 0);
    if (status != EFI_SUCCESS) {
        Error(systemTable->ConOut, status, u"Could not create file \"BOOT.LOG\"");
        return status;
    }

    CHAR16 *msg = (CHAR16 *)L"Hello from SOS !\r\n\r\n";
    UINT64 bufferSize = sizeof(msg) - 1;
    status = (*logFile)->Write(*logFile, &bufferSize, msg);
    if (status != EFI_SUCCESS) {
        Error(systemTable->ConOut, status, u"Could not write in log file");
        return status;
    }
    
    if (!*logFile) {
        Error(systemTable->ConOut, status, u"Failed to create log file\r\n");
        return status;
    } 

    return EFI_SUCCESS;
}

/**
 * \brief Converts a UINT64 (UINT64 in x64) into it string version
 * \param number The number to convert
 * \param buffer The buffer to store the string (must be a utf16 string)
 * \param bufferSize The size of the buffer
 */
EFI_STATUS EFIAPI intToString(UINT64 number, CHAR16 *buffer, UINT64 bufferSize) {
    // Get the number of digits
    UINT8 digits = 0;
    UINT64 temp = number;
    while (temp != 0) {
        digits++;
        temp /= 10;
    }

    // Check if the buffer is large enough
    if (digits + 1 > bufferSize) {
        return (UINT64)EFI_BUFFER_TOO_SMALL;
    }

    // Convert the number to string
    buffer[digits] = '\0'; // Null-terminate the string
    for (UINT8 i = digits; i > 0; i--) {
        buffer[i - 1] = (CHAR16)((number % 10) + '0'); // Convert digit to character
        number /= 10; // Remove the last digit
    }

    return EFI_SUCCESS;
}
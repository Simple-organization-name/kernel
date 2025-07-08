#ifdef __UINT64_C
#   undef __UINT64_C
#   undef __INT64_C
#endif

#include <efi/efi.h>

#undef EFI_FILE_MODE_READ
#undef EFI_FILE_MODE_WRITE
#undef EFI_FILE_MODE_CREATE
#define EFI_FILE_MODE_READ      (UINT64)0x0000000000000001ULL
#define EFI_FILE_MODE_WRITE     (UINT64)0x0000000000000003ULL
#define EFI_FILE_MODE_CREATE    (UINT64)0x8000000000000003ULL

#define EfiPrint(msg) systemTable->ConOut->OutputString(systemTable->ConOut, msg)

static EFI_HANDLE           imageHandle;
static EFI_SYSTEM_TABLE     *systemTable;

static inline void EfiPrintError(EFI_STATUS status, CHAR16 *msg);
static inline void EfiPrintInfo(CHAR16 *msg, UINT16 attr);
EFI_STATUS openRootDir(EFI_FILE_PROTOCOL **root);
EFI_STATUS createLogFile(EFI_FILE_PROTOCOL *root, EFI_FILE_PROTOCOL **logFile);
EFI_STATUS intToString(UINT64 number, CHAR16 *buffer, UINT64 bufferSize);
EFI_STATUS graphicsMode();

/**
 * \brief UEFI entry point
 */
EFI_STATUS EfiMain(EFI_HANDLE _imageHandle, EFI_SYSTEM_TABLE *_systemTable) {
    imageHandle = _imageHandle;
    systemTable = _systemTable;

    SIMPLE_TEXT_OUTPUT_INTERFACE* ConOut = _systemTable->ConOut;
    ConOut->ClearScreen(ConOut);

    EfiPrintInfo(u"Booting up SOS !...\r\n", EFI_GREEN);

    EFI_STATUS status;
    EFI_FILE_PROTOCOL *root = NULL, *logFile = NULL;

    // Open root
    EfiPrint(u"Getting root directory...\r\n");
    status = openRootDir(&root);
    if (status != EFI_SUCCESS) {
        EfiPrintInfo(u"Failed to open root dir\r\n", EFI_MAGENTA);
        while (1) {}
    } else EfiPrintInfo(u"Successfully opened root directory !\r\n", EFI_CYAN);

    // Try to create log file
    EfiPrint(u"Creating log file...\r\n");
    status = createLogFile(root, &logFile);
    if (status != EFI_SUCCESS) EfiPrintInfo(u"Failed to create log file\r\n", EFI_MAGENTA);
    else EfiPrintInfo(u"Log file successfully created !\r\n", EFI_CYAN);

    // Initialize graphics mode
    EfiPrint(u"Initializing graphics mode...\r\n");
    status = graphicsMode();
    if (status != EFI_SUCCESS) EfiPrintInfo(u"Failed to initialize graphics mode\r\n", EFI_MAGENTA);
    else EfiPrintInfo(u"Graphics mode successfully initialized !\r\n", EFI_CYAN);

    while (1) {}
    return EFI_SUCCESS;
}

/**
 * \brief Prints an error message in red
 * \param status The status to print (EFI_STATUS)
 * \param msg The message to print with the status code (must be a utf16 string)
 */
static inline void EfiPrintError(EFI_STATUS status, CHAR16 *msg) {
    systemTable->ConOut->SetAttribute(systemTable->ConOut, EFI_RED);
    EfiPrint(u"Error: ");
    EfiPrint(msg);
    CHAR16 buffer[21];
    if (intToString(status, buffer, 21) == EFI_SUCCESS) {
        EfiPrint(u" (");
        EfiPrint(buffer);
        EfiPrint(u")\r\n");
    } else EfiPrint(u"(N/A)\r\n");
    systemTable->ConOut->SetAttribute(systemTable->ConOut, EFI_WHITE);
}

/**
 * \brief Prints an info message in cyan
 * \param msg The message to print
 */
static inline void EfiPrintInfo(CHAR16 *msg, UINT16 attr) {
    systemTable->ConOut->SetAttribute(systemTable->ConOut, attr);
    EfiPrint(msg);
    systemTable->ConOut->SetAttribute(systemTable->ConOut, EFI_WHITE|EFI_BACKGROUND_BLACK);
}

/**
 * \brief Opens the root directory
 * \param root Where to store the pointer of the EFI_FILE_PROTOCOL of the root directory
 */
EFI_STATUS openRootDir(EFI_FILE_PROTOCOL **root) {
    EFI_BOOT_SERVICES *bootServices = systemTable->BootServices;

    EFI_STATUS status;

    EFI_LOADED_IMAGE_PROTOCOL *loadedImage;
    status = bootServices->HandleProtocol(imageHandle, &(EFI_GUID)EFI_LOADED_IMAGE_PROTOCOL_GUID, (void**)&loadedImage);
    if (status != EFI_SUCCESS) {
        EfiPrintError(status, u"Could not retrieve EFI_LOADED_IMAGE_PROTOCOL");
        return status;
    }

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
    status = bootServices->HandleProtocol(loadedImage->DeviceHandle, &(EFI_GUID)EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID, (void**)&fs);
    if (status != EFI_SUCCESS) {
        EfiPrintError(status, u"Could not retrieve EFI_SIMPLE_FILE_SYSTEM_PROTOCOL");
        return status;
    }

    status = fs->OpenVolume(fs, root);
    if (status != EFI_SUCCESS || !*root) {
        EfiPrintError(status, u"Could not open root");
        return status;
    }

    EFI_FILE_SYSTEM_INFO fileInfo;
    UINT64 bufferSize = sizeof(fileInfo);
    status = (*root)->GetInfo(*root, &(EFI_GUID)EFI_FILE_SYSTEM_INFO_ID, &bufferSize, (void *)&fileInfo);
    if (status != EFI_SUCCESS) {
        EfiPrintError(status, u"Could not get root info");
        return status;
    }

    if (fileInfo.ReadOnly) EfiPrintInfo(u"Read-only filesystem\r\n", EFI_YELLOW);
    else EfiPrint(u"Read/Write filesystem\r\n");

    return EFI_SUCCESS;
}

/**
 * \brief Creates a BOOT.LOG file
 * \param root The pointer to the EFI_FILE_PROTOCOL of the root directory
 * \param logFile Where to store the pointer of the EFI_FILE_PROTOCOL of the log file BOOT.LOG
 */
EFI_STATUS createLogFile(EFI_FILE_PROTOCOL *root, EFI_FILE_PROTOCOL **logFile) {
    EFI_STATUS status;

    EFI_FILE_PROTOCOL *logDir; 
    status = root->Open(root, &logDir, u"\\EFI\\LOGS\\", EFI_FILE_MODE_CREATE, EFI_FILE_DIRECTORY);
    if (status != EFI_SUCCESS) {
        EfiPrintError(status, u"Could not open \"LOGS\" directory");
        return status;
    }

    status = logDir->Open(logDir, logFile, u"BOOT.LOG", EFI_FILE_MODE_WRITE, 0);
    if (status != EFI_SUCCESS|| !*logFile) {
        EfiPrintError(status, u"Could not create file \"BOOT.LOG\"");
        return status;
    }

    CHAR8 msg[] = u8"Hello from SOS !\r\n\r\n";
    UINT64 bufferSize = sizeof(msg) - 1;
    status = (*logFile)->Write(*logFile, &bufferSize, msg);
    if (status != EFI_SUCCESS) {
        EfiPrintError(status, u"Could not write in log file");
        return status;
    }

    return EFI_SUCCESS;
}

/**
 * Setup the graphics mode
 */
EFI_STATUS graphicsMode() {
    EFI_BOOT_SERVICES *bs = systemTable->BootServices;
    EFI_STATUS status;

    EFI_GRAPHICS_OUTPUT_PROTOCOL *graphicsProtocol;
    status = bs->LocateProtocol(&(EFI_GUID)EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID, NULL, (void **)&graphicsProtocol);
    if (status != EFI_SUCCESS) {
        EfiPrintError(status, u"Failed to retrieve EFI_GRAPHICS_OUTPUT_PROTOCOL");
        return status;
    }

    EfiPrint(u"Graphics modes:\r\n");
    UINT32 maxMode = graphicsProtocol->Mode->MaxMode;
    CHAR16 maxModeStr[11];
    intToString(maxMode, maxModeStr, sizeof(maxModeStr));
    for (UINT32 modeIndex = 0; modeIndex < maxMode; modeIndex++) {
        EFI_STATUS _status;
        UINT64 infoSize = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info = NULL;

        _status = graphicsProtocol->QueryMode(graphicsProtocol, modeIndex, &infoSize, &info);
        EfiPrint(u"a ");

        CHAR16 modeIndexStr[11];
        intToString((UINT64)(modeIndex + 1), modeIndexStr, sizeof(modeIndexStr));
        EfiPrint(u"Mode ");
        EfiPrint(modeIndexStr);
        EfiPrint(u"/");
        EfiPrint(maxModeStr);
        EfiPrint(u": ");

        if (_status == EFI_SUCCESS) {
            CHAR16 width[11], height[11];   
            intToString((UINT64)info->HorizontalResolution, width, sizeof(width));
            intToString((UINT64)info->VerticalResolution, height, sizeof(height));

            EfiPrint(width);
            EfiPrint(u"x");
            EfiPrint(height);
            EfiPrint(u"\r\n");
        } else {
            EfiPrint(u"Invalid !\r\n");
        }
    }

    return EFI_SUCCESS;
}

/**
 * \brief Converts a UINT64 (UINT64 in x64) into it string version
 * \param number The number to convert
 * \param buffer The buffer to store the string (must be a utf16 string)
 * \param bufferSize The size of the buffer
 */
EFI_STATUS intToString(UINT64 number, CHAR16 *buffer, UINT64 bufferSize) {
    if (number == 0) {
        if (bufferSize < 2)
            return (EFI_STATUS)EFI_BUFFER_TOO_SMALL;
        buffer[0] = u'0';
        buffer[1] = u'\0';
        return EFI_SUCCESS;
    }

    // Get the number of digits
    UINT8 digits = 0;
    UINT64 temp = number;
    while (temp != 0) {
        digits++;
        temp /= 10;
    }

    // Check if the buffer is large enough
    if ((UINT64)(digits + 1) > bufferSize)
        return (EFI_STATUS)EFI_BUFFER_TOO_SMALL;

    // Convert the number to string
    buffer[digits] = '\0'; // Null-terminate the string
    for (UINT8 i = digits; i > 0; i--) {
        buffer[i - 1] = (CHAR16)((number % 10) + '0'); // Convert digit to character
        number /= 10; // Remove the last digit
    }

    return EFI_SUCCESS;
}

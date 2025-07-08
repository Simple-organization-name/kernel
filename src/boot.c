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
#define statusToString(status, buffer, bufferSize) intToString(status^(1ULL<<63), buffer, bufferSize)


static EFI_HANDLE           imageHandle;
static EFI_SYSTEM_TABLE     *systemTable;
static EFI_FILE_PROTOCOL    *root = NULL,
                            *logFile = NULL;
static struct {
    UINT64 curMode;
    UINT64 cols, rows;
} TextModeInfo;


static inline void EfiPrintError(EFI_STATUS status, CHAR16 *msg);
static inline void EfiPrintAttr(CHAR16 *msg, UINT16 attr);
static inline EFI_STATUS intToString(UINT64 number, CHAR16 *buffer, UINT64 bufferSize);
static inline EFI_INPUT_KEY getKey();

static EFI_STATUS changeTextModeRes();
static EFI_STATUS openRootDir();
static EFI_STATUS createLogFile();
static EFI_STATUS setupGraphicsMode();


/**
 * \brief UEFI entry point
 */
EFI_STATUS EfiMain(EFI_HANDLE _imageHandle, EFI_SYSTEM_TABLE *_systemTable) {
    imageHandle = _imageHandle;
    systemTable = _systemTable;

    EFI_STATUS status;

    SIMPLE_TEXT_OUTPUT_INTERFACE* ConOut = _systemTable->ConOut;
    ConOut->ClearScreen(ConOut);

    EfiPrintAttr(u"Booting up SOS !...\r\n", EFI_GREEN);

    EfiPrint(u"Changing text mode to maximum supported resolution...\r\n");
    status = changeTextModeRes();
    if (EFI_ERROR(status)) EfiPrintAttr(u"Failed to set text mode to maximum resolution\r\n", EFI_YELLOW);
    else EfiPrintAttr(u"Successfully changed text mode to maximum resolution !\r\n", EFI_CYAN);

    // Open root
    EfiPrint(u"Getting root directory...\r\n");
    status = openRootDir(&root);
    if (EFI_ERROR(status)) {
        EfiPrintAttr(u"Failed to open root dir\r\n", EFI_MAGENTA);
        while (1) {}
    } else EfiPrintAttr(u"Successfully opened root directory !\r\n", EFI_CYAN);

    // Try to create log file
    EfiPrint(u"Creating log file...\r\n");
    status = createLogFile(root, &logFile);
    if (EFI_ERROR(status)) EfiPrintAttr(u"Failed to create log file\r\n", EFI_MAGENTA);
    else EfiPrintAttr(u"Log file successfully created !\r\n", EFI_CYAN);

    // Initialize graphics mode
    EfiPrint(u"Initializing graphics mode...\r\n");
    status = setupGraphicsMode();
    if (EFI_ERROR(status)) EfiPrintAttr(u"Failed to initialize graphics mode\r\n", EFI_MAGENTA);
    else EfiPrintAttr(u"Graphics mode successfully initialized !\r\n", EFI_CYAN);

    // at boot services exit must close log file !!!!
    logFile->Close(logFile);

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
    if (statusToString(status, buffer, 21) == EFI_SUCCESS) {
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
static inline void EfiPrintAttr(CHAR16 *msg, UINT16 attr) {
    systemTable->ConOut->SetAttribute(systemTable->ConOut, attr);
    EfiPrint(msg);
    systemTable->ConOut->SetAttribute(systemTable->ConOut, EFI_WHITE|EFI_BACKGROUND_BLACK);
}

/**
 * \brief Converts a UINT64 (UINT64 in x64) into it string version
 * \param number The number to convert
 * \param buffer The buffer to store the string (must be a utf16 string)
 * \param bufferSize The size of the buffer
 */
inline EFI_STATUS intToString(UINT64 number, CHAR16 *buffer, UINT64 bufferSize) {
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

/**
 * \brief Wait for a key press
 * \return The pressed key
 */
EFI_INPUT_KEY getKey() {
    EFI_EVENT ev[1] = {systemTable->ConIn->WaitForKey};
    EFI_INPUT_KEY key = {0};
    UINT64 index = 0;

    systemTable->BootServices->WaitForEvent(1, ev, &index);
    systemTable->ConIn->ReadKeyStroke(systemTable->ConIn, &key);
    return key;
}

/**
 * \brief Set the text mode to the max res
 * \return Returns the number of columns and rows for the current text mode in the global TextModeInfo struct, if success.
 */
EFI_STATUS changeTextModeRes() {
    EFI_STATUS status;
    EFI_SIMPLE_TEXT_OUT_PROTOCOL *prot = systemTable->ConOut;

    UINT64 maxMode = (UINT64)(prot->Mode->MaxMode - 1);

    status = prot->SetMode(prot, maxMode);
    TextModeInfo.curMode = maxMode;
    if (EFI_ERROR(status)) {
        EfiPrintError(status, u"Failed to set text mode to max resolution");
        return status;
    }

    status = prot->QueryMode(prot, maxMode, &TextModeInfo.cols, &TextModeInfo.rows);
    if (EFI_ERROR(status)) {
        EfiPrintError(status, u"Failed to retrieve text mode informations");
        return status;
    }

    return EFI_SUCCESS;
}

/**
 * \brief Opens the root directory
 * \param root Where to store the pointer of the EFI_FILE_PROTOCOL of the root directory
 */
EFI_STATUS openRootDir() {
    EFI_BOOT_SERVICES *bootServices = systemTable->BootServices;

    EFI_STATUS status;

    EFI_LOADED_IMAGE_PROTOCOL *loadedImage;
    status = bootServices->HandleProtocol(imageHandle, &(EFI_GUID)EFI_LOADED_IMAGE_PROTOCOL_GUID, (void**)&loadedImage);
    if (EFI_ERROR(status)) {
        EfiPrintError(status, u"Could not retrieve EFI_LOADED_IMAGE_PROTOCOL");
        return status;
    }

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
    status = bootServices->HandleProtocol(loadedImage->DeviceHandle, &(EFI_GUID)EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID, (void**)&fs);
    if (EFI_ERROR(status)) {
        EfiPrintError(status, u"Could not retrieve EFI_SIMPLE_FILE_SYSTEM_PROTOCOL");
        return status;
    }

    status = fs->OpenVolume(fs, &root);
    if (EFI_ERROR(status) || !root) {
        EfiPrintError(status, u"Could not open root");
        return status;
    }

    EFI_FILE_SYSTEM_INFO fileInfo; 
    UINT64 bufferSize = sizeof(fileInfo) + SIZE_OF_EFI_FILE_SYSTEM_INFO;
    status = root->GetInfo(root, &(EFI_GUID)EFI_FILE_SYSTEM_INFO_ID, &bufferSize, (void *)&fileInfo);
    if (EFI_ERROR(status)) {
        EfiPrintError(status, u"Could not get root info");
        return status;
    }

    if (fileInfo.ReadOnly) EfiPrintAttr(u"Read-only filesystem\r\n", EFI_YELLOW);
    else EfiPrint(u"Read/Write filesystem\r\n");

    return EFI_SUCCESS;
}

/**
 * \brief Creates a BOOT.LOG file
 */
EFI_STATUS createLogFile() {
    EFI_STATUS status;

    EFI_FILE_PROTOCOL *logDir; 
    status = root->Open(root, &logDir, u"\\EFI\\LOGS\\", EFI_FILE_MODE_CREATE, EFI_FILE_DIRECTORY);
    if (EFI_ERROR(status)) {
        EfiPrintError(status, u"Could not open \"LOGS\" directory");
        return status;
    }

    status = logDir->Open(logDir, &logFile, u"BOOT.LOG", EFI_FILE_MODE_CREATE, 0);
    if (EFI_ERROR(status)|| !logFile) {
        EfiPrintError(status, u"Could not create file \"BOOT.LOG\"");
        return status;
    }

    CHAR8 msg[] = u8"Hello from SOS !\r\n";
    UINT64 bufferSize = sizeof(msg) - 1;
    status = logFile->Write(logFile, &bufferSize, msg);
    if (EFI_ERROR(status)) {
        EfiPrintError(status, u"Could not write in log file");
        return status;
    }

    return EFI_SUCCESS;
}

/**
 * \brief Setup the graphics mode
 */
EFI_STATUS setupGraphicsMode() {
    EFI_BOOT_SERVICES *bs = systemTable->BootServices;
    EFI_STATUS status;

    EFI_GRAPHICS_OUTPUT_PROTOCOL *graphicsProtocol;
    status = bs->LocateProtocol(&(EFI_GUID)EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID, NULL, (void **)&graphicsProtocol);
    if (EFI_ERROR(status)) {
        EfiPrintError(status, u"Failed to retrieve EFI_GRAPHICS_OUTPUT_PROTOCOL");
        return status;
    }

    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *protInfo = graphicsProtocol->Mode;
    UINT32 currentIndex = protInfo->Mode, maxMode = protInfo->MaxMode;

    UINT32 curSize;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *cur;
    status = graphicsProtocol->QueryMode(graphicsProtocol, currentIndex, &curSize, &cur);
    if (EFI_ERROR(status)) {
        EfiPrintError(status, u"Failed to retrieve current graphics mode information");
        return status;
    }

    CHAR16 wStr[21], hStr[21];
    intToString(cur->HorizontalResolution, wStr, sizeof(wStr));
    intToString(cur->VerticalResolution, hStr, sizeof(hStr));
    EfiPrint(u"Current graphics resolution: ");
    EfiPrint(wStr); EfiPrint(u"x"); EfiPrint(hStr);
    EfiPrint(u"\r\n");
    EfiPrint(u"> Do you want to change the current resolution ? (y/n)\r\n");

    EFI_INPUT_KEY key = getKey();
    while (key.UnicodeChar != u'y' && key.UnicodeChar != u'n') {
        key = getKey();
    }

    if (key.UnicodeChar == u'y') {
        if (maxMode <= 1) {
            EfiPrint(u"No other resolution available for graphics mode\r\n");
            return EFI_SUCCESS;
        }

        UINT32  cursorIndex = 0,
                viewStart = 0, viewEnd = TextModeInfo.rows;

        BOOLEAN done = FALSE;

        systemTable->ConOut->EnableCursor(systemTable->ConOut, TRUE);
        systemTable->ConOut->SetCursorPosition(systemTable->ConOut, 0, 0);
        while (!done) {
            systemTable->ConOut->ClearScreen(systemTable->ConOut->ClearScreen);

            // Show all the res between view
            for (UINT32 i = viewStart; i < viewEnd && i < maxMode; i++) {
                UINT64 infoSize;
                EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
                graphicsProtocol->QueryMode(graphicsProtocol, i, &infoSize, &info);
            }
        }
    }

    return EFI_SUCCESS;
}

#ifdef __UINT64_C
#   undef __UINT64_C
#   undef __INT64_C
#endif

#include "boot.h"
#include "efi/efi.h"

// Macros
#undef EFI_FILE_MODE_READ
#undef EFI_FILE_MODE_WRITE
#undef EFI_FILE_MODE_CREATE
#define EFI_FILE_MODE_READ      (UINT64)0x0000000000000001ULL
#define EFI_FILE_MODE_WRITE     (UINT64)0x0000000000000003ULL
#define EFI_FILE_MODE_CREATE    (UINT64)0x8000000000000003ULL

#define _EfiPrint(msg) systemTable->ConOut->OutputString(systemTable->ConOut, msg)
#define statusToString(status, buffer, bufferSize) intToString(status^(1ULL<<63), buffer, bufferSize)

// Global variables passed to kernel
Framebuffer framebuffer = {0};
MemMap memmap = {0};

// UEFI preboot functions and global variables
// declared as static to be specific to this file
static EFI_HANDLE           imageHandle;
static EFI_SYSTEM_TABLE     *systemTable;
static EFI_FILE_PROTOCOL    *root = NULL,
                            *logFile = NULL;

static inline uint64_t ucs2ToUtf8(uint16_t *in, uint8_t *out, uint64_t outSize);

static inline EFI_STATUS EfiPrint(CHAR16 *msg);
static inline void EfiPrintError(EFI_STATUS status, CHAR16 *msg);
static inline void EfiPrintAttr(CHAR16 *msg, UINT16 attr);
static inline EFI_STATUS intToString(UINT64 number, CHAR16 *buffer, UINT64 bufferSize);
static inline EFI_INPUT_KEY getKey();

static inline void changeTextModeRes();
static EFI_STATUS setupGraphicsMode();
static EFI_STATUS openRootDir();
static EFI_STATUS createLogFile();
static void exitBootServices();


/**
 * \brief UEFI entry point
 */
EFI_STATUS EfiMain(EFI_HANDLE _imageHandle, EFI_SYSTEM_TABLE *_systemTable) {
    imageHandle = _imageHandle;
    systemTable = _systemTable;

    EFI_STATUS status;

    SIMPLE_TEXT_OUTPUT_INTERFACE* ConOut = _systemTable->ConOut;
    ConOut->ClearScreen(ConOut);

    // Change text mode to max resolution
    changeTextModeRes();

    EfiPrintAttr(u"Booting up SOS !...\r\n", EFI_GREEN);

    // Initialize graphics mode
    EfiPrint(u"Initializing graphics mode...\r\n");
    status = setupGraphicsMode();
    if (EFI_ERROR(status)) EfiPrintAttr(u"Failed to initialize graphics mode\r\n", EFI_MAGENTA);
    else EfiPrintAttr(u"Graphics mode successfully initialized !\r\n", EFI_CYAN);

    // Open root
    EfiPrint(u"Getting root directory...\r\n");
    status = openRootDir(&root);
    if (EFI_ERROR(status)) {
        EfiPrintAttr(u"Failed to open root dir\r\n", EFI_MAGENTA);
        while (1);
    } else EfiPrintAttr(u"Successfully opened root directory !\r\n", EFI_CYAN);

    // Create log file
    EfiPrint(u"Creating log file...\r\n");
    status = createLogFile(root, &logFile);
    if (EFI_ERROR(status)) EfiPrintAttr(u"Failed to create log file\r\n", EFI_MAGENTA);
    else EfiPrintAttr(u"Log file successfully created !\r\n", EFI_CYAN);

    // Get memory map
    // Once the memory map is retrieved, BootServices->ExitBootServices should be called right after it
    EfiPrint(u"Exiting boot services...\r\n");
    exitBootServices();

    uint32_t *fb = (uint32_t *)framebuffer.addr;
    for (uint64_t i = 0; i < framebuffer.size / sizeof(uint32_t); i++) {
        fb[i] = 0xFFFFFFFF;
    }

    while (1);
    return EFI_ABORTED; // Should never be reached
}

/**
 * \brief Converts a UCS-2 string to UTF-8
 * \param in The UCS-2 string to convert
 * \param out The buffer to store the UTF-8 output string
 * \param outSize The size of the out buffer
 * \return The number of bytes written
 */
static inline uint64_t ucs2ToUtf8(uint16_t *in, uint8_t *out, uint64_t outSize) {
    uint64_t i = 0, j = 0;
    while (in[i]) {
        uint16_t wc = in[i++];
        if (wc <= 0x007F) {
            if (j + 2 >= outSize) break;
            out[j++] = (uint8_t)(wc & 0x7F);
        } else if (wc <= 0x07FF) {
            if (j + 3 >= outSize) break;
            out[j++] = (uint8_t)(0xC0 | ((wc >> 6) & 0x1F));
            out[j++] = (uint8_t)(0x80 | (wc & 0x3F));
        } else {
            if (j + 4 >= outSize) break;
            out[j++] = (uint8_t)(0xE0 | ((wc >> 12) & 0x0F));
            out[j++] = (uint8_t)(0x80 | ((wc >> 6) & 0x3F));
            out[j++] = (uint8_t)(0x80 | (wc & 0x3F));
        }
    }

    out[j] = '\0';
    return j;
}

/**
 * \brief Prints a message in the console and log file (if initialized)
 * \param msg The message to print
 */
static inline EFI_STATUS EfiPrint(CHAR16 *msg) {
    EFI_STATUS status;
    
    status = systemTable->ConOut->OutputString(systemTable->ConOut, msg);
    if (EFI_ERROR(status)) return status;

    if (logFile) {
        CHAR8 buf[256] = {0};
        UINT64 size = ucs2ToUtf8(msg, buf, sizeof(buf));

        status = logFile->Write(logFile, &size, buf);
        if (EFI_ERROR(status)) return status;
    }

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
 * \brief Prints an info message with text attribute
 * \param msg The message to print
 * \param attr The EFI attributes
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
static inline EFI_STATUS intToString(UINT64 number, CHAR16 *buffer, UINT64 bufferSize) {
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
static EFI_INPUT_KEY getKey() {
    EFI_EVENT ev[1] = {systemTable->ConIn->WaitForKey};
    EFI_INPUT_KEY key = {0};
    UINT64 index = 0;

    systemTable->BootServices->WaitForEvent(1, ev, &index);
    systemTable->ConIn->ReadKeyStroke(systemTable->ConIn, &key);
    return key;
}

/**
 * \brief Opens the root directory
 * \param root Where to store the pointer of the EFI_FILE_PROTOCOL of the root directory
 */
static EFI_STATUS openRootDir() {
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
static EFI_STATUS createLogFile() {
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
 * \brief Set the text mode to the max res
 */
static inline void changeTextModeRes() {
    EFI_SIMPLE_TEXT_OUT_PROTOCOL *cout = systemTable->ConOut;

    UINT64 maxMode = (UINT64)(cout->Mode->MaxMode - 1);

    cout->SetMode(cout, maxMode);
}

#define CHECK_ERROR(call) if ((INTN)(status = call) < 0) { EfiPrintError(status, u ## #call); return status; }
#define CHECK_ERROR_MSG(call, msg) if ((INTN)(status = call) < 0) { EfiPrintError(status, msg); return status; }

static EFI_STATUS printGfxMode(EFI_GRAPHICS_OUTPUT_PROTOCOL *gfx, UINT32 index) {
    UINTN sizeofInfo;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
    EFI_STATUS status = EFI_SUCCESS;
    CHECK_ERROR(gfx->QueryMode(gfx, index, &sizeofInfo, &info));
    CHAR16 wStr[11] = {0}, hStr[11] = {0};
    CHECK_ERROR(intToString(info->HorizontalResolution, wStr, sizeof(wStr)));
    CHECK_ERROR(intToString(info->VerticalResolution, hStr, sizeof(hStr)));
    CHECK_ERROR(EfiPrint(wStr));
    CHECK_ERROR(EfiPrint(u" x "));
    CHECK_ERROR(EfiPrint(hStr));
    CHECK_ERROR(EfiPrint(u"         "));

    return status;
}

/**
 * \brief Setup the graphics mode
 */
static EFI_STATUS setupGraphicsMode() {
    EFI_BOOT_SERVICES *bs = systemTable->BootServices;
    EFI_STATUS status;

    EFI_GRAPHICS_OUTPUT_PROTOCOL *graphicsProtocol;
    status = bs->LocateProtocol(&(EFI_GUID)EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID, NULL, (void **)&graphicsProtocol);
    if (EFI_ERROR(status)) {
        EfiPrintError(status, u"Failed to retrieve EFI_GRAPHICS_OUTPUT_PROTOCOL");
        return status;
    }

    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *protInfo = graphicsProtocol->Mode;
    UINT32 maxMode = protInfo->MaxMode;

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *cur = protInfo->Info;

    CHAR16 wStr[21], hStr[21];
    intToString(cur->HorizontalResolution, wStr, sizeof(wStr));
    intToString(cur->VerticalResolution, hStr, sizeof(hStr));
    EfiPrint(u"Current graphics resolution: ");
    EfiPrint(wStr); EfiPrint(u"x"); EfiPrint(hStr);
    EfiPrint(u"\r\n");
    EfiPrint(u"> Do you want to change the current resolution ? (y/n)\r\n");

    EFI_INPUT_KEY key = getKey();
    while (key.UnicodeChar != u'y' && key.UnicodeChar != u'n') 
        key = getKey();
    
    if (key.UnicodeChar == u'y') {
        if (maxMode <= 1) {
            EfiPrint(u"No other resolution available for graphics mode\r\n");
        } else {
            EfiPrintAttr(u"\r\nSelect with \u2191 and \u2193 | Confirm with ENTER\r\n\n", EFI_WHITE);
            SIMPLE_TEXT_OUTPUT_INTERFACE *cout = systemTable->ConOut;
            INT32 cursorInitial = cout->Mode->CursorRow;

            BOOLEAN done = FALSE;
            UINT32 mode = 0;
            while (!done) {
                // first, show
                cout->SetCursorPosition(cout, 0, cursorInitial);
                cout->SetAttribute(cout, EFI_DARKGRAY);
                EfiPrint(u"  ");
                printGfxMode(graphicsProtocol, (mode - 2 + maxMode) % maxMode);
                cout->SetAttribute(cout, EFI_LIGHTGRAY);
                EfiPrint(u"\r\n  ");
                printGfxMode(graphicsProtocol, (mode - 1 + maxMode) % maxMode);
                cout->SetAttribute(cout, EFI_WHITE);
                EfiPrint(u"\r\n> ");
                printGfxMode(graphicsProtocol, (mode) % maxMode);
                cout->SetAttribute(cout, EFI_LIGHTGRAY);
                EfiPrint(u"\r\n  ");
                printGfxMode(graphicsProtocol, (mode + 1) % maxMode);
                cout->SetAttribute(cout, EFI_DARKGRAY);
                EfiPrint(u"\r\n  ");
                printGfxMode(graphicsProtocol, (mode + 2) % maxMode);

                // then, update
                key = getKey();
                if (key.ScanCode == 0x01) {
                    mode = (mode - 1 + maxMode) % maxMode;
                } else if (key.ScanCode == 0x02) {
                    mode = (mode + 1) % maxMode;
                } else if (key.UnicodeChar == u'\r') {
                    graphicsProtocol->SetMode(graphicsProtocol, mode);
                    done = TRUE;
                }
            }
        }
    }

    systemTable->ConOut->ClearScreen(systemTable->ConOut);
    systemTable->ConOut->SetCursorPosition(systemTable->ConOut, 0, 0);

    framebuffer.addr = graphicsProtocol->Mode->FrameBufferBase;
    framebuffer.size = graphicsProtocol->Mode->FrameBufferSize;
    framebuffer.width = graphicsProtocol->Mode->Info->HorizontalResolution;
    framebuffer.height = graphicsProtocol->Mode->Info->VerticalResolution;

    return EFI_SUCCESS;
}

#undef CHECK_ERROR
#undef CHECK_ERROR_MSG

/**
 * Prints the memory map
 */
static inline void printMemoryMap() {
    CHAR16 buffer[21]; 
    intToString(memmap.mapSize, buffer, sizeof(buffer));
    EfiPrint(u"Size of memmap: ");
    EfiPrint(buffer);

    intToString(memmap.descSize, buffer, sizeof(buffer));
    EfiPrint(u"\r\nSize of descriptor: ");
    EfiPrint(buffer);

    intToString(memmap.count, buffer, sizeof(buffer));
    EfiPrint(u"\r\nDescriptor count: ");
    EfiPrint(buffer);

    for (UINT16 i = 0; i < memmap.count; i++) {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((char *)memmap.map + i * memmap.descSize);

        intToString(i, buffer, sizeof(buffer));
        EfiPrint(u"\r\n\r\nDescriptor ");
        EfiPrint(buffer);

        intToString(desc->Type, buffer, sizeof(buffer));
        EfiPrint(u"\r\n    Type: ");
        EfiPrint(buffer);

        intToString(desc->PhysicalStart, buffer, sizeof(buffer));
        EfiPrint(u"\r\n    Address: ");
        EfiPrint(buffer);

        intToString(desc->NumberOfPages, buffer, sizeof(buffer));
        EfiPrint(u"\r\n    Number of pages: ");
        EfiPrint(buffer);
    }

    EfiPrint(u"\r\n");
}

/**
 * \brief Exit boot services
 * \return Returns the memory map in the memmap global variable
 * \note Log file is closed
 */
static void exitBootServices() {
    // close log now to not modify memmap later on
    {
        // at boot services exit must close log file !!!!
        EFI_STATUS status = logFile->Close(logFile);
        if (EFI_ERROR(status)) EfiPrintError(status, u"Failed to close log file !");
        else {
            logFile = NULL;
            EfiPrintAttr(u"Log file closed\r\n", EFI_CYAN);
        }
    }

    EFI_BOOT_SERVICES *bs = systemTable->BootServices;

    UINT64 dummy = 0;
    EFI_MEMORY_DESCRIPTOR dummy2;

    UINT32 descriptorVersion;

    EFI_STATUS status = bs->GetMemoryMap(&dummy, &dummy2, &memmap.key, &memmap.descSize, &descriptorVersion);
    if (EFI_ERROR(status) && status != EFI_BUFFER_TOO_SMALL) {
        EfiPrintError(status, u"Failed to get memory map information !");
        while (1);
    }
    
    status = bs->AllocatePool(EfiLoaderData, memmap.mapSize = (dummy * 2), (void **)&memmap.map);
    if (EFI_ERROR(status)) {
        EfiPrintError(status, u"Failed to allocate memory for memmap");
        while (1);
    }

    status = bs->GetMemoryMap(&memmap.mapSize, (EFI_MEMORY_DESCRIPTOR *)memmap.map, &memmap.key, &memmap.descSize, &descriptorVersion);
    if (EFI_ERROR(status)) {
        EfiPrintError(status, u"Failed to get memory map !");
        while (1);
    }

    // must not print before ExitBootServices, else it might modify the memory map and we won't have the latest one.

    memmap.count = memmap.mapSize / memmap.descSize;
    if (memmap.count * memmap.descSize != memmap.mapSize) {
        // EfiPrintAttr(u"Descriptor count * Descriptor Size != Map Size", EFI_YELLOW);
    }
    
    // printMemoryMap();

    // EfiPrint(u"Exiting boot services ...\r\n");
    status = systemTable->BootServices->ExitBootServices(imageHandle, memmap.key);
    if (EFI_ERROR(status)) {
        EfiPrintError(status, u"Failed to exit boot services !");
        while (1);;
    }

}

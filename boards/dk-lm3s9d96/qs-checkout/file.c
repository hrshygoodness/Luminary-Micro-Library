//*****************************************************************************
//
// file.c - Functions related to file access for the qs-checkout application.
//
// Copyright (c) 2008-2012 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 8555 of the DK-LM3S9D96 Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "usblib/usblib.h"
#include "utils/fswrapper.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "httpserver_raw/fs.h"
#include "httpserver_raw/fsdata.h"
#include "fatfs/src/ff.h"
#include "fatfs/src/diskio.h"
#include "drivers/ssiflash.h"
#include "qs-checkout.h"
#include "file.h"
#include "drivers/extram.h"
#include "drivers/set_pinout.h"
#include "drivers/extflash.h"

//*****************************************************************************
//
// This label defines the address at which a file system in external memory
// can be found.
//
//*****************************************************************************
unsigned char *g_pcExternalFileSystem;

//*****************************************************************************
//
// This macro returns a character pointer based on a file system node pointer
// and an offset.
//
//*****************************************************************************
#define FSPTR(ptNode, ulOffset) ((unsigned char *)ptNode + \
                                 (unsigned long)ulOffset)

//*****************************************************************************
//
// Include the position-dependent file system image for the default,
// internal file system.  This file is built using the following command from
// the qs-checkout directory:
//
//     makefsfile -i int_html -o qsfs_data.h
//
//*****************************************************************************
#include "qsfs_data.h"

//*****************************************************************************
//
// The following are data structures used by FatFs.
//
//*****************************************************************************
static FATFS g_sFatFs[2];
static DIR g_sDirObject;
static FIL g_sFile;
static FILINFO g_sFileInfo;

//*****************************************************************************
//
// A global indicating whether or not we have initialized an external memory
// file system image.
//
//*****************************************************************************
static tBoolean g_bInitialized;

//*****************************************************************************
//
// This array describes the various file system mount points.  These are passed
// to the fswrapper module which allows us to use helpful URLs and filenames
// to access the various file systems installed via a single namespace.
//
//*****************************************************************************
static fs_mount_data g_psMountData[] =
{
    {"sdcard",   0,       0, 0, 0}, // SDCard - FAT logical drive 0
    {"usb",      0,       1, 0, 0}, // USB flash stick - FAT logical drive 1

    //
    // The following entry MUST be the second last element in the list.  Add
    // any other fixed mount points above this point.
    //
    {"ram",      (unsigned char *)0, 0, 0, 0}, // RAM-based file system image.
    {NULL,       (unsigned char *)FS_ROOT, 0, 0, 0}  // Default root directory
};

#define NUM_FS_MOUNT_POINTS (sizeof(g_psMountData) / sizeof(fs_mount_data))

#define MOUNT_INDEX_SDCARD  0
#define MOUNT_INDEX_USB     1
#define MOUNT_INDEX_RAM     (NUM_FS_MOUNT_POINTS - 2)
#define MOUNT_INDEX_DEFAULT (NUM_FS_MOUNT_POINTS - 1)

unsigned long g_ulNumMountPoints;


//*****************************************************************************
//
// Defines the size of the buffers that hold the path, or temporary
// data from the SD card or USB flash stick.  There are two buffers allocated
// of this size.  The buffer size must be large enough to hold the longest
// expected full path name, including the file name, and a trailing null
// character.
//
//*****************************************************************************
#define PATH_BUF_SIZE   80

//*****************************************************************************
//
// Error reasons returned by ChangeToDirectory().
//
//*****************************************************************************
#define NO_ERROR            0
#define NAME_TOO_LONG_ERROR 1
#define OPENDIR_ERROR       2

//*****************************************************************************
//
// This buffer holds the full path to the current working directory.
// Initially it is root ("/").
//
//*****************************************************************************
static char g_cCwdBuf[PATH_BUF_SIZE] = "/";
static char g_cCwdMapped[PATH_BUF_SIZE] = "/";

//*****************************************************************************
//
// A temporary data buffer used when manipulating file paths, or reading data
// from the SD card or USB flash stick.
//
//*****************************************************************************
static char g_cTmpBuf[PATH_BUF_SIZE];

//*****************************************************************************
//
// The instance data for the MSC driver.
//
//*****************************************************************************
unsigned long g_ulMSCInstance = 0;

//*****************************************************************************
//
// Maximum length of a string generated using the FilePrintf function.
//
//*****************************************************************************
#define MAX_PRINTF_STRING_LEN   80

//*****************************************************************************
//
// Size of the buffer used by FileCatToUART.
//
//*****************************************************************************
#define READ_BUFFER_SIZE        64

//*****************************************************************************
//
// A structure that holds a mapping between an FRESULT numerical code,
// and a string represenation.  FRESULT codes are returned from the FatFs
// FAT file system driver.
//
//*****************************************************************************
typedef struct
{
    FRESULT fresult;
    char *pcResultStr;
}
tFresultString;

//*****************************************************************************
//
// A macro to make it easy to add result codes to the table.
//
//*****************************************************************************
#define FRESULT_ENTRY(f)        { (f), (#f) }

//*****************************************************************************
//
// A table that holds a mapping between the numerical FRESULT code and
// it's name as a string.  This is used for looking up error codes for
// printing to the console.
//
//*****************************************************************************
static tFresultString g_sFresultStrings[] =
{
    FRESULT_ENTRY(FR_OK),
    FRESULT_ENTRY(FR_NOT_READY),
    FRESULT_ENTRY(FR_NO_FILE),
    FRESULT_ENTRY(FR_NO_PATH),
    FRESULT_ENTRY(FR_INVALID_NAME),
    FRESULT_ENTRY(FR_INVALID_DRIVE),
    FRESULT_ENTRY(FR_DENIED),
    FRESULT_ENTRY(FR_EXIST),
    FRESULT_ENTRY(FR_RW_ERROR),
    FRESULT_ENTRY(FR_WRITE_PROTECTED),
    FRESULT_ENTRY(FR_NOT_ENABLED),
    FRESULT_ENTRY(FR_NO_FILESYSTEM),
    FRESULT_ENTRY(FR_INVALID_OBJECT),
    FRESULT_ENTRY(FR_MKFS_ABORTED)
};

//*****************************************************************************
//
// A macro that holds the number of result codes.
//
//*****************************************************************************
#define NUM_FRESULT_CODES       (sizeof(g_sFresultStrings) / \
                                 sizeof(tFresultString))

//*****************************************************************************
//
// This function returns a string representation of an error code
// that was returned from a function call to FatFs.  It can be used
// for printing human readable error messages.
//
//*****************************************************************************
static const char *
StringFromFresult(FRESULT fresult)
{
    unsigned int uIdx;

    //
    // Enter a loop to search the error code table for a matching
    // error code.
    //
    for(uIdx = 0; uIdx < NUM_FRESULT_CODES; uIdx++)
    {
        //
        // If a match is found, then return the string name of the
        // error code.
        //
        if(g_sFresultStrings[uIdx].fresult == fresult)
        {
            return(g_sFresultStrings[uIdx].pcResultStr);
        }
    }

    //
    // At this point no matching code was found, so return a
    // string indicating unknown error.
    //
    return("UNKNOWN ERROR CODE");
}

//*****************************************************************************
//
// Initialize the external memory file system image.
//
// This function determines if a file system image exists in the external
// memory and, if so, sets it up such that it is accessible by the web server.
//
// \return Returns \b true on success or \b false on failure (no file system
// image was present or we are unable to relocate it from the serial EEPROM).
//
//*****************************************************************************
static tBoolean
FileExternalImageInit(void)
{
    unsigned long ulSize, ulRead;

    //
    // Are we already initialized?
    //
    if(!g_bInitialized)
    {
        //
        // Determine whether a file system image exists in external memory and,
        // if so, determine the size of the image.
        //
        ulSize = FileExternalImageSizeGet();

        //
        // Was an image found?
        //
        if(ulSize)
        {
            //
            // If no other daughter board is detected, assume that SDRAM board
            // is available and attempt to copy the external file system image
            // from SSI flash to SDRAM.
            //
            if(g_eDaughterType == DAUGHTER_NONE)
            {
                //
                // There is a file system image present so we need to copy it to
                // SDRAM and fix up the mount point table to point to the new
                // image.  First, allocate some storage for the image.
                //
                g_pcExternalFileSystem = ExtRAMAlloc(ulSize);

                if(g_pcExternalFileSystem)
                {
                    //
                    // We got the memory so now read the file system image from the
                    // serial flash into the newly allocated buffer.
                    //
                    ulRead = SSIFlashRead(0, ulSize, g_pcExternalFileSystem);
                    if(ulRead != ulSize)
                    {
                        //
                        // We couldn't read the image!  Free the SDRAM buffer and
                        // tell the caller that no external file system image is
                        // available.
                        //
                        ExtRAMFree(g_pcExternalFileSystem);
                        g_pcExternalFileSystem = (unsigned char *)0;
                    }
                    else
                    {
                        //
                        // Everything went well.  Fix up the pointer to the image
                        // in the file system mount point table.
                        //
                        g_psMountData[MOUNT_INDEX_RAM].pcFSImage =
                            g_pcExternalFileSystem;
                    }
                }
            }
            else if (g_eDaughterType == DAUGHTER_SRAM_FLASH)
            {
                //
                // The Flash/SRAM/LCD daughter board is present.  In this case, all
                // we need to do is fix up the pointer to the file system image in
                // external flash.
                //
                g_pcExternalFileSystem = (unsigned char *)EXT_FLASH_BASE;
                g_psMountData[MOUNT_INDEX_RAM].pcFSImage = g_pcExternalFileSystem;
            }
            else
            {
                //
                // With any other daughter card attached, we assume we don't
                // have an external file system image available.
                //
                g_pcExternalFileSystem = (unsigned char *)0;
            }

        }

        //
        // Regardless of whether or not we found an image, we have been
        // initialized.  After this point, the existance of an external memory
        // file system image is determine by whether or not there is a non-NULL
        // pointer in g_pcExternalFileSystem.
        //
        g_bInitialized = true;
    }

    return(g_pcExternalFileSystem ? true : false);
}

//*****************************************************************************
//
// Determines the size of the file system image hosted in external memory.
//
// This function returns the total size of the file system image that is stored.
// in external memory.  If no image is available, 0 is returned.
//
// \return Returns the length of the external-memory file system image in
// bytes or 0 if no image exists.
//
//*****************************************************************************
unsigned long
FileExternalImageSizeGet(void)
{
    unsigned long pulHeader[2];
    unsigned long ulRead;

    //
    // Was the external file system image initialized successfully?
    //
    if(g_pcExternalFileSystem)
    {
        //
        // If the file system has been successfully initialized, we can tell
        // the size by looking at the second 4 bytes in the image.
        //
        return(*((unsigned long *)(g_pcExternalFileSystem + 4)));
    }
    else
    {
        //
        // If the Flash/SRAM/LCD daughter board is installed, we look in the
        // external flash to see if a file system marker is present.  If so,
        // return the image length from the header.
        //
        if(g_eDaughterType == DAUGHTER_SRAM_FLASH)
        {
            if(*(unsigned long *)(EXT_FLASH_BASE) ==
                (unsigned long)FILE_SYSTEM_MARKER)
            {
                //
                // The marker is valid so we assume that a valid file
                // system image exists.  In this case, the size is in the
                // second word we read.
                //
                return(((unsigned long *)(EXT_FLASH_BASE))[1]);
            }
            else
            {
                //
                // We have no valid file system image in the external flash.
                //
                return(0);
            }
        }

        //
        // At this point, we do not have a copy of the file system image in
        // SDRAM so we look in the serial flash to see if there appears to be
        // a valid image there.
        //
        ulRead = SSIFlashRead(0, 8, (unsigned char *)pulHeader);

        //
        // Did we read the bytes successfully?
        //
        if(ulRead == 8)
        {
            //
            // Does the data we read start with the expected 4 byte
            // marker value?
            //
            if(pulHeader[0] == (unsigned long)FILE_SYSTEM_MARKER)
            {
                //
                // The marker is valid so we assume that a valid file
                // system image exists.  In this case, the size is in the
                // second word we read.
                //
                return(pulHeader[1]);
            }
        }
    }

    //
    // If we drop out, this indicates that no valid file system image is
    // available so return 0.
    //
    return(0);
}

//*****************************************************************************
//
// Calls the file system timer procedure.
//
// This function must be called by the application every 10mS.  It provides
// the time reference for the FAT file system.
//
// \return None.
//
//*****************************************************************************
void
FileTickHandler(void)
{
    //
    // Call the FatFs tick timer.
    //
    disk_timerproc();
}

//*****************************************************************************
//
// Initializes the file module and determines whether or not an SD card is
// present
//
// This function initializes the third party FAT implementation and determines
// whether or not a microSD card is currently installed in the EK board slot.
// Absence of a microSD card is not considered a failure here since it will
// be checked for again on each later access request.
//
// \return Returns \b true on success or \b false on failure.
//
//*****************************************************************************
tBoolean
FileInit(void)
{
    FRESULT fresult;
    unsigned long ulNumMountPoints;
    tBoolean bRetcode;

    //
    // Set the default number of mount points in our file system.
    //
    ulNumMountPoints = NUM_FS_MOUNT_POINTS;

    //
    // Copy the External file system image from EEPROM to the required
    // target address.  If the image is not present, remove the node from
    // the file system mount point array.
    //
    bRetcode = FileExternalImageInit();
    if(!bRetcode)
    {
        //
        // The file system could not be initialized so remove the node
        // from the mount point table.  We do this by rewriting it with
        // the information from the last node then reducing the node count
        // by 1.
        //
        g_psMountData[NUM_FS_MOUNT_POINTS - 2] =
            g_psMountData[NUM_FS_MOUNT_POINTS - 1];
        ulNumMountPoints = (NUM_FS_MOUNT_POINTS - 1);
    }

    //
    // Mount the SD card file system using logical disk 0.
    //
    fresult = f_mount(0, &g_sFatFs[0]);
    if(fresult != FR_OK)
    {
        UARTprintf("FileInit: f_mount(0) error: %s\n",
                   StringFromFresult(fresult));
        return(false);
    }

    //
    // Mount the USB stick file system using logical disk 1.
    //
    bRetcode = FileMountUSB(true);

    //
    // Assuming we managed to mount both the FAT logical drives, go ahead
    // and set the mount point table.
    //
    if(bRetcode)
    {
        //
        // Initialize the various file systems and images we will be using.
        //
        bRetcode = fs_init(g_psMountData, ulNumMountPoints);
    }

    //
    // Let the caller know how we got on.
    //
    return(bRetcode);
}

//*****************************************************************************
//
// Mounts or unmounts the USB flash stick as logical drive 1.
//
// \param bMount is \b true to mount the driver or \b false to unmount it.
//
// This function should be called when a USB flash disk is configured or
// removed to add it to or remove it from the file system.
//
// \return Returns \b true on success or \b false on failure.
//
//*****************************************************************************
tBoolean
FileMountUSB(tBoolean bMount)
{
    FRESULT fresult;

    //
    // Mount the USB file system using logical disk 1.
    //
    fresult = f_mount(1, (bMount ? &g_sFatFs[1] : NULL));
    if(fresult != FR_OK)
    {
        UARTprintf("FileInit: f_mount(1) error: %s\n",
                    StringFromFresult(fresult));
        return(false);
    }

    return(true);
}

//*****************************************************************************
//
// Writes a formatted string to a file.
//
// \param pFile is the handle of the file to which the formatted string is
// to be written.
// \param pcString is a pointer to the string to be formatted.  This is a
// standard printf-style string containing % insert markers.
// \param ... Additional parameters as required according to the insert
// markers within the \e pcString string.
//
// This function writes formatted strings to a file whose handle is provided.
// It is directly analogous to the standard C library fprintf() function
// other than the fact that it returns an FRESULT type (indicating any error
// from the low level FAT file system module).
//
// The maximum length of formatted string (containing all inserts and the
// terminating NUL character) is MAX_PRINTF_STRING_LEN.
//
// \return Returns \b FR_OK on success or other return codes on failure.
//
//*****************************************************************************
FRESULT
f_printf(FIL *pFile, const char *pcString, ...)
{
    FRESULT eResult;
    WORD uCount;
    va_list vaArgP;
    char pcBuffer[MAX_PRINTF_STRING_LEN];

    //
    // Start the varargs processing.
    //
    va_start(vaArgP, pcString);

    //
    // Format the string
    //
    uCount = (UINT)uvsnprintf(pcBuffer, MAX_PRINTF_STRING_LEN, pcString,
                              vaArgP);

    //
    //
    // Write the result to the file (assuming something was formatted).
    //
    if(uCount)
    {
        eResult = f_write(pFile, pcBuffer, uCount, &uCount);
    }
    else
    {
        //
        // We formatted a zero-length string so just return FR_OK.
        //
        eResult = FR_OK;
    }

    return(eResult);
}

//*****************************************************************************
//
// Dump the contents of a file on the SD card to UART0.
//
// \param pcFilename points to the name of the file that is to be dumped.
//
// This function echoes the contents of file \e pcFilename on the SD card
// file system to UART0.
//
// \return Returns \b true on success or \b false on failure.
//
//*****************************************************************************
tBoolean
FileCatToUART(char *pcFilename)
{
    FRESULT fresult;
    unsigned short usBytesRead;
    unsigned long ulTotalRead;
    unsigned char pucBuffer[READ_BUFFER_SIZE];

    //
    // Open the file for reading.
    //
    fresult = f_open(&g_sFile, pcFilename, FA_READ);

    //
    // If there was some problem opening the file, then return
    // an error.
    //
    if(fresult != FR_OK)
    {
        UARTprintf("File open error: %s\n", StringFromFresult(fresult));
        return(false);
    }

    //
    // Set the initial value of our byte counter.
    //
    ulTotalRead = 0;

    //
    // Enter a loop to repeatedly read data from the file and display it,
    // until the end of the file is reached.
    //
    do
    {
        //
        // Read a block of data from the file.  Read as much as can fit
        // in the temporary buffer, including a space for the trailing null.
        //
        fresult = f_read(&g_sFile, pucBuffer, READ_BUFFER_SIZE - 1,
                         &usBytesRead);

        //
        // If there was an error reading, then print a newline and
        // return the error to the user.
        //
        if(fresult != FR_OK)
        {
            UARTprintf("File read error: %s\n", StringFromFresult(fresult));
            f_close(&g_sFile);
            return(false);
        }

        //
        // Null terminate the last block that was read to make it a
        // null terminated string that can be used with printf.
        //
        pucBuffer[usBytesRead] = 0;

        //
        // Print the last chunk of the file that was received.
        //
        UARTprintf("%s", pucBuffer);

        //
        // Update our total count.
        //
        ulTotalRead += (unsigned long)usBytesRead;

        //
        // Ensure the UART has caught up with us.
        //
        UARTFlushTx(false);

    //
    // Continue reading until less than the full number of bytes are
    // read.  That means the end of the buffer was reached.
    //
    }
    while(usBytesRead == (READ_BUFFER_SIZE - 1));

    //
    // Output an extra newline just in case.
    //
    UARTprintf("\n");

    //
    // Close the file
    //
    f_close(&g_sFile);

    //
    // Return success.
    //
    return(true);
}

//*****************************************************************************
//
// Determines whether or not an external memory file system image is present.
//
// This function checks to see whether or not a file system image in external
// memory has been mounted.
//
// \return Returns \b true if the external file system image is present or \b
// false otherwise.
//
//*****************************************************************************
tBoolean
FileIsExternalImagePresent(void)
{
    return((g_bInitialized && g_pcExternalFileSystem) ? true : false);
}

//*****************************************************************************
//
// Check that a given logical drive can be accessed.
//
// \param ucDriveNum indicates the logical drive that is to be checked.  0
// indicates the SDCard and 1 indicates the USB stick.
//
// This function attempts to open the root directory of a given logical drive
// to check whether or not the drive is accessible.
//
// \return Returns \b true if the drive is accessible or \b false if not.
//
//*****************************************************************************
tBoolean
FileIsDrivePresent(unsigned char ucDriveNum)
{
    FRESULT eResult;
    char pcPath[4];

    pcPath[0] = '0' + ucDriveNum;
    pcPath[1] = ':';
    pcPath[2] = '/';
    pcPath[3] = '\0';

    eResult = f_opendir(&g_sDirObject, pcPath);
    if(eResult == FR_OK)
    {
        return(true);
    }
    else
    {
        return(false);
    }
}

//*****************************************************************************
//
// Dump the contents of a directory on the SD card to UART0.
//
// \param pcDir points to the name of the directory that is to be dumped.
//
// This function echoes the contents of directory \e pcDir on the SD card
// file system to UART0.
//
// \return Returns \b true on success or \b false on failure.
//
//*****************************************************************************
tBoolean
FileLsToUART(char *pcDir)
{
    unsigned long ulTotalSize;
    unsigned long ulFileCount;
    unsigned long ulDirCount;
    FRESULT fresult;
    FATFS *pFatFs;
    FILINFO sFile;

    //
    // Open the directory for access.
    //
    fresult = f_opendir(&g_sDirObject, pcDir);

    //
    // Check for error and return if there is a problem.
    //
    if(fresult != FR_OK)
    {
        UARTprintf("Dir open error: %s\n",
                   StringFromFresult(fresult));
        return(false);
    }

    ulTotalSize = 0;
    ulFileCount = 0;
    ulDirCount = 0;

    //
    // Give an extra blank line before the listing.
    //
    UARTprintf("\n");

    //
    // Enter loop to enumerate through all directory entries.
    //
    for(;;)
    {
        //
        // Read an entry from the directory.
        //
        fresult = f_readdir(&g_sDirObject, &sFile);

        //
        // Check for error and return if there is a problem.
        //
        if(fresult != FR_OK)
        {
            UARTprintf("Dir read error: %s\n",
                       StringFromFresult(fresult));
            return(false);
        }

        //
        // If the file name is blank, then this is the end of the
        // listing.
        //
        if(!sFile.fname[0])
        {
            break;
        }

        //
        // If the attribue is directory, then increment the directory count.
        //
        if(sFile.fattrib & AM_DIR)
        {
            ulDirCount++;
        }

        //
        // Otherwise, it is a file.  Increment the file count, and
        // add in the file size to the total.
        //
        else
        {
            ulFileCount++;
            ulTotalSize += sFile.fsize;
        }

        //
        // Print the entry information on a single line with formatting
        // to show the attributes, date, time, size, and name.
        //
        UARTprintf("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9u  %s\n",
                    (sFile.fattrib & AM_DIR) ? 'D' : '-',
                    (sFile.fattrib & AM_RDO) ? 'R' : '-',
                    (sFile.fattrib & AM_HID) ? 'H' : '-',
                    (sFile.fattrib & AM_SYS) ? 'S' : '-',
                    (sFile.fattrib & AM_ARC) ? 'A' : '-',
                    (sFile.fdate >> 9) + 1980,
                    (sFile.fdate >> 5) & 15,
                     sFile.fdate & 31,
                    (sFile.ftime >> 11),
                    (sFile.ftime >> 5) & 63,
                     sFile.fsize,
                     sFile.fname);

        //
        // Ensure the UART has caught up with us.
        //
        UARTFlushTx(false);
    }

    //
    // Print summary lines showing the file, dir, and size totals.
    //
    UARTprintf("\n%4u File(s),%10u bytes total\n%4u Dir(s)", ulFileCount,
               ulTotalSize, ulDirCount);

    //
    // Get the free space.
    //
    fresult = f_getfree((pcDir[0] == '1' ? "1:/" : "0:/") , &ulTotalSize,
                        &pFatFs);

    //
    // Check for error and return if there is a problem.
    //
    if(fresult != FR_OK)
    {
        UARTprintf("Get free open error: %s\n", StringFromFresult(fresult));
        return(false);
    }

    //
    // Display the amount of free space that was calculated.
    //
    UARTprintf(", %10uK bytes free\n",
               (ulTotalSize * pFatFs->sects_clust) / 2);

    //
    // Made it to here, return with no errors.
    //
    return(true);
}

//*****************************************************************************
//
// This function implements the "cd" command.  It takes an argument
// that specifes the directory to make the current working directory.
// Path separators must use a forward slash "/".  The argument to cd
// can be one of the following:
// * root ("/")
// * a fully specified path ("/my/path/to/mydir")
// * a single directory name that is in the current directory ("mydir")
// * parent directory ("..")
//
// It does not understand relative paths, so dont try something like this:
// ("../my/new/path")
//
// Once the new directory is specified, it attempts to open the directory
// to make sure it exists.  If the new path is opened successfully, then
// the current working directory (cwd) is changed to the new path.
//
// In cases of error, the pulReason parameter will be written with one of
// NAME_TOO_LONG_ERROR, OPENDIR_ERROR or NO_ERROR.
//
//*****************************************************************************
static FRESULT
ChangeToDirectory(char *pcDirectory, unsigned long *pulReason)
{
    unsigned int uIdx;
    FRESULT fresult;
    tBoolean bRetcode;
    char pcMapped[PATH_BUF_SIZE];

    //
    // Copy the current working path into a temporary buffer so
    // it can be manipulated.
    //
    strcpy(g_cTmpBuf, g_cCwdBuf);

    //
    // If the first character is /, then this is a fully specified
    // path, and it should just be used as-is.
    //
    if(pcDirectory[0] == '/')
    {
        //
        // Make sure the new path is not bigger than the cwd buffer.
        //
        if(strlen(pcDirectory) + 1 > sizeof(g_cCwdBuf))
        {
            *pulReason = NAME_TOO_LONG_ERROR;
            return(FR_OK);
        }

        //
        // If the new path name (in argv[1])  is not too long, then
        // copy it into the temporary buffer so it can be checked.
        //
        else
        {
            strncpy(g_cTmpBuf, pcDirectory, sizeof(g_cTmpBuf));
        }
    }

    //
    // If the argument is .. then attempt to remove the lowest level
    // on the CWD.
    //
    else if(!strcmp(pcDirectory, ".."))
    {
        //
        // Get the index to the last character in the current path.
        //
        uIdx = strlen(g_cTmpBuf) - 1;

        //
        // Back up from the end of the path name until a separator (/)
        // is found, or until we bump up to the start of the path.
        //
        while((g_cTmpBuf[uIdx] != '/') && (uIdx > 1))
        {
            //
            // Back up one character.
            //
            uIdx--;
        }

        //
        // Now we are either at the lowest level separator in the
        // current path, or at the beginning of the string (root).
        // So set the new end of string here, effectively removing
        // that last part of the path.
        //
        g_cTmpBuf[uIdx] = 0;
    }

    //
    // Otherwise this is just a normal path name from the current
    // directory, and it needs to be appended to the current path.
    //
    else
    {
        //
        // Test to make sure that when the new additional path is
        // added on to the current path, there is room in the buffer
        // for the full new path.  It needs to include a new separator,
        // and a trailing null character.
        //
        if(strlen(g_cTmpBuf) + strlen(pcDirectory) + 1 + 1 > sizeof(g_cCwdBuf))
        {
            *pulReason = NAME_TOO_LONG_ERROR;
            return(FR_INVALID_OBJECT);
        }

        //
        // The new path is okay, so add the separator and then append
        // the new directory to the path.
        //
        else
        {
            //
            // If not already at the root level, then append a /
            //
            if(strcmp(g_cTmpBuf, "/"))
            {
                strcat(g_cTmpBuf, "/");
            }

            //
            // Append the new directory to the path.
            //
            strcat(g_cTmpBuf, pcDirectory);
        }
    }

    //
    // Get the index to the last character in the current path.
    //
    uIdx = strlen(g_cTmpBuf) - 1;

    //
    // Check to see if there is a trailing slash and, if so, get rid of it.
    //
    if(g_cTmpBuf[uIdx] == '/')
    {
        g_cTmpBuf[uIdx] = '\0';
    }

    //
    // Map the path we just derived into the FatFS namespace.
    //
    bRetcode = fs_map_path(g_cTmpBuf, pcMapped, PATH_BUF_SIZE);

    if(!bRetcode)
    {
        UARTprintf("Path is invalid or not in the FAT file system.\n");
        *pulReason = OPENDIR_ERROR;
        return(FR_INVALID_OBJECT);
    }

    //
    // Tell the user what the new FAT path is.
    //
    UARTprintf("Mapped directory: %s\n", pcMapped);

    //
    // At this point, a candidate new directory path is in chTmpBuf.
    // Try to open it to make sure it is valid.
    //
    fresult = f_opendir(&g_sDirObject, pcMapped);

    //
    // If it cant be opened, then it is a bad path.  Inform
    // user and return.
    //
    if(fresult != FR_OK)
    {
        *pulReason = OPENDIR_ERROR;
        return(fresult);
    }

    //
    // Otherwise, it is a valid new path, so copy it into the CWD and update
    // the screen.
    //
    else
    {
        *pulReason = NO_ERROR;
        strncpy(g_cCwdBuf, g_cTmpBuf, sizeof(g_cCwdBuf));
        strncpy(g_cCwdMapped, pcMapped, sizeof(g_cCwdMapped));
    }

    //
    // Return success.
    //
    return(FR_OK);
}

//*****************************************************************************
//
// This function implements the "cd" command.  It takes an argument
// that specifes the directory to make the current working directory.
// Path separators must use a forward slash "/".  The argument to cd
// can be one of the following:
// * root ("/")
// * a fully specified path ("/my/path/to/mydir")
// * a single directory name that is in the current directory ("mydir")
// * parent directory ("..")
//
// It does not understand relative paths, so dont try something like this:
// ("../my/new/path")
//
// Once the new directory is specified, it attempts to open the directory
// to make sure it exists.  If the new path is opened successfully, then
// the current working directory (cwd) is changed to the new path.
//
//*****************************************************************************
int
Cmd_cd(int argc, char *argv[])
{
    unsigned long ulReason;
    FRESULT fresult;

    //
    // Try to change to the directory provided on the command line.
    //
    fresult = ChangeToDirectory(argv[1], &ulReason);

    //
    // If an error was reported, try to offer some helpful information.
    //
    if(fresult != FR_OK)
    {
        switch(ulReason)
        {
            case OPENDIR_ERROR:
                UARTprintf("Error opening new directory.\n");
                break;

            case NAME_TOO_LONG_ERROR:
                UARTprintf("Resulting path name is too long.\n");
                break;

            default:
                UARTprintf("An unrecognized error was reported.\n");
                break;
        }
    }
    else
    {
        //
        // Tell the user what happened.
        //
        UARTprintf("Changed to %s", g_cCwdBuf);
    }

    //
    // Return the appropriate error code.
    //
    return(fresult);
}

//*****************************************************************************
//
// This function implements the "pwd" command.  It simply prints the
// current working directory.
//
//*****************************************************************************
int
Cmd_pwd(int argc, char *argv[])
{
    //
    // Print the CWD to the console.
    //
    UARTprintf("%s\n", g_cCwdBuf);

    //
    // Wait for the UART transmit buffer to empty.
    //
    UARTFlushTx(false);

    //
    // Return success.
    //
    return(0);
}

//*****************************************************************************
//
// This function implements the "cat" command.  It reads the contents of
// a file and prints it to the console.  This should only be used on
// text files.  If it is used on a binary file, then a bunch of garbage
// is likely to printed on the console.
//
//*****************************************************************************
int
Cmd_cat(int argc, char *argv[])
{
    FRESULT fresult;
    unsigned short usBytesRead;

    //
    // First, check to make sure that the current path (CWD), plus
    // the file name, plus a separator and trailing null, will all
    // fit in the temporary buffer that will be used to hold the
    // file name.  The file name must be fully specified, with path,
    // to FatFs.
    //
    if(strlen(g_cCwdBuf) + strlen(argv[1]) + 1 + 1 > sizeof(g_cTmpBuf))
    {
        UARTprintf("Resulting path name is too long\n");
        return(0);
    }

    //
    // Copy the current path to the temporary buffer so it can be manipulated.
    //
    strcpy(g_cTmpBuf, g_cCwdMapped);

    //
    // If not already at the root level, then append a separator.
    //
    if(strcmp("/", g_cCwdMapped))
    {
        strcat(g_cTmpBuf, "/");
    }

    //
    // Now finally, append the file name to result in a fully specified file.
    //
    strcat(g_cTmpBuf, argv[1]);

    //
    // Open the file for reading.
    //
    fresult = f_open(&g_sFile, g_cTmpBuf, FA_READ);

    //
    // If there was some problem opening the file, then return
    // an error.
    //
    if(fresult != FR_OK)
    {
        return(fresult);
    }

    //
    // Enter a loop to repeatedly read data from the file and display it,
    // until the end of the file is reached.
    //
    do
    {
        //
        // Read a block of data from the file.  Read as much as can fit
        // in the temporary buffer, including a space for the trailing null.
        //
        fresult = f_read(&g_sFile, g_cTmpBuf, sizeof(g_cTmpBuf) - 1,
                         &usBytesRead);

        //
        // If there was an error reading, then print a newline and
        // return the error to the user.
        //
        if(fresult != FR_OK)
        {
            f_close(&g_sFile);
            UARTprintf("\n");
            return(fresult);
        }

        //
        // Null terminate the last block that was read to make it a
        // null terminated string that can be used with printf.
        //
        g_cTmpBuf[usBytesRead] = 0;

        //
        // Print the last chunk of the file that was received.
        //
        UARTprintf("%s", g_cTmpBuf);

        //
        // Wait for the UART transmit buffer to empty.
        //
        UARTFlushTx(false);

    //
    // Continue reading until less than the full number of bytes are
    // read.  That means the end of the buffer was reached.
    //
    }
    while(usBytesRead == sizeof(g_cTmpBuf) - 1);

    //
    // Close the file
    //
    f_close(&g_sFile);

    //
    // Return success.
    //
    return(0);
}


//*****************************************************************************
//
// This function implements the "ls" command.  It opens the current
// directory and enumerates through the contents, and prints a line for
// each item it finds.  It shows details such as file attributes, time and
// date, and the file size, along with the name.  It shows a summary of
// file sizes at the end along with free space.
//
//*****************************************************************************
int
Cmd_ls(int argc, char *argv[])
{
    unsigned long ulTotalSize, ulItemCount, ulFileCount, ulDirCount;
    FRESULT fresult;

    //
    // Open the current directory for access.
    //
    fresult = f_opendir(&g_sDirObject, g_cCwdMapped);

    //
    // Check for error and return if there is a problem.
    //
    if(fresult != FR_OK)
    {
        //
        // Ensure that the error is reported.
        //
        UARTprintf("Error opening file!\n");
        return(fresult);
    }

    ulTotalSize = 0;
    ulFileCount = 0;
    ulDirCount = 0;
    ulItemCount = 0;

    //
    // Give an extra blank line before the listing.
    //
    UARTprintf("\n");

    //
    // Enter loop to enumerate through all directory entries.
    //
    for(;;)
    {
        //
        // Read an entry from the directory.
        //
        fresult = f_readdir(&g_sDirObject, &g_sFileInfo);

        //
        // Check for error and return if there is a problem.
        //
        if(fresult != FR_OK)
        {
            return(fresult);
        }

        //
        // If the file name is blank, then this is the end of the
        // listing.
        //
        if(!g_sFileInfo.fname[0])
        {
            break;
        }

        //
        // Print the entry information on a single line with formatting
        // to show the attributes, date, time, size, and name.
        //
        UARTprintf("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9u  %s\n",
                 (g_sFileInfo.fattrib & AM_DIR) ? 'D' : '-',
                 (g_sFileInfo.fattrib & AM_RDO) ? 'R' : '-',
                 (g_sFileInfo.fattrib & AM_HID) ? 'H' : '-',
                 (g_sFileInfo.fattrib & AM_SYS) ? 'S' : '-',
                 (g_sFileInfo.fattrib & AM_ARC) ? 'A' : '-',
                 (g_sFileInfo.fdate >> 9) + 1980,
                 (g_sFileInfo.fdate >> 5) & 15,
                 g_sFileInfo.fdate & 31,
                 (g_sFileInfo.ftime >> 11),
                 (g_sFileInfo.ftime >> 5) & 63,
                 g_sFileInfo.fsize,
                 g_sFileInfo.fname);

        //
        // If the attribute is directory, then increment the directory count.
        //
        if(g_sFileInfo.fattrib & AM_DIR)
        {
            ulDirCount++;
        }

        //
        // Otherwise, it is a file.  Increment the file count, and
        // add in the file size to the total.
        //
        else
        {
            ulFileCount++;
            ulTotalSize += g_sFileInfo.fsize;
        }

        //
        // Move to the next entry in the item array we use to populate the
        // list box.
        //
        ulItemCount++;

        //
        // Wait for the UART transmit buffer to empty.
        //
        UARTFlushTx(false);

    }   // endfor

    //
    // Print summary lines showing the file, dir, and size totals.
    //
    UARTprintf("\n%4u File(s),%10u bytes total\n%4u Dir(s)",
                ulFileCount, ulTotalSize, ulDirCount);

    //
    // Wait for the UART transmit buffer to empty.
    //
    UARTFlushTx(false);

    //
    // Made it to here, return with no errors.
    //
    return(0);
}

//*****************************************************************************
//
// Checks to see if the supplied filename is in a given directory and has a
// given extension.
//
// \return Returns \b true if the filename passes the check or \b false
// otherwise.
//
//*****************************************************************************
static tBoolean
FileCheckFilename(const char *pcDir, const char *pcExt,
                  const unsigned char *pcFilename)
{
    unsigned long ulLoop;
    unsigned long ulCount;

    //
    // Check that the filename starts with the string passed in pcDir.
    //
    ulLoop = 0;
    while(pcDir[ulLoop] != (char)0)
    {
        //
        // If we reach the end of the filename or hit a character that does not
        // match, the filename doesn't meet our criterion so return
        // immediately.
        //
        if(!pcFilename[ulLoop] || (pcFilename[ulLoop] != pcDir[ulLoop]))
        {
            return(false);
        }

        ulLoop++;
    }

    //
    // At this point, ulLoop points to the first character after the directory.
    // Now we need to find the '.' but make sure we don't see any '/'s on the
    // way.
    //
    while(pcFilename[ulLoop] && (pcFilename[ulLoop] != '.') &&
          (pcFilename[ulLoop] != '/'))
    {
        ulLoop++;
    }

    //
    // If we found anything other than '.', the filename doesn't match.
    //
    if(pcFilename[ulLoop] != '.')
    {
        return(false);
    }

    //
    // Now check that the extension matches and that the filename ends
    // immediately after the extension.
    //
    ulLoop++;
    ulCount = 0;
    while(pcFilename[ulLoop] == pcExt[ulCount])
    {
        //
        // If we reached the terminating NULL, the extensions match so return
        // true.
        //
        if(pcFilename[ulLoop] == '\0')
        {
            return(true);
        }

        //
        // Move on to the next character.
        //
        ulLoop++;
        ulCount++;
    }

    //
    // If we drop out of the previous loop, there was a miscompare in the
    // extension so return false.
    //
    return(false);
}

//*****************************************************************************
//
// Counts the number of files with .jpg extension in the "images" directory of
// the External file system image.
//
// This function is used by the image viewer application and relies upon the
// presence of a file system image in external memory containing an "images"
// directory.  It traverses the files in the file system image and counts the
// number of files in the "images" directory with a ".jpg" extension.
//
// \return Returns the number of .jpg files found.
//
//*****************************************************************************
unsigned long
FileCountJPEGFiles(void)
{
    unsigned long ulCount, ulFSSize;
    const struct fsdata_file *ptTree;
    const struct fsdata_file *ptEnd;

    //
    // If the External file system is not present, return immediately.
    //
    if(!FileIsExternalImagePresent())
    {
        return(0);
    }

    //
    // Get the file system size and a pointer to the first node.
    //
    ulFSSize = *(unsigned long *)(g_pcExternalFileSystem + 4);
    ptTree = (struct fsdata_file *)((char *)g_pcExternalFileSystem + 8);
    ptEnd = (struct fsdata_file *)((char *)ptTree + ulFSSize);

    //
    // Zero our file count and offset through the file system image data.
    //
    ulCount = 0;

    //
    // We now traverse the file system list looking for images that are in
    // the "images" directory and which have extension ".jpg".
    //
    while((ptTree < ptEnd) &&
          (FSPTR(ptTree, ptTree->name) < (unsigned char *)ptEnd) &&
          (ptTree->next != 0))
    {
        //
        // Determine if this file meets the criteria of being in the "images"
        // directory and having extension ".jpg"
        //
        ulCount += (FileCheckFilename("/images/", "jpg",
                                      FSPTR(ptTree, ptTree->name)) ? 1 : 0);

        //
        // Move on to the next file in the image.
        //
        ptTree = (struct fsdata_file *)FSPTR(ptTree, ptTree->next);
    }

    return(ulCount);
}

//*****************************************************************************
//
// Returns information on the "ulIndex-th" JPEG file in the images directory.
//
// \param ulIndex is the index of the JPEG file that is to be queried.
// \param ppcFilename will be written with a pointer to the filename if found.
// \param pulLen will be written with the length of the file if found.
// \param ppucData will be written with a pointer to the first byte of the file
// data if found.
//
// This function scans the external file system image for JPEG images within the
// "images" directory and returns information on the ulIndex-th file if it
// exists.  This is used by the image viewer application to move back and
// forward through the JPEGs in the external file system image.
//
// \return Returns \b true if the image was found or \b false otherwise.
//
//*****************************************************************************
tBoolean
FileGetJPEGFileInfo(unsigned long ulIndex, char **ppcFilename,
                    unsigned long *pulLen, unsigned char **ppucData)
{
    unsigned long ulCount, ulFSSize;
    const struct fsdata_file *ptTree;
    const struct fsdata_file *ptEnd;
    tBoolean bRetcode;

    //
    // If the External file system is not present, return immediately.
    //
    if(!FileIsExternalImagePresent())
    {
        return(0);
    }

    //
    // Get the file system size and a pointer to the first node.
    //
    ulFSSize = *(unsigned long *)(g_pcExternalFileSystem + 4);
    ptTree = (struct fsdata_file *)((char *)g_pcExternalFileSystem + 8);
    ptEnd = (struct fsdata_file *)((char *)ptTree + ulFSSize);

    //
    // Zero our file count and offset through the file system image data.
    //
    ulCount = 0;

    //
    // We now traverse the file system list looking for images that are in
    // the "images" directory and which have extension ".jpg".
    //
    while((ptTree < ptEnd) &&
          (FSPTR(ptTree, ptTree->name) < (unsigned char *)ptEnd) &&
          (ptTree->next != 0))
    {
        //
        // Determine if this file meets the criteria of being in the "images"
        // directory and having extension ".jpg"
        //
        bRetcode = FileCheckFilename("/images/", "jpg",
                                     FSPTR(ptTree, ptTree->name));

        //
        // Have we found a matching file?
        //
        if(bRetcode)
        {
            //
            // Yes - we found a JPEG.  Is it the one we are looking for?
            //
            if(ulIndex == ulCount)
            {
                //
                // This is the actual JPEG we are looking for.  Return info
                // to the caller.
                //
                *ppcFilename = (char *)FSPTR(ptTree, ptTree->name);
                *pulLen = (unsigned long)ptTree->len;
                *ppucData = (unsigned char *)FSPTR(ptTree, ptTree->data);
                return(true);
            }
            else
            {
                //
                // This is not the file we are looking for.  Increment our
                // count and keep looking.
                //
                ulCount++;
            }
        }

        //
        // Move on to the next file in the image.
        //
        ptTree = (struct fsdata_file *)FSPTR(ptTree, ptTree->next);
    }

    //
    // If we get here, the file could not be found.
    //
    return(false);

}


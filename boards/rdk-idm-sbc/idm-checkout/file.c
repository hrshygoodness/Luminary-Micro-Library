//*****************************************************************************
//
// file.c - Functions related to file access for the RDK-IDM-SBC checkout
//          application.
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
// This is part of revision 9453 of the RDK-IDM-SBC Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
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
#include "idm-checkout.h"
#include "file.h"
#include "drivers/sdram.h"

//*****************************************************************************
//
// This label defines the SDRAM address to which we copied the web site image
// image stored in the serial flash.
//
//*****************************************************************************
unsigned char *g_pcSDRAMFileSystem;

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
// the idm_checkout directory:
//
//     makefsfile -i int_html -o idmfs_data.h
//
//*****************************************************************************
#include "idmfs_data.h"

//*****************************************************************************
//
// The following are data structures used by FatFs.
//
//*****************************************************************************
static FATFS g_sFatFs[2];
static DIR g_sDirObject;
static FIL g_sFile;

//*****************************************************************************
//
// A global indicating whether or not we have initialised the file system image
// that we copy from serial flash to SDRAM.
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
    //
    // The following entry MUST be the second last element in the list.  Add
    // any other fixed mount points above this point.
    //
    {"ram",      (unsigned char *)0, 0, 0, 0}, // RAM-based file system image.
    {NULL,       (unsigned char *)FS_ROOT, 0, 0, 0}  // Default root directory
};

#define NUM_FS_MOUNT_POINTS (sizeof(g_psMountData) / sizeof(fs_mount_data))

#define MOUNT_INDEX_SDCARD  0
#define MOUNT_INDEX_RAM     (NUM_FS_MOUNT_POINTS - 2)
#define MOUNT_INDEX_DEFAULT (NUM_FS_MOUNT_POINTS - 1)

unsigned long g_ulNumMountPoints;

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
// Initialize the SDRAM-based file system image.
//
// This function determines if a file system image exists in the serial flash
// and, if so, copies it to SDRAM in preparation for use.
//
// \return Returns \b true on success or \b false on failure (no file system
// image present in serial EEPROM or unable to write the image to SDRAM).
//
//*****************************************************************************
static tBoolean
FileSDRAMImageInit(void)
{
    unsigned long ulSize, ulRead;

    //
    // Are we already initialized?
    //
    if(!g_bInitialized)
    {
        //
        // Determine whether a file system image exists in the serial flash and,
        // if so, determine the size of the image.
        //
        ulSize = FileSDRAMImageSizeGet();

        //
        // Was an image found?
        //
        if(ulSize)
        {
            //
            // There is a file system image present so we need to copy it to
            // SDRAM and fix up the mount point table to point to the new
            // image.  First, allocate some storage for the image.
            //
            g_pcSDRAMFileSystem = ExtRAMAlloc(ulSize);

            if(g_pcSDRAMFileSystem)
            {
                //
                // We got the memory so now read the file system image from the
                // serial flash into the newly allocated buffer.
                //
                ulRead = SSIFlashRead(0, ulSize, g_pcSDRAMFileSystem);
                if(ulRead != ulSize)
                {
                    //
                    // We couldn't read the image!  Free the SDRAM buffer and
                    // tell the caller that no SDRAM file system image is
                    // available.
                    //
                    ExtRAMFree(g_pcSDRAMFileSystem);
                    g_pcSDRAMFileSystem = (unsigned char *)0;
                }
                else
                {
                    //
                    // Everything went well.  Fix up the pointer to the image
                    // in the file system mount point table.
                    //
                    g_psMountData[MOUNT_INDEX_RAM].pcFSImage =
                        g_pcSDRAMFileSystem;
                }
            }
        }

        //
        // Regardless of whether or not we found an image, we have been
        // initialized.  After this point, the existance of an SDRAM-based
        // file system image is determine by whether or not there is a non-NULL
        // pointer in g_pcSDRAMFileSystem.
        //
        g_bInitialized = true;
    }

    return(g_pcSDRAMFileSystem ? true : false);
}

//*****************************************************************************
//
// Determines the size of the file system image hosted in SDRAM.
//
// This function returns the total size of the file system image that is being
// served from SDRAM.  If no image is available, 0 is returns.
//
// \return None.
//
//*****************************************************************************
unsigned long
FileSDRAMImageSizeGet(void)
{
    unsigned long pulHeader[2];
    unsigned long ulRead;

    //
    // Was the file system image successfully copied from serial flash to
    // SDRAM?
    //
    if(g_pcSDRAMFileSystem)
    {
        //
        // If the file system has been successfully initialized, we can tell
        // the size by looking at the second 4 bytes in the image.
        //
        return(*((unsigned long *)(g_pcSDRAMFileSystem + 4)));
    }
    else
    {
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
    // Copy the SDRAM file system image from EEPROM to the required
    // target address.  If the image is not present, remove the node from
    // the file system mount point array.
    //
    bRetcode = FileSDRAMImageInit();
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
    // Mount the SD card file system, using logical disk 0.
    //
    fresult = f_mount(0, &g_sFatFs[0]);
    if(fresult != FR_OK)
    {
        UARTprintf("FileInit: f_mount(0) error: %s\n",
                   StringFromFresult(fresult));
        return(false);
    }

    //
    // Initialize the various file systems and images we will be using.
    //
    bRetcode = fs_init(g_psMountData, ulNumMountPoints);

    //
    // Let the caller know how we got on.
    //
    return(bRetcode);
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
// Determines whether or not an SDRAM file system image is present.
//
// This function checks to see whether or not an SDRAM file system image has
// been mounted.
//
// \return Returns \b true if the SDRAM file system is present or \b false
// otherwise.
//
//*****************************************************************************
tBoolean
FileIsSDRAMImagePresent(void)
{
    return((g_bInitialized && g_pcSDRAMFileSystem) ? true : false);
}

//*****************************************************************************
//
// Check that a given logical drive can be accessed.
//
// \param ucDriveNum indicates the logical drive that is to be checked.  0
// indicates the SDCard.
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
// the SDRAM file system image.
//
// This function is used by the image viewer application and relies upon the
// presence of an SDRAM-based file system image containing an "images"
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
    // If the SDRAM file system is not present, return immediately.
    //
    if(!FileIsSDRAMImagePresent())
    {
        return(0);
    }

    //
    // Get the file system size and a pointer to the first node.
    //
    ulFSSize = *(unsigned long *)(g_pcSDRAMFileSystem + 4);
    ptTree = (struct fsdata_file *)((char *)g_pcSDRAMFileSystem + 8);
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
// This function scans the SDRAM file system image for JPEG images within the
// "images" directory and returns information on the ulIndex-th file if it
// exists.  This is used by the image viewer application to move back and
// forward through the JPEGs in SDRAM.
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
    // If the SDRAM file system is not present, return immediately.
    //
    if(!FileIsSDRAMImagePresent())
    {
        return(0);
    }

    //
    // Get the file system size and a pointer to the first node.
    //
    ulFSSize = *(unsigned long *)(g_pcSDRAMFileSystem + 4);
    ptTree = (struct fsdata_file *)((char *)g_pcSDRAMFileSystem + 8);
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


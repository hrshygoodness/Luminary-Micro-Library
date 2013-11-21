//*****************************************************************************
//
// file.c - Functions related to file saving for the Quickstart Oscilloscope
//          application.
//
// Copyright (c) 2008-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the EK-LM3S3748 Firmware Package.
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
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "fatfs/src/ff.h"
#include "fatfs/src/diskio.h"
#include "data-acq.h"
#include "file.h"
#include "qs-scope.h"
#include "renderer.h"

//*****************************************************************************
//
// Structures related to Windows bitmaps
//
//*****************************************************************************

#ifdef ewarm
#pragma pack(1)
#endif

typedef struct
{
    unsigned short usType;
    unsigned long  ulSize;
    unsigned short usReserved1;
    unsigned short usReserved2;
    unsigned long  ulOffBits;
}
PACKED tBitmapFileHeader;

typedef struct
{
    unsigned long ulSize;
    long lWidth;
    long lHeight;
    unsigned short usPlanes;
    unsigned short usBitCount;
    unsigned long  ulCompression;
    unsigned long  ulSizeImage;
    long lXPelsPerMeter;
    long lYPelsPerMeter;
    unsigned long ulClrUsed;
    unsigned long ulClrImportant;
}
PACKED tBitmapInfoHeader;

typedef struct
{
    unsigned char cBlue;
    unsigned char cGreen;
    unsigned char cRed;
    unsigned char cReserved;
}
PACKED tRGBQuad;

#ifdef ewarm
#pragma pack()
#endif

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
// Buffer size required to store an 8.3 filename with leading "D:/"
// (rounded up to 20 for neatness).
//
//*****************************************************************************
#define MAX_FILENAME_LEN        20

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
    tBoolean bRetcode;

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
    // Try to open the root directory of the SD card.  This is basically just
    // a check to see if the card is there and formatted.
    //
    bRetcode = FileIsDrivePresent(0);
    if(bRetcode)
    {
        UARTprintf("Opened root directory - SD card present.\n");
    }
    else
    {
        //
        // We can't open the root directory.  Don't fail the init call since
        // we will try again later if and when the user asks to save their
        // data.
        //
        UARTprintf("No SD card found.  Error: %s\n",
                   StringFromFresult(fresult));
    }

    return(true);
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
static FRESULT
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
// Writes the supplied capture data to a file in CSV format.
//
// \param pCapData points to a structure containing information on the
// captured sample data that is to be written to a file.
// \param bSDCard is \b true to write the file to an installed micro SDCard
// or \b false to write to the USB flash stick.
//
// This function writes a CSV (comma separated value) file containing captured
// sample data and timestamps to the root directory of the FAT file system
// on the attached SD card.  The filename is "scopeXXX.csv" where "XXX" is
// a 3 digit decimal number chosen such that it does not clash with any
// existing file in the directory.
//
// \return Returns \b true on success or \b false on failure.
//
//*****************************************************************************
tBoolean
FileWriteCSV(tDataAcqCaptureStatus *pCapData, tBoolean bSDCard)
{
    char pcFilename[MAX_FILENAME_LEN];
    tBoolean bRetcode;
    FRESULT eResult;
    unsigned long ulSample;
    unsigned long ulCount;
    unsigned long ulAmV;
    unsigned long ulBmV;
    unsigned long ulATime;
    unsigned long ulBTime;

    //
    // Find the next free filename in the root directory of the appropriate
    // disk.
    //
    bRetcode = FileFindNextFilename(pcFilename, bSDCard ? 0 : 1,
                                    MAX_FILENAME_LEN, ".csv");

    //
    // If we couldn't get a suitable filename, exit immediately with an error.
    //
    if(!bRetcode)
    {
        return(bRetcode);
    }

    //
    // Open the new file for writing.
    //
    eResult = f_open(&g_sFile, pcFilename, FA_WRITE | FA_CREATE_NEW);
    if(eResult != FR_OK)
    {
        UARTprintf("FileWriteCSV: f_open(%s) error: %s\n",
                    pcFilename, StringFromFresult(eResult));
        RendererSetAlert("Error writing CSV file!", 200);
        return(false);
    }

    //
    // Write header information to the file
    //
    f_printf(&g_sFile, "Oscilloscope Data\n");
    if(pCapData->bDualMode)
    {
        f_printf(&g_sFile, "Channel 1,, Channel 2\n");
        f_printf(&g_sFile, "Time (uS), Sample (mV), Time (uS), Sample (mV)\n");
    }
    else
    {
        f_printf(&g_sFile, "Channel 1\n");
        f_printf(&g_sFile, "Time (uS), Sample (mV)\n");
    }

    //
    // Now write the data to the file formatted in CSV format suitable for
    // use with spreadsheets.
    //
    ulSample = pCapData->ulStartIndex;
    ulCount = 0;

    while(ulCount < pCapData->ulSamplesCaptured)
    {
        //
        // Does this data represent a single or dual channel capture?
        //
        if(pCapData->bDualMode)
        {
            //
            // Extract the sample and capture time taking care of the sample
            // order in the buffer.
            //
            ulAmV = ADC_SAMPLE_TO_MV(pCapData->pusBuffer[ulSample +
                                           (pCapData->bBSampleFirst ? 1 : 0)]);
            ulBmV = ADC_SAMPLE_TO_MV(pCapData->pusBuffer[ulSample +
                                           (pCapData->bBSampleFirst ? 0 : 1)]);

            ulATime = pCapData->ulSamplePerioduS * (ulCount / 2) +
                      (pCapData->bBSampleFirst ?
                        pCapData->ulSampleOffsetuS : 0);
            ulBTime = pCapData->ulSamplePerioduS * (ulCount / 2) +
                      (pCapData->bBSampleFirst ?
                        0 : pCapData->ulSampleOffsetuS);

            //
            // Store dual channel data.
            //
            f_printf(&g_sFile, "%6d, %6d, %6d, %6d\n", ulATime, ulAmV, ulBTime,
                     ulBmV);

            //
            // Move on to the next sample pair.
            //
            ulCount += 2;
            ulSample+= 2;
        }
        else
        {
            //
            // Store single channel data.
            //
            f_printf(&g_sFile, "%6d, %6d\n",
                     (pCapData->ulSamplePerioduS * ulCount),
                     ADC_SAMPLE_TO_MV(pCapData->pusBuffer[ulSample]));

            //
            // Move on to the next sample, taking care to wrap if we reach
            // the end of the buffer.
            //
            ulCount++;
            ulSample++;
        }

        //
        // Handle the wrap at the end of the buffer.
        //
        if(ulSample >= pCapData->ulMaxSamples)
        {
            ulSample -= pCapData->ulMaxSamples;
        }
    }

    //
    // Flush any buffered writes to the device.
    //
    f_sync(&g_sFile);

    //
    // Close the file
    //
    eResult = f_close(&g_sFile);
    if(eResult != FR_OK)
    {
        UARTprintf("FileWriteCSV: f_close error: %s\n",
                    StringFromFresult(eResult));
        RendererSetAlert("Error writing CSV file!", 200);
        return(false);
    }
    else
    {
        RendererSetAlert("CSV file written.", 200);
        return(true);
    }
}

//*****************************************************************************
//
// Writes the supplied capture data to a file as a Windows bitmap
//
// \param pCapData points to a structure containing information on the
// captured sample data that is to be written to a file.
// \param bSDCard is \b true to write the file to an installed micro SDCard
// or \b false to write to the USB flash stick.
//
// This function writes a 4bpp Windows bitmap file containing the captured
// waveform to the root directory of the FAT file system on the attached
// SD card.  The filename is "scopeXXX.bmp" where "XXX" is
// a 3 digit decimal number chosen such that it does not clash with any
// existing file in the directory.
//
// \return Returns \b true on success or \b false on failure.
//
//*****************************************************************************
tBoolean
FileWriteBitmap(tDataAcqCaptureStatus *pCapData, tBoolean bSDCard)
{
    char pcFilename[MAX_FILENAME_LEN];
    WORD usCount;
    unsigned long ulLineIndex;
    unsigned long ulLoop;
    FRESULT eResult;
    tBoolean bRetcode;
    tBitmapFileHeader sBmpHdr;
    tBitmapInfoHeader sBmpInfo;
    tRGBQuad sRGB;

    //
    // Find the next free filename in the root directory of the appropriate
    // disk.
    //
    bRetcode = FileFindNextFilename(pcFilename, bSDCard ? 0 : 1,
                                    MAX_FILENAME_LEN, ".bmp");

    //
    // If we couldn't get a suitable filename, exit immediately with an error.
    //
    if(!bRetcode)
    {
        return(bRetcode);
    }

    //
    // Open the new file for writing.
    //
    eResult = f_open(&g_sFile, pcFilename, FA_WRITE | FA_CREATE_NEW);
    if(eResult != FR_OK)
    {
        UARTprintf("FileWriteBitmap: f_open(%s) error: %s\n",
                    pcFilename, StringFromFresult(eResult));
        RendererSetAlert("Error writing\nbitmap!", 200);
        return(false);
    }

    //
    // Write the BITMAPFILEHEADER structure to the file
    //
    sBmpHdr.usType = 0x4D42; // "BM"
    sBmpHdr.ulSize = (sizeof(tBitmapFileHeader) + sizeof(tBitmapInfoHeader) +
                      (16 * sizeof(tRGBQuad)) +
                      (WAVEFORM_HEIGHT * (WAVEFORM_WIDTH + 1) / 2));
    sBmpHdr.usReserved1 = 0;
    sBmpHdr.usReserved2 = 0;
    sBmpHdr.ulOffBits = sizeof(tBitmapFileHeader) +
                        sizeof(tBitmapInfoHeader) + (16 * sizeof(tRGBQuad));

    eResult = f_write(&g_sFile, &sBmpHdr, sizeof(tBitmapFileHeader), &usCount);

    //
    // Write the BITMAPINFO structure to the file
    //
    sBmpInfo.ulSize = sizeof(tBitmapInfoHeader);
    sBmpInfo.lWidth = WAVEFORM_WIDTH;
    sBmpInfo.lHeight = WAVEFORM_HEIGHT;
    sBmpInfo.usPlanes = 1;
    sBmpInfo.usBitCount = 4;
    sBmpInfo.ulCompression = 0; // BI_RGB
    sBmpInfo.ulSizeImage = 0;
    sBmpInfo.lXPelsPerMeter = 20000;
    sBmpInfo.lYPelsPerMeter = 20000;
    sBmpInfo.ulClrUsed = 0;
    sBmpInfo.ulClrImportant = WAVEFORM_NUM_COLORS;

    eResult = f_write(&g_sFile, &sBmpInfo, sizeof(tBitmapInfoHeader),
                      &usCount);

    //
    // Write the palette to the file
    //
    for(ulLoop = 0; ulLoop < 16; ulLoop++)
    {
        if(ulLoop < WAVEFORM_NUM_COLORS)
        {
            sRGB.cRed = (g_pulPalette[ulLoop] & ClrRedMask) >>
                         ClrRedShift;
            sRGB.cBlue = (g_pulPalette[ulLoop] & ClrBlueMask) >>
                          ClrBlueShift;
            sRGB.cGreen = (g_pulPalette[ulLoop] & ClrGreenMask) >>
                           ClrGreenShift;
            sRGB.cReserved = 0;
        }
        else
        {
            sRGB.cRed = 0;
            sRGB.cBlue = 0;
            sRGB.cGreen = 0;
            sRGB.cReserved = 0;
        }

        eResult = f_write(&g_sFile, &sRGB, sizeof(tRGBQuad), &usCount);
    }

    //
    // Write the bitmap bits to the file (remembering that Windows bitmaps
    // are upside down when BITMAPINFOHEADER.biHeight is positive).
    //
    ulLineIndex = OFFSCREEN_BUF_SIZE - ((WAVEFORM_WIDTH + 1) / 2);

    for(ulLoop = WAVEFORM_HEIGHT; ulLoop > 0; ulLoop--)
    {
        eResult = f_write(&g_sFile, &g_pucOffscreenImage[ulLineIndex],
                          ((WAVEFORM_WIDTH + 1) / 2), &usCount);
        ulLineIndex -= ((WAVEFORM_WIDTH + 1) / 2);
    }

    //
    // Flush any buffered writes to the device.
    //
    f_sync(&g_sFile);

    //
    // Close the file
    //
    eResult = f_close(&g_sFile);
    if(eResult != FR_OK)
    {
        RendererSetAlert("Error writing bitmap!", 200);

        UARTprintf("FileWriteBitmap: f_close error: %s\n",
                    StringFromFresult(eResult));
        RendererSetAlert("Error writing\nbitmap!", 200);
        return(false);
    }
    else
    {
        RendererSetAlert("Bitmap written.", 200);
        return(true);
    }
}

//*****************************************************************************
//
// Finds an unused, unique filename for use in the SD card file system.
//
// \param pcFilename points to a buffer into which the filename will be
// written.
// \param ucDriveNum is the logical drive which the new file is intended to
// be written to.  Valid values are 0 for the SDCard or 1 for USB.
// \param ulLen provides the length of the \e pcFilename buffer in bytes.  This
// must be at least 17 for the function to work correctly.
// \param pcExt is a pointer to a string containing the desired 3 character
// filename extension.
//
// This function queries the content of the root directory on the SD card
// file system and returns a new filename of the form "D:/scopeXXX.EXT" where
// "D" is the logical drive number, "XXX" is a 3 digit decimal number and
// "EXT" is the extension passed in \e pcExt.
//
// \note The value "XXX" will be the lowest number which allows a new,
// unused filename to be created.  If, for example, files "scope000.csv" and
// "scope002.csv" exist in the directory, this function will return
// "scope001.csv" if passed "csv" in the \e pcExt parameter rather than
// "scope003.csv".
//
// \return Returns \b true on success or \b false on failure.
//
//*****************************************************************************
tBoolean
FileFindNextFilename(char *pcFilename, unsigned char ucDriveNum,
                     unsigned long ulLen, const char *pcExt)
{
    unsigned long ulLoop;
    FRESULT eResult;

    //
    // Check parameter validity.
    //
    ASSERT(ulLen >= 17);
    ASSERT(pcFilename);
    ASSERT((ucDriveNum == 0) || (ucDriveNum == 1));

    //
    // Loop through the possible filenames until we find one we can't open.
    //
    for(ulLoop = 0; ulLoop < 1000; ulLoop++)
    {
        //
        // Generate a possible filename.
        //
        usnprintf(pcFilename, ulLen, "%d:/scope%03d.%s", ucDriveNum, ulLoop,
                  pcExt);

        //
        // Try to open this file.
        //
        eResult = f_open(&g_sFile, pcFilename, FA_OPEN_EXISTING);

        //
        // If the file doesn't exist, we've found a suitable filename.
        //
        if(eResult == FR_NO_FILE)
        {
            break;
        }
        else
        {
            //
            // If the return code is FR_OK, this implies that the file already
            // exists so we need to close this file and try the next possible
            // filename.
            //
            if(eResult == FR_OK)
            {
                //
                // Close the file.
                //
                f_close(&g_sFile);
            }
            else
            {
                //
                // Some other error was reported.  Abort the function and
                // return an error.
                //
                RendererSetAlert("Can't write file.\nDrive present?", 200);
                UARTprintf("File open error: %s\n",
                           StringFromFresult(eResult));
                return(false);
            }
        }
    }

    //
    // If we drop out with ulLoop == 1000, we found the root directory
    // contains 1000 files called D:/scope????.<ext> already.  Fail the call.
    //
    if(ulLoop == 1000)
    {
        RendererSetAlert("Too many files on disk.", 200);
        return(false);
    }
    else
    {
        //
        // If we get here, the pcFilename buffer now contains a suitable name
        // for a file which does not already exist.  Tell the caller that all
        // is well.
        //
        return(true);
    }
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

//*****************************************************************************
//
// fatwrapper.c - A simple wrapper allowing access to binary fonts stored in
//                the FAT file system.
//
// Copyright (c) 2011-2012 Texas Instruments Incorporated.  All rights reserved.
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
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "utils/ustdlib.h"
#include "utils/uartstdio.h"
#include "third_party/fatfs/src/ff.h"
#include "third_party/fatfs/src/diskio.h"

//*****************************************************************************
//
// The number of font block headers that we cache when a font is opened.
//
//*****************************************************************************
#define MAX_FONT_BLOCKS 16

//*****************************************************************************
//
// The amount of memory set aside to hold compressed data for a single glyph.
// Fonts for use with the graphics library limit compressed glyphs to 256 bytes.
// If you are sure your fonts contain small glyphs, you could reduce this to
// save space.
//
//*****************************************************************************
#define MAX_GLYPH_SIZE 256

//*****************************************************************************
//
// Instance data for a single loaded font.
//
//*****************************************************************************
typedef struct
{
    //
    // The FATfs file object associated with the font.
    //
    FIL sFile;

    //
    // The font header as read from the file.
    //
    tFontWide sFontHeader;

    //
    // Storage for the font block table.
    //
    tFontBlock pBlocks[MAX_FONT_BLOCKS];

    //
    // A marker indicating whether or not the structure is in use.
    //
    tBoolean bInUse;

    //
    // The codepoint of the character whose glyph data is currently stored in
    // pucGlyphStore.
    //
    unsigned long ulCurrentGlyph;

    //
    // Storage for the compressed data of the latest glyph.  In a more complex
    // implementation, you would likely want to cache this data to reduce
    // slow disk interaction.
    //
    unsigned char pucGlyphStore[MAX_GLYPH_SIZE];
}
tFontFile;

//*****************************************************************************
//
// Workspace for FATfs.
//
//*****************************************************************************
FATFS g_sFatFs;

//*****************************************************************************
//
// Instance data for a single loaded font.  This implementation only supports
// a single font open at any one time.  If you wanted to implement something
// more general, a memory manager could be used to allocate these structures
// dynamically in FATWrapperFontLoad().
//
//*****************************************************************************
tFontFile g_sFontFile;

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
tFresultString g_sFresultStrings[] =
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
// Error reasons returned by ChangeDirectory().
//
//*****************************************************************************
#define NAME_TOO_LONG_ERROR 1
#define OPENDIR_ERROR       2

//*****************************************************************************
//
// A macro that holds the number of result codes.
//
//*****************************************************************************
#define NUM_FRESULT_CODES (sizeof(g_sFresultStrings) / sizeof(tFresultString))

//*****************************************************************************
//
// Internal function prototypes.
//
//*****************************************************************************
static const char *StringFromFresult(FRESULT fresult);
static void FATWrapperFontInfoGet(unsigned char *pucFontId,
                                  unsigned char *pucFormat,
                                  unsigned char *pucWidth,
                                  unsigned char *pucHeight,
                                  unsigned char *pucBaseline);
static const unsigned char *FATWrapperFontGlyphDataGet(
                                  unsigned char *pucFontId,
                                  unsigned long ulCodePoint,
                                  unsigned char *pucWidth);
static unsigned short FATWrapperFontCodepageGet(unsigned char *pucFontId);
static unsigned short FATWrapperFontNumBlocksGet(unsigned char *pucFontId);
static unsigned long FATWrapperFontBlockCodepointsGet(
                                 unsigned char *pucFontId,
                                 unsigned short usBlockIndex,
                                 unsigned long *pulStart);

//*****************************************************************************
//
// Access function pointers required to complete the tFontWrapper structure
// for this font.
//
//*****************************************************************************
const tFontAccessFuncs g_sFATFontAccessFuncs =
{
    FATWrapperFontInfoGet,
    FATWrapperFontGlyphDataGet,
    FATWrapperFontCodepageGet,
    FATWrapperFontNumBlocksGet,
    FATWrapperFontBlockCodepointsGet
};

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
// Returns information about a font previously loaded using FATFontWrapperLoad.
//
//*****************************************************************************
static void
FATWrapperFontInfoGet(unsigned char *pucFontId, unsigned char *pucFormat,
                      unsigned char *pucWidth, unsigned char *pucHeight,
                      unsigned char *pucBaseline)
{
    tFontFile *pFont;

    //
    // Parameter sanity check.
    //
    ASSERT(pucFontId);
    ASSERT(pucFormat);
    ASSERT(pucWidth);
    ASSERT(pucHeight);
    ASSERT(pucBaseline);

    //
    // Get a pointer to our instance data.
    //
    pFont = (tFontFile *)pucFontId;

    ASSERT(pFont->bInUse);

    //
    // Return the requested information.
    //
    *pucFormat = pFont->sFontHeader.ucFormat;
    *pucWidth =  pFont->sFontHeader.ucMaxWidth;
    *pucHeight =  pFont->sFontHeader.ucHeight;
    *pucBaseline =  pFont->sFontHeader.ucBaseline;
}

//*****************************************************************************
//
// Returns the codepage used by the font whose handle is passed.
//
//*****************************************************************************
static unsigned short
FATWrapperFontCodepageGet(unsigned char *pucFontId)
{
    tFontFile *pFont;

    //
    // Parameter sanity check.
    //
    ASSERT(pucFontId);

    //
    // Get a pointer to our instance data.
    //
    pFont = (tFontFile *)pucFontId;

    ASSERT(pFont->bInUse);

    //
    // Return the codepage identifier from the font.
    //
    return(pFont->sFontHeader.usCodepage);
}

//*****************************************************************************
//
// Returns the number of glyph blocks supported by a particular font.
//
//*****************************************************************************
static unsigned short
FATWrapperFontNumBlocksGet(unsigned char *pucFontId)
{
    tFontFile *pFont;

    //
    // Parameter sanity check.
    //
    ASSERT(pucFontId);

    //
    // Get a pointer to our instance data.
    //
    pFont = (tFontFile *)pucFontId;

    ASSERT(pFont->bInUse);

    //
    // Return the number of glyph blocks contained in the font.
    //
    return(pFont->sFontHeader.usNumBlocks);
}

//*****************************************************************************
//
// Read a given font block header from the provided file.
//
//*****************************************************************************
static tBoolean
FATWrapperFontBlockHeaderGet(FIL *pFile, tFontBlock *pBlock,
                             unsigned long ulIndex)
{
    FRESULT fresult;
    WORD ulRead;

    //
    // Set the file pointer to the position of the block header we want.
    //
    fresult = f_lseek(pFile, sizeof(tFontWide) +
                      (sizeof(tFontBlock) * ulIndex));
    if(fresult == FR_OK)
    {
        //
        // Now read the block header.
        //
        fresult = f_read(pFile, pBlock, sizeof(tFontBlock), &ulRead);
        if((fresult == FR_OK) && (ulRead == sizeof(tFontBlock)))
        {
            return(true);
        }
    }

    //
    // If we get here, we experienced an error so return a bad return code.
    //
    return(false);
}

//*****************************************************************************
//
// Returns information on the glyphs contained within a given font block.
//
//*****************************************************************************
static unsigned long
FATWrapperFontBlockCodepointsGet(unsigned char *pucFontId,
                                 unsigned short usBlockIndex,
                                 unsigned long *pulStart)
{
    tFontBlock sBlock;
    tFontFile *pFont;
    tBoolean bRetcode;

    //
    // Parameter sanity check.
    //
    ASSERT(pucFontId);

    //
    // Get a pointer to our instance data.
    //
    pFont = (tFontFile *)pucFontId;

    ASSERT(pFont->bInUse);

    //
    // Have we been passed a valid block index?
    //
    if(usBlockIndex >= pFont->sFontHeader.usNumBlocks)
    {
        //
        // No - return an error.
        //
        return(0);
    }

    //
    // Is this block header cached?
    //
    if(usBlockIndex < MAX_FONT_BLOCKS)
    {
        //
        // Yes - return the information from our cached copy.
        //
        *pulStart = pFont->pBlocks[usBlockIndex].ulStartCodepoint;
        return(pFont->pBlocks[usBlockIndex].ulNumCodepoints);
    }
    else
    {
        //
        // We don't have the block header cached so read it from the
        // SDCard.  First move the file pointer to the expected position.
        //
        bRetcode = FATWrapperFontBlockHeaderGet(&pFont->sFile, &sBlock,
                                                usBlockIndex);
        if(bRetcode)
        {
            *pulStart = sBlock.ulStartCodepoint;
            return(sBlock.ulNumCodepoints);
        }
        else
        {
            UARTprintf("Error reading block header!\n");
        }
    }

    //
    // If we get here, something horrible happened so return a failure.
    //
    *pulStart = 0;
    return(0);
}

//*****************************************************************************
//
// Retrieves the data for a particular font glyph.  This function returns
// a pointer to the glyph data in linear, random access memory if the glyph
// exists or NULL if not.
//
//*****************************************************************************
static const unsigned char *
FATWrapperFontGlyphDataGet(unsigned char *pucFontId,
                           unsigned long ulCodepoint,
                           unsigned char *pucWidth)
{
    tFontFile *pFont;
    unsigned long ulLoop, ulGlyphOffset, ulTableOffset;
    WORD ulRead;
    tFontBlock sBlock;
    tFontBlock *pBlock;
    tBoolean bRetcode;
    FRESULT fresult;

    //
    // Parameter sanity check.
    //
    ASSERT(pucFontId);
    ASSERT(pucWidth);

    //
    // If passed a NULL codepoint, return immediately.
    //
    if(!ulCodepoint)
    {
        return(0);
    }

    //
    // Get a pointer to our instance data.
    //
    pFont = (tFontFile *)pucFontId;

    ASSERT(pFont->bInUse);

    //
    // Look for the trivial case - do we have this glyph in our glyph store
    // already?
    //
    if(pFont->ulCurrentGlyph == ulCodepoint)
    {
        //
        // We struck gold - we already have this glyph in our buffer.  Return
        // the width (from the second byte of the data) and a pointer to the
        // glyph data.
        //
        *pucWidth = pFont->pucGlyphStore[1];
        return(pFont->pucGlyphStore);
    }

    //
    // First find the block that contains the glyph we've been asked for.
    //
    for(ulLoop = 0; ulLoop < pFont->sFontHeader.usNumBlocks; ulLoop++)
    {
        if(ulLoop < MAX_FONT_BLOCKS)
        {
            pBlock = &pFont->pBlocks[ulLoop];
        }
        else
        {
            bRetcode = FATWrapperFontBlockHeaderGet(&pFont->sFile, &sBlock,
                                                    ulLoop);
            pBlock = &sBlock;
            if(!bRetcode)
            {
                //
                // We failed to read the block header so return an error.
                //
                return(0);
            }
        }

        //
        // Does the requested character exist in this block?
        //
        if((ulCodepoint >= (pBlock->ulStartCodepoint)) &&
          ((ulCodepoint < (pBlock->ulStartCodepoint +
            pBlock->ulNumCodepoints))))
        {
            //
            // The glyph is in this block. Calculate the offset of it's
            // glyph table entry in the file.
            //
            ulTableOffset = pBlock->ulGlyphTableOffset +
                            ((ulCodepoint - pBlock->ulStartCodepoint) *
                            sizeof(unsigned long));

            //
            // Move the file pointer to the offset position.
            //
            fresult = f_lseek(&pFont->sFile, ulTableOffset);

            if(fresult != FR_OK)
            {
                return(0);
            }

            //
            // Read the glyph data offset.
            //
            fresult = f_read(&pFont->sFile, &ulGlyphOffset,
                             sizeof(unsigned long), &ulRead);

            //
            // Return if there was an error or if the offset is 0 (which
            // indicates that the character is not included in the font.
            //
            if((fresult != FR_OK) || (ulRead != sizeof(unsigned long)) ||
                !ulGlyphOffset)
            {
                return(0);
            }

            //
            // Move the file pointer to the start of the glyph data remembering
            // that the glyph table offset is relative to the start of the
            // block not the start of the file (so we add the table offset
            // here).
            //
            fresult = f_lseek(&pFont->sFile, (pBlock->ulGlyphTableOffset +
                              ulGlyphOffset));

            if(fresult != FR_OK)
            {
                return(0);
            }

            //
            // Read the first byte of the glyph data to find out how long it
            // is.
            //
            fresult = f_read(&pFont->sFile, pFont->pucGlyphStore,
                             1, &ulRead);

            if((fresult != FR_OK) || !ulRead)
            {
                return(0);
            }

            //
            // Now read the glyph data.
            //
            fresult = f_read(&pFont->sFile, pFont->pucGlyphStore + 1,
                             pFont->pucGlyphStore[0] - 1, &ulRead);

            if((fresult != FR_OK) || (ulRead != (pFont->pucGlyphStore[0] - 1)))
            {
                return(0);
            }

            //
            // If we get here, things are good. Return a pointer to the glyph
            // store data.
            //
            pFont->ulCurrentGlyph = ulCodepoint;
            *pucWidth = pFont->pucGlyphStore[1];
            return(pFont->pucGlyphStore);
        }
    }

    //
    // If we get here, the codepoint doesn't exist in the font so return an
    // error.
    //
    return(0);
}

//*****************************************************************************
//
// Prepares the FAT file system font wrapper for use.
//
// This function must be called before any attempt to use a font stored on the
// FAT file system.  It initializes FATfs for use.
//
// \return Returns \b true on success or \b false on failure.
//
//*****************************************************************************
tBoolean
FATFontWrapperInit(void)
{
    FRESULT fresult;

    //
    // Mount the file system, using logical disk 0.
    //
    fresult = f_mount(0, &g_sFatFs);
    if(fresult != FR_OK)
    {
        //
        // We failed to mount the volume.  Tell the user and return an error.
        //
        UARTprintf("f_mount error: %s (%d)\n", StringFromFresult(fresult),
                fresult);
        return(false);
    }

    //
    // All is well so tell the caller.
    //
    return(true);
}

//*****************************************************************************
//
// Provides the FATfs timer tick.
//
// This function must be called every 10mS or so by the application.  It
// provides the time reference required by the FAT file system.
//
// \return None.
//
//*****************************************************************************
void
FATWrapperSysTickHandler(void)
{
    //
    // Call the FatFs tick timer.
    //
    disk_timerproc();
}

//*****************************************************************************
//
// Prepares a font in the FATfs file system for use by the graphics library.
//
// This function must be called to prepare a font for use by the graphics
// library.  It opens the font file whose name is passed and reads the
// header information.  The value returned should be written into the
// pucFontId field of the tFontWrapper structure that will be passed to
// graphics library.
//
// This is a very simple (and slow) implementation.  More complex wrappers
// may also initialize a glyph cache here.
//
// \return On success, returns a non-zero pointer identifying the font.  On
// error, zero is returned.
//
//*****************************************************************************
unsigned char *
FATFontWrapperLoad(char *pcFilename)
{
    FRESULT fresult;
    WORD ulRead, ulToRead;

    UARTprintf("Attempting to load font %s from FAT file system.\n",
               pcFilename);

    //
    // Make sure a font is not already open.
    //
    if(g_sFontFile.bInUse)
    {
        //
        // Oops - someone tried to load a new font without unloading the
        // previous one.
        //
        UARTprintf("Another font is already loaded!\n");
        return(0);
    }

    //
    // Try to open the file whose name we've been given.
    //
    fresult = f_open(&g_sFontFile.sFile, pcFilename, FA_READ);
    if(fresult != FR_OK)
    {
        //
        // We can't open the file.  Either the file doesn't exist or there is
        // no SDCard installed.  Regardless, return an error.
        //
        UARTprintf("Error %s (%d) from f_open.\n", StringFromFresult(fresult),
                   fresult);
        return(0);
    }

    //
    // We opened the file successfully.  Does it seem to contain a valid
    // font?  Read the header and see.
    //
    fresult = f_read(&g_sFontFile.sFile, &g_sFontFile.sFontHeader,
                     sizeof(tFontWide), &ulRead);
    if((fresult == FR_OK) && (ulRead == sizeof(tFontWide)))
    {
        //
        // We read the font header.  Is the format correct? We only support
        // wide fonts via wrappers.
        //
        if((g_sFontFile.sFontHeader.ucFormat != FONT_FMT_WIDE_UNCOMPRESSED) &&
           (g_sFontFile.sFontHeader.ucFormat != FONT_FMT_WIDE_PIXEL_RLE))
        {
            //
            // This is not a supported font format.
            //
            UARTprintf("Unrecognized font format. Failing "
                       "FATFontWrapperLoad.\n");
            f_close(&g_sFontFile.sFile);
            return(0);
        }

        //
        // The format seems to be correct so read as many block headers as we
        // have storage for.
        //
        ulToRead = (g_sFontFile.sFontHeader.usNumBlocks > MAX_FONT_BLOCKS) ?
                    MAX_FONT_BLOCKS * sizeof(tFontBlock) :
                    g_sFontFile.sFontHeader.usNumBlocks * sizeof(tFontBlock);

        fresult = f_read(&g_sFontFile.sFile, &g_sFontFile.pBlocks, ulToRead,
                         &ulRead);
        if((fresult == FR_OK) && (ulRead == ulToRead))
        {
            //
            // All is well.  Tell the caller the font was opened successfully.
            //
            UARTprintf("Font %s opened successfully.\n", pcFilename);
            g_sFontFile.bInUse = true;
            return((unsigned char *)&g_sFontFile);
        }
        else
        {
            UARTprintf("Error %s (%d) reading block headers. Read %d, exp %d bytes.\n",
                       StringFromFresult(fresult), fresult, ulRead, ulToRead);
            f_close(&g_sFontFile.sFile);
            return(0);
        }
    }
    else
    {
        //
        // We received an error while reading the file header so fail the call.
        //
        UARTprintf("Error %s (%d) reading font header.\n", StringFromFresult(fresult),
                   fresult);
        f_close(&g_sFontFile.sFile);
        return(0);
    }
}

//*****************************************************************************
//
// Frees a font and cleans up once an application has finished using it.
//
// This function releases all resources allocated during a previous call to
// FATFontWrapperLoad().  The caller must not make any further use of the
// font following this call unless another call to FATFontWrapperLoad() is
// made.
//
// \return None.
//
//*****************************************************************************
void FATFontWrapperUnload(unsigned char *pucFontId)
{
    tFontFile *pFont;
    FRESULT fresult;

    //
    // Parameter sanity check.
    //
    ASSERT(pucFontId);

    //
    // Get a pointer to our instance data.
    //
    pFont = (tFontFile *)pucFontId;

    //
    // Make sure a font is already open.  If not, just return.
    //
    if(!pFont->bInUse)
    {
        return;
    }

    //
    // Close the font file.
    //
    UARTprintf("Unloading font... \n");
    fresult = f_close(&pFont->sFile);
    if(fresult != FR_OK)
    {
        UARTprintf("Error %s (%d) from f_close.\n", StringFromFresult(fresult),
                   fresult);
    }

    //
    // Clean up our instance data.
    //
    pFont->bInUse = false;
    pFont->ulCurrentGlyph = 0;
}

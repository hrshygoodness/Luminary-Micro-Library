//*****************************************************************************
//
// string.c - Routines for drawing text.
//
// Copyright (c) 2007-2011 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 7611 of the Stellaris Graphics Library.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "grlib/grlib.h"

//*****************************************************************************
//
//! \addtogroup primitives_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// The character printed by GrStringDraw in place of any character in the
// string which does not appear in the font.  When using a font which does not
// include this character, a space is left instead.
//
//*****************************************************************************
#define ABSENT_CHAR_REPLACEMENT '.'

//*****************************************************************************
//
// Counts the number of zeros at the start of a word.  This macro uses
// compiler-specific constructs to perform an inline insertion of the "clz"
// instruction, which counts the leading zeros directly.
//
//*****************************************************************************
#if defined(ewarm)
#include <intrinsics.h>
#define NumLeadingZeros(x)      __CLZ(x)
#endif
#if defined(codered) || defined(gcc) || defined(sourcerygxx)
#define NumLeadingZeros(x) __extension__                        \
        ({                                                      \
            register unsigned long __ret, __inp = x;            \
            __asm__("clz %0, %1" : "=r" (__ret) : "r" (__inp)); \
            __ret;                                              \
        })
#endif
#if defined(rvmdk) || defined(__ARMCC_VERSION)
#define NumLeadingZeros(x)      __clz(x)
#endif
#if defined(ccs)
//
// The CCS/TI compiler _norm intrinsic function will generate an inline CLZ
// instruction.
//
#define NumLeadingZeros(x)      _norm(x)
#endif

//*****************************************************************************
//
//! Determines the width of a string.
//!
//! \param pContext is a pointer to the drawing context to use.
//! \param pcString is the string in question.
//! \param lLength is the length of the string.
//!
//! This function determines the width of a string (or portion of the string)
//! when drawn with a particular font.  The \e lLength parameter allows a
//! portion of the string to be examined without having to insert a NULL
//! character at the stopping point (would not be possible if the string was
//! located in flash); specifying a length of -1 will cause the width of the
//! entire string to be computed.
//!
//! \return Returns the width of the string in pixels.
//
//*****************************************************************************
long
GrStringWidthGet(const tContext *pContext, const char *pcString, long lLength)
{
    const unsigned short *pusOffset;
    const unsigned char *pucData;
    unsigned char ucFirst, ucLast, ucAbsent;
    long lWidth;

    //
    // Check the arguments.
    //
    ASSERT(pContext);
    ASSERT(pcString);

    //
    // Get some pointers to relevant information in the font to make things
    // easier, and give the compiler a hint about extraneous loads that it can
    // avoid.
    //
    if(pContext->pFont->ucFormat & FONT_EX_MARKER)
    {
        tFontEx *pFont;

        pFont = (tFontEx *)(pContext->pFont);

        pucData = pFont->pucData;
        pusOffset = pFont->pusOffset;
        ucFirst = pFont->ucFirst;
        ucLast = pFont->ucLast;

        //
        // Does the default absent character replacement exist in the font?
        //
        if((ABSENT_CHAR_REPLACEMENT >= ucFirst) &&
           (ABSENT_CHAR_REPLACEMENT <= ucLast))
        {
            //
            // Yes - use the standard character when an absent character is
            // found.
            //
            ucAbsent = ABSENT_CHAR_REPLACEMENT;
        }
        else
        {
            //
            // The default absent character is not present in the font so use
            // the first character (we only use its width here) instead.
            //
            ucAbsent = pFont->ucFirst;
        }
    }
    else
    {
        pucData = pContext->pFont->pucData;
        pusOffset = pContext->pFont->pusOffset;
        ucFirst = 32;
        ucLast = 126;
        ucAbsent = ABSENT_CHAR_REPLACEMENT;
    }

    //
    // Loop through the characters in the string.
    //
    for(lWidth = 0; *pcString && lLength; pcString++, lLength--)
    {
        //
        // Get a pointer to the font data for the next character from the
        // string.  If there is not a glyph for the next character, replace it
        // with a ".".
        //
        if((*pcString >= ucFirst) && (*pcString <= ucLast))
        {
            //
            // Add the width of this character as drawn with the given font.
            //
            lWidth += pucData[pusOffset[*pcString - ucFirst] + 1];
        }
        else
        {
            //
            // This character does not exist in the font so replace it with
            // a '.' instead.  This matches the approach taken in GrStringDraw
            // and ensures that the width returned here represents the
            // rendered dimension of the string.
            //
            lWidth += pucData[pusOffset[ucAbsent - ucFirst] + 1];
        }
    }

    //
    // Return the width of the string.
    //
    return(lWidth);
}

//*****************************************************************************
//
//! Draws a string.
//!
//! \param pContext is a pointer to the drawing context to use.
//! \param pcString is a pointer to the string to be drawn.
//! \param lLength is the number of characters from the string that should be
//! drawn on the screen.
//! \param lX is the X coordinate of the upper left corner of the string
//! position on the screen.
//! \param lY is the Y coordinate of the upper left corner of the string
//! position on the screen.
//! \param bOpaque is true of the background of each character should be drawn
//! and false if it should not (leaving the background as is).
//!
//! This function draws a string of test on the screen.  The \e lLength
//! parameter allows a portion of the string to be examined without having to
//! insert a NULL character at the stopping point (which would not be possible
//! if the string was located in flash); specifying a length of -1 will cause
//! the entire string to be rendered (subject to clipping).
//!
//! \return None.
//
//*****************************************************************************
void
GrStringDraw(const tContext *pContext, const char *pcString, long lLength,
             long lX, long lY, unsigned long bOpaque)
{
    long lIdx, lX0, lY0, lCount, lOff, lOn, lBit;
    const unsigned char *pucData;
    const unsigned char *pucGlyphs;
    const unsigned short *pusOffset;
    unsigned char ucFirst, ucLast, ucAbsent;
    tContext sCon;

    //
    // Check the arguments.
    //
    ASSERT(pContext);
    ASSERT(pcString);

    //
    // Copy the drawing context into a local structure that can be modified.
    //
    sCon = *pContext;

    //
    // Extract various parameters from the font depending upon whether it's
    // in the tFont or tFontEx format.
    //
    if(pContext->pFont->ucFormat & FONT_EX_MARKER)
    {
        tFontEx *pFont;

        pFont = (tFontEx *)(pContext->pFont);

        pucGlyphs = pFont->pucData;
        pusOffset = pFont->pusOffset;
        ucFirst = pFont->ucFirst;
        ucLast = pFont->ucLast;

        //
        // Does the default absent character replacement exist in the font?
        //
        if((ABSENT_CHAR_REPLACEMENT >= ucFirst) &&
           (ABSENT_CHAR_REPLACEMENT <= ucLast))
        {
            //
            // Yes - use the standard character when an absent character is
            // found.
            //
            ucAbsent = ABSENT_CHAR_REPLACEMENT;
        }
        else
        {
            //
            // The default absent character is not present in the font so use
            // the first character instead.
            //
            ucAbsent = pFont->ucFirst;
        }
    }
    else
    {
        pucGlyphs = pContext->pFont->pucData;
        pusOffset = pContext->pFont->pusOffset;
        ucFirst = 32;
        ucLast = 126;
        ucAbsent = ABSENT_CHAR_REPLACEMENT;
    }

    //
    // Loop through the characters in the string.
    //
    while(*pcString && lLength--)
    {
        //
        // Stop drawing the string if the right edge of the clipping region has
        // been exceeded.
        //
        if(lX > sCon.sClipRegion.sXMax)
        {
            break;
        }

        //
        // Get a pointer to the font data for the next character from the
        // string.  If there is not a glyph for the next character, replace it
        // with the "absent" character (usually '.').
        //
        if((*pcString >= ucFirst) && (*pcString <= ucLast))
        {
            pucData = (pucGlyphs + pusOffset[*pcString - ucFirst]);
        }
        else
        {
            pucData = (pucGlyphs + pusOffset[ucAbsent - ucFirst]);
        }
        pcString++;

        //
        // See if the entire character is to the left of the clipping region.
        //
        if((lX + pucData[1]) < sCon.sClipRegion.sXMin)
        {
            //
            // Increment the X coordinate by the width of the character.
            //
            lX += pucData[1];

            //
            // Go to the next character in the string.
            //
            continue;
        }

        //
        // Loop through the bytes in the encoded data for this glyph.
        //
        for(lIdx = 2, lX0 = 0, lBit = 0, lY0 = 0; lIdx < pucData[0]; )
        {
            //
            // See if the bottom of the clipping region has been exceeded.
            //
            if((lY + lY0) > sCon.sClipRegion.sYMax)
            {
                //
                // Stop drawing this character.
                //
                break;
            }

            //
            // See if the font is uncompressed.
            //
            if((sCon.pFont->ucFormat & ~FONT_EX_MARKER) ==
                FONT_FMT_UNCOMPRESSED)
            {
                //
                // Count the number of off pixels from this position in the
                // glyph image.
                //
                for(lOff = 0; lIdx < pucData[0]; )
                {
                    //
                    // Get the number of zero pixels at this position.
                    //
                    lCount = NumLeadingZeros(pucData[lIdx] << (24 + lBit));

                    //
                    // If there were more than 8, then it is a "false" result
                    // since it counted beyond the end of the current byte.
                    // Therefore, simply limit it to the number of pixels
                    // remaining in this byte.
                    //
                    if(lCount > 8)
                    {
                        lCount = 8 - lBit;
                    }

                    //
                    // Increment the number of off pixels.
                    //
                    lOff += lCount;

                    //
                    // Increment the bit position within the byte.
                    //
                    lBit += lCount;

                    //
                    // See if the end of the byte has been reached.
                    //
                    if(lBit == 8)
                    {
                        //
                        // Advance to the next byte and continue counting off
                        // pixels.
                        //
                        lBit = 0;
                        lIdx++;
                    }
                    else
                    {
                        //
                        // Since the end of the byte was not reached, there
                        // must be an on pixel.  Therefore, stop counting off
                        // pixels.
                        //
                        break;
                    }
                }

                //
                // Count the number of on pixels from this position in the
                // glyph image.
                //
                for(lOn = 0; lIdx < pucData[0]; )
                {
                    //
                    // Get the number of one pixels at this location (by
                    // inverting the data and counting the number of zeros).
                    //
                    lCount = NumLeadingZeros(~(pucData[lIdx] << (24 + lBit)));

                    //
                    // If there were more than 8, then it is a "false" result
                    // since it counted beyond the end of the current byte.
                    // Therefore, simply limit it to the number of pixels
                    // remaining in this byte.
                    //
                    if(lCount > 8)
                    {
                        lCount = 8 - lBit;
                    }

                    //
                    // Increment the number of on pixels.
                    //
                    lOn += lCount;

                    //
                    // Increment the bit position within the byte.
                    //
                    lBit += lCount;

                    //
                    // See if the end of the byte has been reached.
                    //
                    if(lBit == 8)
                    {
                        //
                        // Advance to the next byte and continue counting on
                        // pixels.
                        //
                        lBit = 0;
                        lIdx++;
                    }
                    else
                    {
                        //
                        // Since the end of the byte was not reached, there
                        // must be an off pixel.  Therefore, stop counting on
                        // pixels.
                        //
                        break;
                    }
                }
            }

            //
            // Otherwise, the font is compressed with a pixel RLE scheme.
            //
            else
            {
                //
                // See if this is a byte that encodes some on and off pixels.
                //
                if(pucData[lIdx])
                {
                    //
                    // Extract the number of off pixels.
                    //
                    lOff = (pucData[lIdx] >> 4) & 15;

                    //
                    // Extract the number of on pixels.
                    //
                    lOn = pucData[lIdx] & 15;

                    //
                    // Skip past this encoded byte.
                    //
                    lIdx++;
                }

                //
                // Otherwise, see if this is a repeated on pixel byte.
                //
                else if(pucData[lIdx + 1] & 0x80)
                {
                    //
                    // There are no off pixels in this encoding.
                    //
                    lOff = 0;

                    //
                    // Extract the number of on pixels.
                    //
                    lOn = (pucData[lIdx + 1] & 0x7f) * 8;

                    //
                    // Skip past these two encoded bytes.
                    //
                    lIdx += 2;
                }

                //
                // Otherwise, this is a repeated off pixel byte.
                //
                else
                {
                    //
                    // Extract the number of off pixels.
                    //
                    lOff = pucData[lIdx + 1] * 8;

                    //
                    // There are no on pixels in this encoding.
                    //
                    lOn = 0;

                    //
                    // Skip past these two encoded bytes.
                    //
                    lIdx += 2;
                }
            }

            //
            // Loop while there are any off pixels.
            //
            while(lOff)
            {
                //
                // See if the bottom of the clipping region has been exceeded.
                //
                if((lY + lY0) > sCon.sClipRegion.sYMax)
                {
                    //
                    // Ignore the remainder of the on pixels.
                    //
                    break;
                }

                //
                // See if there is more than one on pixel that will fit onto
                // the current row.
                //
                if((lOff > 1) && ((lX0 + 1) < pucData[1]))
                {
                    //
                    // Determine the number of on pixels that will fit on this
                    // row.
                    //
                    lCount = (((lX0 + lOff) > pucData[1]) ? pucData[1] - lX0 :
                              lOff);

                    //
                    // If this row is within the clipping region, draw a
                    // horizontal line that corresponds to the sequence of on
                    // pixels.
                    //
                    if(((lY + lY0) >= sCon.sClipRegion.sYMin) && bOpaque)
                    {
                        sCon.ulForeground = pContext->ulBackground;
                        GrLineDrawH(&sCon, lX + lX0, lX + lX0 + lCount - 1,
                                    lY + lY0);
                    }

                    //
                    // Decrement the count of on pixels by the number on this
                    // row.
                    //
                    lOff -= lCount;

                    //
                    // Increment the X offset by the number of on pixels.
                    //
                    lX0 += lCount;
                }

                //
                // Otherwise, there is only a single on pixel that can be
                // drawn.
                //
                else
                {
                    //
                    // If this pixel is within the clipping region, then draw
                    // it.
                    //
                    if(((lX + lX0) >= sCon.sClipRegion.sXMin) &&
                       ((lX + lX0) <= sCon.sClipRegion.sXMax) &&
                       ((lY + lY0) >= sCon.sClipRegion.sYMin) && bOpaque)
                    {
                        DpyPixelDraw(pContext->pDisplay, lX + lX0, lY + lY0,
                                     pContext->ulBackground);
                    }

                    //
                    // Decrement the count of on pixels.
                    //
                    lOff--;

                    //
                    // Increment the X offset.
                    //
                    lX0++;
                }

                //
                // See if the X offset has reached the right side of the
                // character glyph.
                //
                if(lX0 == pucData[1])
                {
                    //
                    // Increment the Y offset.
                    //
                    lY0++;

                    //
                    // Reset the X offset to the left side of the character
                    // glyph.
                    //
                    lX0 = 0;
                }
            }

            //
            // Loop while there are any on pixels.
            //
            while(lOn)
            {
                //
                // See if the bottom of the clipping region has been exceeded.
                //
                if((lY + lY0) > sCon.sClipRegion.sYMax)
                {
                    //
                    // Ignore the remainder of the on pixels.
                    //
                    break;
                }

                //
                // See if there is more than one on pixel that will fit onto
                // the current row.
                //
                if((lOn > 1) && ((lX0 + 1) < pucData[1]))
                {
                    //
                    // Determine the number of on pixels that will fit on this
                    // row.
                    //
                    lCount = (((lX0 + lOn) > pucData[1]) ? pucData[1] - lX0 :
                              lOn);

                    //
                    // If this row is within the clipping region, draw a
                    // horizontal line that corresponds to the sequence of on
                    // pixels.
                    //
                    if((lY + lY0) >= sCon.sClipRegion.sYMin)
                    {
                        sCon.ulForeground = pContext->ulForeground;
                        GrLineDrawH(&sCon, lX + lX0, lX + lX0 + lCount - 1,
                                    lY + lY0);
                    }

                    //
                    // Decrement the count of on pixels by the number on this
                    // row.
                    //
                    lOn -= lCount;

                    //
                    // Increment the X offset by the number of on pixels.
                    //
                    lX0 += lCount;
                }

                //
                // Otherwise, there is only a single on pixel that can be
                // drawn.
                //
                else
                {
                    //
                    // If this pixel is within the clipping region, then draw
                    // it.
                    //
                    if(((lX + lX0) >= sCon.sClipRegion.sXMin) &&
                       ((lX + lX0) <= sCon.sClipRegion.sXMax) &&
                       ((lY + lY0) >= sCon.sClipRegion.sYMin))
                    {
                        DpyPixelDraw(pContext->pDisplay, lX + lX0, lY + lY0,
                                     pContext->ulForeground);
                    }

                    //
                    // Decrement the count of on pixels.
                    //
                    lOn--;

                    //
                    // Increment the X offset.
                    //
                    lX0++;
                }

                //
                // See if the X offset has reached the right side of the
                // character glyph.
                //
                if(lX0 == pucData[1])
                {
                    //
                    // Increment the Y offset.
                    //
                    lY0++;

                    //
                    // Reset the X offset to the left side of the character
                    // glyph.
                    //
                    lX0 = 0;
                }
            }
        }

        //
        // Increment the X coordinate by the width of the character.
        //
        lX += pucData[1];
    }
}

//*****************************************************************************
//
// Definitions and variables used by the decompression routine for the string
// table.
//
//*****************************************************************************
#define SC_MAX_INDEX            2047
#define SC_IS_NULL              0x0000ffff
#define SC_GET_LEN(v)           ((v) >> (32 - 5))
#define SC_GET_INDEX(v)         (((v) >> 16) & SC_MAX_INDEX)
#define SC_GET_OFF(v)           ((v) & SC_IS_NULL)

#define SC_FLAG_COMPRESSED      0x00008000
#define SC_OFFSET_M             0x00007fff

//*****************************************************************************
//
// The globals that hold the shortcuts to various locations and values in the
// table.
//
//*****************************************************************************
static const unsigned long *g_pulStringTable;
static const unsigned short *g_pusLanguageTable;
static const unsigned char *g_pucStringData;

static unsigned short g_usLanguage;
static unsigned short g_usNumLanguages;
static unsigned short g_usNumStrings;

//*****************************************************************************
//
//! This function sets the location of the current string table.
//!
//! \param pvTable is a pointer to a string table that was generated by the
//! string compression utility.
//!
//! This function is used to set the string table to use for strings in an
//! application.  This string table is created by the string compression
//! utility.  This function is used to swap out multiple string tables if the
//! application requires more than one table.  It does not allow using more
//! than one string table at a time.
//!
//! \return None.
//
//*****************************************************************************
void
GrStringTableSet(const void *pvTable)
{
    //
    // Save the number of languages and number of strings.
    //
    g_usNumStrings = ((unsigned short *)pvTable)[0];
    g_usNumLanguages = ((unsigned short *)pvTable)[1];

    //
    // Save a pointer to the Language Identifier table.
    //
    g_pusLanguageTable = (unsigned short *)pvTable + 2;

    //
    // Save a pointer to the String Index table.
    //
    g_pulStringTable = (unsigned long *)(g_pusLanguageTable +
                                         g_usNumLanguages);

    //
    // Save a pointer to the String Data.
    //
    g_pucStringData = (unsigned char *)(g_pulStringTable +
                                        (g_usNumStrings * g_usNumLanguages));
}

//*****************************************************************************
//
//! This function sets the current language for strings returned by the
//! GrStringGet() function.
//!
//! \param usLangID is one of the language identifiers provided in the string
//! table.
//!
//! This function is used to set the language identifier for the strings
//! returned by the GrStringGet() function.  The \e usLangID parameter should
//! match one of the identifiers that was included in the string table.  These
//! are provided in a header file in the graphics library and must match the
//! values that were passed through the sting compression utility.
//!
//! \return This function returns 0 if the language was not found and a
//! non-zero value if the laguage was found.
//
//*****************************************************************************
unsigned long
GrStringLanguageSet(unsigned short usLangID)
{
    long lLang;

    //
    // Search for the requested language.
    //
    for(lLang = 0; lLang < g_usNumLanguages; lLang++)
    {
        //
        // Once found, break out and save the new language.
        //
        if(g_pusLanguageTable[lLang] == usLangID)
        {
            break;
        }
    }

    //
    // Only accept the language if it was found, otherwise continue using
    // previous language.
    //
    if(lLang != g_usNumLanguages)
    {
        g_usLanguage = lLang;
        return(1);
    }

    return(0);
}

//*****************************************************************************
//
//! This function returns a string from the current string table.
//!
//! \param iIndex is the index of the string to retrieve.
//! \param pcData is the pointer to the buffer to store the string into.
//! \param ulSize is the size of the buffer provided by pcData.
//!
//! This function will return a string from the string table in the language
//! set by the GrStringLanguageSet() function.  The value passed in \e iIndex
//! parameter is the string that is being requested and will be returned in
//! the buffer provided in the \e pcData parameter.  The amount of data
//! returned will be limited by the ulSize parameter.
//!
//! \return Returns the number of valid bytes returned in the \e pcData buffer.
//
//*****************************************************************************
unsigned long
GrStringGet(long lIndex, char *pcData, unsigned long ulSize)
{
    unsigned long ulLen, ulOffset, ulSubCode[16];
    long lPos, lIdx, lBit, lSkip, lBuf;
    unsigned char *pucBufferOut;
    const unsigned char *pucString;

    ASSERT(lIndex < g_usNumStrings);
    ASSERT(pcData != 0);

    //
    // Initialize the output buffer state.
    //
    lPos = 0;
    pucBufferOut = 0;

    //
    // if built up from another string, we need to process that
    // this could nest multiple layers, so we follow in
    //
    ulSubCode[lPos] = g_pulStringTable[(g_usLanguage * g_usNumStrings) +
                                       lIndex];

    if(SC_GET_LEN(ulSubCode[lPos]))
    {
        //
        // recurse down
        //
        while(lPos < 16)
        {
            //
            // Copy over the partial (if any) from a previous string.
            //
            lIdx = SC_GET_INDEX(ulSubCode[lPos++]);

            ulSubCode[lPos] = g_pulStringTable[(g_usLanguage *
                                                g_usNumStrings) + lIdx];

            if(!SC_GET_LEN(ulSubCode[lPos]))
            {
                //
                // not linked, just string
                //
                break;
            }
        }
    }

    //
    // Now work backwards out.
    //
    lIdx = 0;

    //
    // Build up the string in pieces.
    //
    while(lPos >= 0)
    {
        //
        // Get the offset in string table.
        //
        ulOffset = SC_GET_OFF(ulSubCode[lPos]);

        if(ulOffset == SC_IS_NULL)
        {
            //
            // An empty string.
            //
            pcData[lIdx] = 0;
        }
        else if(ulOffset & SC_FLAG_COMPRESSED)
        {
            //
            // This is a compressed string so initialize the pointer to the
            // compressed data.
            //
            pucString = g_pucStringData + (ulOffset & SC_OFFSET_M);

            //
            // Initialize the bit variables.
            //
            lBit = 0;
            lSkip = 0;

            //
            // Make a pointer to the current buffer out location.
            //
            pucBufferOut = (unsigned char *)pcData + lIdx;

            //
            // If the out buffer is beyond the maximum size then just break
            // out and return what we have so far.
            //
            if((char *)pucBufferOut > (pcData + ulSize))
            {
                break;
            }

            //
            // Now build up real string by decompressing bits.
            //
            if(!SC_GET_LEN(ulSubCode[lPos]) && SC_GET_INDEX(ulSubCode[lPos]))
            {
                lSkip = SC_GET_INDEX(ulSubCode[lPos]);

                if(lPos)
                {
                    ulLen = SC_GET_LEN(ulSubCode[lPos - 1]);
                }
                else
                {
                    ulLen = (lSkip & 0x3f);
                }

                lSkip >>= 6;
                lIdx += ulLen;
                ulLen += lSkip;
            }
            else if(lPos)
            {
                //
                // Get the length of the partial string.
                //
                ulLen = SC_GET_LEN(ulSubCode[lPos - 1]) - lIdx;
                lIdx += ulLen;
            }
            else if(!SC_GET_LEN(ulSubCode[0]) && SC_GET_INDEX(ulSubCode[0]))
            {
                ulLen = SC_GET_INDEX(ulSubCode[0]);
                lSkip = ulLen >> 6;
                ulLen = (ulLen & 0x3f) + lSkip;
            }
            else
            {
                //
                // Arbitrary as null character ends the string.
                //
                ulLen = 1024;
            }

            for(; ulLen; ulLen--)
            {
                //
                // Packed 6 bits for each char
                //
                *pucBufferOut = (*pucString >> lBit) & 0x3f;

                if(lBit >= 2)
                {
                    *pucBufferOut |= (*++pucString << (8 - lBit)) & 0x3f;
                }

                lBit = (lBit + 6) & 0x7;

                if(!*pucBufferOut)
                {
                    //
                    // end of string
                    //
                    break;
                }

                if(lSkip)
                {
                    lSkip--;
                    continue;
                }

                //
                // Put back removed bit
                //
                *pucBufferOut |= 0x40;

                //
                // Now look for a few special chars we mapped up into other
                // characters.
                //
                if(*pucBufferOut == '`')
                {
                    *pucBufferOut = ' ';
                }
                else if(*pucBufferOut == '~')
                {
                    *pucBufferOut = '-';
                }
                else if(*pucBufferOut == 0x7f)
                {
                    *pucBufferOut = '.';
                }
                else if(*pucBufferOut == '\\')
                {
                    *pucBufferOut = ':';
                }

                //
                // Increment the pointer and break out if the pointer is now
                // beyond the end of the buffer provided.
                //
                pucBufferOut++;

                if((char *)pucBufferOut >= (pcData + ulSize))
                {
                    break;
                }
            }
        }
        else if(lPos)
        {
            //
            // Part of another string
            //
            ulLen = SC_GET_LEN(ulSubCode[lPos - 1]) - lIdx;

            //
            // Prevent this copy from going beyond the end of the buffer
            // provided.
            //
            if((lIdx + ulLen) > ulSize)
            {
                ulLen = ulSize - lIdx;
            }

            //
            // Copy this portion of the string to the output buffer.
            //
            for(lBuf = 0; lBuf < ulLen; lBuf++)
            {
                pcData[lIdx + lBuf] = g_pucStringData[ulOffset + lBuf];
            }

            lIdx += ulLen;
        }
        else if(SC_GET_INDEX(ulSubCode[0]) && !SC_GET_LEN(ulSubCode[0]))
        {
            //
            // Copy this portion of the string to the output buffer.
            //
            for(lBuf = 0; lBuf < SC_GET_INDEX(ulSubCode[0]); lBuf++)
            {
                if((lIdx + lBuf) < ulSize)
                {
                    pcData[lIdx + lBuf] = g_pucStringData[ulOffset + lBuf];
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            //
            // The last piece is the string ending
            //
            for(lBuf = 0; lBuf < (ulSize - lIdx); lBuf++)
            {
                //
                // Copy the string to the output buffer.
                //
                pcData[lIdx + lBuf] = g_pucStringData[ulOffset + lBuf];

                //
                // If a null is hit then terminate the copy.
                //
                if(pcData[lIdx + lBuf] == 0)
                {
                    break;
                }
            }
        }
        lPos--;
    }

    //
    // Return the number of bytes copied into the output buffer.
    //
    if(pucBufferOut)
    {
        ulLen = ((unsigned long)pucBufferOut - (unsigned long)pcData);

        //
        // Null terminate the string if there is room.
        //
        if(ulLen < ulSize)
        {
            pcData[ulLen] = 0;
        }
    }
    else
    {
        ulLen = 0;
    }

    return(ulLen);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

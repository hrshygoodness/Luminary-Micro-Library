//*****************************************************************************
//
// ftrasterize.c - Program to convert FreeType-compatible fonts to compressed
//                 bitmap fonts using FreeType.
//
// Copyright (c) 2007-2010 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 6594 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#include <ft2build.h>
#include FT_FREETYPE_H
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//*****************************************************************************
//
// The structure that describes each character glyph.
//
//*****************************************************************************
typedef struct
{
    //
    // The width of the bitmap representation of this glyph.
    //
    int iWidth;

    //
    // The height of the bitmap representation of this glyph.
    //
    int iRows;

    //
    // The number of bytes in each row of the bitmap representation of this
    // glyph (this may be more than width * 8).
    //
    int iPitch;

    //
    // The number of rows between the baseline and the top of this glyph.
    //
    int iBitmapTop;

    //
    // The minimum X value in the glyph bitmap.
    //
    int iMinX;

    //
    // The maximum X value in the glyph bitmap.
    //
    int iMaxX;

    //
    // The bitmap representation of this glyph.
    //
    unsigned char *pucData;

    //
    // The compressed data describing this glyph.
    //
    unsigned char *pucChar;
}
tGlyph;

//*****************************************************************************
//
// An array of glyphs for the ASCII characters 32 (space) through 126 (~).
//
//*****************************************************************************
tGlyph g_pGlyphs[95];

//*****************************************************************************
//
// Prints the usage message for this application.
//
//*****************************************************************************
void
Usage(char *argv)
{
    fprintf(stderr, "Usage: %s [-b] [-f <filename>] [-i] [-m] [-s <size>] "
                    "<font>\n", basename(argv));
    fprintf(stderr, "Converts any font that FreeType recognizes into a "
                    "compressed font for use by\n");
    fprintf(stderr, "the Stellaris Graphics Library.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  -b            Specifies that this is a bold font.\n");
    fprintf(stderr, "  -f <filename> Specifies the base name for this font "
                    "[default=\"font\"].\n");
    fprintf(stderr, "  -i            Specifies that this is an italic "
                    "font.\n");
    fprintf(stderr, "  -m            Specifies that this is a monospaced "
                    "font.\n");
    fprintf(stderr, "  -s <size>     Specifies the size of this font "
                    "[default=20]\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Example:\n");
    fprintf(stderr, "  %s -f foobar -s 24 foobar.ttf\n", basename(argv));
    fprintf(stderr, "Produces fontfoobar24.c with a 24 point version of the "
                    "font in foobar.ttf.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Report bugs to <support_lmi@ti.com>\n");
}

//*****************************************************************************
//
// The main application that converts a FreeType-compatible font into a
// compressed raster font for use by the Stellaris Graphics Library.
//
//*****************************************************************************
int
main(int argc, char *argv[])
{
    int iSize, iOpt, iXMin, iYMin, iXMax, iYMax, iX, iY, iWidth;
    int iPrevBit, iBit, iZero, iOne, iBold, iItalic, iMono;
    char *pcFilename, *pcCapFilename;
    unsigned char pucChar[512];
    FT_UInt uiChar, uiIndex;
    FT_Library pLibrary;
    FT_GlyphSlot pSlot;
    FT_Bitmap *ppMap;
    tGlyph *pGlyph;
    FT_Face pFace;
    FILE *pFile;

    //
    // Set the defaults for the things that can be specified from the command
    // line.
    //
    pcFilename = "font";
    iSize = 20;
    iBold = 0;
    iItalic = 0;
    iMono = 0;

    //
    // Process the command line arguments with getopt.
    //
    while((iOpt = getopt(argc, argv, "bf:ims:")) != -1)
    {
        //
        // See which argument was found.
        //
        switch(iOpt)
        {
            //
            // The "-b" switch was found.
            //
            case 'b':
            {
                //
                // Indicate that this is a bold font.
                //
                iBold = 1;

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // The "-f" switch was found.
            //
            case 'f':
            {
                //
                // Save the specified filename.
                //
                pcFilename = optarg;

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // The "-i" switch was found.
            //
            case 'i':
            {
                //
                // Indicate that this is an italic font.
                //
                iItalic = 1;

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // The "-m" switch was found.
            //
            case 'm':
            {
                //
                // Indicate that this is a monospaced font.
                //
                iMono = 1;

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // The "-s" switch was found.
            //
            case 's':
            {
                //
                // Save the font size.
                //
                iSize = strtoul(optarg, 0, 0);

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // An unknown switch was found.
            //
            default:
            {
                //
                // Print out the usage for this application.
                //
                Usage(argv[0]);

                //
                // Return a failure.
                //
                return(1);
            }
        }
    }

    //
    // There must be one additional argument on the command line.
    //
    if(optind != (argc - 1))
    {
        //
        // There is not another argument, so print out the usage for this
        // application.
        //
        Usage(argv[0]);

        //
        // Return a failure.
        //
        return(1);
    }

    //
    // Check to see if the size is reasonable.
    //
    if((iSize <= 0) || (iSize > 100))
    {
        fprintf(stderr, "%s: The font size must be from 1 to 100, "
                        "inclusive.\n", basename(argv[0]));
        return(1);
    }

    //
    // Initialize the FreeType library.
    //
    if(FT_Init_FreeType(&pLibrary) != 0)
    {
        fprintf(stderr, "%s: Unable to initialize the FreeType library.\n",
                basename(argv[0]));
        return(1);
    }

    //
    // Load the specified font file into the FreeType library.
    //
    if(FT_New_Face(pLibrary, argv[optind], 0, &pFace) != 0)
    {
        fprintf(stderr, "%s: Unable to open font file '%s'\n",
                basename(argv[0]), argv[optind]);
        return(1);
    }

    //
    // Select the Adobe Custom character map if it exists (it is the proper
    // character map to use for the Computer Modern fonts).  If it does not
    // exist, then select the Unicode character map.
    //
    if(FT_Select_Charmap(pFace, FT_ENCODING_ADOBE_CUSTOM) != 0)
    {
        FT_Select_Charmap(pFace, FT_ENCODING_UNICODE);
    }

    //
    // Set the size of the character based on the specified size.
    //
    if(iMono)
    {
        FT_Set_Char_Size(pFace, iSize * 56, iSize * 64, 0, 72);
    }
    else
    {
        FT_Set_Char_Size(pFace, 0, iSize * 64, 0, 72);
    }

    //
    // Get some pointers to make things easier.
    //
    pSlot = pFace->glyph;
    ppMap = &(pSlot->bitmap);

    //
    // No need to render the space character; it contains nothing.
    //
    g_pGlyphs[0].iWidth = 0;
    g_pGlyphs[0].iRows = 0;
    g_pGlyphs[0].iPitch = 0;
    g_pGlyphs[0].iBitmapTop = 0;
    g_pGlyphs[0].pucData = 0;

    //
    // Loop through the ASCII characters.
    //
    for(uiChar = 1; uiChar < 0x5f; uiChar++)
    {
        //
        // Get the index of this character.
        //
        uiIndex = FT_Get_Char_Index(pFace, uiChar + 0x20);

        //
        // Load the glyph for this character.
        //
        if(FT_Load_Glyph(pFace, uiIndex,
                         FT_LOAD_DEFAULT | FT_LOAD_TARGET_MONO) == 0)
        {
            //
            // If this is an outline glyph, then render it.
            //
            if(pSlot->format == FT_GLYPH_FORMAT_OUTLINE)
            {
                FT_Render_Glyph(pSlot, FT_RENDER_MODE_MONO);
            }

            //
            // Save the relevant information about this character glyph.
            //
            g_pGlyphs[uiChar].iWidth = ppMap->width;
            g_pGlyphs[uiChar].iRows = ppMap->rows;
            g_pGlyphs[uiChar].iPitch = ppMap->pitch;
            g_pGlyphs[uiChar].iBitmapTop = pSlot->bitmap_top;
            g_pGlyphs[uiChar].pucData = malloc(ppMap->rows * ppMap->pitch);
            memcpy(g_pGlyphs[uiChar].pucData, ppMap->buffer,
                   ppMap->rows * ppMap->pitch);
            if((ppMap->width == 0) && (uiChar != 0))
            {
                printf("%s: Warning: Zero width glyph for '%c'\n",
                       basename(argv[0]), uiChar + 0x20);
            }
        }
        else
        {
            printf("%s: Warning: Could not load glyph for '%c'\n",
                   basename(argv[0]), uiChar + 0x20);
        }
    }

    //
    // Free the font.
    //
    FT_Done_Face(pFace);

    //
    // Close the FreeType library.
    //
    FT_Done_FreeType(pLibrary);

    //
    // Loop through the glyphs, finding the minimum and maximum X and Y values
    // for the glyphs.
    //
    for(pGlyph = g_pGlyphs, iYMin = 0, iYMax = 0, iWidth = 0;
        pGlyph < (g_pGlyphs + 95); pGlyph++)
    {
        //
        // Adjust the minimum Y value if necessary.
        //
        if(pGlyph->iBitmapTop > iYMin)
        {
            iYMin = pGlyph->iBitmapTop;
        }

        //
        // Adjust the maximum Y value if necessary.
        //
        if((pGlyph->iBitmapTop - pGlyph->iRows + 1) < iYMax)
        {
            iYMax = pGlyph->iBitmapTop - pGlyph->iRows + 1;
        }

        //
        // Find the minimum and maximum X value for this glyph.  Loop through
        // the rows of the bitmap for this glyph.
        //
        for(iY = 0, iXMin = 1000000, iXMax = 0; iY < pGlyph->iRows; iY++)
        {
            //
            // Find the minimum X for this row of the glyph.
            //
            for(iX = 0; iX < pGlyph->iWidth; iX++)
            {
                if(pGlyph->pucData[(iY * pGlyph->iPitch) + (iX / 8)] &
                   (1 << (7 - (iX & 7))))
                {
                    if(iX < iXMin)
                    {
                        iXMin = iX;
                    }
                    break;
                }
            }

            //
            // Find the maximum X for this row of the glyph.
            //
            for(iX = pGlyph->iWidth - 1; iX >= 0; iX--)
            {
                if(pGlyph->pucData[(iY * pGlyph->iPitch) + (iX / 8)] &
                   (1 << (7 - (iX & 7))))
                {
                    if(iX > iXMax)
                    {
                        iXMax = iX;
                    }
                    break;
                }
            }
        }

        //
        // If this glyph has no bitmap data (typically just the space
        // character), then provide a default minimum and maximum X value.
        //
        if(iXMin == 1000000)
        {
            iXMin = 0;
            iXMax = (3 * iSize) / 10;
        }

        //
        // If this glyph is wider than any previously encountered glyph, then
        // remember its width as the maximum width in this font.
        //
        if((iXMax - iXMin) > iWidth)
        {
            iWidth = iXMax - iXMin;
        }

        //
        // Save the minimum and maximum X value for this glyph.
        //
        pGlyph->iMinX = iXMin;
        pGlyph->iMaxX = iXMax;
    }

    //
    // Loop through the glyphs, compressing each one.
    //
    for(pGlyph = g_pGlyphs, uiChar = 32; pGlyph != (g_pGlyphs + 0x5f);
        pGlyph++, uiChar++)
    {
        //
        // Determine the width and starting position for this glyph based on
        // if this is a monospaced or proportional font.
        //
        if(iMono)
        {
            //
            // For monospaced fonts, horizontally center the glyph in the
            // character cell, providing uniform padding on either side of the
            // glyph.
            //
            iXMin = (pGlyph->iMinX - ((iWidth + 1 + (iSize / 10) -
                                       pGlyph->iMaxX + pGlyph->iMinX) / 2));
            iXMax = iXMin + iWidth + 1 + (iSize / 10);
        }
        else
        {
            //
            // For proportionally-spaced fonts, left-align the glyph in the
            // character cell and provide uniform inter-character padding on
            // the right side.
            //
            iXMin = pGlyph->iMinX;
            iXMax = pGlyph->iMaxX + 1 + (iSize / 10);
        }

        //
        // Loop through the rows of this glyph, extended the glyph as necessary
        // to be the size of the maximal bounding box for this font.
        //
        for(iY = 0, iZero = 0, iOne = 0, iBit = 0, iOpt = 0;
            iY < (iYMin - iYMax + 1); iY++)
        {
            //
            // Loop through the columns of this row, extending the glyph as
            // necessary.
            //
            for(iX = iXMin; iX < iXMax; iX++)
            {
                //
                // Save the value of the previous bit.
                //
                iPrevBit = iBit;

                //
                // See if this bit is within the bitmap data for this glyph.
                //
                if((iY >= (iYMin - pGlyph->iBitmapTop)) &&
                   (iY < (iYMin - pGlyph->iBitmapTop + pGlyph->iRows)) &&
                   (iX >= pGlyph->iMinX) && (iX <= pGlyph->iMaxX))
                {
                    //
                    // Extract this bit from the bitmap data for this glyph.
                    //
                    if(pGlyph->pucData[((iY - iYMin + pGlyph->iBitmapTop) *
                                        pGlyph->iPitch) + (iX / 8)] &
                       (1 << (7 - (iX & 7))))
                    {
                        iBit = 1;
                    }
                    else
                    {
                        iBit = 0;
                    }
                }

                //
                // Otherwise, bits not within the bitmap data for the glyph are
                // filled with zero.
                //
                else
                {
                    iBit = 0;
                }

                //
                // See if this is the very first bit in the glyph.
                //
                if((iX == iXMin) && (iY == 0))
                {
                    //
                    // Set the number of zero and one bits based on the value
                    // of the first bit.
                    //
                    iZero = 1 - iBit;
                    iOne = iBit;

                    //
                    // Go to the next bit.
                    //
                    continue;
                }

                //
                // See if this pixel value matches the previous pixel value.
                //
                if(iBit == iPrevBit)
                {
                    //
                    // Increment the appropriate pixel count.
                    //
                    if(iBit == 0)
                    {
                        iZero++;
                    }
                    else
                    {
                        iOne++;
                    }

                    //
                    // Keep counting if the end of the pixel array has not been
                    // reached.
                    //
                    if((iY != (iYMin - iYMax)) || (iX != (iXMax - 1)))
                    {
                        continue;
                    }
                }

                //
                // See if the pixel value is one (which means that the previous
                // pixel value would be zero).
                //
                else if(iBit == 1)
                {
                    //
                    // Increment the count of one pixels.
                    //
                    iOne++;

                    //
                    // Keep counting if the end of the pixel array has not been
                    // reached.
                    //
                    if((iY != (iYMin - iYMax)) || (iX != (iXMax - 1)))
                    {
                        continue;
                    }
                }

                //
                // See if there are more than 45 zero pixels, which will be
                // encoded as a repeated byte.
                //
                while(iZero > 45)
                {
                    //
                    // Encode the zero bits as a repeated zero byte.
                    //
                    pucChar[iOpt++] = 0x00;
                    pucChar[iOpt++] = (iZero > 1016) ? 127 : (iZero / 8);

                    //
                    // Remove the zero bits that were just output.
                    //
                    iZero -= (iZero > 1016) ? 1016 : (iZero & ~7);
                }

                //
                // Loop while there are more than 15 zero pixels.
                //
                while(iZero > 15)
                {
                    //
                    // Output a byte that produces 15 zero pixels and no one
                    // pixels.
                    //
                    pucChar[iOpt++] = 0xf0;

                    //
                    // Decrement the count of zero pixels by the number just
                    // output.
                    //
                    iZero -= 15;
                }

                //
                // See if there are more than 15 one pixels.
                //
                if(iOne > 15)
                {
                    //
                    // Output a byte that produces the remaining zero pixels
                    // and 15 one pixels.
                    //
                    pucChar[iOpt++] = (iZero << 4) | 15;

                    //
                    // Decrement the count of one pixels by the number just
                    // output.
                    //
                    iOne -= 15;

                    //
                    // See if there are more than 45 one pixels, which will be
                    // encoded as a repeated byte.
                    //
                    while(iOne > 45)
                    {
                        //
                        // Encode the one bits as a repeated one byte.
                        //
                        pucChar[iOpt++] = 0x00;
                        pucChar[iOpt++] = (0x80 |
                                           ((iOne > 1016) ? 127 : (iOne / 8)));

                        //
                        // Remove the one bits that were just output.
                        //
                        iOne -= (iOne > 1016) ? 1016 : (iOne & ~7);
                    }

                    //
                    // Loop while there are more one pixels.
                    //
                    while(iOne)
                    {
                        //
                        // See if ther are than 15 one pixels left.
                        //
                        if(iOne > 15)
                        {
                            //
                            // Output a byte that produces no zero pixels and
                            // 15 one pixels.
                            //
                            pucChar[iOpt++] = 0x0f;

                            //
                            // Decrement the count of one pixels by the number
                            // just copied.
                            //
                            iOne -= 15;
                        }
                        else
                        {
                            //
                            // Output a byte that produces the remaining one
                            // pixels.
                            //
                            pucChar[iOpt++] = iOne;

                            //
                            // There are no more one pixels to output.
                            //
                            break;
                        }
                    }
                }

                //
                // Otherwise, see if there are any zero or one pixels that need
                // to be output.
                //
                else if(iZero || iOne)
                {
                    //
                    // Output a byte that produces the remaining zero and one
                    // pixels.  There is guaranteed to be less than 16 of each
                    // at this point.
                    //
                    pucChar[iOpt++] = (iZero << 4) | iOne;
                }

                //
                // Bytes are output when a zero pixel following a one pixel is
                // encountered, so the loop should be restarted with the zero
                // pixel count set to 1 (for the pixel that caused the output)
                // and the one pixel count set to 0 (since no one pixels have
                // been found yet in the new span).
                //
                iZero = 1;
                iOne = 0;

                //
                // Print an error and return if the size of the compressed
                // character is too large.
                //
                if(iOpt > 254)
                {
                    fprintf(stderr, "%s: Character '%c' was larger than 256 "
                                    "bytes!\n", basename(argv[0]), uiChar);
                    return(1);
                }
            }
        }

        //
        // Save the compressed data for this glyph.
        //
        pGlyph->pucChar = malloc(iOpt + 2);
        pGlyph->pucChar[0] = iOpt + 2;
        pGlyph->pucChar[1] = iXMax - iXMin;
        memcpy(pGlyph->pucChar + 2, pucChar, iOpt);
    }

    //
    // Convert the filename to all lower case letters.
    //
    for(iX = 0; iX < strlen(pcFilename); iX++)
    {
        if((pcFilename[iX] >= 'A') && (pcFilename[iX] <= 'Z'))
        {
            pcFilename[iX] += 0x20;
        }
    }

    //
    // Copy the filename and capitalize the first character.
    //
    pcCapFilename = malloc(strlen(pcFilename) + 1);
    strcpy(pcCapFilename, pcFilename);
    pcCapFilename[0] -= 0x20;

    //
    // Open the file to which the compressed font will be written.
    //
    sprintf(pucChar, "font%s%d%s%s.c", pcFilename, iSize, iBold ? "b" : "",
            iItalic ? "i" : "");
    pFile = fopen(pucChar, "w");

    //
    // Write the header to the output file.
    //
    fprintf(pFile, "//********************************************************"
                   "*********************\n");
    fprintf(pFile, "//\n");
    fprintf(pFile, "// This file is generated by ftrasterize; DO NOT EDIT BY "
                   "HAND!\n");
    fprintf(pFile, "//\n");
    fprintf(pFile, "//********************************************************"
                   "*********************\n");
    fprintf(pFile, "\n");
    fprintf(pFile, "#include \"grlib/grlib.h\"\n");
    fprintf(pFile, "\n");

    //
    // Get the total size of the compressed font.
    //
    for(pGlyph = g_pGlyphs, iOpt = 0; pGlyph < (g_pGlyphs + 95); pGlyph++)
    {
        iOpt += pGlyph->pucChar[0];
    }

    //
    // Write the font details to the output file.  The memory usage of the font
    // is a close approximation that may vary based on the compiler used and
    // the compiler options.
    //
    fprintf(pFile, "//********************************************************"
                   "*********************\n");
    fprintf(pFile, "//\n");
    fprintf(pFile, "// Details of this font:\n");
    fprintf(pFile, "//     Style: %s\n", pcFilename);
    fprintf(pFile, "//     Size: %d point\n", iSize);
    fprintf(pFile, "//     Bold: %s\n", iBold ? "yes" : "no");
    fprintf(pFile, "//     Italic: %s\n", iItalic ? "yes" : "no");
    fprintf(pFile, "//     Memory usage: %d bytes\n", ((iOpt + 3) & ~3) + 200);
    fprintf(pFile, "//\n");
    fprintf(pFile, "//********************************************************"
                   "*********************\n");
    fprintf(pFile, "\n");

    //
    // Write the compressed font data array header to the output file.
    //
    fprintf(pFile, "//********************************************************"
                   "*********************\n");
    fprintf(pFile, "//\n");
    fprintf(pFile, "// The compressed data for the %d point %s%s%s font.\n",
            iSize, pcCapFilename, iBold ? " bold" : "",
            iItalic ? " italic" : "");
    fprintf(pFile, "//\n");
    fprintf(pFile, "//********************************************************"
                   "*********************\n");
    fprintf(pFile, "static const unsigned char g_puc%s%d%s%sData[%d] =\n",
            pcCapFilename, iSize, iBold ? "b" : "", iItalic ? "i" : "", iOpt);
    fprintf(pFile, "{\n");

    //
    // Loop through the glyphs.
    //
    for(pGlyph = g_pGlyphs, iOpt = 0; pGlyph != (g_pGlyphs + 95); pGlyph++)
    {
        //
        // Loop through the bytes of the compressed data for this glyph.
        //
        for(iX = 0; iX < pGlyph->pucChar[0]; iX++)
        {
            //
            // Output this byte of the compressed glyph data, along with any
            // special formatting required to make the output file look
            // readable.
            //
            if(iOpt == 0)
            {
                fprintf(pFile, "   ");
            }
            else if((iOpt % 12) == 0)
            {
                fprintf(pFile, "\n   ");
            }
            fprintf(pFile, " %3d,", pGlyph->pucChar[iX]);
            iOpt++;
        }
    }

    //
    // Close the compressed data array.
    //
    if((iOpt % 12) != 0)
    {
        fprintf(pFile, "\n");
    }
    fprintf(pFile, "};\n");
    fprintf(pFile, "\n");

    //
    // Write the font definition header to the output file.
    //
    fprintf(pFile, "//********************************************************"
                   "*********************\n");
    fprintf(pFile, "//\n");
    fprintf(pFile, "// The font definition for the %d point %s%s%s font.\n",
            iSize, pcCapFilename, iBold ? " bold" : "",
            iItalic ? " italic" : "");
    fprintf(pFile, "//\n");
    fprintf(pFile, "//********************************************************"
                   "*********************\n");

    //
    // Write the font definition to the output file.
    //
    fprintf(pFile, "const tFont g_sFont%s%d%s%s =\n", pcCapFilename, iSize,
            iBold ? "b" : "", iItalic ? "i" : "");
    fprintf(pFile, "{\n");
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    // The format of the font.\n");
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    FONT_FMT_PIXEL_RLE,\n");
    fprintf(pFile, "\n");
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    // The maximum width of the font.\n");
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    %d,\n", iWidth);
    fprintf(pFile, "\n");
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    // The height of the font.\n");
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    %d,\n", iYMin - iYMax + 1);
    fprintf(pFile, "\n");
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    // The baseline of the font.\n");
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    %d,\n", iYMin);
    fprintf(pFile, "\n");
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    // The offset to each character in the font.\n");
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    {\n");
    for(iY = 0, iOpt = 0; iY < 12; iY++)
    {
        fprintf(pFile, "       ");
        for(iX = 0; iX < 8; iX++)
        {
            if((iY != 11) || (iX != 7))
            {
                fprintf(pFile, " %4d,", iOpt);
                iOpt += g_pGlyphs[(iY * 8) + iX].pucChar[0];
            }
        }
        fprintf(pFile, "\n");
    }
    fprintf(pFile, "    },\n");
    fprintf(pFile, "\n");
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    // A pointer to the actual font data\n");
    fprintf(pFile, "    //\n");
    fprintf(pFile, "    g_puc%s%d%s%sData\n", pcCapFilename, iSize,
            iBold ? "b" : "", iItalic ? "i" : "");
    fprintf(pFile, "};\n");

    //
    // Close the output file.
    //
    fclose(pFile);

    //
    // Success.
    //
    return(0);
}

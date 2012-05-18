//*****************************************************************************
//
// pnmtoc.c - Program to convert a NetPBM image to a C array.
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
// This is part of revision 8555 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//*****************************************************************************
//
// The set of colors that have been identified in the image.
//
//*****************************************************************************
unsigned long g_pulColors[256];

//*****************************************************************************
//
// The number of colors that have been identified in the image.
//
//*****************************************************************************
unsigned long g_ulNumColors;

//*****************************************************************************
//
// Compares two colors based on their grayscale intensity.  This is used by
// qsort() to sort the color palette.
//
//*****************************************************************************
int
ColorComp(const void *pvA, const void *pvB)
{
    long lA, lB;

    //
    // Extract the grayscale value for the two colors.
    //
    lA = ((30 * ((*(unsigned long *)pvA >> 16) & 255)) +
          (59 * ((*(unsigned long *)pvA >> 8) & 255)) +
          (11 * (*(unsigned long *)pvA & 255)));
    lB = ((30 * ((*(unsigned long *)pvB >> 16) & 255)) +
          (59 * ((*(unsigned long *)pvB >> 8) & 255)) +
          (11 * (*(unsigned long *)pvB & 255)));

    //
    // Return a value that indicates their relative ordering.
    //
    return(lA - lB);
}

//*****************************************************************************
//
// Extracts the unique colors from the input image, resulting in the palette
// for the image.
//
//*****************************************************************************
void
GetNumColors(unsigned char *pucData, unsigned long ulWidth,
             unsigned long ulHeight, unsigned long ulMono)
{
    unsigned long ulCount, ulColor, ulIdx;

    //
    // Loop through the pixels of the image.
    //
    g_ulNumColors = 0;
    for(ulCount = 0; ulCount < (ulWidth * ulHeight); ulCount++)
    {
        //
        // Extract the color of this pixel.
        //
        if(ulMono)
        {
            //
            // For a mono pixel, just set the R, G and B components to the
            // sample value from the file.
            //
            ulColor = ((pucData[ulCount] << 16) |
                       (pucData[ulCount] << 8) |
                       pucData[ulCount]);
        }
        else
        {
            ulColor = ((pucData[ulCount * 3] << 16) |
                       (pucData[(ulCount * 3) + 1] << 8) |
                       pucData[(ulCount * 3) + 2]);
        }

        //
        // Loop through the current palette to see if this color is already in
        // the palette.
        //
        for(ulIdx = 0; ulIdx < g_ulNumColors; ulIdx++)
        {
            if(g_pulColors[ulIdx] == ulColor)
            {
                break;
            }
        }

        //
        // See if this color is already in the palette.
        //
        if(ulIdx == g_ulNumColors)
        {
            //
            // If there are already 256 colors in the palette, then there is no
            // room for this color.  Simply return and indicate that the
            // palette is too big.
            //
            if(g_ulNumColors == 256)
            {
                g_ulNumColors = 256 * 256 * 256;
                return;
            }

            //
            // Add this color to the palette.
            //
            g_pulColors[g_ulNumColors++] = ulColor;
        }
    }

    //
    // Sort the palette entries based on their grayscale intensity.
    //
    qsort(g_pulColors, g_ulNumColors, sizeof(unsigned long), ColorComp);
}

//*****************************************************************************
//
// Encodes the image using 1 bit per pixel (providing two colors).
//
//*****************************************************************************
unsigned long
Encode1BPP(unsigned char *pucData, unsigned long ulWidth,
           unsigned long ulHeight, unsigned long ulMono)
{
    unsigned long ulX, ulColor, ulIdx, ulByte, ulBit, ulLength;
    unsigned char *pucOutput;

    //
    // Set the output pointer to the beginning of the input data.
    //
    pucOutput = pucData;

    //
    // Loop through the rows of the image.
    //
    ulLength = 0;
    while(ulHeight--)
    {
        //
        // Loop through the output bytes of this row.
        //
        for(ulX = 0; ulX < ulWidth; ulX += 8)
        {
            //
            // Loop through the columns in this byte.
            //
            for(ulBit = 8, ulByte = 0; ulBit > 0; ulBit--)
            {
                //
                // See if this column exists in the image.
                //
                if((ulX + (8 - ulBit)) < ulWidth)
                {
                    //
                    // Extract the color of this pixel.
                    //
                    if(ulMono)
                    {
                        //
                        // For a mono pixel, just set the R, G and B components
                        // to the sample value from the file.
                        //
                        ulColor = ((*pucData << 16) | (*pucData << 8) |
                                   *pucData);
                        pucData++;
                    }
                    else
                    {
                        //
                        // Extract the 3 components for this color pixel
                        //
                        ulColor = *pucData++ << 16;
                        ulColor |= *pucData++ << 8;
                        ulColor |= *pucData++;
                    }

                    //
                    // Select the entry of the palette that matches this pixel.
                    //
                    for(ulIdx = 0; ulIdx < g_ulNumColors; ulIdx++)
                    {
                        if(g_pulColors[ulIdx] == ulColor)
                        {
                            break;
                        }
                    }
                }
                else
                {
                    //
                    // This column does not exist in the image, so provide a
                    // zero padding bit in its place.
                    //
                    ulIdx = 0;
                }

                //
                // Insert this bit into the byte.
                //
                ulByte |= ulIdx << (ulBit - 1);
            }

            //
            // Store this byte into the output image.
            //
            *pucOutput++ = ulByte;
            ulLength++;
        }
    }

    //
    // Return the number of bytes in the encoded output.
    //
    return(ulLength);
}

//*****************************************************************************
//
// Encodes the image using 4 bits per pixel (providing up to 16 colors).
//
//*****************************************************************************
unsigned long
Encode4BPP(unsigned char *pucData, unsigned long ulWidth,
           unsigned long ulHeight, unsigned long ulMono)
{
    unsigned long ulX, ulColor, ulIdx1, ulIdx2, ulCount, ulLength;
    unsigned char *pucOutput;

    //
    // Set the output pointer to the beginning of the input data.
    //
    pucOutput = pucData;

    //
    // Loop through the rows of the image.
    //
    ulLength = 0;
    while(ulHeight--)
    {
        //
        // Loop through the output bytes of this row.
        //
        for(ulX = 0; ulX < ulWidth; ulX += 2)
        {
            //
            // Extract the color of this pixel.
            //
            if(ulMono)
            {
                //
                // For a mono pixel, just set the R, G and B components
                // to the sample value from the file.
                //
                ulColor = ((*pucData << 16) | (*pucData << 8) |
                           *pucData);
                pucData++;
            }
            else
            {
                //
                // Extract the 3 components for this color pixel
                //
                ulColor = *pucData++ << 16;
                ulColor |= *pucData++ << 8;
                ulColor |= *pucData++;
            }

            //
            // Select the entry of the palette that matches this pixel.
            //
            for(ulIdx1 = 0; ulIdx1 < g_ulNumColors; ulIdx1++)
            {
                if(g_pulColors[ulIdx1] == ulColor)
                {
                    break;
                }
            }

            //
            // See if the second pixel exists in the image.
            //
            if((ulX + 1) == ulWidth)
            {
                //
                // The second pixel does not exist in the image, so provide a
                // zero padding nibble in its place.
                //
                ulIdx2 = 0;
            }
            else
            {
                //
                // Extract the color of the second pixel.
                //
                if(ulMono)
                {
                    //
                    // For a mono pixel, just set the R, G and B components
                    // to the sample value from the file.
                    //
                    ulColor = ((*pucData << 16) | (*pucData << 8) |
                               *pucData);
                    pucData++;
                }
                else
                {
                    //
                    // Extract the 3 components for this color pixel
                    //
                    ulColor = *pucData++ << 16;
                    ulColor |= *pucData++ << 8;
                    ulColor |= *pucData++;
                }

                //
                // Select the entry of the palette that matches this pixel.
                //
                for(ulIdx2 = 0; ulIdx2 < g_ulNumColors; ulIdx2++)
                {
                    if(g_pulColors[ulIdx2] == ulColor)
                    {
                        break;
                    }
                }
            }

            //
            // Combine the two nibbles and store the byte into the output
            // image.
            //
            *pucOutput++ = (ulIdx1 << 4) | ulIdx2;
            ulLength++;
        }
    }

    //
    // Return the number of bytes in the encoded output.
    //
    return(ulLength);
}

//*****************************************************************************
//
// Encodes the image using 8 bits per pixel (providing up to 256 colors).
//
//*****************************************************************************
unsigned long
Encode8BPP(unsigned char *pucData, unsigned long ulWidth,
           unsigned long ulHeight, unsigned long ulMono)
{
    unsigned long ulX, ulColor, ulIdx, ulLength;
    unsigned char *pucOutput;

    //
    // Set the output pointer to the beginning of the input data.
    //
    pucOutput = pucData;

    //
    // Loop through the rows of the image.
    //
    ulLength = 0;
    while(ulHeight--)
    {
        //
        // Loop through the columns of the image.
        //
        for(ulX = 0; ulX < ulWidth; ulX++)
        {
            //
            // Extract the color of this pixel.
            //
            if(ulMono)
            {
                //
                // For a mono pixel, just set the R, G and B components
                // to the sample value from the file.
                //
                ulColor = ((*pucData << 16) | (*pucData << 8) |
                           *pucData);
                pucData++;
            }
            else
            {
                //
                // Extract the 3 components for this color pixel
                //
                ulColor = *pucData++ << 16;
                ulColor |= *pucData++ << 8;
                ulColor |= *pucData++;
            }

            //
            // Select the entry of the palette that matches this pixel.
            //
            for(ulIdx = 0; ulIdx < g_ulNumColors; ulIdx++)
            {
                if(g_pulColors[ulIdx] == ulColor)
                {
                    break;
                }
            }

            //
            // Store the byte into the output image.
            //
            *pucOutput++ = ulIdx;
            ulLength++;
        }
    }

    //
    // Return the number of bytes in the encoded output.
    //
    return(ulLength);
}

//*****************************************************************************
//
// Compresses the image data using the Lempel-Ziv-Storer-Szymanski compression
// algorithm.
//
//*****************************************************************************
unsigned long
CompressData(unsigned char *pucData, unsigned long ulLength)
{
    unsigned long ulEncodedLength, ulIdx, ulSize, ulMatch, ulMatchLen, ulCount;
    unsigned char pucDictionary[32], *pucOutput, *pucPtr, ucBits, pucEncode[9];

    //
    // Allocate a buffer to hold the compressed output.  In certain cases, the
    // "compressed" output may be larger than the input data.  In all cases,
    // the first several bytes of the compressed output will be larger than the
    // input data, making an in-place compression impossible.
    //
    pucOutput = pucPtr = malloc(((ulLength * 9) + 7) / 8);

    //
    // Clear the dictionary.
    //
    memset(pucDictionary, 0, sizeof(pucDictionary));

    //
    // Reset the state of the encoded sequence.
    //
    ucBits = 0;
    pucEncode[0] = 0;
    ulEncodedLength = 0;

    //
    // Loop through the input data.
    //
    for(ulCount = 0; ulCount < ulLength; ulCount++)
    {
        //
        // Loop through the current dictionary.
        //
        for(ulIdx = 0, ulMatchLen = 0; ulIdx < sizeof(pucDictionary); ulIdx++)
        {
            //
            // See if this input byte matches this byte of the dictionary.
            //
            if(pucDictionary[ulIdx] == pucData[ulCount])
            {
                //
                // Loop through the next bytes of the input, comparing them to
                // the next bytes of the dictionary.  This determines the
                // length of this match of this portion of the input data to
                // this portion of the dictonary.
                //
                for(ulSize = 1; (ulIdx + ulSize) < sizeof(pucDictionary);
                    ulSize++)
                {
                    if(pucDictionary[ulIdx + ulSize] !=
                       pucData[ulCount + ulSize])
                    {
                        break;
                    }
                }

                //
                // If the match is at least three bytes (since one or two bytes
                // can be encoded just as well or better using a literal
                // encoding instead of a dictionary reference) and the match is
                // longer than any previously found match, then remember the
                // position and length of this match.
                //
                if((ulSize > 2) && (ulSize > ulMatchLen))
                {
                    ulMatch = ulIdx;
                    ulMatchLen = ulSize;
                }
            }
        }

        //
        // See if a match was found.
        //
        if(ulMatchLen != 0)
        {
            //
            // The maximum length match that can be encoded is 9 bytes, so
            // limit the match length if required.
            //
            if(ulMatchLen > 9)
            {
                ulMatchLen = 9;
            }

            //
            // Indicate that this byte of the encoded data is a dictionary
            // reference.
            //
            pucEncode[0] |= (1 << (7 - ucBits));

            //
            // Save the dictionary reference in the encoded data stream.
            //
            pucEncode[ucBits + 1] = (ulMatch << 3) | (ulMatchLen - 2);

            //
            // Shift the dictionary by the number of bytes in the match and
            // copy that many bytes from the input data stream into the
            // dictionary.
            //
            memcpy(pucDictionary, pucDictionary + ulMatchLen,
                   sizeof(pucDictionary) - ulMatchLen);
            memcpy(pucDictionary + sizeof(pucDictionary) - ulMatchLen,
                   pucData + ulCount, ulMatchLen);

            //
            // Increment the count of input bytes consumed by the size of the
            // dictionary match.
            //
            ulCount += ulMatchLen - 1;
        }
        else
        {
            //
            // Save the literal byte in the encoded data stream.
            //
            pucEncode[ucBits + 1] = pucData[ulCount];

            //
            // Shift the dictionary by the single literal byte and copy that
            // byte from the input stream into the dictionary.
            //
            memcpy(pucDictionary, pucDictionary + 1,
                   sizeof(pucDictionary) - 1);
            pucDictionary[sizeof(pucDictionary) - 1] = pucData[ulCount];
        }

        //
        // Increment the count of flag bits that have been used.
        //
        ucBits++;

        //
        // See if all eight flag bits have been used.
        //
        if(ucBits == 8)
        {
            //
            // Copy this 9 byte encoded sequence to the output.
            //
            memcpy(pucPtr, pucEncode, 9);
            pucPtr += 9;
            ulEncodedLength += 9;

            //
            // Reset the encoded sequence state.
            //
            ucBits = 0;
            pucEncode[0] = 0;
        }
    }

    //
    // See if there is any residual data left in the encoded sequence buffer.
    //
    if(ucBits != 0)
    {
        //
        // Copy the residual data from the encoded sequence buffer to the
        // output.
        //
        memcpy(pucPtr, pucEncode, ucBits + 1);
        ulEncodedLength += ucBits + 1;
    }

    //
    // If the encoded length of the data is larger than the unencoded length of
    // the data, then discard the encoded data.
    //
    if(ulEncodedLength > ulLength)
    {
        free(pucOutput);
        return(ulLength);
    }

    //
    // Coyp the encoded data to the input data buffer.
    //
    memcpy(pucData, pucOutput, ulEncodedLength);

    //
    // Free the temporary encoded data buffer.
    //
    free(pucOutput);

    //
    // Return the length of the encoded data, setting the flag in the size that
    // indicates that the data is encoded.
    //
    return(ulEncodedLength | 0x80000000);
}

//*****************************************************************************
//
// Prints a C array definition corresponding to the processed image.
//
//*****************************************************************************
void
OutputData(unsigned char *pucData, unsigned long ulWidth,
           unsigned long ulHeight, unsigned long ulLength)
{
    unsigned long ulIdx, ulCount;

    //
    // Print the image header.
    //
    printf("const unsigned char g_pucImage[] =\n");
    printf("{\n");

    //
    // Print the image format based on the number of colors and the use of
    // compression.
    //
    if(g_ulNumColors <= 2)
    {
        if(ulLength & 0x80000000)
        {
            printf("    IMAGE_FMT_1BPP_COMP,\n");
        }
        else
        {
            printf("    IMAGE_FMT_1BPP_UNCOMP,\n");
        }
    }
    else if(g_ulNumColors <= 16)
    {
        if(ulLength & 0x80000000)
        {
            printf("    IMAGE_FMT_4BPP_COMP,\n");
        }
        else
        {
            printf("    IMAGE_FMT_4BPP_UNCOMP,\n");
        }
    }
    else
    {
        if(ulLength & 0x80000000)
        {
            printf("    IMAGE_FMT_8BPP_COMP,\n");
        }
        else
        {
            printf("    IMAGE_FMT_8BPP_UNCOMP,\n");
        }
    }

    //
    // Strip the compression flag from the image length.
    //
    ulLength &= 0x7fffffff;

    //
    // Print the width and height of the image.
    //
    printf("    %ld, %ld,\n", ulWidth & 255, ulWidth / 256);
    printf("    %ld, %ld,\n", ulHeight & 255, ulHeight / 256);
    printf("\n");

    //
    // For 4 and 8 bit per pixel formats, print out the color palette.
    //
    if(g_ulNumColors > 2)
    {
        printf("    %ld,\n", g_ulNumColors - 1);
        for(ulIdx = 0; ulIdx < g_ulNumColors; ulIdx++)
        {
            printf("    0x%02lx, 0x%02lx, 0x%02lx,\n",
                   g_pulColors[ulIdx] & 255, (g_pulColors[ulIdx] >> 8) & 255,
                   (g_pulColors[ulIdx] >> 16) & 255);
        }
        printf("\n");
    }

    //
    // Loop through the image data bytes.
    //
    for(ulIdx = 0, ulCount = 0; ulIdx < ulLength; ulIdx++)
    {
        //
        // If this is the first byte of an output line, then provide the
        // required indentation.
        //
        if(ulCount++ == 0)
        {
            printf("   ");
        }

        //
        // Print the value of this data byte.
        //
        printf(" 0x%02x,", pucData[ulIdx]);

        //
        // If this is the last byte on an output line, then output a newline.
        //
        if(ulCount == 12)
        {
            printf("\n");
            ulCount = 0;
        }
    }

    //
    // If a partial line of bytes has been output, then output a newline.
    //
    if(ulCount != 0)
    {
        printf("\n");
    }

    //
    // Close the array definition.
    //
    printf("};\n");
}

//*****************************************************************************
//
// Prints the usage message for this application.
//
//*****************************************************************************
void
Usage(char *pucProgram)
{
    fprintf(stderr, "Usage: %s [OPTION] [FILE]\n", basename(pucProgram));
    fprintf(stderr, "Converts a Netpbm file to a C array for use by the "
                    "Stellaris Graphics Library.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  -c  Compresses the image using Lempel-Ziv-Storer-"
                    "Szymanski\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "The image format is chosen based on the number of colors "
                    "in the image; for\n");
    fprintf(stderr, "example, if there are 12 colors in the image, the 4BPP "
                    "image format is used.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Report bugs to <support_lmi@ti.com>.\n");
}

//*****************************************************************************
//
// The main application that converts a NetPBM image into a C array for use by
// the Stellaris Graphics Library.
//
//*****************************************************************************
int
main(int argc, char *argv[])
{
    unsigned long ulLength, ulWidth, ulHeight, ulMax, ulCount, ulMono, ulBitmap;
    unsigned char *pucData;
    int iOpt, iCompress;
    FILE *pFile;

    //
    // Compression of the image is off by default.
    //
    iCompress = 0;

    //
    // Loop through the switches found on the command line.
    //
    while((iOpt = getopt(argc, argv, "ch")) != -1)
    {
        //
        // Determine which switch was identified.
        //
        switch(iOpt)
        {
            //
            // The "-c" switch was found.
            //
            case 'c':
            {
                //
                // Enable compression of the image.
                //
                iCompress = 1;

                //
                // This switch has been handled.
                //
                break;
            }

            //
            // The "-h" switch, or an unknown switch, was found.
            //
            case 'h':
            default:
            {
                //
                // Display the usage information.
                //
                Usage(argv[0]);

                //
                // Return an error.
                //
                return(1);
            }
        }
    }

    //
    // There must be one additional argument on the command line, which
    // provides the name of the image file.
    //
    if((optind + 1) != argc)
    {
        //
        // Display the usage information.
        //
        Usage(argv[0]);

        //
        // Return an error.
        //
        return(1);
    }

    //
    // Open the input image file.
    //
    pFile = fopen(argv[optind], "rb");
    if(!pFile)
    {
        fprintf(stderr, "%s: Unable to open input file '%s'\n",
                basename(argv[0]), argv[optind]);
        return(1);
    }

    //
    // Get the length of the input image file.
    //
    fseek(pFile, 0, SEEK_END);
    ulLength = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);

    //
    // Allocate a memory buffer to store the input image file.
    //
    pucData = malloc(ulLength);
    if(!pucData)
    {
        fprintf(stderr, "%s: Unable to allocate buffer for file.\n",
                basename(argv[0]));
        return(1);
    }

    //
    // Read the input image file into the memory buffer.
    //
    if(fread(pucData, 1, ulLength, pFile) != ulLength)
    {
        fprintf(stderr, "%s: Unable to read file data.\n", basename(argv[0]));
        return(1);
    }

    //
    // Close the input image file.
    //
    fclose(pFile);

    //
    // Parse the file header to extract the width and height of the image, and
    // to verify that the image file is a format that is recognized.
    //
    if((pucData[0] != 'P') || ((pucData[1] != '4') && (pucData[1] != '6') &&
       (pucData[1] != '5')))
    {
        fprintf(stderr, "%s: '%s' is not a supported PNM file.\n",
                basename(argv[0]), argv[1]);
        return(1);
    }

    //
    // Are we dealing with a color or grayscale image?
    //
    ulMono = (pucData[1] == '5') ? 1 : 0;

    //
    // Are we dealing with a (1bpp) bitmap?
    //
    ulBitmap = (pucData[1] == '4') ? 1 : 0;

    //
    // Loop through the values to be read from the header.
    //
    for(ulCount = 0, ulMax = 2; ulCount < 3; )
    {
        //
        // Loop until a non-whitespace character is found.
        //
        while((pucData[ulMax] == ' ') || (pucData[ulMax] == '\t') ||
              (pucData[ulMax] == '\r') || (pucData[ulMax] == '\n'))
        {
            ulMax++;
        }

        //
        // If this is a '#', then it starts a comment line.  Ignore comment
        // lines.
        //
        if(pucData[ulMax] == '#')
        {
            //
            // Loop until the end of the line is found.
            //
            while((pucData[ulMax] != '\r') && (pucData[ulMax] != '\n'))
            {
                ulMax++;
            }

            //
            // Restart the loop.
            //
            continue;
        }

        //
        // Read this value from the file.
        //
        if(ulCount == 0)
        {
            if(sscanf(pucData + ulMax, "%lu", &ulWidth) != 1)
            {
                fprintf(stderr, "%s: '%s' has an invalid width.\n",
                        basename(argv[0]), argv[1]);
                return(1);
            }
        }
        else if(ulCount == 1)
        {
            if(sscanf(pucData + ulMax, "%lu", &ulHeight) != 1)
            {
                fprintf(stderr, "%s: '%s' has an invalid height.\n",
                        basename(argv[0]), argv[1]);
                return(1);
            }

            //
            // We've finished reading the header if this is a bitmap so force
            // the loop to exit.
            //
            if(ulBitmap)
            {
                ulCount = 2;
            }
        }
        else
        {
            //
            // Read the maximum number of colors.  We ignore this value but
            // need to skip past it.  Note that, if this is a bitmap, we will
            // never get here.
            //
            if(sscanf(pucData + ulMax, "%lu", &ulCount) != 1)
            {
                fprintf(stderr, "%s: '%s' has an invalid maximum value.\n",
                        basename(argv[0]), argv[1]);
                return(1);
            }
            ulCount = 2;
        }
        ulCount++;

        //
        // Skip past this value.
        //
        while((pucData[ulMax] != ' ') && (pucData[ulMax] != '\t') &&
              (pucData[ulMax] != '\r') && (pucData[ulMax] != '\n'))
        {
            ulMax++;
        }
    }

    //
    // Find the end of this line.
    //
    while((pucData[ulMax] != '\r') && (pucData[ulMax] != '\n'))
    {
        ulMax++;
    }

    //
    // Skip this end of line marker.
    //
    ulMax++;
    if((pucData[ulMax] == '\r') || (pucData[ulMax] == '\n'))
    {
        ulMax++;
    }

    //
    // Is this a bitmap?
    //
    if(!ulBitmap)
    {
        //
        // No - get the number of distinct colors in the image.
        //
        GetNumColors(pucData + ulMax, ulWidth, ulHeight, ulMono);

        //
        // Determine how many colors are in the image.
        //
        if(g_ulNumColors <= 2)
        {
            //
            // There are 1 or 2 colors in the image, so encode it with 1 bit
            // per pixel.
            //
            ulLength = Encode1BPP(pucData + ulMax, ulWidth, ulHeight, ulMono);
        }
        else if(g_ulNumColors <= 16)
        {
            //
            // There are 3 through 16 colors in the image, so encode it with
            // 4 bits per pixel.
            //
            ulLength = Encode4BPP(pucData + ulMax, ulWidth, ulHeight, ulMono);
        }
        else if(g_ulNumColors <= 256)
        {
            //
            // There are 17 through 256 colors in the image, so encode it with
            // 8 bits per pixel.
            //
            ulLength = Encode8BPP(pucData + ulMax, ulWidth, ulHeight, ulMono);
        }
        else
        {
            //
            // There are more than 256 colors in the image, which is not
            // supported.
            //
            fprintf(stderr, "%s: Image contains too many colors!\n",
                    basename(argv[0]));
            return(1);
        }
    }
    else
    {
        //
        // This is a bitmap so the palette consists of black and white only.
        //
        g_ulNumColors = 2;

        //
        // Set up the palette needed for the data. PBM format defines 1 as a
        // black pixel and 0 as a white one but we need to invert this to
        // match the StellarisWare graphics library format.
        //
        g_pulColors[1] = 0x00FFFFFF;
        g_pulColors[0] = 0x00000000;

        //
        // The data as read from the file is fine so we don't need to do any
        // reformatting now that the palette is set up.  Just set up the length
        // of the bitmap data remembering that each line is padded to a whole
        // byte.  First check that the data we read is actually the correct
        // length.
        //
        if((ulLength - ulMax) < (((ulWidth + 7) / 8) * ulHeight))
        {
            fprintf(stderr, "%s: Image data truncated. Size %d bytes "
                    "but dimensions are %d x %d.\n", (ulLength - ulMax),
                    ulWidth, ulHeight);
            return(1);
        }

        //
        // Set the length to the expected value.
        //
        ulLength = ((ulWidth + 7) / 8) * ulHeight;

        //
        // Invert the image data to make 1 bits foreground (white) and 0
        // bits background (black).
        //
        for(ulCount = 0; ulCount < ulLength; ulCount++)
        {
            *(pucData + ulMax + ulCount) = ~(*(pucData + ulMax + ulCount));
        }
    }

    //
    // Compress the image data if requested.
    //
    if(iCompress)
    {
        ulLength = CompressData(pucData + ulMax, ulLength);
    }

    //
    // Print the C array corresponding to the image data.
    //
    OutputData(pucData + ulMax, ulWidth, ulHeight, ulLength);

    //
    // Success.
    //
    return(0);
}

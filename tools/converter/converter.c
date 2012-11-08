//*****************************************************************************
//
// converter.c - Program to convert 16-bit mono PCM files into C arrays for use
//               by the Class-D audio driver.
//
// Copyright (c) 2007-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 9453 of the Stellaris Firmware Development Package.
//
//*****************************************************************************

#include <libgen.h>
#include <stdio.h>
#include <unistd.h>

//*****************************************************************************
//
// The possible modes of the application.
//
//*****************************************************************************
#define MODE_NONE               0
#define MODE_PCM                1
#define MODE_ADPCM              2

//*****************************************************************************
//
// The adjustment to the step index based on the value of an encoded sample.
// The sign bit is ignored when using this table (that is, only the lower three
// bits are used).
//
//*****************************************************************************
static const char g_pcADPCMIndex[8] =
{
    -1, -1, -1, -1, 2, 4, 6, 8
};

//*****************************************************************************
//
// The differential values for the ADPCM decoder.  One of these is selected
// based on the step index.
//
//*****************************************************************************
static const unsigned short g_pusADPCMStep[89] =
{
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41,
    45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209,
    230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876,
    963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749,
    3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630,
    9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623,
    27086, 29794, 32767
};

//*****************************************************************************
//
// The current step index for the ADPCM decoder.  This selects a differential
// value from g_pusADPCMStep.
//
//*****************************************************************************
static long g_lStepIndex;

//*****************************************************************************
//
// The previous output of the ADPCM decoder.
//
//*****************************************************************************
static short g_sPreviousOutput;

//*****************************************************************************
//
// Initializes the ADPCM decoder.
//
//*****************************************************************************
static void
ADPCMInit(void)
{
    //
    // Reset the step index and previous output values to zero.
    //
    g_lStepIndex = 0;
    g_sPreviousOutput = 0;
}

//*****************************************************************************
//
// Decodes a sample of ADPCM data.
//
//*****************************************************************************
static short
ADPCMDecode(char cCode)
{
    long lStep;

    //
    // Compute the sample delta based on the current nibble and step size.
    //
    lStep = (((2 * (cCode & 7)) + 1) * g_pusADPCMStep[g_lStepIndex]) / 8;

    //
    // Add or subtract the delta to the previous sample value, clipping if
    // necessary.
    //
    if(cCode & 8)
    {
        lStep = (long)g_sPreviousOutput - lStep;
        if(lStep < -32768)
        {
            lStep = -32768;
        }
    }
    else
    {
        lStep = (long)g_sPreviousOutput + lStep;
        if(lStep > 32767)
        {
            lStep = 32767;
        }
    }

    //
    // Store the generated sample.
    //
    g_sPreviousOutput = (short)lStep;

    //
    // Adjust the step size index based on the current nibble, clipping the
    // value if required.
    //
    g_lStepIndex += g_pcADPCMIndex[cCode & 7];
    if(g_lStepIndex < 0)
    {
        g_lStepIndex = 0;
    }
    if(g_lStepIndex > 88)
    {
        g_lStepIndex = 88;
    }

    //
    // Return the generated sample.
    //
    return((short)lStep);
}

//*****************************************************************************
//
// Encodes a sample to ADPCM.
//
//*****************************************************************************
static char
ADPCMEncode(short sSample)
{
    long lStep, lCode, lSign;

    //
    // Compute the difference between this sample and the previous output of
    // the decoder.
    //
    lStep = sSample - g_sPreviousOutput;

    //
    // Determine the sign of the difference, and make the difference positive
    // if it is negative.
    //
    if(lStep < 0)
    {
        lSign = 1;
        lStep = 0 - lStep;
    }
    else
    {
        lSign = 0;
    }

    //
    // Determine the encoded value based on the current step size in the
    // decoder.
    //
    lCode = (4 * lStep) / g_pusADPCMStep[g_lStepIndex];
    if(lCode > 7)
    {
        lCode = 7;
    }

    //
    // Place the sign bit into the encoded value.
    //
    if(lSign)
    {
        lCode |= 8;
    }

    //
    // Update the decoder based on this new sample.
    //
    ADPCMDecode(lCode);

    //
    // Return the encoded ADPCM value.
    //
    return(lCode);
}

//*****************************************************************************
//
// Prints out usage information for the application.
//
//*****************************************************************************
void
Usage(char *pcFilename)
{
    fprintf(stderr, "Usage: %s [OPTION]... [INPUT FILE]\n",
            basename(pcFilename));
    fprintf(stderr, "Converts a raw 16-bit, mono PCM input file to a C array "
            "with PCM or ADPCM.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Options are:\n");
    fprintf(stderr, "  -a            Encode to 4-bit IMA ADPCM\n");
    fprintf(stderr, "  -c COUNT      Maximum number of output bytes\n");
    fprintf(stderr, "  -n NAME       Specify the name of the C array\n");
    fprintf(stderr, "  -o FILENAME   Specify the name of the output file\n");
    fprintf(stderr, "  -p            Encode to 8-bit PCM\n");
    fprintf(stderr, "  -s SKIP       Number of initial input samples to "
            "skip\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "If the input filename is not specified, standard input "
            "will be used.\n");
    fprintf(stderr, "If the output filename is not specified, standard output "
            "will be used.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Version 9453\n");
    fprintf(stderr, "Report bugs to <support_lmi@ti.com>.\n");
}

//*****************************************************************************
//
// An application to convert from 16-bit PCM data to either 8-bit PCM or 4-bit
// ADPCM data.
//
//*****************************************************************************
int
main(int argc, char *argv[])
{
    long lCode, lMode, lCount, lSkip, lIdx;
    char *pcInput, *pcOutput, *pcArray;
    short psSample[4];
    FILE *pIn, *pOut;

    //
    // Initialize the name of the input file, output file, and the name of the
    // C array to produce.
    //
    pcInput = NULL;
    pcOutput = NULL;
    pcArray = "g_pucAudioData";

    //
    // Set the default number of samples to skip and bytes to produce.
    //
    lSkip = 0;
    lCount = 0x7fffffff;

    //
    // Initialize the mode to none, requiring that one be specified on the
    // command line.
    //
    lMode = MODE_NONE;

    //
    // Parse the command line arguments.
    //
    while((lCode = getopt(argc, argv, "?ac:hn:o:ps:")) != -1)
    {
        //
        // See which flag was found.
        //
        switch(lCode)
        {
            //
            // Encoding to ADPCM has been requested.
            //
            case 'a':
            {
                lMode = MODE_ADPCM;
                break;
            }

            //
            // The count of bytes to produce has been specified.
            //
            case 'c':
            {
                lCount = atoi(optarg);
                break;
            }

            //
            // The usage information has been requested.
            //
            case 'h':
            case '?':
            {
                Usage(argv[0]);
                return(1);
            }

            //
            // The name of the C array has been specified.
            //
            case 'n':
            {
                pcArray = optarg;
                break;
            }

            //
            // The name of the output file has been specified.
            //
            case 'o':
            {
                pcOutput = optarg;
                break;
            }

            //
            // Encoding to PCM has been requested.
            //
            case 'p':
            {
                lMode = MODE_PCM;
                break;
            }

            //
            // The number of samples to skip has been specified.
            //
            case 's':
            {
                lSkip = atoi(optarg);
                break;
            }

            //
            // An unknown argument has been specified.
            //
            default:
            {
                fprintf(stderr, "Try `%s -h' for more information.\n",
                        basename(argv[0]));
                return(1);
            }
        }
    }

    //
    // See if an encoding mode was specified.
    //
    if(lMode == MODE_NONE)
    {
        //
        // Indicate that an encoding mode must be specified.
        //
        fprintf(stderr, "%s: An output mode must be specified!\n",
                basename(argv[0]));
        fprintf(stderr, "\n");
        Usage(argv[0]);
        return(1);
    }

    //
    // Make sure that there are 0 or 1 arguments left on the command line.
    //
    if((optind != argc) && (optind != (argc - 1)))
    {
        //
        // Indicate that too many arguments have been specified.
        //
        fprintf(stderr, "%s: Too many arguments specified!\n",
                basename(argv[0]));
        fprintf(stderr, "\n");
        Usage(argv[0]);
        return(1);
    }

    //
    // See if there is another argument on the command line.
    //
    if(optind < argc)
    {
        //
        // The name of the input file has been specified.
        //
        pcInput = argv[optind];
    }

    //
    // See if an input file was specified.
    //
    if(pcInput)
    {
        //
        // Open the specified input file.
        //
        pIn = fopen(pcInput, "rb");
        if(!pIn)
        {
            fprintf(stderr, "%s: Unable to open input file '%s'!\n",
                    basename(argv[0]), pcInput);
            return(1);
        }
    }
    else
    {
        //
        // Open stardard input as the input file.
        //
        pIn = fdopen(0, "rb");
        if(!pIn)
        {
            fprintf(stderr, "%s: Unable to open stdin!\n", basename(argv[0]));
            return(1);
        }
    }

    //
    // See if an output file was specified.
    //
    if(pcOutput)
    {
        //
        // Create the specified output file.
        //
        pOut = fopen(pcOutput, "wb");
        if(!pOut)
        {
            fprintf(stderr, "%s: Unable to create output file '%s'!\n",
                    basename(argv[0]), pcOutput);
            return(1);
        }
    }
    else
    {
        //
        // Open standard output as the output file.
        //
        pOut = fdopen(1, "w");
        if(!pOut)
        {
            fprintf(stderr, "%s: Unable to open stdout!\n", basename(argv[0]));
            return(1);
        }
    }

    //
    // If samples should be skipped at the beginning of the input file, then
    // consume them now.  fread() is used instead of fseek() since the latter
    // will not work for standard input.
    //
    while(lSkip)
    {
        fread(&psSample, 1, 2, pIn);
        lSkip--;
    }

    //
    // Print out the file header.
    //
    fprintf(pOut, "// AUTOMATICALLY GENERATED FILE -- DO NOT EDIT!\n");
    fprintf(pOut, "\n");
    fprintf(pOut, "const unsigned char %s[] =\n", pcArray);
    fprintf(pOut, "{\n");

    //
    // See if the audio should be encoded to PCM.
    //
    if(lMode == MODE_PCM)
    {
        //
        // Loop while there are more samples to be processed.
        //
        for(lIdx = 0; (fread(&psSample, 1, 2, pIn) == 2) && lCount;
            lCount--, lIdx++)
        {
            //
            // Convert this 16-bit signed sample to an 8-bit unsigned value.
            //
            lCode = ((long)psSample[0] + 32768) / 256;

            //
            // Output this value.
            //
            if(lIdx == 0)
            {
                fprintf(pOut, "    0x%02lx,", lCode & 255);
            }
            else if(lIdx == 11)
            {
                fprintf(pOut, " 0x%02lx,\n", lCode & 255);
                lIdx = -1;
            }
            else
            {
                fprintf(pOut, " 0x%02lx,", lCode & 255);
            }
        }

        //
        // Output an additional newline if there is any output on the current
        // line.
        //
        if(lIdx != 0)
        {
            fprintf(pOut, "\n");
        }
    }

    //
    // Otherwise, see if the audio should be encoded to ADPCM.
    //
    else if(lMode == MODE_ADPCM)
    {
        //
        // Initialize the ADPCM decoder.
        //
        ADPCMInit();

        //
        // Loop while there are more samples to be processed.
        //
        for(lIdx = 0; (fread(&psSample, 1, 4, pIn) == 4) && lCount;
            lCount--, lIdx++)
        {
            //
            // Convert this pair of 16-bit signed samples to a pair of 4-bit
            // ADPCM samples packed into a single byte.
            //
            lCode = ADPCMEncode(psSample[0]) << 4;
            lCode |= ADPCMEncode(psSample[1]);

            //
            // Output this value.
            //
            if(lIdx == 0)
            {
                fprintf(pOut, "    0x%02lx,", lCode & 255);
            }
            else if(lIdx == 11)
            {
                fprintf(pOut, " 0x%02lx,\n", lCode & 255);
                lIdx = -1;
            }
            else
            {
                fprintf(pOut, " 0x%02lx,", lCode & 255);
            }
        }

        //
        // Output an additional newline if there is any output on the current
        // line.
        //
        if(lIdx != 0)
        {
            fprintf(pOut, "\n");
        }
    }

    //
    // Print out the file footer.
    //
    fprintf(pOut, "};\n");

    //
    // Close the input and output files.
    //
    fclose(pOut);
    fclose(pIn);

    //
    // Success.
    //
    return(0);
}

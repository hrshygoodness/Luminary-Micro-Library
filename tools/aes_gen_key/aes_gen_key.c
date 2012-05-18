//*****************************************************************************
//
// aes_gen_key.c - Program to generate pre-expanded AES keys.
//
// Copyright (c) 2009-2012 Texas Instruments Incorporated.  All rights reserved.
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

#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "aes.h"

//*****************************************************************************
//
// The version of the application.
//
//*****************************************************************************
const unsigned short g_usApplicationVersion = 8555;

//*****************************************************************************
//
// Option keys to use for the command line options.
//
//*****************************************************************************
#define OPT_DATA        'a'
#define OPT_CODE        'x'
#define OPT_ENCRYPT     'e'
#define OPT_DECRYPT     'd'
#define OPT_KEYSIZE     's'
#define OPT_KEY         'k'
#define OPT_HELP        'h'
#define OPT_VERSION     'v'

//*****************************************************************************
//
// AES key expansion mode and operation definitions.
//
//*****************************************************************************
#define MODE_CODE       1
#define MODE_DATA       2
#define OPER_ENCRYPT    1
#define OPER_DECRYPT    2

//*****************************************************************************
//
// This structure holds the command line options for getopt_long().
//
//*****************************************************************************
static struct option sLongOpts[] =
{
    {"data",        no_argument,        0,      OPT_DATA},
    {"code",        no_argument,        0,      OPT_CODE},
    {"encrypt",     no_argument,        0,      OPT_ENCRYPT},
    {"decrypt",     no_argument,        0,      OPT_DECRYPT},
    {"keysize",     required_argument,  0,      OPT_KEYSIZE},
    {"key",         required_argument,  0,      OPT_KEY},
    {"version",     no_argument,        0,      OPT_VERSION},
    {"help",        no_argument,        0,      OPT_HELP},
    {NULL,          0,                  0,              0}
};

//*****************************************************************************
//
// This structure holds help info for the command line usage help.
//
//*****************************************************************************
struct helpinfo
{
    const char *argname;
    const char *briefhelp;
};

static struct helpinfo sHelpInfo[] =
{
    {"",            "generate expanded key as (a)rray of data"},
    {"",            "generate expanded key as e(x)ecutable code"},
    {"",            "generate expanded key for (e)ncryption"},
    {"",            "generate expanded key for (d)decryption"},
    {"KEYSIZE",     "(s)ize of the key in bits (128, 192, or 256)"},
    {"KEY",         "(k)ey value in hexadecimal"},
    {"",            "show version"},
    {"",            "show this help"}
};

//*****************************************************************************
//
// Prints the version of the program.
//
//*****************************************************************************
void
ShowVersion(void)
{
    printf("\naes_gen_key, version %u\n", g_usApplicationVersion);
    printf("Copyright (c) 2009-2012 Texas Instruments Incorporated.  All rights reserved.\n\n");
}

//*****************************************************************************
//
// Prints usage help for the user
//
//*****************************************************************************
void
ShowUsage(void)
{
    char szOptPlusArg[64];
    unsigned int idx;

    //
    // Printf usage message
    //
    fprintf(stdout,
            "\nUsage: aes_gen_key [OPTIONS] --keysize=[SIZE]"
            " --key=[KEYSTRING] [FILE]\n");
    fprintf(stdout, "\nOPTIONS are:\n");

    //
    // List all of the possible options, showing short and long form
    // with a brief help about the option
    //
    for(idx = 0; sLongOpts[idx].name; idx++)
    {
        sprintf(szOptPlusArg, "-%c, --%s %s",
                sLongOpts[idx].val, sLongOpts[idx].name,
                sHelpInfo[idx].argname);
        fprintf(stdout, " %-24s %s\n", szOptPlusArg, sHelpInfo[idx].briefhelp);
    }

    //
    // Printf some summary help info
    //
    printf("\nThe --key and --keysize options are mandatory.  Only one each of\n");
    printf("--data or --code, and --encrypt or --decrypt should be chosen.  If\n");
    printf("not specified otherwise then the default is --data --encrypt\n");
    printf("\nFILE is the name of the file that will be created that contains\n");
    printf("contains the expanded key.  This file is in the form of a C header\n");
    printf("file, and should be included in your application.\n");
    printf("\n");
}

//*****************************************************************************
//
// An application to generate pre-expanded AES keys for use in Stellaris
// AES applications.
//
//*****************************************************************************
int
main(int argc, char *argv[])
{
    int iOpt;
    int iModeChosen = 0;
    int iOperChosen = 0;
    int iKeyBits = 0;
    int idx;
    int iArrSize;
    int pos;
    char *pcKeyString;
    aes_context ctx;
    unsigned char ucKeyBuf[8 * 4];
    unsigned codes[68*2];
    unsigned temp[68*2];
    FILE *fpOut;

    //
    // Set the initial key string NULL
    //
    pcKeyString = NULL;

    //
    // If the user did not provide any command line args, then print usage
    // and quit.
    //
    if(argc == 1)
    {
        ShowUsage();
        return(0);
    }

    //
    // Enter loop to process all the command line options.
    //
    for(;;)
    {
        //
        // Get the next short form (-) or long form (--) option
        //
        iOpt = getopt_long(argc, argv, "axeds:k:hv", sLongOpts, NULL);

        //
        // A value of -1 means the end of the list of options
        //
        if(iOpt == -1)
        {
            break;
        }

        //
        // Process each option
        //
        switch(iOpt)
        {
            //
            // The option found is not recognized.
            // Print error message, show usage, and quit.
            //
            case '?':
            {
                fprintf(stderr, "\nFound bad command line option \"%s\"\n",
                        argv[optind - 1]);
                ShowUsage();
                return(1);
            }

            //
            // User specified data mode.  Make sure that only one of --data
            // or --code was chosen.
            //
            case OPT_DATA:
            {
                if(iModeChosen)
                {
                    fprintf(stderr,
                            "You can only choose one of --data or --code\n");
                    return(1);
                }
                iModeChosen = MODE_DATA;
                break;
            }

            //
            // User specified code mode.  Make sure that only one of --data
            // or --code was chosen.
            //
            case OPT_CODE:
            {
                if(iModeChosen)
                {
                    fprintf(stderr,
                            "You can only choose one of --data or --code\n");
                    return(1);
                }
                iModeChosen = MODE_CODE;
                break;
            }

            //
            // User specified encrypt operation.  Make sure that only one of
            // --decrypt or --encrypt was chosen.
            //
            case OPT_ENCRYPT:
            {
                if(iOperChosen)
                {
                    fprintf(stderr,
                        "You can only choose one of --decrypt or --encrypt\n");
                    return(1);
                }
                iOperChosen = OPER_ENCRYPT;
                break;
            }

            //
            // User specified decrypt operation.  Make sure that only one of
            // --decrypt or --encrypt was chosen.
            //
            case OPT_DECRYPT:
            {
                if(iOperChosen)
                {
                    fprintf(stderr,
                        "You can only choose one of --decrypt or --encrypt\n");
                    return(1);
                }
                iOperChosen = OPER_DECRYPT;
                break;
            }

            //
            // User specified the key string.  Save it for later use.
            //
            case OPT_KEY:
            {
                pcKeyString = optarg;
                break;
            }

            //
            // User specified the key size.
            //
            case OPT_KEYSIZE:
            {
                iKeyBits = atoi(optarg);
                break;
            }

            //
            // User asks for program version.
            //
            case OPT_VERSION:
            {
                ShowVersion();
                return(0);
            }

            //
            // User asks for program help.
            //
            case OPT_HELP:
            default:
            {
                ShowUsage();
                return(0);
            }
        }
    }

    //
    // Adjust the command line position to account for the options.
    //
    argc -= optind;
    argv += optind;

    //
    // Make sure a key length was specified.
    //
    if(iKeyBits == 0)
    {
        fprintf(stderr, "You must specify a key size with --keysize\n");
        return(1);
    }

    //
    // Make sure the specified key length is a valid choice.
    //
    if((iKeyBits != 128) && (iKeyBits != 196) && (iKeyBits != 256))
    {
        fprintf(stderr, "You specified a key length of %d\n", iKeyBits);
        fprintf(stderr, "Key length much be 128, 196, or 256\n");
        return(1);
    }

    //
    // Make sure that the user provided a key string.
    //
    if(!pcKeyString)
    {
        fprintf(stderr, "You must specify the key with --key\n");
        return(1);
    }

    //
    // Make sure the key string length matches the specified key size.
    //
    if (strlen(pcKeyString) != (iKeyBits / 4))
    {
        fprintf(stderr,
                "Invalid key, expected %d chars (%d bits worth), got %d\n",
                iKeyBits / 4, iKeyBits, strlen(pcKeyString));
        return(1);
    }

    //
    // There should be one more command line arg left, that is the file name.
    //
    if(argc != 1)
    {
        fprintf(stderr, "You must provide a file name after all the options\n");
        return(1);
    }

    //
    // If the mode was not specified then use default.
    //
    if(!iModeChosen)
    {
        iModeChosen = MODE_DATA;
    }

    //
    // If the operation was not specified then use default.
    //
    if(!iOperChosen)
    {
        iOperChosen = OPER_ENCRYPT;
    }

    //
    // Open the output file for writing
    //
    fpOut = fopen(argv[0], "w");
    if (!fpOut)
    {
        fprintf(stderr, "Unable to open output file: '%s'\n", argv[0]);
        return(1);
    }

    //
    // Print the first part of the file header.
    //
    fprintf(fpOut,
    "//*****************************************************************************\n"
    "//\n"
    "// %s - generated using the aes_gen_key utility from Texas Instruments\n"
    "//\n"
    "// Key String:    %s\n"
    "// Key Length:    %d bits\n"
    "// Key Operation: %s\n"
    "// Key expanded as a %s.\n"
    "//\n"
    "//*****************************************************************************\n"
    "\n",
    argv[0], pcKeyString, iKeyBits,
    (iOperChosen == OPER_ENCRYPT) ? "Encryption" : "Decryption",
    (iModeChosen == MODE_DATA) ? "data structure" : "C code function");

    //
    // Convert the key string into a numeric buffer so it can be expanded.
    //
    for (idx = 0; idx < iKeyBits / 8; idx++)
    {
        sscanf(pcKeyString, "%2X", &ucKeyBuf[idx]);
        pcKeyString += 2;
    }

    //
    // Call the AES library function to expand the key.
    //
    if(iOperChosen == OPER_ENCRYPT)
    {
        aes_setkey_enc(&ctx, ucKeyBuf, iKeyBits);
        pcKeyString = "Encrypt";
    }
    else
    {
        aes_setkey_dec(&ctx, ucKeyBuf, iKeyBits);
        pcKeyString = "Decrypt";
    }

    //
    // Determine the array size needed to hold the expanded key
    //
    switch(iKeyBits)
    {
        default:
        case 128:
        {
            iArrSize = 44;
            break;
        }
        case 192:
        {
            iArrSize = 54;
            break;
        }
        case 256:
        {
            iArrSize = 68;
            break;
        }
    }

    //
    // If data mode, create a macro in the file that maps the function name
    // to the data array containing the expanded key.  Then create the first
    // line of the data array.
    //
    if(iModeChosen == MODE_DATA)
    {
        fprintf(fpOut, "#define AESExpanded%sKeyData() g_uExpanded%sKey\n"
                       "static const unsigned g_uExpanded%sKey[%d] =\n"
                       "{\n",
                       pcKeyString, pcKeyString, pcKeyString, iArrSize);
    }

    //
    // If code mode, then create the function name, and sort the expanded
    // key data for efficiency.
    //
    else
    {
        fprintf(fpOut,
                "void AESLoadExpanded%sKey(unsigned short *pusExpandedKey)\n"
                "{\n", pcKeyString);

        //
        // This loop stores the expanded key as 16 bit words in a temporary
        // array.  The position in the array is also stored so that the
        // value can be shuffled later if needed.
        //
        for (idx = pos = 0; idx < iArrSize; idx++, pos += 2)
        {
            temp[pos] = (unsigned short)ctx.buf[idx];
            temp[pos] |= pos<<16;
            temp[pos+1] = (unsigned short)(ctx.buf[idx]>>16);
            temp[pos+1] |= (pos+1)<<16;
        }

        //
        // This loop sorts the data so that if any values are the same,
        // they are placed adjacent to each other when the values are
        // assigned.  Placing repeated value assignments together will
        // allow the compiler to save some code by reusing the assignment
        // value.
        //
        for (idx = pos = 0; pos < iArrSize*2; pos++)
        {
            int find;
            if(pos && !temp[pos])
            {
                //
                // The value at this position in the temp array has already
                // been copied to the code array, so move on to the next one.
                //
                continue;
            }

            //
            // Copy the data from the temporary array to the codes array.
            //
            codes[idx++] = temp[pos];

            //
            // This loop searches the rest of the temporary array to see if
            // there are any other data entries that have the same value as
            // this one.
            //
            for(find = pos + 1; find < iArrSize * 2; find++)
            {
                if (temp[find] &&
                    (unsigned short)temp[find] == (unsigned short)temp[pos])
                {                               // hoist up
                    codes[idx++] = temp[find];
                    temp[find] = 0;
                }
            }
        }
    }

    //
    // Loop through the entire array and generate the data structure or
    // assignments for the C function.
    //
    for (idx = 0; idx < iArrSize; idx++)
    {
        //
        // If data mode is used, then print out the values in the array
        //
        if(iModeChosen == MODE_DATA)
        {
            //
            // Every 4 values, start a new line
            //
            if(!(idx & 3))
            {
                fprintf(fpOut, "\n    ");
            }

            //
            // Print the data value at this array location
            //
            fprintf(fpOut, "0x%08X, ", (unsigned)ctx.buf[idx]);
        }

        //
        // Else, if code mode is used, print out the C statements to
        // perform the assignments.
        //
        else
        {
            fprintf(fpOut, "    pusExpandedKey[%2d] = 0x%04X;\n"
                           "    pusExpandedKey[%2d] = 0x%04X;\n",
                    codes[(idx * 2) + 0] >> 16,
                    (unsigned short)codes[(idx * 2) + 0],
                    codes[(idx * 2) + 1] >> 16,
                    (unsigned short)codes[(idx * 2) + 1]);
        }
    }

    //
    // Print out the closing of the data array or C function.
    //
    if(iModeChosen == MODE_DATA)
    {
        fprintf(fpOut, "\n};\n");
    }
    else
    {
        fprintf(fpOut, "}\n");
    }

    fclose(fpOut);
    return(0);
}
